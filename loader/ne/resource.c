/*
 * NE resource functions
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 * Copyright 1997 Alex Korobka
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "windows.h"
#include "global.h"
#include "ldt.h"
#include "module.h"
#include "neexe.h"
#include "resource.h"
#include "callback.h"
#include "debug.h"

#define NEXT_TYPEINFO(pTypeInfo) ((NE_TYPEINFO *)((char*)((pTypeInfo) + 1) + \
                                   (pTypeInfo)->count * sizeof(NE_NAMEINFO)))

/***********************************************************************
 *           NE_FindNameTableId
 *
 * Find the type and resource id from their names.
 * Return value is MAKELONG( typeId, resId ), or 0 if not found.
 */
static DWORD NE_FindNameTableId( NE_MODULE *pModule, LPCSTR typeId, LPCSTR resId )
{
    NE_TYPEINFO *pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->res_table + 2);
    NE_NAMEINFO *pNameInfo;
    HGLOBAL16 handle;
    WORD *p;
    DWORD ret = 0;
    int count;

    for (; pTypeInfo->type_id != 0;
	   pTypeInfo = (NE_TYPEINFO *)((char*)(pTypeInfo+1) +
					pTypeInfo->count * sizeof(NE_NAMEINFO)))
    {
	if (pTypeInfo->type_id != 0x800f) continue;
	pNameInfo = (NE_NAMEINFO *)(pTypeInfo + 1);
	for (count = pTypeInfo->count; count > 0; count--, pNameInfo++)
	{
            TRACE(resource, "NameTable entry: type=%04x id=%04x\n",
                              pTypeInfo->type_id, pNameInfo->id );
            handle = LoadResource16( pModule->self, 
				   (HRSRC16)((int)pNameInfo - (int)pModule) );
            for(p = (WORD*)LockResource16(handle); p && *p; p = (WORD *)((char*)p+*p))
            {
                TRACE(resource,"  type=%04x '%s' id=%04x '%s'\n",
                                  p[1], (char *)(p+3), p[2],
                                  (char *)(p+3)+strlen((char *)(p+3))+1 );
                /* Check for correct type */

                if (p[1] & 0x8000)
                {
                    if (!HIWORD(typeId)) continue;
                    if (lstrcmpi32A( typeId,
                                     (char *)(p + 3) )) continue;
                }
                else if (HIWORD(typeId) || (((DWORD)typeId & ~0x8000)!= p[1]))
                  continue;

                /* Now check for the id */

                if (p[2] & 0x8000)
                {
                    if (!HIWORD(resId)) continue;
                    if (lstrcmpi32A( resId,
                               (char*)(p+3)+strlen((char*)(p+3))+1 )) continue;
                    
                }
                else if (HIWORD(resId) || (((DWORD)resId & ~0x8000) != p[2]))
                  continue;

                /* If we get here, we've found the entry */

                TRACE(resource, "  Found!\n" );
                ret = MAKELONG( p[1], p[2] );
                break;
            }
            FreeResource16( handle );
            if (ret) return ret;
	}
    }
    return 0;
}

/***********************************************************************
 *           NE_FindTypeSection
 *
 * Find header struct for a particular resource type.
 */
static NE_TYPEINFO* NE_FindTypeSection( NE_MODULE *pModule, 
					NE_TYPEINFO *pTypeInfo, LPCSTR typeId )
{
    /* start from pTypeInfo */

    if (HIWORD(typeId) != 0)  /* Named type */
    {
	LPCSTR str = typeId;
	BYTE len = strlen( str );
	while (pTypeInfo->type_id)
	{
	    if (!(pTypeInfo->type_id & 0x8000))
	    {
		BYTE *p = (BYTE*)pModule + pModule->res_table + pTypeInfo->type_id;
		if ((*p == len) && !lstrncmpi32A( p+1, str, len ))
		{
		    TRACE(resource, "  Found type '%s'\n", str );
		    return pTypeInfo;
		}
	    }
	    TRACE(resource, "  Skipping type %04x\n", pTypeInfo->type_id );
	    pTypeInfo = NEXT_TYPEINFO(pTypeInfo);
	}
    }
    else  /* Numeric type id */
    {
	WORD id = LOWORD(typeId) | 0x8000;
	while (pTypeInfo->type_id)
	{
            if (pTypeInfo->type_id == id)
	    {
		TRACE(resource, "  Found type %04x\n", id );
		return pTypeInfo;
	    }
	    TRACE(resource, "  Skipping type %04x\n", pTypeInfo->type_id );
	    pTypeInfo = NEXT_TYPEINFO(pTypeInfo);
	}
    }
    return NULL;
}

/***********************************************************************
 *           NE_FindResourceFromType
 *
 * Find a resource once the type info structure has been found.
 */
static HRSRC16 NE_FindResourceFromType( NE_MODULE *pModule,
                                        NE_TYPEINFO *pTypeInfo, LPCSTR resId )
{
    BYTE *p;
    int count;
    NE_NAMEINFO *pNameInfo = (NE_NAMEINFO *)(pTypeInfo + 1);

    if (HIWORD(resId) != 0)  /* Named resource */
    {
        LPCSTR str = resId;
        BYTE len = strlen( str );
        for (count = pTypeInfo->count; count > 0; count--, pNameInfo++)
        {
            if (pNameInfo->id & 0x8000) continue;
            p = (BYTE *)pModule + pModule->res_table + pNameInfo->id;
            if ((*p == len) && !lstrncmpi32A( p+1, str, len ))
                return (HRSRC16)((int)pNameInfo - (int)pModule);
        }
    }
    else  /* Numeric resource id */
    {
        WORD id = LOWORD(resId) | 0x8000;
        for (count = pTypeInfo->count; count > 0; count--, pNameInfo++)
            if (pNameInfo->id == id) 
	      return (HRSRC16)((int)pNameInfo - (int)pModule);
    }
    return 0;
}


/***********************************************************************
 *           NE_DefResourceHandler
 *
 * This is the default LoadProc() function. 
 */
HGLOBAL16 WINAPI NE_DefResourceHandler( HGLOBAL16 hMemObj, HMODULE16 hModule,
                                        HRSRC16 hRsrc )
{
    int  fd;
    NE_MODULE* pModule = NE_GetPtr( hModule );
    if (pModule && (fd = NE_OpenFile( pModule )) >= 0)
    {
	HGLOBAL16 handle;
	WORD sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
	NE_NAMEINFO* pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);

        TRACE(resource, "loading, pos=%d, len=%d\n",
		     (int)pNameInfo->offset << sizeShift,
		     (int)pNameInfo->length << sizeShift );
	if( hMemObj )
	    handle = GlobalReAlloc16( hMemObj, pNameInfo->length << sizeShift, 0 );
	else
	    handle = AllocResource( hModule, hRsrc, 0 );

	if( handle )
	{
            lseek( fd, (int)pNameInfo->offset << sizeShift, SEEK_SET );
            read( fd, GlobalLock16( handle ), (int)pNameInfo->length << sizeShift );
	}
	return handle;
    }
    return (HGLOBAL16)0;
}

/***********************************************************************
 *           NE_InitResourceHandler
 *
 * Fill in 'resloader' fields in the resource table.
 */
BOOL32 NE_InitResourceHandler( HMODULE16 hModule )
{
    NE_MODULE *pModule = NE_GetPtr( hModule );
    NE_TYPEINFO *pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->res_table + 2);

    FARPROC16 handler = MODULE_GetWndProcEntry16("DefResourceHandler");

    TRACE(resource,"InitResourceHandler[%04x]\n", hModule );

    while(pTypeInfo->type_id)
    {
	pTypeInfo->resloader = handler;
	pTypeInfo = NEXT_TYPEINFO(pTypeInfo);
    }
    return TRUE;
}


/**********************************************************************
 *	SetResourceHandler	(KERNEL.43)
 */
FARPROC16 WINAPI SetResourceHandler( HMODULE16 hModule, SEGPTR typeId,
                                     FARPROC16 resourceHandler )
{
    FARPROC16 prevHandler = NULL;
    NE_MODULE *pModule = NE_GetPtr( hModule );
    NE_TYPEINFO *pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->res_table + 2);

    if (!pModule || !pModule->res_table) return NULL;

    TRACE( resource, "module=%04x type=%s\n",
           hModule, debugres_a(PTR_SEG_TO_LIN(typeId)) );

    for (;;)
    {
	if (!(pTypeInfo = NE_FindTypeSection( pModule, pTypeInfo, PTR_SEG_TO_LIN(typeId) )))
            break;
        prevHandler = pTypeInfo->resloader;
        pTypeInfo->resloader = resourceHandler;
        pTypeInfo = NEXT_TYPEINFO(pTypeInfo);
    }
    return prevHandler;
}


/**********************************************************************
 *	    NE_FindResource
 */
HRSRC16 NE_FindResource( NE_MODULE *pModule, LPCSTR name, LPCSTR type )
{
    NE_TYPEINFO *pTypeInfo;
    HRSRC16 hRsrc;

    if (!pModule || !pModule->res_table) return 0;

    TRACE( resource, "module=%04x name=%s type=%s\n", 
           pModule->self, debugres_a(PTR_SEG_TO_LIN(name)),
           debugres_a(PTR_SEG_TO_LIN(type)) );

    if (HIWORD(name))  /* Check for '#xxx' name */
    {
	LPCSTR ptr = name;
	if (ptr[0] == '#')
	    if (!(name = (LPCSTR)atoi( ptr + 1 )))
            {
                WARN(resource, "Incorrect resource name: %s\n", ptr);
                return 0;
	    }
    }

    if (HIWORD(type))  /* Check for '#xxx' type */
    {
	LPCSTR ptr = type;
	if (ptr[0] == '#')
            if (!(type = (LPCSTR)atoi( ptr + 1 )))
            {
                WARN(resource, "Incorrect resource type: %s\n", ptr);
                return 0;
            }
    }

    if (HIWORD(type) || HIWORD(name))
    {
        DWORD id = NE_FindNameTableId( pModule, type, name );
        if (id)  /* found */
        {
            type = (LPCSTR)(int)LOWORD(id);
            name = (LPCSTR)(int)HIWORD(id);
        }
    }

    pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->res_table + 2);

    for (;;)
    {
	if (!(pTypeInfo = NE_FindTypeSection( pModule, pTypeInfo, type )))
            break;
        if ((hRsrc = NE_FindResourceFromType(pModule, pTypeInfo, name)))
        {
            TRACE(resource, "    Found id %08lx\n", (DWORD)name );
            return hRsrc;
        }
        TRACE(resource, "    Not found, going on\n" );
        pTypeInfo = NEXT_TYPEINFO(pTypeInfo);
    }

    WARN(resource, "failed!\n");
    return 0;
}


/**********************************************************************
 *	    AllocResource    (KERNEL.66)
 */
HGLOBAL16 WINAPI AllocResource( HMODULE16 hModule, HRSRC16 hRsrc, DWORD size)
{
    NE_NAMEINFO *pNameInfo=NULL;
    WORD sizeShift;

    NE_MODULE *pModule = NE_GetPtr( hModule );
    if (!pModule || !pModule->res_table || !hRsrc) return 0;

    TRACE( resource, "module=%04x res=%04x size=%ld\n", hModule, hRsrc, size );

    sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
    pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);
    if (size < (DWORD)pNameInfo->length << sizeShift)
        size = (DWORD)pNameInfo->length << sizeShift;
    return GLOBAL_Alloc( GMEM_FIXED, size, hModule, FALSE, FALSE, FALSE );
}


/**********************************************************************
 *      DirectResAlloc    (KERNEL.168)
 *
 * Check Schulman, p. 232 for details
 */
HGLOBAL16 WINAPI DirectResAlloc( HINSTANCE16 hInstance, WORD wType,
                                 UINT16 wSize )
{
    TRACE(resource,"(%04x,%04x,%04x)\n",
                     hInstance, wType, wSize );
    if (!(hInstance = GetExePtr( hInstance ))) return 0;
    if(wType != 0x10)	/* 0x10 is the only observed value, passed from
                           CreateCursorIndirect. */
        TRACE(resource, "(wType=%x)\n", wType);
    return GLOBAL_Alloc(GMEM_MOVEABLE, wSize, hInstance, FALSE, FALSE, FALSE);
}


/**********************************************************************
 *	    NE_AccessResource
 */
INT16 NE_AccessResource( NE_MODULE *pModule, HRSRC16 hRsrc )
{
    HFILE16 fd;

    if (!pModule || !pModule->res_table || !hRsrc) return -1;

    TRACE(resource, "module=%04x res=%04x\n", pModule->self, hRsrc );

    if ((fd = _lopen16( NE_MODULE_NAME(pModule), OF_READ )) != HFILE_ERROR16)
    {
        WORD sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
        NE_NAMEINFO *pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);
        _llseek16( fd, (int)pNameInfo->offset << sizeShift, SEEK_SET );
    }
    return fd;
}


/**********************************************************************
 *	    NE_SizeofResource
 */
DWORD NE_SizeofResource( NE_MODULE *pModule, HRSRC16 hRsrc )
{
    NE_NAMEINFO *pNameInfo=NULL;
    WORD sizeShift;

    if (!pModule || !pModule->res_table) return 0;

    TRACE(resource, "module=%04x res=%04x\n", pModule->self, hRsrc );

    sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
    pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);
    return (DWORD)pNameInfo->length << sizeShift;
}


/**********************************************************************
 *	    NE_LoadResource
 */
HGLOBAL16 NE_LoadResource( NE_MODULE *pModule, HRSRC16 hRsrc )
{
    NE_TYPEINFO *pTypeInfo;
    NE_NAMEINFO *pNameInfo = NULL;
    int d;

    TRACE( resource, "module=%04x res=%04x\n", pModule->self, hRsrc );
    if (!hRsrc || !pModule || !pModule->res_table) return 0;

    /* First, verify hRsrc (just an offset from pModule to the needed pNameInfo) */

    d = pModule->res_table + 2;
    pTypeInfo = (NE_TYPEINFO *)((char *)pModule + d);
    while( hRsrc > d )
    {
	if (pTypeInfo->type_id == 0)
		break; /* terminal entry */
	d += sizeof(NE_TYPEINFO) + pTypeInfo->count * sizeof(NE_NAMEINFO);
	if (hRsrc < d)
	{
	    if( ((d - hRsrc)%sizeof(NE_NAMEINFO)) == 0 )
	    {
		pNameInfo = (NE_NAMEINFO *)(((char *)pModule) + hRsrc);
		break;
	    }
	    else 
		break; /* NE_NAMEINFO boundary mismatch */
	}
	pTypeInfo = (NE_TYPEINFO *)(((char *)pModule) + d);
    }

    if (pNameInfo)
    {
	if (pNameInfo->handle
	    && !(GlobalFlags16(pNameInfo->handle) & GMEM_DISCARDED))
	{
	    pNameInfo->usage++;
	    TRACE(resource, "  Already loaded, new count=%d\n",
			      pNameInfo->usage );
	}
	else
	{
	    if (pTypeInfo->resloader)
                pNameInfo->handle = Callbacks->CallResourceHandlerProc(
                    pTypeInfo->resloader, pNameInfo->handle, pModule->self, hRsrc );
	    else /* this is really bad */
	    {
		ERR(resource, "[%04x]: Missing resource handler!\n", pModule->self);
                pNameInfo->handle = NE_DefResourceHandler(
                                         pNameInfo->handle, pModule->self, hRsrc );
	    }

	    if (pNameInfo->handle)
	    {
		pNameInfo->usage++;
		pNameInfo->flags |= NE_SEGFLAGS_LOADED;
	    }
	}
	return pNameInfo->handle;
    }
    return 0;
}


/**********************************************************************
 *	    NE_FreeResource
 */
BOOL16 NE_FreeResource( NE_MODULE *pModule, HGLOBAL16 handle )
{
    NE_TYPEINFO *pTypeInfo;
    NE_NAMEINFO *pNameInfo;
    WORD count;

    if (!handle || !pModule || !pModule->res_table) return handle;

    TRACE(resource, "handle=%04x\n", handle );

    pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->res_table + 2);
    while (pTypeInfo->type_id)
    {
        pNameInfo = (NE_NAMEINFO *)(pTypeInfo + 1);
        for (count = pTypeInfo->count; count > 0; count--)
        {
            if (pNameInfo->handle == handle)
            {
                if (pNameInfo->usage > 0) pNameInfo->usage--;
                if (pNameInfo->usage == 0)
                {
                    GlobalFree16( pNameInfo->handle );
                    pNameInfo->handle = 0;
		    pNameInfo->flags &= ~NE_SEGFLAGS_LOADED;
                }
                return 0;
            }
            pNameInfo++;
        }
        pTypeInfo = (NE_TYPEINFO *)pNameInfo;
    }

    TRACE(resource, "[%04x]: no intrinsic resource for %04x, assuming DirectResAlloc()!\n", 
          pModule->self, handle );
    GlobalFree16( handle ); 
    return handle;
}

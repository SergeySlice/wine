/*
 * WineCfg Staging panel
 *
 * Copyright 2014 Michael Müller
 * Copyright 2015 Sebastian Lackner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#define COBJMACROS

#include "config.h"

#include <windows.h>
#include <wine/unicode.h>
#include <wine/debug.h>

#include "resource.h"
#include "winecfg.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

/*
 * Command stream multithreading
 */
static BOOL csmt_get(void)
{
    BOOL ret;
    char *value = get_reg_key(config_key, keypath("DllRedirects"), "wined3d", NULL);
    ret = (value && !strcmp(value, "wined3d-csmt.dll"));
    HeapFree(GetProcessHeap(), 0, value);
    return ret;
}
static void csmt_set(BOOL status)
{
    set_reg_key(config_key, keypath("DllRedirects"), "wined3d", status ? "wined3d-csmt.dll" : NULL);
}

//Slice - switch for AlwaysOffscreen
static BOOL aos_get(void)
{
    BOOL ret;
    char *value = get_reg_key(config_key, keypath("Direct3D"), "AlwaysOffscreen", "disabled");
    ret = (value && !strcmp(value, "enabled"));
    HeapFree(GetProcessHeap(), 0, value);
    return ret;
}
static void aos_set(BOOL status)
{
    set_reg_key(config_key, keypath("Direct3D"), "AlwaysOffscreen", status ? "enabled" : "disabled");
}

//Slice - switch for UseGLSL
static BOOL glsl_get(void)
{
    BOOL ret;
    char *value = get_reg_key(config_key, keypath("Direct3D"), "UseGLSL", "enabled");
    ret = (value && !strcmp(value, "enabled"));
    HeapFree(GetProcessHeap(), 0, value);
    return ret;
}
static void glsl_set(BOOL status)
{
    set_reg_key(config_key, keypath("Direct3D"), "UseGLSL", status ? "enabled" : "disabled");
}

//Slice - switch for StrictDrawOrdering
static BOOL sdo_get(void)
{
    BOOL ret;
    char *value = get_reg_key(config_key, keypath("Direct3D"), "StrictDrawOrdering", "disabled");
    ret = (value && !strcmp(value, "enabled"));
    HeapFree(GetProcessHeap(), 0, value);
    return ret;
}
static void sdo_set(BOOL status)
{
    set_reg_key(config_key, keypath("Direct3D"), "StrictDrawOrdering", status ? "enabled" : "disabled");
}

//Slice - OffscreenRenderingMode
static BOOL fbo_get(void)
{
    BOOL ret;
    char *value = get_reg_key(config_key, keypath("Direct3D"), "OffscreenRenderingMode", "fbo");
    ret = (value && !strcmp(value, "fbo"));
    HeapFree(GetProcessHeap(), 0, value);
    return ret;
}
static void fbo_set(BOOL status)
{
    set_reg_key(config_key, keypath("Direct3D"), "OffscreenRenderingMode", status ? "fbo" : "backbuffer");
}

//Slice - DirectDrawRenderer
static BOOL ogl_get(void)
{
    BOOL ret;
    char *value = get_reg_key(config_key, keypath("Direct3D"), "DirectDrawRenderer", "opengl");
    ret = (value && !strcmp(value, "opengl"));
    HeapFree(GetProcessHeap(), 0, value);
    return ret;
}
static void ogl_set(BOOL status)
{
    set_reg_key(config_key, keypath("Direct3D"), "DirectDrawRenderer", status ? "opengl" : "gdi");
}

static BOOL cfc_get(void)
{
    BOOL ret;
    char *value = get_reg_key(config_key, keypath("Direct3D"), "CheckFloatConstants", "enabled");
    ret = (value && !strcmp(value, "enabled"));
    HeapFree(GetProcessHeap(), 0, value);
    return ret;
}
static void cfc_set(BOOL status)
{
    set_reg_key(config_key, keypath("Direct3D"), "CheckFloatConstants", status ?  "enabled" : "disabled");
}

static void load_staging_settings(HWND dialog)
{
    CheckDlgButton(dialog, IDC_ENABLE_CSMT, csmt_get() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_ENABLE_AOS,   aos_get() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_ENABLE_GLSL, glsl_get() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_ENABLE_SDO,   sdo_get() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_ENABLE_FBO,   fbo_get() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_ENABLE_OGL,   ogl_get() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_ENABLE_CFC,   cfc_get() ? BST_CHECKED : BST_UNCHECKED);
}

INT_PTR CALLBACK StagingDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        break;

    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->code == PSN_SETACTIVE)
            load_staging_settings(hDlg);
        break;

    case WM_SHOWWINDOW:
        set_window_title(hDlg);
        break;

    case WM_DESTROY:
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) != BN_CLICKED) break;
        switch (LOWORD(wParam))
        {
            case IDC_ENABLE_CSMT:
                csmt_set(IsDlgButtonChecked(hDlg, IDC_ENABLE_CSMT) == BST_CHECKED);
                SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                return TRUE;
            case IDC_ENABLE_AOS:
                aos_set(IsDlgButtonChecked(hDlg, IDC_ENABLE_AOS) == BST_CHECKED);
                SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                return TRUE;                
            case IDC_ENABLE_GLSL:
                glsl_set(IsDlgButtonChecked(hDlg, IDC_ENABLE_GLSL) == BST_CHECKED);
                SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                return TRUE;
            case IDC_ENABLE_SDO:
                sdo_set(IsDlgButtonChecked(hDlg, IDC_ENABLE_SDO) == BST_CHECKED);
                SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                return TRUE;
            case IDC_ENABLE_FBO:
                fbo_set(IsDlgButtonChecked(hDlg, IDC_ENABLE_FBO) == BST_CHECKED);
                SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                return TRUE;
            case IDC_ENABLE_OGL:
                ogl_set(IsDlgButtonChecked(hDlg, IDC_ENABLE_OGL) == BST_CHECKED);
                SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                return TRUE;
            case IDC_ENABLE_CFC:
                cfc_set(IsDlgButtonChecked(hDlg, IDC_ENABLE_CFC) == BST_CHECKED);
                SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                return TRUE;
        }
        break;
    }
    return FALSE;
}

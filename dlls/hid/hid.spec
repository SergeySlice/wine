@ stub HidD_FlushQueue
@ stdcall HidD_FreePreparsedData(ptr)
@ stdcall HidD_GetAttributes(long ptr)
@ stub HidD_GetConfiguration
@ stdcall HidD_GetFeature(long ptr long)
@ stdcall HidD_GetHidGuid(ptr)
@ stub HidD_GetIndexedString
@ stdcall HidD_GetInputReport(long ptr long)
@ stdcall HidD_GetManufacturerString(long ptr long)
@ stub HidD_GetMsGenreDescriptor
@ stdcall HidD_GetNumInputBuffers(long ptr)
@ stub HidD_GetPhysicalDescriptor
@ stdcall HidD_GetPreparsedData(ptr ptr)
@ stdcall HidD_GetProductString(long ptr long)
@ stdcall HidD_GetSerialNumberString(long ptr long)
@ stub HidD_Hello
@ stub HidD_SetConfiguration
@ stdcall HidD_SetFeature(long ptr long)
@ stdcall HidD_SetNumInputBuffers(long long)
@ stdcall HidD_SetOutputReport(long ptr long)
@ stdcall HidP_GetButtonCaps(long ptr ptr ptr)
@ stdcall HidP_GetCaps(ptr ptr)
@ stdcall HidP_GetData(long ptr ptr ptr ptr long)
@ stub HidP_GetExtendedAttributes
@ stub HidP_GetLinkCollectionNodes
@ stdcall HidP_GetScaledUsageValue(long long long long ptr ptr ptr long)
@ stdcall HidP_GetSpecificButtonCaps(long long long long ptr ptr ptr)
@ stdcall HidP_GetSpecificValueCaps(long long long long ptr ptr ptr)
@ stdcall HidP_GetUsageValue(long long long long ptr ptr ptr long)
@ stub HidP_GetUsageValueArray
@ stdcall HidP_GetUsages(long long long ptr ptr ptr ptr long)
@ stdcall HidP_GetUsagesEx(long long ptr ptr ptr ptr long)
@ stdcall HidP_GetValueCaps(long ptr ptr ptr)
@ stdcall HidP_InitializeReportForID(long long ptr ptr long)
@ stdcall HidP_MaxDataListLength(long ptr)
@ stdcall HidP_MaxUsageListLength(long long ptr)
@ stub HidP_SetData
@ stub HidP_SetScaledUsageValue
@ stdcall HidP_SetUsageValue(long long long long long ptr ptr long)
@ stub HidP_SetUsageValueArray
@ stub HidP_SetUsages
@ stdcall HidP_TranslateUsagesToI8042ScanCodes(ptr long long ptr ptr ptr)
@ stub HidP_UnsetUsages
@ stub HidP_UsageListDifference

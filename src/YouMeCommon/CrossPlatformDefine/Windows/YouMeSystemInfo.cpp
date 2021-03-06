//
//  Application.mm
//  YouMeVoiceEngine
//
//  Created by wanglei on 15/9/17.
//  Copyright (c) 2015年 tencent. All rights reserved.
//
#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <algorithm>
#include <iphlpapi.h>
#include <wbemidl.h>
#include <comutil.h>

#include "YouMeRegistry.h"
#include "YouMeSystemInfo.h"
#include <YouMeCommon/CryptUtil.h>
#include "../PlatformDef.h"

#ifdef _DEBUG
#pragma comment(lib, "comsuppwd.lib")
#else
#pragma comment(lib, "comsuppw.lib")
#endif // _DEBUG

#pragma comment(lib,"Iphlpapi.lib")
#pragma comment(lib, "wbemuuid.lib")

#define ERROR_GUID_OK 0x0
#define ERROR_GUID_CPU 0x1
#define ERROR_GUID_BD 0x2
#define ERROR_GUID_MAC 0x4
//////////////////////////////////////////////////////////////////////////

#define PAGE_SIZE 0x1000
std::wstring BlStrFormatW(const WCHAR* szFormat, ...)
{
	const ULONG maxsize = PAGE_SIZE;
	WCHAR szBuffer[maxsize + 1] = { 0 };

	va_list argList;
	va_start(argList, szFormat);

	_vsnwprintf_s(szBuffer, maxsize, szFormat, argList);

	va_end(argList);

	return szBuffer;
}

std::wstring BlStrToUpperW(LPCWSTR pString)
{
	std::wstring str = pString;
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);

	return str.c_str();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define NetworkDeviceKey \
    L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002bE10318}"

ULONG GetDeviceCharact(LPCWSTR TargetId)
{
    HANDLE RegHandle = NULL;
    LSTATUS lResult = ERROR_SUCCESS;
    ULONG SubkeyCount = 0, MaxSubkeyLen = 0;
    ULONG i = 0;
    TCHAR SubkeyName[16] = {0};
    ULONG SubkeyLen;
    WCHAR SubkeyValue[MAX_PATH] = {0};
    
    RegHandle = BlRegOpenKey(
                             HKEY_LOCAL_MACHINE,
                             NetworkDeviceKey,
                             KEY_READ,
                             FALSE
                             );
    if (!RegHandle)
    {
        return 0;
    }
    
    lResult = BlRegQuerySubkey(
                               RegHandle,
                               &SubkeyCount,
                               &MaxSubkeyLen
                               );
    if (lResult != ERROR_SUCCESS)
    {
        return 0;
    }
    
    for(i = 0; i < SubkeyCount; i++)
    {
        SubkeyLen = MaxSubkeyLen * sizeof(TCHAR);
        
        lResult = BlRegEnumSubkey(
                                  RegHandle,
                                  i,
                                  SubkeyName,
                                  &SubkeyLen
                                  );
        if (lResult == ERROR_SUCCESS)
        {
            std::wstring subkey = BlStrFormatW(
                                               L"%s\\%s",
                                               NetworkDeviceKey,
                                               SubkeyName
                                               ).data();
            
            BlRegGetString(
                           HKEY_LOCAL_MACHINE,
                           subkey.c_str(),
                           L"NetCfgInstanceId",
                           SubkeyValue
                           );
            if (!_wcsicmp(SubkeyValue, TargetId))
            {
                return BlRegGetDword(
                                     HKEY_LOCAL_MACHINE,
                                     subkey.c_str(),
                                     L"Characteristics"
                                     );
            }
        }
    }
    
    return 0;
}

#define NCF_PHYSICAL 0x4

BOOL DeviceIsPhysical(std::wstring id)
{
    ULONG c = GetDeviceCharact(id.c_str());
    
    return (c == 0) || ((c & NCF_PHYSICAL) == NCF_PHYSICAL);
}

BOOL MacIsModem(BYTE* Address)
{
    std::wstring straddr = BlStrFormatW(
                                        L"%02X%02X%02X%02X%02X%02X",
                                        Address[0], Address[1],
                                        Address[2], Address[3],
                                        Address[4], Address[5]
                                        ).data();
    
    return (straddr == L"005345000000");
}

std::wstring GetMacAddrs(__inout PULONG ErrorCode)
{
    std::wstring ret = L"";
    
    PIP_ADAPTER_INFO pAdapterInfos = NULL;
    PIP_ADAPTER_INFO pAdapter = NULL;
    ULONG ulLen = 0;
    
    GetAdaptersInfo(pAdapterInfos, &ulLen);
    pAdapterInfos = (PIP_ADAPTER_INFO)new char[ulLen];
    
    RtlZeroMemory(pAdapterInfos, ulLen);
    
    if (GetAdaptersInfo(pAdapterInfos, &ulLen) == ERROR_SUCCESS)
    {
        pAdapter = pAdapterInfos;
        
        while (pAdapter)
        {
			std::string strAdapterName = pAdapter->AdapterName;
			std::wstring macaddr = UTF8TOXString(strAdapterName);
            
            if(DeviceIsPhysical(macaddr)&&(!MacIsModem(pAdapter->Address)))
            {
                if (pAdapter->AddressLength >= 6)
                {
                    std::wstring straddr = BlStrFormatW(
                                                        L"%02X%02X%02X%02X%02X%02X",
                                                        pAdapter->Address[0], pAdapter->Address[1],
                                                        pAdapter->Address[2], pAdapter->Address[3],
                                                        pAdapter->Address[4], pAdapter->Address[5]
                                                        ).data();
                    
                    if(ret.size() > 0)
                    {
                        ret += ';';
                    }
                    
                    ret += straddr;
                }
            }
            pAdapter = pAdapter->Next;
        }
    }
    
    if (ret.empty() && ErrorCode)
    {
        *ErrorCode |= ERROR_GUID_MAC;
    }
    
	if (pAdapterInfos != NULL)
	{
		delete[]((CHAR*)pAdapterInfos);
	}
    
    return ret;
}

//通过GetAdaptersInfo函数（适用于Windows 2000及以上版本）
std::wstring GetMacAddressByAdaptersInfo()
{
	std::wstring result;
	ULONG out_buf_len = 0;
	PIP_ADAPTER_INFO adapter_info = NULL;
	GetAdaptersInfo(adapter_info, &out_buf_len);
	adapter_info = (IP_ADAPTER_INFO *)malloc(out_buf_len);
	if (adapter_info == NULL)
	{
		return result;
	}
	RtlZeroMemory(adapter_info, out_buf_len);
	result.resize(32);

	if (GetAdaptersInfo(adapter_info, &out_buf_len) == NO_ERROR)
	{
		PIP_ADAPTER_INFO adapter = adapter_info;
		for (; adapter != NULL; adapter = adapter->Next)
		{
			// 确保是以太网
			if (adapter->Type != MIB_IF_TYPE_ETHERNET)
				continue;
			// 确保MAC地址的长度为 00-00-00-00-00-00
			if (adapter->AddressLength != 6)
				continue;

			swprintf(&result[0], 32, L"%02x%02x%02x%02x%02x%02x",
				int(adapter->Address[0]),
				int(adapter->Address[1]),
				int(adapter->Address[2]),
				int(adapter->Address[3]),
				int(adapter->Address[4]),
				int(adapter->Address[5]));

			break;
		}
	}

	free(adapter_info);

	return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _CPU_REGS
{
	INT Eax;
	INT Ebx;
	INT Ecx;
	INT Edx;

}CPU_REGS, *PCPU_REGS;

BOOL
BlpCallCpuId(
CPU_REGS* CpuReg,
ULONG CmdId
)
{
	INT Reg[4] = { 0 };

	__try
	{
		__cpuid(Reg, CmdId);

		CpuReg->Eax = Reg[0];
		CpuReg->Ebx = Reg[1];
		CpuReg->Ecx = Reg[2];
		CpuReg->Edx = Reg[3];
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return FALSE;
	}

	return TRUE;
}


std::wstring GetCPUID(__inout PULONG ErrorCode)
{
    CPU_REGS CpuReg = {0};
    WCHAR idstr[64] = {0};
    
    BlpCallCpuId(&CpuReg, 1);
    
    if (!CpuReg.Eax && ErrorCode)
    {
        *ErrorCode |= ERROR_GUID_CPU;
    }
    
    StringCchPrintfW(idstr, 64, L"%0.8X", CpuReg.Eax);
    
    return idstr;
}

const std::wstring GetBoardSN(__inout PULONG ErrorCode)
{
    HRESULT hres = E_FAIL;
    HRESULT hInit = E_FAIL;
    IWbemLocator *pLoc = NULL;
    IWbemServices *pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;
    IWbemClassObject *pclsObj = NULL;
    std::wstring ret = L"";
    
    do
    {
        hInit = CoInitializeEx(0, COINIT_MULTITHREADED);
        
        if (FAILED(hInit) && (hInit != RPC_E_CHANGED_MODE))
        {
            break;
        }
        
        hres = CoInitializeSecurity(
                                    NULL,
                                    -1,
                                    NULL,
                                    NULL,
                                    RPC_C_AUTHN_LEVEL_DEFAULT,
                                    RPC_C_IMP_LEVEL_IMPERSONATE,
                                    NULL,
                                    EOAC_NONE,
                                    NULL
                                    );
        if (FAILED(hres) && RPC_E_TOO_LATE != hres)
        {
            break;
        }
        
        hres = CoCreateInstance(
                                CLSID_WbemLocator,
                                0,
                                CLSCTX_INPROC_SERVER,
                                IID_IWbemLocator,
                                (LPVOID*)&pLoc
                                );
        if (FAILED(hres))
        {
            break;
        }
        
        hres = pLoc->ConnectServer(
                                   _bstr_t(L"ROOT\\CIMV2"),
                                   NULL,
                                   NULL,
                                   0,
                                   NULL,
                                   0,
                                   0,
                                   &pSvc
                                   );
        if (FAILED(hres))
        {
            break;
        }
        
        hres = CoSetProxyBlanket(
                                 pSvc,
                                 RPC_C_AUTHN_WINNT,
                                 RPC_C_AUTHZ_NONE,
                                 NULL,
                                 RPC_C_AUTHN_LEVEL_CALL,
                                 RPC_C_IMP_LEVEL_IMPERSONATE,
                                 NULL,
                                 EOAC_NONE
                                 );
        if (FAILED(hres))
        {
            break;
        }
        
        hres = pSvc->ExecQuery(
                               bstr_t("WQL"),
                               bstr_t("SELECT SerialNumber FROM Win32_BaseBoard"),
                               WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                               NULL,
                               &pEnumerator
                               );
        if (FAILED(hres))
        {
            break;
        }
        
        ULONG uReturn = 0;
        
        while (pEnumerator)
        {
            HRESULT hr = pEnumerator->Next(
                                           WBEM_INFINITE,
                                           1,
                                           &pclsObj,
                                           &uReturn
                                           );
            if(0 == uReturn)
            {
                break;
            }
            
            VARIANT vtProp;
            
            hr = pclsObj->Get(
                              L"SerialNumber",
                              0, 
                              &vtProp,
                              0, 0
                              );
            if(WBEM_S_NO_ERROR == hr)
            {
                if(VT_BSTR == vtProp.vt)
                {
                    ret = vtProp.bstrVal;
                }
            }
            VariantClear(&vtProp);
        }
        
    } while (FALSE);    
    
    if(pSvc != NULL)
    {
        pSvc->Release();
    }
    if(pLoc != NULL)
    {
        pLoc->Release();
    }
    if(NULL != pEnumerator)
    {
        pEnumerator->Release();
    }
    if(NULL != pclsObj)
    {
        pclsObj->Release();
    }
    if(hInit != RPC_E_CHANGED_MODE)
    {
        CoUninitialize();
    }
    
    ret = BlStrToUpperW(ret.c_str()).data();
    
    if(ret == L"NONE")
    {
        ret = L"";
    }
    
    if (ret.empty() && ErrorCode)
    {
        *ErrorCode |= ERROR_GUID_BD;
    }
    
    return ret;
}

const std::wstring
GenerateSysUUID(
               __inout PULONG ErrorCode
               )
{
    /*std::wstring cpuid= GetCPUID(ErrorCode);
    std::wstring boardsn = GetBoardSN(ErrorCode);
    std::wstring macaddr = GetMacAddrs(ErrorCode);
    
    BOOL bRet = FALSE;
    WCHAR szBuffer[1024] = {0};
    std::wstring RetGuid = L"";
    
    HRESULT nResult = StringCchPrintfW(
                                       szBuffer,
                                       1024,
                                       L"{%s-%s-%s}",
                                       cpuid.c_str(),
                                       boardsn.c_str(),
                                       macaddr.c_str()
                                       );
    if(SUCCEEDED(nResult))
    {
		std::wstring strBuffer = szBuffer;
		std::string strAnsi = XStringToUTF8(strBuffer);
        
		RetGuid = youmecommon::CCryptUtil::MD5(strAnsi);
    }
    
    return RetGuid.c_str();*/


	std::wstring macaddr = GetMacAddressByAdaptersInfo();
	if (!macaddr.empty())
	{
		return macaddr;
	}
	/*macaddr = GetMacAddressByNetBIOS();
	if (!macaddr.empty())
	{
		return macaddr;
	}*/
	macaddr = GetBoardSN(ErrorCode);
	return macaddr;
}
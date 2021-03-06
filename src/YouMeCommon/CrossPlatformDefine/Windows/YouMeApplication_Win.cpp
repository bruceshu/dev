//
//  Application.mm
//  YouMeVoiceEngine
//
//  Created by wanglei on 15/9/17.
//  Copyright (c) 2015年 tencent. All rights reserved.
//
#include <windows.h>
#include <iostream>
#include <string>
#include <strsafe.h>
#include <ShlObj.h>
#include "YouMeApplication_Win.h"
#include "YouMeSystemInfo.h"

YouMeApplication_Win::YouMeApplication_Win ()
{

}

YouMeApplication_Win::~YouMeApplication_Win ()
{
}


XString YouMeApplication_Win::getCpuChip()
{
	return __XT("");
}

XString YouMeApplication_Win::getBrand()
{
	return L"Microsoft";
}

XString YouMeApplication_Win::getSystemVersion()
{
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	BOOL bOsVersionInfoEx;
	ZeroMemory(&si, sizeof(SYSTEM_INFO));
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO *)&osvi);
	if (!bOsVersionInfoEx)
	{
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if (!GetVersionEx((OSVERSIONINFO *)&osvi))
			return L"";
	}

	wchar_t wchVer[32] = { 0 };
	StringCchPrintfW(wchVer, 32, L"%d.%d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);

	return wchVer;
}

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL(WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);
std::wstring YouMeApplication_Win::getCpuArchive()
{
	SYSTEM_INFO si;
	memset(&si, 0, sizeof(SYSTEM_INFO));
	PGNSI pGNSI = (PGNSI)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
	if (NULL != pGNSI)
	{
		pGNSI(&si);
	}
	else
	{
		GetSystemInfo(&si);
	}

	switch (si.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_INTEL:
	{
		return L"x86";
	}
	case PROCESSOR_ARCHITECTURE_AMD64:
	{
		return L"x86_64";
	}
	case PROCESSOR_ARCHITECTURE_IA64:
	{
		return L"IA64";
	}
	case PROCESSOR_ARCHITECTURE_ARM:
	{
		return L"ARM";
	}
	default:
		break;
	}

	return L"unknown";
}

XString YouMeApplication_Win::getPackageName()
{
	return L"";
}

XString YouMeApplication_Win::getUUID()
{
	return GenerateSysUUID(NULL);
}

XString YouMeApplication_Win::getModel()
{
	return L"";
}

XString YouMeApplication_Win::getDocumentPath()
{
    WCHAR szSystem32Path[MAX_PATH] = {0};
	SHGetSpecialFolderPathW(NULL, szSystem32Path, CSIDL_LOCAL_APPDATA, FALSE);
    std::wstring wstrSystem32Path = szSystem32Path;
    return wstrSystem32Path;
}

XString YouMeApplication_Win::getCachePath()
{
	WCHAR szSystem32Path[MAX_PATH] = { 0 };
	SHGetSpecialFolderPathW(NULL, szSystem32Path, CSIDL_LOCAL_APPDATA, FALSE);
	std::wstring wstrSystem32Path = szSystem32Path;
	return wstrSystem32Path;
}

/**************************** 条件编译选项和头文件 ****************************/
#ifdef WIN32

#pragma warning(disable:4267)
#include <tchar.h>

#endif // end of WIN32

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <regex>
#include "StringUtil.hpp"
#include <YouMeCommon/XSharedArray.h>


/********************************** 宏、常量 **********************************/

/********************************** 数据类型 **********************************/
#ifndef ANDROID_LLVM
#if defined(ANDROID) || defined(__ANDROID__)
int wcscasecmp(const wchar_t *s1, const wchar_t *s2)
{
    while (towlower(*s1) == towlower(*s2))
    {
        if (0 == *s1)
        {
            return 0;
        }
        s1++;
        s2++;
    }
    
    return(towlower(*s1) - towlower(*s2));
}
#endif
#endif
template<>
std::string CStringUtilT<char>::to_string(unsigned int i)
{
    char out[16];
#if defined(WIN32)
    _ultoa(i, out, 10);
#else
    sprintf(out, "%u", i);
#endif
    return out;
}

template<>
std::wstring CStringUtilT<wchar_t>::to_string(unsigned int i)
{
    wchar_t out[16];
#if defined(WIN32)
    _ultow(i, out, 10);
#else
    swprintf(out, sizeof(out) / sizeof(wchar_t), L"%u", i);
#endif
    return out;
}

template<>
std::string CStringUtilT<char>::to_string(int i)
{
    char out[16];
#if defined(WIN32)
    _ltoa(i, out, 10);
#else
    sprintf(out, "%d", i);
#endif	
    return out; 
}

template<>
std::wstring CStringUtilT<wchar_t>::to_string(int i)
{
    wchar_t out[16];
#if defined(WIN32)
    _ltow(i, out, 10);
#else
    swprintf(out, sizeof(out) / sizeof(wchar_t), L"%d", i);
#endif	
    return out; 
}

template<>
std::string CStringUtilT<char>::to_string(XUINT64 i)
{
    char out[32];
#if defined(WIN32)
    _ui64toa(i, out, 10);
#else
    sprintf(out, "%llu", (long long unsigned int)i);
#endif
    return out;
}

template<>
std::wstring CStringUtilT<wchar_t>::to_string(XUINT64 i)
{
    wchar_t out[32];
#if defined(WIN32)
    _ui64tow(i, out, 10);
#else
    swprintf(out, sizeof(out) / sizeof(wchar_t), L"%llu", (long long unsigned int)i);
#endif
    return out;
}

template<>
std::string CStringUtilT<char>::to_string(XINT64 i)
{
    char out[32];
#if defined(WIN32)
    _i64toa(i, out, 10);
#else
    sprintf(out, "%lld", (long long int)i);
#endif
    return out;
}

template<>
std::wstring CStringUtilT<wchar_t>::to_string(XINT64 i)
{
    wchar_t out[32];
#if defined(WIN32)
    _i64tow(i, out, 10);
#else
    swprintf(out, sizeof(out) / sizeof(wchar_t), L"%lld", (long long int)i);
#endif
    return out;
}


template<>
unsigned int CStringUtilT<char>::str_to_uint32(const char* s)
{
#if defined(WIN32)
    return atol(s);
#else
    unsigned int out = 0;
    sscanf(s, "%u", &out);
    return out;
#endif
}

template<>
unsigned int CStringUtilT<wchar_t>::str_to_uint32(const wchar_t* s)
{
#if defined(WIN32)
    return _wtol(s);
#else
    unsigned int out = 0;
    swscanf(s, L"%u", &out);
    return out;
#endif
}

template<>
int CStringUtilT<char>::str_to_sint32(const char* s)
{
#if defined(WIN32)
    return atol(s);
#else
    return atoi(s);
#endif
}

template<>
int CStringUtilT<wchar_t>::str_to_sint32(const wchar_t* s)
{
#if defined(WIN32)
    return _wtol(s);
#else
    int out = 0;
    swscanf(s, L"%i", &out);
    return out;
#endif
}

template<>
int CStringUtilT<char>::compare_nocase(const char* str1, const char* str2)
{
#if defined(WIN32)
	return _stricmp(str1, str2);
#else
	return strcasecmp(str1, str2);
#endif
}

template<>
int CStringUtilT<wchar_t>::compare_nocase(const wchar_t* str1, const wchar_t* str2)
{
#if defined(WIN32)
	return _wcsicmp(str1, str2);
#else
	return wcscasecmp(str1, str2);
#endif
}

template<>
XUINT64 CStringUtilT<char>::str_to_uint64(const char* s)
{
#if defined(WIN32)
    return _atoi64(s);
#else	
    long long unsigned int out = 0;
    sscanf(s, "%llu", &out);
	return (XUINT64)out;
#endif
}

template<>
XUINT64 CStringUtilT<wchar_t>::str_to_uint64(const wchar_t* s)
{
#if defined(WIN32)
    return _wtoi64(s);
#else	
    long long unsigned int out = 0;
    swscanf(s, L"%llu", &out);
    return (XUINT64)out;
#endif
}

template<>
XINT64 CStringUtilT<char>::str_to_sint64(const char* s)
{
#if defined(WIN32)
    return _atoi64(s);
#else
    return (XINT64)atoll(s);
#endif
}

template<>
XINT64 CStringUtilT<wchar_t>::str_to_sint64(const wchar_t* s)
{
#if defined(WIN32)
    return _wtoi64(s);
#else
    long long int out = 0;
    swscanf(s, L"%llu", &out);
    return (XINT64)out;    
#endif
}


template<>
bool CStringUtilT<char>::str_to_bool(const char* s)
{
#if defined(WIN32)
    return ( 0 == _stricmp(s, "true") || 0 == _stricmp(s, "1") );
#else
    return (0 == strcasecmp(s, "true") || 0 == strcasecmp(s, "1"));
#endif
}



template<>
bool CStringUtilT<wchar_t>::str_to_bool(const wchar_t* s)
{
#if defined(WIN32)
    return ( 0 == _wcsicmp(s, L"true") || 0 == _wcsicmp(s, L"1") );
#else
    return ( 0 == wcscasecmp(s, L"true") || 0 == wcscasecmp(s, L"1") );
#endif
}

template<>
double CStringUtilT<char>::str_to_double(const char* s)
{
	return atof(s);
}

template<>
double CStringUtilT<wchar_t>::str_to_double(const wchar_t* s)
{
#if defined(WIN32)
	return _wtof(s);
#else
	double out = 0;
	swscanf(s, L"%f", &out);
	return out;
#endif
}

template<>
std::string CStringUtilT<char>::to_string(double i)
{
	char out[32] = { 0 };
#if defined(WIN32)
	sprintf_s(out, 32, "%f", i);
#else
	snprintf(out, 32, "%f", i);
#endif
	return std::string(out);
}

template<>
std::wstring CStringUtilT<wchar_t>::to_string(double i)
{
	wchar_t out[32] = { 0 };
	swprintf(out, sizeof(out) / sizeof(*out), L"%f", i);
	return std::wstring(out);
}

template<>
std::string CStringUtilT<char>::formatString(const char* format, ...)
{
	int nSize = 0;
	size_t nBuffsize = 128;
	char* pszData = (char*)malloc(nBuffsize);
	memset(pszData, 0, nBuffsize);
	while (true)
	{
		va_list args;
		va_start(args, format);
#ifdef WIN32
		nSize = _vsnprintf_s(pszData, nBuffsize, _TRUNCATE, format, args);
#else
		nSize = vsnprintf(pszData, nBuffsize, format, args);
#endif
		if (nSize == -1 || nSize > nBuffsize)
		{
			nBuffsize *= 2;
			if (nBuffsize > 65535)
			{
				break;
			}
			pszData = (char*)realloc(pszData, nBuffsize);
		}
		else
		{
			va_end(args);
			std::string strResult(pszData);
			free(pszData);
			return strResult;
		}
		va_end(args);
	}
	free(pszData);
	return std::string();
}

template<>
std::wstring CStringUtilT<wchar_t>::formatString(const wchar_t* format, ...)
{
	int nSize = 0;
	size_t nBuffsize = 128;
	wchar_t* pszData = (wchar_t*)malloc(nBuffsize * sizeof(wchar_t));
	memset(pszData, 0, nBuffsize * sizeof(wchar_t));

	while (true)
	{
		va_list args;
		va_start(args, format);

#ifdef WIN32
		nSize = _vsnwprintf_s(pszData, nBuffsize, _TRUNCATE, format, args);
#else
		nSize = vswprintf(pszData, nBuffsize, format, args);
#endif
		if (nSize > 0)
		{
			va_end(args);
			std::wstring strResult(pszData);
			free(pszData);
			return strResult;
		}
		else
		{
			nBuffsize *= 2;
			if (nBuffsize > 65535)
			{
				break;
			}
			pszData = (wchar_t*)realloc(pszData, nBuffsize * sizeof(wchar_t));
		}
		va_end(args);
	}

	free(pszData);
	return std::wstring();
}

template<>
unsigned int CStringUtilT<char>::IPToInt(const char* addr)
{
	std::regex pattern("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");
	if (!std::regex_match(addr, pattern))
	{
		return 0;
	}

	unsigned int result = 0;
	if (addr == NULL || strlen(addr) == 0)
	{
		return result;
	}
	unsigned int temp[4] = { 0 };
	int nCount = 0;
	const char* pszTemp = addr;
	const char* pszStart = pszTemp;
	while (nCount < 4)
	{
		if (*pszTemp == '.' || *pszTemp == '\0')
		{
			youmecommon::CXSharedArray<char> szTemp(pszTemp - pszStart + 1);
			memset(szTemp.Get(), 0, pszTemp - pszStart + 1);
			memcpy(szTemp.Get(), pszStart, pszTemp - pszStart);
			temp[nCount] = atoi(szTemp.Get());
			if (temp[nCount] > 255)
			{
				return result;
			}
			++nCount;
			pszStart = pszTemp;
			++pszStart;
		}
		if (*pszTemp == '\0')
		{
			break;
		}
		++pszTemp;
	}

	if (nCount < 4)
	{
		return result;
	}

	result |= temp[3];
	result |= temp[2] << 8;
	result |= temp[1] << 16;
	result |= temp[0] << 24;

	return result;
}

template<>
std::string CStringUtilT<char>::IntToIP(unsigned int ip)
{
	std::string strAddr = "";
	if (ip > 4294967295)
	{
		return strAddr;
	}

	for (int i = 4; i > 0; i--)
	{
		unsigned int nTmp = (ip & (0xFF << (i - 1) * 8)) >> (i - 1) * 8;
		char szTmp[4] = { 0 };
		sprintf(szTmp, "%u", nTmp);
		strAddr += szTmp;
		strAddr += ".";
	}
	if (strAddr.length() > 1)
	{
		strAddr[strAddr.length() - 1] = 0;
	}
	return strAddr;
}
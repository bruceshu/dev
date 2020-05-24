#include "TranslateUtil.h"
#include <regex>
#include <YouMeCommon/DownloadUploadManager.h>
#include <YouMeCommon/Log.h>
#include <json/json.h>
#include <YouMeCommon/utf8.h>


//#define GOOGLE_URL __XT("https://translate.google.cn")
//#define TRANSLATE_URL __XT("https://translate.google.cn/translate_a/t?")

void CTranslateUtil::Init(const XString& strDomain, const XString& strCigPath, const XString& strRegex)
{
	m_ullFirstTKK = 0;
	m_ullSecondTKK = 0;
	m_strDomain = strDomain;
    m_strTranslateURL = m_strDomain + strCigPath;
    m_strRegex = XStringToUTF8(strRegex);
}

XString CTranslateUtil::Translate(const XString& strSrc, const XString&strOrigLanguage, const XString& strDestLanguage, XString& strOrigLanguageRet)
{
    std::map<std::string, std::string> mapHeaders;
    mapHeaders["content-length"] = "0";
    mapHeaders["user-agent"] = "Mozilla/5.0";
    std::string strResponse;
    if ((0 == m_ullFirstTKK) || (0 == m_ullSecondTKK))
    {
        //
        CDownloadUploadManager::HttpRequest(m_strDomain, "", strResponse, false, 3, &mapHeaders);
        if (strResponse.empty())
        {
            XString::size_type pos =  m_strDomain.find_last_of(__XT("."));
            if (pos != XString::npos)
            {
                XString oldDomain = m_strDomain.substr(pos + 1);
                std::transform(oldDomain.begin(), oldDomain.end(), oldDomain.begin(), ::tolower);
                XString newDomain;
                if (oldDomain == __XT("com"))
                {
                    newDomain = __XT("cn");
                }
                else if (oldDomain == __XT("cn"))
                {
                    newDomain = __XT("com");
                }
                m_strDomain.replace(pos + 1, oldDomain.length(), newDomain);
                m_strTranslateURL.replace(pos + 1, oldDomain.length(), newDomain);
                CDownloadUploadManager::HttpRequest(m_strDomain, "", strResponse, false, 3, &mapHeaders);
            }
        }
        if (GetTKK(strResponse) != 0)
        {
            return __XT("");
        }
    }
    
    //
    std::string strTK;
    GetTk(XStringToUTF8(strSrc), strTK);
    
    //
    strResponse = "";
    XStringStream strURL;

    strURL << m_strTranslateURL << __XT("client=webapp&sl=") << strOrigLanguage  <<__XT("&tk=") << UTF8TOXString(strTK) << __XT("&tl=") << strDestLanguage << __XT("&q=") << UTF8TOXString(CDownloadUploadManager::URLEncode(XStringToUTF8(strSrc)));
    
    CDownloadUploadManager::HttpRequest(strURL.str(), "", strResponse, false,-1, &mapHeaders);
    
    youmecommon::Value receiversValue;
    youmecommon::Reader jsonReader;
    XString AAA = UTF8TOXString(strResponse);
    if (!jsonReader.parse(strResponse, receiversValue))
    {
        return __XT("");
    }
    std::string strTranste;
    if (receiversValue.isArray())
    {
        if (receiversValue.size() == 1)
        {
            strTranste = receiversValue[(youmecommon::UInt)0].asString();
        }
        else if (receiversValue.size() == 2)
        {
            strTranste = receiversValue[(youmecommon::UInt)0].asString();
            strOrigLanguageRet = UTF8TOXString(receiversValue[(youmecommon::UInt)1].asString());
        }
        
        return UTF8TOXString(strTranste);
        
    }
    return __XT("");
}

XString CTranslateUtil::TranslateV2(const XString& strSrc, const XString&strOrigLanguage, const XString& strDestLanguage, XString& strOrigLanguageRet)
{
	std::map<std::string, std::string> mapHeaders;
	mapHeaders["Content-Type"] = "application/json; charset=utf-8";    
    mapHeaders["Content-Length"] = "0";
    mapHeaders["user-agent"] = "YIMMozilla/5.0";
    
	std::string strResponse = "";
	XStringStream strURL;
    
    if (strOrigLanguage == __XT("auto"))
    {
        strURL << m_strTranslateURL << __XT("&q=") << UTF8TOXString(CDownloadUploadManager::URLEncode(XStringToUTF8(strSrc))) << __XT("&target=") << strDestLanguage ;
    }
    else
    {
        strURL << m_strTranslateURL << __XT("&q=") << UTF8TOXString(CDownloadUploadManager::URLEncode(XStringToUTF8(strSrc))) << __XT("&source=") << strOrigLanguage  << __XT("&target=") << strDestLanguage ;
    }
	CDownloadUploadManager::HttpRequest(strURL.str(), "", strResponse, true,-1, &mapHeaders);
    
	youmecommon::Value receiversValue;
	youmecommon::Reader jsonReader;
	
	if (!jsonReader.parse(strResponse, receiversValue))
	{
		return __XT("");
	}
	std::string strTranste;
    
    if (receiversValue.isMember("data"))
    {
        youmecommon::Value jsonData = receiversValue["data"];
        
        if(jsonData.isMember("translations"))
        {
            youmecommon::Value jsonTranslations = jsonData["translations"];
            
            if (jsonTranslations.isArray())
            {
                for(int i = 0; i < (int)jsonTranslations.size(); i++)
                {
                    youmecommon::Value jsonTranslatedText = jsonTranslations[i];

                    strTranste = jsonTranslatedText["translatedText"].asString();
                    if (strOrigLanguage == __XT("auto"))
                    {
                        strOrigLanguageRet = UTF8TOXString(jsonTranslatedText["detectedSourceLanguage"].asString());
                    }
                }
            }
        }
        return UTF8TOXString(strTranste);
    }
    
    return __XT("");
}


int CTranslateUtil::GetTKK(const std::string& strSrc )
{
	std::string::size_type pos = strSrc.find("TKK=");
	if (std::string::npos == pos)
	{
		YouMe_LOG_Error(__XT("get tkk error"));
		return -5;
	}

	std::string strA = strSrc.substr(pos, 64);
	const std::regex pattern(".*'(\\d+)\\.(\\d+)'.*");
	std::match_results<std::string::const_iterator> result;
	bool valid = std::regex_match(strA, result, pattern);
	if (!valid)
	{
		YouMe_LOG_Error(__XT("not match  tkk:"));
		return -1;
	}

	if (result.size() < 3)
	{
		return -2;
	}

	m_ullFirstTKK = strtoll(result[1].str().c_str(), 0, 0);
	m_ullSecondTKK = strtoll(result[2].str().c_str(), 0, 0);

	return 0;
}



void CTranslateUtil::GetTk(const std::string& strSrc, std::string& strTk)
{
	//std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cvt;
	//std::u16string u16_cvt_str = cvt.from_bytes(strSrc);

	std::vector<unsigned short> u16_cvt_str;
	youmecommon::utf8to16(strSrc.begin(), strSrc.end(), std::back_inserter(u16_cvt_str));



	std::vector<XINT64> vecParse;
	for (unsigned int i = 0; i < u16_cvt_str.size(); ++i)
	{
		XINT64 charCode = u16_cvt_str[i];
		if (128 > charCode)
		{
			vecParse.push_back(charCode);
		}
		else
		{
            if (2048 > charCode)
			{
				vecParse.push_back(charCode >> 6 | 192);
			}
			else
			{
				if ((55296 == (charCode & 64512))
					&& (i + 1 < u16_cvt_str.size())
					&& (56320 == (u16_cvt_str[i + 1] & 64512)))
				{
					charCode = 65536 + ((charCode & 1023) << 10) + (u16_cvt_str[++i] & 1023);
					vecParse.push_back(charCode >> 18 | 240);
					vecParse.push_back(charCode >> 12 & 63 | 128);
				}
				else
				{
					vecParse.push_back(charCode >> 12 | 224);
					vecParse.push_back(charCode >> 6 & 63 | 128);
				}
			}

			vecParse.push_back(charCode & 63 | 128);
		}

	}

	XINT64 ullMagic = m_ullFirstTKK;
	for (unsigned int i = 0; i < vecParse.size(); ++i)
	{
		ullMagic += vecParse[i];
		ullMagic = GetMagic(ullMagic, "+-a^+6");
	}

	ullMagic = GetMagic(ullMagic, "+-3^+b+-f");
	ullMagic ^= m_ullSecondTKK;

	if (ullMagic < 0)
	{
		ullMagic = (ullMagic & 2147483647) + 2147483648;
	}

	ullMagic = ullMagic % 1000000;
	XINT64 iMagic2 = ullMagic ^ m_ullFirstTKK;

	std::stringstream sstrTk;
	sstrTk << ullMagic << "." << iMagic2;

	strTk = sstrTk.str();
}



XINT64 CTranslateUtil::GetMagic(XINT64 ullMagic, const std::string& strExpr)
{
	if (strExpr.length() < 3)
	{
		return 0;
	}

	for (unsigned int i = 0; i < strExpr.length() - 2; i += 3)
	{
		XINT64 c = strExpr[i + 2];
		if ((c >= '0') && (c <= '9'))
		{
			c = c - '0';
		}

		if ('a' <= c)
		{
			c = c - 87;
		}

		if ('+' == strExpr[i + 1])
		{
			c = ((XUINT64)ullMagic) >> c;
		}
		else
		{
			c = ullMagic << c;
		}

		if ('+' == strExpr[i])
		{
			ullMagic = ullMagic + c & 4294967295;
		}
		else
		{
			ullMagic = ullMagic ^ c;
		}

	}

	return ullMagic;
}

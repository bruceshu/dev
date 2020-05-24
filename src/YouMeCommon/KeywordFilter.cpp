#include "KeywordFilter.h"
#include <YouMeCommon/StringUtil.hpp>
#include <YouMeCommon/tinyxml/tinyxml.h>
#include <YouMeCommon/StringUtil.hpp>
#include <fstream>
#include <cctype>
#include <algorithm>
#include <regex>
#include <YouMeCommon/utf8/checked.h>
CKeywordFilter::CKeywordFilter(const XString& strReplaceTxt)
{
#ifdef WIN32
	for (int i = 0; i < strReplaceTxt.size(); i++)
	{
		m_strReplaceTextU16.push_back(strReplaceTxt.at(i));
	}
#else
	::youmecommon::utf8to16(strReplaceTxt.begin(), strReplaceTxt.end(), std::back_inserter(m_strReplaceTextU16));
#endif // WIN32
	m_bLoadSuccess = false;
	m_pRoot = std::shared_ptr<TrieNode>(new TrieNode);
}


CKeywordFilter::~CKeywordFilter()
{
	
}

bool CKeywordFilter::LoadFromFile(const XString&strSrcPath)
{
	/*
#ifdef WIN32
	std::wifstream inStream(strSrcPath, std::ios::in);
#else*/
	std::ifstream inStream(strSrcPath, std::ios::in);
//#endif // WIN32
	if (!inStream.is_open())
	{
		return false;
	}
	char szWord[512];
	while (!inStream.eof())
	{
		memset(szWord, 0, sizeof(szWord));
		inStream.getline(szWord, sizeof(szWord));
		//以竖线分开
		XString strText = UTF8TOXString2(szWord);
		int iPos = strText.find(__XT("|"));
		if (iPos != XString::npos)
		{
			AddKeyword(strText.substr(0, iPos),CStringUtil::str_to_sint32(strText.substr(iPos + 1, strText.length())));
		}
		else
		{
			AddKeyword(strText, 0);
		}
	}
	inStream.close();
	m_bLoadSuccess = true;
	return true;
}

void CKeywordFilter::AddKeyword(const XString& strKeyWord, int iLevel)
{
	XString strUpperKeyword1 = CStringUtil::replace_text(strKeyWord, __XT("\r"), __XT(""));
	std::transform(strUpperKeyword1.begin(), strUpperKeyword1.end(), strUpperKeyword1.begin(), ::toupper);

	std::vector<unsigned short> u16_cvt_str;
#ifdef WIN32
	for (int i = 0; i < strUpperKeyword1.size(); i++)
	{
		u16_cvt_str.push_back(strUpperKeyword1.at(i));
	}
#else
	::youmecommon::utf8to16(strUpperKeyword1.begin(), strUpperKeyword1.end(), std::back_inserter(u16_cvt_str));
#endif // WIN32

//转换成UTF16， 无论哪个平台下多正常

	std::shared_ptr<TrieNode> pSearchNode = m_pRoot;
	for (int i = 0; i < u16_cvt_str.size(); i++)
	{
		unsigned short character = u16_cvt_str.at(i);
		std::map<unsigned short, std::shared_ptr<TrieNode> >::iterator it = pSearchNode->childs.find(character);
		if (it == pSearchNode->childs.end())
		{
			std::shared_ptr<TrieNode> pChild(new TrieNode);
			pSearchNode->childs[character] = pChild;
			pSearchNode = pChild;
		}
		else
		{
			pSearchNode = it->second;
		}
	}
	pSearchNode->bEnd = true;
	pSearchNode->iLevel = iLevel;
}

bool CKeywordFilter::ContainKeyword(const XString& strSrc, int* pLevel)
{
	XString strResult;
	return InterSearchFilterResult(strSrc, strResult, pLevel);
}

XString CKeywordFilter::GetFilterResult(const XString& strSrc, int* pLevel)
{
	XString strResult;
	InterSearchFilterResult(strSrc, strResult, pLevel);
	return strResult;
}

bool CKeywordFilter::InterSearchFilterResult(const XString& strSrc1, XString& strFilterResult, int * pLevel)
{
	std::vector<unsigned short> result;

	std::vector<unsigned short> u16_cvt_str_src;
#ifdef WIN32
	for (int i = 0; i < strSrc1.size(); i++)
	{
		u16_cvt_str_src.push_back(strSrc1.at(i));
	}
#else
	::youmecommon::utf8to16(strSrc1.begin(), strSrc1.end(), std::back_inserter(u16_cvt_str_src));
#endif // WIN32



	XString strUperSrc1 = strSrc1;
	std::transform(strUperSrc1.begin(), strUperSrc1.end(), strUperSrc1.begin(), ::toupper);
	std::vector<unsigned short> u16_cvt_str;
#ifdef WIN32
	for (int i = 0; i < strUperSrc1.size(); i++)
	{
		u16_cvt_str.push_back(strUperSrc1.at(i));
	}
#else
	::youmecommon::utf8to16(strUperSrc1.begin(), strUperSrc1.end(), std::back_inserter(u16_cvt_str));
#endif // WIN32

	std::shared_ptr<TrieNode> searchNode = m_pRoot;
	int startFindIndex = -1;
	int searchIndex = 0;
	//保存一个最大等级
	int iMaxLevel = 0;
	//是否已经查找到过
	bool bHasFindKeyword = false;
	//上一次查找到的最大的结束索引
	int iLastEndIndex = -1;
	while ((searchIndex < u16_cvt_str.size()) || (iLastEndIndex != -1) || (startFindIndex !=-1 ))
	{
		if (searchIndex == u16_cvt_str.size())
		{
			//到头了，说明需要重新设置search位置
			if (iLastEndIndex != -1)
			{
				//上一次找到了结束的，那就从结束的下一个开始
				searchIndex = iLastEndIndex + 1;
				for (int i = 0; i < m_strReplaceTextU16.size(); i++)
				{
					result.push_back(m_strReplaceTextU16.at(i));
				}

			}
			else
			{
				//上一次没找到，那就从start 的下一个开始
				searchIndex = startFindIndex + 1;
				result.push_back(u16_cvt_str_src.at(startFindIndex));
			}			
			searchNode = m_pRoot;					
			iLastEndIndex = -1;
			startFindIndex = -1;
		}
		if (searchIndex >= u16_cvt_str.size())
		{
			break;
		}
		unsigned short character = u16_cvt_str.at(searchIndex);
		std::map<unsigned short, std::shared_ptr<TrieNode>>::iterator it = searchNode->childs.find(character);
			//从根查找，没有找到换下一个
		if (it == searchNode->childs.end())
		{
			//如果上一次是查找到了的，那就需要从根搜索				
			if (startFindIndex != -1)
			{				
				if (iLastEndIndex != -1)
				{
					//上一次查找的最大长度,说明已经查找到了，需要替换掉					
					searchIndex = iLastEndIndex + 1;
					for (int i = 0; i < m_strReplaceTextU16.size(); i++)
					{
						result.push_back(m_strReplaceTextU16.at(i));
					}
				}
				else
				{
					result.push_back(u16_cvt_str_src.at(startFindIndex));
					searchIndex = startFindIndex + 1;
				}
				iLastEndIndex = -1;
				startFindIndex = -1;
				
			}
			else
			{
				//result << u16_cvt_str_src.at(searchIndex);
				result.push_back(u16_cvt_str_src.at(searchIndex));
				searchIndex++;
			}
			searchNode = m_pRoot;
			continue;
		}
		if (startFindIndex == -1)
		{
			startFindIndex = searchIndex;
		}
		searchNode = it->second;
		if (searchNode->bEnd)
		{
			bHasFindKeyword = true;
			iLastEndIndex = searchIndex;
			//结束匹配到了，重新开始搜索		
			if (searchNode->iLevel > iMaxLevel)
			{
				iMaxLevel = searchNode->iLevel;
			}
			
		}
		searchIndex++;
	}
	
	
	/*if (startFindIndex != -1)
	{
		if (iLastEndIndex != -1)
		{
			result << m_strReplaceText;
			result << strSrc.substr(iLastEndIndex+1, strSrc.length() - iLastEndIndex );
		}
		else
		{
			for (int i = startFindIndex; i < u16_cvt_str_src.size(); i++)
			{
				//result << u16_cvt_str_src.at(i);
				result.push_back(u16_cvt_str_src.at(i));
			}
			
		}		
	}*/
	if (NULL != pLevel)
	{
		*pLevel = iMaxLevel;
	}
#ifdef WIN32
	for (int i = 0; i < result.size(); i++)
	{
		strFilterResult += (result[i]);
	}
#else
	std::vector<char> utf8Reslut;
	youmecommon::utf16to8(result.begin(), result.end(), std::back_inserter(utf8Reslut));
	for (int i = 0; i < utf8Reslut.size(); i++)
	{
		strFilterResult += (utf8Reslut[i]);
	}
#endif // WIN32

	
	
	return bHasFindKeyword;
}

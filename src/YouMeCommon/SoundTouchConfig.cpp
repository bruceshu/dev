#include "SoundTouchConfig.h"
#include <YouMeCommon/XFile.h>
#include <YouMeCommon/Log.h>
#include <YouMeCommon/XSharedArray.h>
#include <YouMeCommon/StringUtil.hpp>


#ifdef WIN32
#include <tchar.h>
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#endif
#include <time.h>
CSoundTouchConfig::CSoundTouchConfig()
{
}


CSoundTouchConfig::~CSoundTouchConfig()
{
}

bool CSoundTouchConfig::LoadConfi(const XString& strConfigPath)
{
	youmecommon::CXFile file;
	int iError = file.LoadFile(strConfigPath, youmecommon::CXFile::Mode_OpenExist_ReadOnly);
	if (iError != 0)
	{
		YouMe_LOG_Error(__XT("打开文件失败:%s"), strConfigPath.c_str());
		return false;
	}
	//读取数据长度
	unsigned short uDataLen = 0;
	if (file.Read((byte*)&uDataLen, 2) != 2)
	{
		return false;
	}
	uDataLen = ntohs(uDataLen);
	//读出密码
	int iPasswd = 0;
	if (file.Read((byte*)&iPasswd, 4) != 4)
	{
		return false;
	}
	iPasswd = ntohl(iPasswd);
	//读取数据
	youmecommon::CXSharedArray<char> pData(uDataLen+1);
	pData.Get()[uDataLen] = 0;
	file.Read((byte*)pData.Get(), uDataLen);

	int j = 0;
	for (int i = 0; i < uDataLen; i++)
	{
		pData.Get()[i] ^= ((byte*)&iPasswd)[j % 4];
		j++;
	}
	std::string strConfig = pData.Get();
	//解析参数
	std::vector<XString> configs;
	CStringUtil::splitString(UTF8TOXString(strConfig), __XT("|"), configs);
	if (configs.size() !=3)
	{
		YouMe_LOG_Error(__XT("配置解析失败:%s"), strConfigPath.c_str());
		return false;
	}
	m_iTempo = CStringUtil::str_to_sint32(configs[0]);
	m_iTones = CStringUtil::str_to_sint32(configs[1]);
	m_iRateChang = CStringUtil::str_to_sint32(configs[2]);
	return true;
}

bool CSoundTouchConfig::SaveConfig(const XString& strConfigPath)
{
	youmecommon::CXFile file;
	int iError = file.LoadFile(strConfigPath, youmecommon::CXFile::Mode_CREATE_ALWAYS);
	if (iError !=0)
	{
		YouMe_LOG_Error(__XT("创建文件失败:%s"),strConfigPath.c_str());
		return false;
	}
	XStringStream strConfigStream;
	strConfigStream << m_iTempo << __XT("|") << m_iTones << __XT("|") << m_iRateChang;
	XString strConfig = strConfigStream.str();

	std::string strUTF8Config = XStringToUTF8(strConfig);
	int iPasswd = time(NULL) + 10101;
	youmecommon::CXSharedArray<byte> pConfigErtBuffer(2 + 4 + strUTF8Config.length());
	//写入数据长度
	unsigned short netorderDataLen = htons(strUTF8Config.length());
	memcpy(pConfigErtBuffer.Get(), &netorderDataLen, 2);
	//写入密码
	int netorderPasswdLen = htonl(iPasswd);
	memcpy(pConfigErtBuffer.Get() + 2, &netorderPasswdLen, 4);
	byte* pEncrtyPtr = pConfigErtBuffer.Get() + 6;//偏移到6 字节处
	//开始循环加密数据，异或
	int j = 0;
	for (size_t i = 0; i < strUTF8Config.length(); i++)
	{
		pEncrtyPtr[i] = strUTF8Config.at(i) ^ ((byte*)&iPasswd)[j % 4];
		j++;
	}
	file.Write(pConfigErtBuffer.Get(), pConfigErtBuffer.GetBufferLen()); 

	return true;
}

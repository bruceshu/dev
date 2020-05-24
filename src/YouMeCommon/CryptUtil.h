#pragma once
#include <YouMeCommon/CrossPlatformDefine/PlatformDef.h>
#include <YouMeCommon/XSharedArray.h>
namespace youmecommon
{
class CCryptUtil
{
public:
	CCryptUtil(void);
	~CCryptUtil(void);

	/************************************************************************/
	/*                                   BASE64                             */
	/************************************************************************/
	static bool Base64Decoder( const std::string& input ,CXSharedArray<char>& pBuffer);
	static bool Base64Decoder( const byte* buffer,int iLength ,CXSharedArray<char>& pBuffer);
	static void Base64Encoder( const std::string& input ,std::string& output);
	static void Base64Encoder( const byte* buffer,int iLength ,std::string& output);


	/************************************************************************/
	/*                                      MD5                             */
	/************************************************************************/
	static XString MD5(const std::string& strPlainText);
	static XString MD5(const byte* pBuffer, int iSize);

	//文件MD5
	static XString MD5File(const XString& strFilePath);
	/************************************************************************/
	/* SHA1                                                                       */
	/************************************************************************/
	static XString SHA1(const XString& strSrc);

	//字符串加解密封装 ， 输入都是0结尾的字符串. 方便外面使用，内部需要转成utf8处理
	static XString EncryptString(const XString& strPlainText,const XString&strPasswd);
	static XString DecryptString(const XString& strEncText, const XString& strPasswd);

	//二进制数据的加解密  Key VI 8字节长度

	static std::string EncryptByDES(const char* pszPlainText,int iBufferLen, const std::string strKey, const std::string& strIV);
	static std::string DecryptByDES(const char* pszPlainText, int iBufferLen,const std::string strKey, const std::string& strIV);


	//为了避免外部直接传入Key IV ,这里封装一个直接传入 passwd 的加解密
	static std::string EncryptByDES_S(const char* pszBuffer,int iBufLen, const XString&strPasswd);
	static std::string DecryptByDES_S(const char* pszBuffer, int iBufLen, const XString&strPasswd);

	// 默认Key IV变量置0， 仅输入 passwd 进行AES加解密
	static std::string EncryptByAES(const char* pszBuffer, int iBufLen, std::string& strPasswd);
	static std::string DecryptByAES(const char* pszBuffer, int iBufLen, std::string& strPasswd);
};
}
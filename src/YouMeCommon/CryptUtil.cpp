#include "CryptUtil.h"
#include <iomanip>
#include <YouMeCommon/crypto/md5/md5.h>
#include <YouMeCommon/XFile.h>
#include <XSharedArray.h>
#include <YouMeCommon/StringUtil.hpp>
#include <YouMeCommon/SHA1.h>

#if !(YOUME_MINI)
#include <cryptopp/cryptlib.h>
#include <cryptopp/modes.h>         
#include <cryptopp/filters.h>       
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/des.h>
#include <cryptopp/base64.h>
#include <cryptopp/aes.h>
#endif

namespace youmecommon
{
CCryptUtil::CCryptUtil(void)
{
}

CCryptUtil::~CCryptUtil(void)
{

}

bool CCryptUtil::Base64Decoder( const std::string& input ,CXSharedArray<char>& pBuffer )
{
	return Base64Decoder((const byte*)input.c_str(), input.length(),pBuffer);
}


#define TSK_BASE64_PAD '='
#define TSK_BASE64_DECODE_BLOCK_SIZE	4


bool CCryptUtil::Base64Decoder( const byte* buffer,int iLength ,CXSharedArray<char>& pBuffer )
{
	static const unsigned char TSK_BASE64_DECODE_ALPHABET[256] =
	{
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff,
		62,
		0xff, 0xff, 0xff,
		63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff,
	};

	int i, pay_size;
	int output_size = 0;

	/* Caller provided his own buffer? */
	pBuffer.Allocate(iLength + 1);


	/* Count pads and remove them from the base64 string */
	for (i = iLength, pay_size = iLength; i > 0; i--){
		if (buffer[i - 1] == TSK_BASE64_PAD) {
			pay_size--;
		}
		else{
			break;
		}
	}

	/* Reset i */
	i = 0;

	if (pay_size < TSK_BASE64_DECODE_BLOCK_SIZE){
		goto quantum;
	}

	do{
		*(pBuffer.Get() + output_size++) = (TSK_BASE64_DECODE_ALPHABET[buffer[i]] << 2
			| TSK_BASE64_DECODE_ALPHABET[buffer[i + 1]] >> 4);
		*(pBuffer.Get() + output_size++) = (TSK_BASE64_DECODE_ALPHABET[buffer[i + 1]] << 4
			| TSK_BASE64_DECODE_ALPHABET[buffer[i + 2]] >> 2);
		*(pBuffer.Get() + output_size++) = (TSK_BASE64_DECODE_ALPHABET[buffer[i + 2]] << 6
			| TSK_BASE64_DECODE_ALPHABET[buffer[i + 3]]);

		i += TSK_BASE64_DECODE_BLOCK_SIZE;
	} while ((i + TSK_BASE64_DECODE_BLOCK_SIZE) <= pay_size);

quantum:

	if ((iLength - pay_size) == 1){
		*(pBuffer.Get() + output_size++) = (TSK_BASE64_DECODE_ALPHABET[buffer[i]] << 2
			| TSK_BASE64_DECODE_ALPHABET[buffer[i + 1]] >> 4);
		*(pBuffer.Get() + output_size++) = (TSK_BASE64_DECODE_ALPHABET[buffer[i + 1]] << 4
			| TSK_BASE64_DECODE_ALPHABET[buffer[i + 2]] >> 2);
	}
	else if ((iLength - pay_size) == 2){
		*(pBuffer.Get() + output_size++) = (TSK_BASE64_DECODE_ALPHABET[buffer[i]] << 2
			| TSK_BASE64_DECODE_ALPHABET[buffer[i + 1]] >> 4);
	}
	pBuffer.ReSize(output_size);
	return true;
}

void CCryptUtil::Base64Encoder( const std::string& input ,std::string& output )
{
	Base64Encoder((const byte*)input.c_str(), input.length(),output);
}

void CCryptUtil::Base64Encoder(const byte* buffer, int iLength, std::string& sResult)
{
	static const std::string sBase64Table(
		// 0000000000111111111122222222223333333333444444444455555555556666
		// 0123456789012345678901234567890123456789012345678901234567890123
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
		);
	static const char cFillChar = '=';
	std::string::size_type   nLength = iLength;

	// Allocate memory for the converted string
	sResult.reserve(nLength * 8 / 6 + 1);

	for (std::string::size_type nPos = 0; nPos < nLength; nPos++) {
		char cCode;

		// Encode the first 6 bits
		cCode = (buffer[nPos] >> 2) & 0x3f;
		sResult.append(1, sBase64Table[cCode]);

		// Encode the remaining 2 bits with the next 4 bits (if present)
		cCode = (buffer[nPos] << 4) & 0x3f;
		if (++nPos < nLength)
			cCode |= (buffer[nPos] >> 4) & 0x0f;
		sResult.append(1, sBase64Table[cCode]);

		if (nPos < nLength) {
			cCode = (buffer[nPos] << 2) & 0x3f;
			if (++nPos < nLength)
				cCode |= (buffer[nPos] >> 6) & 0x03;

			sResult.append(1, sBase64Table[cCode]);
		}
		else {
			++nPos;
			sResult.append(1, cFillChar);
		}

		if (nPos < nLength) {
			cCode = buffer[nPos] & 0x3f;
			sResult.append(1, sBase64Table[cCode]);
		}
		else {
			sResult.append(1, cFillChar);
		}
	}
}

XString CCryptUtil::MD5(const std::string& strPlainText)
{
    return MD5((const byte*)strPlainText.c_str(), strPlainText.length());
}

XString CCryptUtil::MD5(const byte* pBuffer, int iSize)
{
    unsigned char digest[16] = {0};
    memset(digest, 0x00, sizeof(digest));
    
    youmecommon::MD5_CTX ctx;
    youmecommon::MD5_Init(&ctx);
    youmecommon::MD5_Update(&ctx, pBuffer, iSize);
    youmecommon::MD5_Final(digest, &ctx);
	return CStringUtil::bytes_to_hex_string(digest, 16);
}

XString CCryptUtil::MD5File(const XString& strFilePath)
{
	CXFile file;
	if (0 != file.LoadFile(strFilePath, CXFile::Mode_OpenExist))
	{
		return __XT("");
	}

	char buffer[1204] = { 0 };
	unsigned char digest[16] = { 0 };
	memset(digest, 0x00, sizeof(digest));
	youmecommon::MD5_CTX ctx;
	youmecommon::MD5_Init(&ctx);
	while (true)
	{
		int iReadLen = file.Read((byte*)buffer, 1024);
		if (iReadLen <=0)
		{
			break;
		}
		youmecommon::MD5_Update(&ctx, buffer, iReadLen);
	}
	
	youmecommon::MD5_Final(digest, &ctx);
	return CStringUtil::bytes_to_hex_string(digest, 16);

}

XString CCryptUtil::SHA1(const XString& strSrc)
{
	CSHA1 sha1;
	std::string strUTF8Src = XStringToUTF8(strSrc);
	sha1.Update((const unsigned char*)strUTF8Src.c_str() ,strUTF8Src.length());
	sha1.Final();
	return  sha1.GetHash();
}

XString CCryptUtil::EncryptString(const XString& strPlainText, const XString&strPasswd)
{
	if (strPasswd.empty())
	{
		return strPlainText;
	}
	//加解密实际是针对一段缓存加密，这些先转成 窄字符的 UTF8编码
	std::string strUTF8PlainText = XStringToUTF8(strPlainText);
	std::string strUTF8Passwd = XStringToUTF8(strPasswd);
	//将密码 进行64 字节变换
	XString strDealPasswd1 = MD5(strUTF8Passwd);
	XString strDealPasswd2 = MD5(strUTF8Passwd+strUTF8Passwd);
	std::string strLastPasswd = XStringToUTF8((strDealPasswd1 + strDealPasswd2));



	int iPasswdLen = strLastPasswd.length();
	for (int i = 0; i < strUTF8PlainText.size();i++)
	{
		strUTF8PlainText[i] ^= strLastPasswd[i % iPasswdLen];
	}
	std::string strBase64Text;
	Base64Encoder(strUTF8PlainText, strBase64Text);
	return UTF8TOXString(strBase64Text);

}

XString CCryptUtil::DecryptString(const XString& strEncText, const XString& strPasswd)
{
	if (strPasswd.empty())
	{
		return strEncText;
	}
	std::string strUTF8PlainText = XStringToUTF8(strEncText);
	std::string strUTF8Passwd = XStringToUTF8(strPasswd);

	//将密码 进行64 字节变换
	XString strDealPasswd1 = MD5(strUTF8Passwd);
	XString strDealPasswd2 = MD5(strUTF8Passwd + strUTF8Passwd);
	std::string strLastPasswd = XStringToUTF8((strDealPasswd1 + strDealPasswd2));

	int iPasswdLen = strLastPasswd.length();


	CXSharedArray<char> pBase64DecodText;
	Base64Decoder(strUTF8PlainText, pBase64DecodText);
	//二进制的密文
	for (int i = 0; i < pBase64DecodText.GetBufferLen(); i++)
	{
		pBase64DecodText.Get()[i] ^= strLastPasswd[i % iPasswdLen];
	}
	return UTF8TOXString(std::string(pBase64DecodText.Get(),pBase64DecodText.GetBufferLen()));
}


std::string CCryptUtil::EncryptByDES(const char* pszPlainText, int iBufferLen, const std::string strKey, const std::string& strIV)
{
#if !(YOUME_MINI)
	try
	{
		std::string CipherText;
		CryptoPP::CBC_Mode<CryptoPP::DES>::Encryption
			Encryptor((const byte*)strKey.c_str(), strKey.length(), (const byte*)strIV.c_str());

		// Encryption
		CryptoPP::StringSource(
			(const byte*)pszPlainText,
			iBufferLen,
			true,
			new CryptoPP::StreamTransformationFilter(
			Encryptor,
			new CryptoPP::StringSink(CipherText)
			) // StreamTransformationFilter
			); // StringSource
		return CipherText;
	}
	catch (...)
	{

	}
#endif
	return std::string(pszPlainText,iBufferLen);
}

std::string CCryptUtil::DecryptByDES(const char* pszPlainText, int iBufferLen, const std::string strKey, const std::string& strIV)
{
#if !(YOUME_MINI)
	try
	{
		int len = iBufferLen / 2;
		CryptoPP::CBC_Mode<CryptoPP::DES>::Decryption
			Decryptor((const byte*)strKey.c_str(), strKey.length(), (const byte*)strIV.c_str());

		std::string RecoveredText;
		// Decryption
		CryptoPP::StringSource((const byte*)pszPlainText, iBufferLen, true,
			new CryptoPP::StreamTransformationFilter(Decryptor,
			new CryptoPP::StringSink(RecoveredText)
			) // StreamTransformationFilter
			); // StringSource

		return RecoveredText;
	}
	catch (...)
	{

	}
#endif
	return std::string(pszPlainText,iBufferLen);
}


std::string CCryptUtil::EncryptByDES_S(const char* pszBuffer, int iBufLen, const XString&strPasswd)
{
	if (strPasswd.empty())
	{
		return std::string(pszBuffer,iBufLen);
	}
	std::string strUTF8Passwd = XStringToUTF8(strPasswd);
	//将密码 进行64 字节变换
	std::string strKeyPasswd = XStringToUTF8(MD5(strUTF8Passwd));
	std::string strIVPasswd = XStringToUTF8(MD5(strUTF8Passwd + strUTF8Passwd));
	return EncryptByDES(pszBuffer,iBufLen, strKeyPasswd.substr(0, 8), strIVPasswd.substr(0, 8));
}

std::string CCryptUtil::DecryptByDES_S(const char* pszBuffer, int iBufLen, const XString&strPasswd)
{
	if (strPasswd.empty())
	{
		return std::string(pszBuffer,iBufLen);
	}
	//加解密实际是针对一段缓存加密，这些先转成 窄字符的 UTF8编码
	std::string strUTF8Passwd = XStringToUTF8(strPasswd);
	//将密码 进行64 字节变换
	std::string strKeyPasswd = XStringToUTF8(MD5(strUTF8Passwd));
	std::string strIVPasswd = XStringToUTF8(MD5(strUTF8Passwd + strUTF8Passwd));

	return DecryptByDES(pszBuffer,iBufLen, strKeyPasswd.substr(0, 8), strIVPasswd.substr(0, 8));

}

std::string CCryptUtil::EncryptByAES(const char* pszBuffer, int iBufLen, std::string& strPasswd)
{
#if !(YOUME_MINI)
	if (strPasswd.empty()) {
		return std::string(pszBuffer,iBufLen);
	}

	std::string cipherText;
	const byte* pszPasswd = (const byte *)strPasswd.c_str();

	// CBC mode need initial IV
	CryptoPP::byte iv[ CryptoPP::AES::BLOCKSIZE ];
	memset( iv, 0x00, CryptoPP::AES::BLOCKSIZE );

	// aes encrypt processing, cbc mode
	CryptoPP::AES::Encryption aesEncryption(pszPasswd, CryptoPP::AES::DEFAULT_KEYLENGTH);
	CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption( aesEncryption, iv );
	CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink( cipherText ));

	stfEncryptor.Put( reinterpret_cast<const unsigned char*>( pszBuffer ), iBufLen );
	stfEncryptor.MessageEnd();

	return cipherText;
#else
    return "";
    
#endif
}

std::string CCryptUtil::DecryptByAES(const char* pszBuffer, int iBufLen, std::string& strPasswd)
{
#if !(YOUME_MINI)
	if (strPasswd.empty())
	{
		return std::string(pszBuffer,iBufLen);
	}

	std::string decryptedText;

	const byte* pszPasswd = (const byte *)strPasswd.c_str();
	
	// CBC mode need initial IV
	CryptoPP::byte iv[ CryptoPP::AES::BLOCKSIZE ];
	memset( iv, 0x00, CryptoPP::AES::BLOCKSIZE );

	// aes decrypt processing, cbc mode
	CryptoPP::AES::Decryption aesDecryption(pszPasswd, CryptoPP::AES::DEFAULT_KEYLENGTH);
	CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption( aesDecryption, iv );
	CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink( decryptedText ));
	stfDecryptor.Put( reinterpret_cast<const unsigned char*>( pszBuffer ), iBufLen);
	stfDecryptor.MessageEnd();

	return decryptedText;
#else
    return "";
    
#endif
}
}

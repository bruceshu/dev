#include "DownloadUploadManager.h"
#include "TimeUtil.h"
#include <time.h>
#include <fstream>
#include <YouMeCommon/json/json.h>
#include <YouMeCommon/XFile.h>
#include <YouMeCommon/Log.h>
#include <YouMeCommon/CrossPlatformDefine/PlatformDef.h>
#include <YouMeCommon/StringUtil.hpp>
#include <XSharedArray.h>

#define SINGLE_FILE_MAX_SIZE 8 * 1024 *1024
#define SLINCE_SIZE 1024 *1024

youmecommon::CURLSH* CDownloadUploadManager::m_shareHandle = NULL;
std::map<std::string, progressCallback_t> CDownloadUploadManager::m_uploadFileSizeMap;

size_t CDownloadUploadManager::OnUploadWrite(void *buffer, size_t size, size_t nmemb, void *user_p)
{
	std::string* pResponse = (std::string*)user_p;
	pResponse->append((char*)buffer, nmemb);
	return nmemb;
}

size_t CDownloadUploadManager::UploadReadCallback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t stRead = fread(ptr, size, nmemb, (FILE*)stream);
	XUINT64 curTime = youmecommon::CTimeUtil::GetTimeOfDay_MS();
	std::map<std::string, progressCallback_t>::iterator itr = m_uploadFileSizeMap.find(CStringUtilT<char>::to_string((XUINT64)stream));
	if (itr != m_uploadFileSizeMap.end())
	{
		if (itr->second.iUploadProgressCallback != NULL) {
			itr->second.upload_size += stRead;

			//每100ms回调一次，或者上传结束了回调
			if (curTime - itr->second.lastTime >= 100 || itr->second.upload_size == itr->second.fileTotleSize) {
				float percent = (float)(itr->second.upload_size * 10000/itr->second.fileTotleSize)/100;	//保留小数点后面两位
				itr->second.iUploadProgressCallback->onUploadProgressCallback(itr->second.msgSerial, percent);
				itr->second.lastTime = curTime;
			}
		}
	}

	return stRead;
}

//下载文件的回掉
size_t CDownloadUploadManager::OnDownloadFile(void *buffer, size_t size, size_t nmemb, void *user_p)
{
	youmecommon::CXFile* pFile = (youmecommon::CXFile*)user_p;
	//写文件
	size_t dwWrittenBytes = 0;
	const byte* pTmpBuffer = (const byte*)buffer;
	while (dwWrittenBytes < nmemb)
	{
		size_t dwWriteBytes = (size_t)pFile->Write(pTmpBuffer + dwWrittenBytes, nmemb - dwWrittenBytes);
		//这里需要考虑一下空间满的情况
		if (dwWriteBytes <= 0)
		{
			return CURL_READFUNC_ABORT;
		}
		else
		{
			dwWrittenBytes += dwWriteBytes;
		}
	}
	return nmemb;
}

//保存http 的各种请求的头部信息
size_t CDownloadUploadManager::OnHttpHead(void *buffer, size_t size, size_t nmemb, void *user_p)
{
	std::string* pResponse = (std::string*)user_p;
	pResponse->append((char*)buffer, nmemb);
	return nmemb;
}

bool CDownloadUploadManager::UploadFile(const XString&strUrl, const XString& strSrcPath, std::map<std::string, std::string>& httpHead, std::string& strResponse)
{
	return UploadFileToUpYun(strUrl, strSrcPath, true, httpHead, strResponse);
}

bool CDownloadUploadManager::UploadFile(const XString&strUrl, const XString& strSrcPath, std::map<std::string, std::string>& httpHead, std::string& strResponse, int bucketType)
{
	if (2 == bucketType) // COS
	{
		return UploadFilePost(strUrl, strSrcPath, httpHead);
	}
	else // AWS
	{
		return UploadFileToUpYun(strUrl, strSrcPath, true, httpHead, strResponse);
	}
}

bool CDownloadUploadManager::UploadFile(const XString&strUrl, const XString& strSrcPath, std::map<std::string, std::string>& httpHead, void * param, std::string& strResponse, int bucketType, IUploadProgressCallback * iUploadProgressCallback)
{
	if (2 == bucketType) // COS
	{
		return UploadFilePost(strUrl, strSrcPath, httpHead);
	}
	else // AWS
	{
		return UploadFileToUpYunWithProgress(strUrl, strSrcPath, true, httpHead, param, strResponse, iUploadProgressCallback);
	}
}

bool CDownloadUploadManager::HttpRequest(const XString& strUrl, const std::string& strBody, std::string& strResponse, bool bPost /*= true*/, int iTimeOut /*= -1*/, std::map<std::string, std::string>*pHead /*= NULL*/)
{
	youmecommon::CURL * easyHandle = NULL;
	bool bSuccess = false;
	do
	{
		easyHandle = youmecommon::curl_easy_init();
		if (NULL == easyHandle)
		{
			YouMe_LOG_Info(__XT("curl init fail"));
			break;
		}
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_URL, XStringToUTF8(strUrl).c_str());
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_NOSIGNAL, 1);

		if (bPost)
		{
			curl_easy_setopt(easyHandle, youmecommon::CURLOPT_POST, 1);
			curl_easy_setopt(easyHandle, youmecommon::CURLOPT_POSTFIELDS, strBody.c_str());
		}
		
		if (NULL == m_shareHandle)
		{
			m_shareHandle = youmecommon::curl_share_init();
			curl_share_setopt(m_shareHandle, youmecommon::CURLSHOPT_SHARE, youmecommon::CURL_LOCK_DATA_DNS);
		}
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_SHARE, m_shareHandle);
		if (iTimeOut != -1)
		{
			curl_easy_setopt(easyHandle, youmecommon::CURLOPT_TIMEOUT, iTimeOut);
		}
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_AUTOREFERER, 1); // 以下3个为重定向设置
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_FOLLOWLOCATION, 1); //返回的头部中有Location(一般直接请求的url没找到)，则继续请求Location对应的数据 
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_WRITEFUNCTION, OnUploadWrite);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_SSL_VERIFYPEER, 0L);
		struct youmecommon::curl_slist *headers = NULL; /* init to NULL is important */
		if (NULL != pHead)
		{
			std::map<std::string, std::string>::iterator begin = pHead->begin();
			std::map<std::string, std::string>::iterator end = pHead->end();
			for (; begin != end; begin++)
			{
				std::stringstream strStream;
				strStream << begin->first << (": ") << begin->second;
				//描述
				headers = youmecommon::curl_slist_append(headers, strStream.str().c_str());
			}
			curl_easy_setopt(easyHandle, youmecommon::CURLOPT_HTTPHEADER, headers);
		}

		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_WRITEDATA, (void *)&strResponse);
		// 执行上传操作
		int errorcode = youmecommon::curl_easy_perform(easyHandle);
		long httpCode = 0;
		curl_easy_getinfo(easyHandle, youmecommon::CURLINFO_RESPONSE_CODE, &httpCode);
		YouMe_LOG_Info(__XT("CURLcode:%d httpCode:%d"), errorcode, httpCode);
		if (NULL != headers)
		{
			youmecommon::curl_slist_free_all(headers);
		}
		if (0 == errorcode)
		{
			bSuccess = true;
		}
	} while (0);
	if (NULL != easyHandle)
	{
		youmecommon::curl_easy_cleanup(easyHandle);
	}

	return bSuccess;
}

bool CDownloadUploadManager::UploadFileWithForm(const XString&strUrl, const XString& strSrcPath, std::map<std::string, std::string>& httpHead, std::string& strResponse)
{
	//饶了好大一圈，终于可以开始干活了。
	youmecommon::CURL* easyHandle = youmecommon::curl_easy_init();
	if (easyHandle == NULL)
	{
		return false;
	}

#ifdef OS_ANDROID
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_NOSIGNAL, 1);
#else
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_NOSIGNAL, 1);
#endif
    curl_easy_setopt(easyHandle, youmecommon::CURLOPT_URL, XStringToUTF8(strUrl).c_str());
	if (NULL == m_shareHandle)
	{
		m_shareHandle = youmecommon::curl_share_init();
		curl_share_setopt(m_shareHandle, youmecommon::CURLSHOPT_SHARE, youmecommon::CURL_LOCK_DATA_DNS);
	}
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_SHARE, m_shareHandle);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_DNS_CACHE_TIMEOUT, 15);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_CONNECTTIMEOUT, 15);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_SSL_VERIFYPEER, 0L);
	//struct curl_httppost *formpost = NULL;

	struct youmecommon::curl_slist *headers = NULL;

	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_WRITEFUNCTION, OnUploadWrite);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_WRITEDATA, (void *)&strResponse);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_NOPROGRESS, 0);


	//你Y 就不支持中文，当然仅限于坑爹的windwos系统
	struct youmecommon::curl_httppost *lastptr = NULL;
	struct youmecommon::curl_httppost *formpost = NULL;
		
	youmecommon::curl_formadd(&formpost, &lastptr, youmecommon::CURLFORM_COPYNAME, "file", youmecommon::CURLFORM_FILE, XStringToLocal(strSrcPath).c_str(), youmecommon::CURLFORM_END);
	youmecommon::curl_easy_setopt(easyHandle, youmecommon::CURLOPT_HTTPPOST, formpost);

	if (httpHead.size() > 0)
	{
		for (std::map<std::string, std::string>::iterator itr = httpHead.begin(); itr != httpHead.end(); ++itr)
		{
			std::stringstream strStream;
			strStream << itr->first << ": " << itr->second;
			headers = youmecommon::curl_slist_append(headers, strStream.str().c_str());
		}
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_HTTPHEADER, headers);
	}
	youmecommon::CURLcode errorcode = youmecommon::curl_easy_perform(easyHandle);
	if (NULL != formpost)
	{
		curl_formfree(formpost);
	}
	
	long httpCode = 0;
	curl_easy_getinfo(easyHandle, youmecommon::CURLINFO_RESPONSE_CODE, &httpCode);
	youmecommon::curl_easy_cleanup(easyHandle);
	YouMe_LOG_Info(__XT("CURLcode:%d httpCode:%d"), errorcode, httpCode);
	if (headers != NULL)
	{
		youmecommon::curl_slist_free_all(headers);
	}
	if ((errorcode != youmecommon::CURLE_OK) || (httpCode != 200))
	{
		YouMe_LOG_Info(__XT("url:%s"), strUrl.c_str());
		return false;
	}
	return true;
}

unsigned char CDownloadUploadManager::ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}
std::string CDownloadUploadManager::URLEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ')
			strTemp += "+";
		else
		{
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] % 16);
		}
	}
	return strTemp;
}

bool CDownloadUploadManager::DownloadFile(const XString&strUrl, const XString&strSavePath)
{
	std::map<std::string, std::string> httpHead;
	std::string strResponse;
	return UploadFileToUpYun(strUrl, strSavePath, false, httpHead, strResponse);
}

bool CDownloadUploadManager::UploadFileToUpYun(const XString&strUrl, const XString&strSavePath, bool bUpload, std::map<std::string, std::string>& httpHead, std::string& strResponse)
{
	//饶了好大一圈，终于可以开始干活了。
	youmecommon::CURL* easyHandle = youmecommon::curl_easy_init();
	if (easyHandle == NULL)
	{
		return false;
	}
    
	XString strDealURL = strUrl;

#ifdef OS_ANDROID
    int iFind = strUrl.find(__XT("https://"));
    if (0 == iFind)
    {
        strDealURL = __XT("http://") + strUrl.substr(iFind + strlen("https://"), strUrl.length());
    }
#endif

    curl_easy_setopt(easyHandle, youmecommon::CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(easyHandle, youmecommon::CURLOPT_URL, XStringToUTF8(strDealURL).c_str());
    curl_easy_setopt(easyHandle, youmecommon::CURLOPT_NOSIGNAL, 1);

	if (NULL == m_shareHandle)
	{
		m_shareHandle = youmecommon::curl_share_init();
		curl_share_setopt(m_shareHandle, youmecommon::CURLSHOPT_SHARE, youmecommon::CURL_LOCK_DATA_DNS);
	}
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_SHARE, m_shareHandle);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_DNS_CACHE_TIMEOUT, 15);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_CONNECTTIMEOUT, 15);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_SSL_VERIFYPEER, 0L);
	//struct curl_httppost *formpost = NULL;

	FILE* file = NULL;
	struct youmecommon::curl_slist *headers = NULL;
	if (!bUpload)
	{		
		XString strUpDIR = youmecommon::CXFile::get_upper_dir(strSavePath.c_str());
		youmecommon::CXFile::make_dir_tree(strUpDIR.c_str());
#ifdef WIN32
		errno_t errCode = _wfopen_s(&file, strSavePath.c_str(), __XT("wb+"));
		if (errCode != 0 || NULL == file)
		{
			youmecommon::curl_easy_cleanup(easyHandle);
			return false;
		}
#else
		file = fopen(strSavePath.c_str(), __XT("wb+"));
		if (NULL == file)
		{
			youmecommon::curl_easy_cleanup(easyHandle);
			return false;
		}
#endif
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_WRITEFUNCTION, OnDownloadFile);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_WRITEDATA, (void *)&file);
	}
	else
	{
#ifdef WIN32
		errno_t errCode = _wfopen_s(&file, strSavePath.c_str(), __XT("rb"));
		if (errCode != 0 || NULL == file)
		{
			youmecommon::curl_easy_cleanup(easyHandle);
			return false;
		}
#else
		file = fopen(strSavePath.c_str(), __XT("rb"));
		if (NULL == file)
		{
			youmecommon::curl_easy_cleanup(easyHandle);
			return false;
		}
#endif
		fseek(file, 0, SEEK_END);
		long fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_WRITEFUNCTION, OnUploadWrite);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_WRITEDATA, (void *)&strResponse);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_PUT, 1L);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_READFUNCTION, UploadReadCallback);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_READDATA, file);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_INFILESIZE, (curl_off_t)fileSize);

		//你Y 就不支持中文，当然仅限于坑爹的windwos系统
		//struct curl_httppost *lastptr = NULL;
		//curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "file", CURLFORM_FILE, XStringToLocal(strSavePath).c_str(), CURLFORM_END);
		//curl_easy_setopt(easyHandle, CURLOPT_HTTPPOST, formpost);
		
		if (httpHead.size() > 0)
		{		
			for (std::map<std::string, std::string>::iterator itr = httpHead.begin(); itr != httpHead.end(); ++itr)
			{
				std::stringstream strStream;
				strStream << itr->first << ": " << itr->second;
				headers = youmecommon::curl_slist_append(headers, strStream.str().c_str());
			}
			curl_easy_setopt(easyHandle, youmecommon::CURLOPT_HTTPHEADER, headers);
		}
	}
	youmecommon::CURLcode errorcode = youmecommon::curl_easy_perform(easyHandle);
	//if (NULL != formpost)
	//{
	//	curl_formfree(formpost);
	//}

	fclose(file);
	long httpCode = 0;
	curl_easy_getinfo(easyHandle, youmecommon::CURLINFO_RESPONSE_CODE, &httpCode);
	youmecommon::curl_easy_cleanup(easyHandle);
    YouMe_LOG_Info(__XT("CURLcode:%d httpCode:%d"),errorcode,httpCode);
	if (headers != NULL)
	{
		youmecommon::curl_slist_free_all(headers);
	}
	if ((errorcode != youmecommon::CURLE_OK) || (httpCode != 200))
	{
		//如果是下载的话则判断一下下载是否成功，失败的话就删掉文件
		if (!bUpload)
		{
#ifdef WIN32
			::DeleteFileW(strSavePath.c_str());
#else
			remove(strSavePath.c_str());
#endif // WIN32	
		}
		YouMe_LOG_Info(__XT("url:%s"), strUrl.c_str());
		return false;
	}
	return true;
}

bool CDownloadUploadManager::UploadFileToUpYunWithProgress(const XString&strUrl, const XString&strSavePath, bool bUpload, std::map<std::string, std::string>& httpHead, void * param, std::string& strResponse, IUploadProgressCallback * iUploadProgressCallback)
{
	youmecommon::CURL* easyHandle = youmecommon::curl_easy_init();
	if (easyHandle == NULL)
	{
		return false;
	}
    
	XString strDealURL = strUrl;

#ifdef OS_ANDROID
    int iFind = strUrl.find(__XT("https://"));
    if (0 == iFind)
    {
        strDealURL = __XT("http://") + strUrl.substr(iFind + strlen("https://"), strUrl.length());
    }
#endif

    curl_easy_setopt(easyHandle, youmecommon::CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(easyHandle, youmecommon::CURLOPT_URL, XStringToUTF8(strDealURL).c_str());
    curl_easy_setopt(easyHandle, youmecommon::CURLOPT_NOSIGNAL, 1);

	if (NULL == m_shareHandle)
	{
		m_shareHandle = youmecommon::curl_share_init();
		curl_share_setopt(m_shareHandle, youmecommon::CURLSHOPT_SHARE, youmecommon::CURL_LOCK_DATA_DNS);
	}
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_SHARE, m_shareHandle);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_DNS_CACHE_TIMEOUT, 15);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_CONNECTTIMEOUT, 15);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(easyHandle, youmecommon::CURLOPT_SSL_VERIFYPEER, 0L);
	//struct curl_httppost *formpost = NULL;

	FILE* file = NULL;
	struct youmecommon::curl_slist *headers = NULL;
	if (!bUpload)
	{		
		XString strUpDIR = youmecommon::CXFile::get_upper_dir(strSavePath.c_str());
		youmecommon::CXFile::make_dir_tree(strUpDIR.c_str());
#ifdef WIN32
		errno_t errCode = _wfopen_s(&file, strSavePath.c_str(), __XT("wb+"));
		if (errCode != 0 || NULL == file)
		{
			youmecommon::curl_easy_cleanup(easyHandle);
			return false;
		}
#else
		file = fopen(strSavePath.c_str(), __XT("wb+"));
		if (NULL == file)
		{
			youmecommon::curl_easy_cleanup(easyHandle);
			return false;
		}
#endif
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_WRITEFUNCTION, OnDownloadFile);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_WRITEDATA, (void *)&file);
	}
	else
	{
#ifdef WIN32
		errno_t errCode = _wfopen_s(&file, strSavePath.c_str(), __XT("rb"));
		if (errCode != 0 || NULL == file)
		{
			youmecommon::curl_easy_cleanup(easyHandle);
			return false;
		}
#else
		file = fopen(strSavePath.c_str(), __XT("rb"));
		if (NULL == file)
		{
			youmecommon::curl_easy_cleanup(easyHandle);
			return false;
		}
#endif
		fseek(file, 0, SEEK_END);
		long fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		// 文件流地址与上传了多少字节保持对应关系，当前初始化流地址上传数据为0
		progressCallback_t uploadProgress = {param, 0, (size_t)fileSize, 0, iUploadProgressCallback};
		m_uploadFileSizeMap.insert(std::map<std::string, progressCallback_t>::value_type(CStringUtilT<char>::to_string((XUINT64)file), uploadProgress));

		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_WRITEFUNCTION, OnUploadWrite);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_WRITEDATA, (void *)&strResponse);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_PUT, 1L);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_READFUNCTION, UploadReadCallback);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_READDATA, file);
		curl_easy_setopt(easyHandle, youmecommon::CURLOPT_INFILESIZE, (curl_off_t)fileSize);

		//你Y 就不支持中文，当然仅限于坑爹的windwos系统
		//struct curl_httppost *lastptr = NULL;
		//curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "file", CURLFORM_FILE, XStringToLocal(strSavePath).c_str(), CURLFORM_END);
		//curl_easy_setopt(easyHandle, CURLOPT_HTTPPOST, formpost);
		
		if (httpHead.size() > 0)
		{		
			for (std::map<std::string, std::string>::iterator itr = httpHead.begin(); itr != httpHead.end(); ++itr)
			{
				std::stringstream strStream;
				strStream << itr->first << ": " << itr->second;
				headers = youmecommon::curl_slist_append(headers, strStream.str().c_str());
			}
			curl_easy_setopt(easyHandle, youmecommon::CURLOPT_HTTPHEADER, headers);
		}
	}
	youmecommon::CURLcode errorcode = youmecommon::curl_easy_perform(easyHandle);
	//if (NULL != formpost)
	//{
	//	curl_formfree(formpost);
	//}

	std::map<std::string, progressCallback_s>::iterator itr = m_uploadFileSizeMap.find(CStringUtilT<char>::to_string((XUINT64)file));
	if (itr != m_uploadFileSizeMap.end())
	{
		m_uploadFileSizeMap.erase(itr);
	}

	fclose(file);
	long httpCode = 0;
	curl_easy_getinfo(easyHandle, youmecommon::CURLINFO_RESPONSE_CODE, &httpCode);
	youmecommon::curl_easy_cleanup(easyHandle);
    YouMe_LOG_Info(__XT("CURLcode:%d httpCode:%d"),errorcode,httpCode);
	if (headers != NULL)
	{
		youmecommon::curl_slist_free_all(headers);
	}
	if ((errorcode != youmecommon::CURLE_OK) || (httpCode != 200))
	{
		//如果是下载的话则判断一下下载是否成功，失败的话就删掉文件
		if (!bUpload)
		{
#ifdef WIN32
			::DeleteFileW(strSavePath.c_str());
#else
			remove(strSavePath.c_str());
#endif // WIN32	
		}
		YouMe_LOG_Info(__XT("url:%s"), strUrl.c_str());
		return false;
	}
	return true;
}

bool CDownloadUploadManager::UploadFilePost(const XString& url, const XString& localPath, std::map<std::string, std::string>& httpHeads)
{
	youmecommon::CXFile file;
	file.LoadFile(localPath, youmecommon::CXFile::Mode_OpenExist_ReadOnly);
	XINT64 fileSize = file.GetFileSize();
	file.Close();
	
	std::string response;
	if (fileSize <= SINGLE_FILE_MAX_SIZE)	// 8M以内文件一次上传，否则分块上传
	{
		response = UploadFileSingle(url, localPath, httpHeads);
	}
	else
	{
		response = UploadFileSlice(url, localPath, fileSize, httpHeads);
	}

	youmecommon::Value responseJson;
	youmecommon::Reader jsonReader;
	if (!jsonReader.parse(response, responseJson))
	{
		return false;
	}
	if (responseJson.isMember("code") && responseJson["code"].asInt() == 0)
	{
		return true;
	}
	else
	{
		XString result = UTF8TOXString(response);
		YouMe_LOG_Error(__XT("upload failed:%s"), result.c_str());
		return false;
	}
}

std::string CDownloadUploadManager::UploadFileSingle(const XString& url, const XString& localPath, std::map<std::string, std::string>& httpHeads)
{
	youmecommon::CXFile file;
	if (file.LoadFile(localPath, youmecommon::CXFile::Mode_OpenExist_ReadOnly) != 0)
	{
		YouMe_LOG_Error(__XT("open file error"));
		return "";
	}
	XINT64 fileSize = file.GetFileSize();
	youmecommon::CXSharedArray<unsigned char> buffer(fileSize);
	if (file.Read(buffer.Get(), fileSize) != fileSize)
	{
		YouMe_LOG_Error(__XT("read file error"));
		return "";
	}
	file.Close();

	std::map<std::string, std::string> userParams;
	userParams["op"] = "upload";
	userParams["insertOnly"] = "0";

	std::string strUrl = XStringToUTF8(url);
	return SendFilePost(strUrl, httpHeads, userParams, buffer.Get(), buffer.GetBufferLen());
}

std::string CDownloadUploadManager::UploadFileSlice(const XString& url, const XString& localPath, unsigned int fileSize, std::map<std::string, std::string>& httpHeads)
{
	std::string strUrl = XStringToUTF8(url);
	std::string initResponse = FileUploadSliceInit(strUrl, fileSize, httpHeads);
	youmecommon::Value initResponseJson;
	youmecommon::Reader jsonReader;
	if (!jsonReader.parse(initResponse, initResponseJson))
	{
		return initResponse;
	}
	if (!initResponseJson.isMember("code") || initResponseJson["code"].asInt() != 0)
	{
		return initResponse;
	}
	if (initResponseJson["data"].isMember("access_url"))	//秒传
	{
		return initResponse;
	}

	std::string strPath = XStringToUTF8(localPath);
	std::string uploadResponse = FileUploadSliceData(strUrl, strPath, fileSize, httpHeads, initResponse);
	youmecommon::Value uploadResponseJson;
	if (!jsonReader.parse(uploadResponse, uploadResponseJson))
	{
		return uploadResponse;
	}
	if (!initResponseJson.isMember("code") || uploadResponseJson["code"].asInt() != 0)
	{
		return uploadResponse;
	}

	return FileUploadSliceFinish(strUrl, strPath, fileSize, httpHeads, uploadResponse);
}

std::string CDownloadUploadManager::FileUploadSliceInit(const std::string& url, unsigned int fileSize, std::map<std::string, std::string>& httpHeads)
{
	std::map<std::string, std::string> user_params;
	user_params["op"] = "upload_slice_init";
	user_params["insertOnly"] = "0";
	user_params["slice_size"] = CStringUtilT<char>::to_string(SLINCE_SIZE);
	user_params["filesize"] = CStringUtilT<char>::to_string(fileSize);

	return SendFilePost(url, httpHeads, user_params, NULL, 0);
}

std::string CDownloadUploadManager::FileUploadSliceData(const std::string& url, const std::string& localPath, unsigned int fileSize, std::map<std::string, std::string>& httpHeads, const std::string& response)
{
	youmecommon::Value responseJson;
	youmecommon::Reader jsonReader;
	if (!jsonReader.parse(response, responseJson))
	{
		return "";
	}
	if (!responseJson.isMember("data"))
	{
		return "";
	}
	int sliceSize = responseJson["data"]["slice_size"].asInt();
	std::string sessionID = responseJson["data"]["session"].asString();
	/*int serialUpload = 0;
	if (initResponseJson["data"].isMember("serial_upload"))
	{
		serialUpload = initResponseJson["data"]["serial_upload"].asInt();
	}*/

	std::ifstream file(localPath.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open())
	{
		return "";
	}

	std::string uploadResponse;
	unsigned long long offset = 0;
	youmecommon::CXSharedArray<unsigned char> buffer(SLINCE_SIZE);
	while (offset < fileSize)
	{
		file.read((char *)buffer.Get(), SLINCE_SIZE);
		size_t readSize = file.gcount();
		if (readSize == 0 && file.eof())
		{
			break;
		}

		std::map<std::string, std::string> userParams;
		userParams["op"] = "upload_slice_data";
		userParams["session"] = sessionID;
		userParams["offset"] = CStringUtilT<char>::to_string(offset);

		uploadResponse = SendFilePost(url, httpHeads, userParams, buffer.Get(), readSize);
		youmecommon::Value uploadResponseJson;
		if (!jsonReader.parse(uploadResponse, uploadResponseJson))
		{
			break;
		}
		if (!uploadResponseJson.isMember("code") || uploadResponseJson["code"].asInt() != 0)
		{
			break;
		}
		
		offset += readSize;
	}

	file.close();

	return uploadResponse;
}

std::string CDownloadUploadManager::FileUploadSliceFinish(const std::string& url, const std::string& localPath, unsigned int fileSize, std::map<std::string, std::string>& httpHeads, const std::string& response)
{
	youmecommon::Value responseJson;
	youmecommon::Reader jsonReader;
	if (!jsonReader.parse(response, responseJson))
	{
		return "";
	}
	if (!responseJson.isMember("data"))
	{
		return "";
	}
	std::string sessionID = responseJson["data"]["session"].asString();
	std::map<std::string, std::string> userParams;
	userParams["op"] = "upload_slice_finish";
	userParams["session"] = sessionID;
	userParams["filesize"] = CStringUtilT<char>::to_string(fileSize);

	return SendFilePost(url, httpHeads, userParams, NULL, 0);
}

std::string CDownloadUploadManager::SendFilePost(const std::string& url, std::map<std::string, std::string>& httpHeaders, std::map<std::string, std::string>& userParams, const unsigned char* buffer, unsigned int size)
{
	std::string response;
	youmecommon::CURL *easy_curl = youmecommon::curl_easy_init();
	if (NULL == easy_curl)
	{
		return response;
	}
	curl_easy_setopt(easy_curl, youmecommon::CURLOPT_URL, url.c_str());
	curl_easy_setopt(easy_curl, youmecommon::CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(easy_curl, youmecommon::CURLOPT_TIMEOUT_MS, 10000);
	curl_easy_setopt(easy_curl, youmecommon::CURLOPT_CONNECTTIMEOUT_MS, 10000);
	curl_easy_setopt(easy_curl, youmecommon::CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(easy_curl, youmecommon::CURLOPT_SSL_VERIFYPEER, 1);
	curl_easy_setopt(easy_curl, youmecommon::CURLOPT_POST, 1);
	curl_easy_setopt(easy_curl, youmecommon::CURLOPT_WRITEFUNCTION, OnUploadWrite);
	curl_easy_setopt(easy_curl, youmecommon::CURLOPT_WRITEDATA, (void *)&response);

	struct youmecommon::curl_slist *headers = NULL;
	if (httpHeaders.size() > 0)
	{
		for (std::map<std::string, std::string>::const_iterator itr = httpHeaders.begin(); itr != httpHeaders.end(); ++itr)
		{
			std::stringstream strStream;
			strStream << itr->first << ": " << itr->second;
			headers = youmecommon::curl_slist_append(headers, strStream.str().c_str());
		}
		curl_easy_setopt(easy_curl, youmecommon::CURLOPT_HTTPHEADER, headers);
	}

	struct youmecommon::curl_httppost *firstitem = NULL;
	struct youmecommon::curl_httppost *lastitem = NULL;
	std::string param_key = "";
	std::string param_value = "";
	for (std::map<std::string, std::string>::const_iterator it = userParams.begin(); it != userParams.end(); ++it)
	{
		param_key = it->first;
		param_value = it->second;
		youmecommon::curl_formadd(&firstitem, &lastitem, youmecommon::CURLFORM_COPYNAME, param_key.c_str(), youmecommon::CURLFORM_COPYCONTENTS, param_value.c_str(), youmecommon::CURLFORM_END);
	}

	if (size != 0)
	{
		curl_formadd(&firstitem, &lastitem, youmecommon::CURLFORM_COPYNAME, "filecontent", youmecommon::CURLFORM_BUFFER, "data", youmecommon::CURLFORM_BUFFERPTR, buffer, youmecommon::CURLFORM_BUFFERLENGTH, size, youmecommon::CURLFORM_END);
	}
	youmecommon::curl_easy_setopt(easy_curl, youmecommon::CURLOPT_HTTPPOST, firstitem);

	youmecommon::CURLcode errorcode = youmecommon::curl_easy_perform(easy_curl);

	long httpCode = 0;
	curl_easy_getinfo(easy_curl, youmecommon::CURLINFO_RESPONSE_CODE, &httpCode);

	youmecommon::curl_formfree(firstitem);
	youmecommon::curl_easy_cleanup(easy_curl);
	if (headers != NULL)
	{
		youmecommon::curl_slist_free_all(headers);
	}
	if (errorcode != youmecommon::CURLE_OK || httpCode != 200)
	{
		XString strUrl = UTF8TOXString(url);
		YouMe_LOG_Info(__XT("CURLcode:%d httpCode:%d url:%s"), errorcode, httpCode, strUrl.c_str());
	}
	return response;
}

#include "LuaDownloadUploadManager.h"
#include <YouMeCommon/DownloadUploadManager.h>
const char CLuaDownloadUploadManager::className[] = "LuaDownloadUploadManager";
const CLuaDownloadUploadManager::FunctionType CLuaDownloadUploadManager::methods[] = { { "HttpRequest", &CLuaDownloadUploadManager::HttpRequest },
	{ 0 }
};

const CLuaDownloadUploadManager::PropertyType CLuaDownloadUploadManager::properties[] = { 0 };

int CLuaDownloadUploadManager::HttpRequest(youmecommon::lua_State* L)
{
	const char* pszUrl = lua_tostring(L, 1);
	const char* pszBody = lua_tostring(L, 2);
	if ((NULL == pszUrl) || (NULL == pszBody))
	{
		return 0;
	}
	std::string strResponse;
	bool bSuccess = CDownloadUploadManager::HttpRequest(UTF8TOXString2(pszUrl), pszBody, strResponse);
	lua_pushboolean(L, bSuccess);
	lua_pushstring(L, strResponse.c_str());
	return 2;
}

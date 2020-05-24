#include <stdio.h>

#include <YouMeCommon/CrossPlatformDefine/PlatformDef.h>
#include <YouMeCommon/StringUtil.hpp>
#include <YouMeCommon/DownloadUploadManager.h>
#include <YouMeCommon/DownloadUploadManager.h>
#include <YouMeCommon/YouMeDataChannel.h>
#include <YouMeCommon/record.h>
#include <YouMeCommon/DataReport.h>
#include <YouMeCommon/keywordFilter.h>
#include <YouMeCommon/SDKValidate.hpp>
#include <YouMeCommon/CrossPlatformDefine/OSX/YouMeApplication_OSX.H>
#include <thread>



#define SDK_VER 4144			//SDK版本
#define SERVICE_ID 20150418
#define SDK_NAME "YouMeIM"
#define PACKET_ENCRYT_KEY_LEN 16
#define PROTOCOL_VERSION 1
#define PROTOCOL_VERSION_HTTPS 2

#define SDK_AUTH_URL __XT("conn.youme.im")
#define SDK_AUTH_URL_DEV __XT("d.conn.youme.im")
#define SDK_AUTH_URL_TEST __XT("t.conn.youme.im")
#define SDK_AUTH_URL_BUSSINESS __XT("b.conn.youme.im")
#define SDK_INVALIDATE_DEFAULT_IP __XT("106.75.35.102")
#define SDK_INVALIDATE_DEFAULT_IP_2 __XT("47.91.162.99")


/*
#include "SpeechListenManager.h"
#include "WinSpeechManager.h"
#include <YouMeCommon/KeywordFilter.h>

#include <YouMeCommon/lua-5.3.2/LuaDownloadUploadManager.h>

class MySpeechListen :public ISpeechListen
{


	virtual void OnSpeechInit(int iErrorcode) override
	{
	}

	virtual void OnSpeechResult(XUINT64 ullSerial, int iErrorcode, const XString& strResult, const XString& strWavPath) override
	{
	}

};*/

int main(int argc,char** argv)
{
    
    printf("输入ip:");
    char ipBuffer[128] = { 0 };
    scanf("%s", ipBuffer);
    
    
    printf("输入端口号:");
    int iPort;
    scanf("%d", &iPort);
    
    
    
    YouMeApplication_OSX *pSystemProvider = new YouMeApplication_OSX;
    pSystemProvider->setAppKey(__XT("YOUMEBC2B3171A7A165DC10918A7B50A4B939F2A187D0"));
    pSystemProvider->setAppSecret(__XT("r1+ih9rvMEDD3jUoU+nj8C7VljQr7Tuk4TtcByIdyAqjdl5lhlESU0D+SoRZ30sopoaOBg9EsiIMdc8R16WpJPNwLYx2WDT5hI/HsLl1NJjQfa9ZPuz7c/xVb8GHJlMf/wtmuog3bHCpuninqsm3DRWiZZugBTEj2ryrhK7oZncBAAE="));
    pSystemProvider->setSDKVersion(SDK_VER);
    
    
    
    youmecommon::CSDKValidate sdkValid(pSystemProvider, NULL);
    
    youmecommon::SDKInvalidateParam invalidateParam;
    invalidateParam.serviceID = SERVICE_ID;
    invalidateParam.serviceType = YOUMEServiceProtocol::SERVICE_IM;
    invalidateParam.sdkName = SDK_NAME;
    invalidateParam.protocolVersion = PROTOCOL_VERSION;
    invalidateParam.port = iPort;
    invalidateParam.defaultIP.push_back(UTF8TOXString2(ipBuffer));
    
    XString strAppKey = pSystemProvider->getAppKey();
    if (strAppKey.length() < 8)
    {
        YouMe_LOG_Error(__XT("APPKey is invalid"));
        return youmecommon::SDKValidateErrorcode_Fail;
    }
    XString strDomain = strAppKey.substr(strAppKey.length() - 8) + __XT(".");
    strDomain +=  __XT("cn.imcfg.youme.im");
    invalidateParam.domain = strDomain;
    
    
    
    XString strZoneCode = __XT("cn");
    std::map<std::string, youmecommon::CXAny> configurations;
    
    youmecommon::CXCondWait sdkValidWait;
    sdkValid.StartValidate(invalidateParam, configurations, &sdkValidWait, strZoneCode);
    
    
    return 0;

/*
	youmecommon::lua_State* L = youmecommon::luaL_newstate();
	luaL_openlibs(L);
	CLuaDownloadUploadManager::Register(L);

	int code = luaL_dofile(L, "C:\\YouMe\\youmecommon\\src\\Test\\test.lua");
	const char* pMsg = lua_tostring(L, -1);
	if (NULL != pMsg)
	{
		printf(pMsg);
	}
	lua_close(L);




	printf("按下 1，回车开始说话,如果觉得屏幕没有刷新，可以按下任意一个方向键");
	CWinSpeechManager* pWinSpeech = new CWinSpeechManager;
	pWinSpeech->Init("577f51fe");
	pWinSpeech->SetSpeechListen(new MySpeechListen);
	while (true)
	{
		int  command = getchar();
		switch (command)
		{
		case '1': //开始说话
		{
			pWinSpeech->StartSpeech(1000);
		}
		break;
		case '2': //结束说话
		{
			pWinSpeech->StopSpeech();
		}
		break;

		}
	}*/


    return 0;
}
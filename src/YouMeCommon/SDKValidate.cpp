#include "SDKValidate.hpp"
#include <YouMeCommon/SyncTCP.h>
#include <YouMeCommon/Log.h>
#include <YouMeCommon/RSAUtil.h>
#include <YouMeCommon/CryptUtil.h>
#include <YouMeCommon/StringUtil.hpp>
#include <YouMeCommon/XDNSParse.h>
#include <YouMeCommon/profiledb.h>
#include <YouMeCommon/pb/youme_comm.pb.h>
#include <YouMeCommon/pb/youme_service_head.h>
#include <YouMeCommon/pb/youme_comm_conf.pb.h>
#include <YouMeCommon/TimeUtil.h>


#define PACKET_ENCRYT_KEY_LEN 16
#include <iostream>
namespace youmecommon
{

	XUINT64 CSDKValidate::m_nMsgSerial = CTimeUtil::GetTimeOfDay_MS();

	bool CSDKValidate::InterParseSecret(const XString &strSecret,CRSAUtil& rsa)
	{
		bool bParseSuccess = false;
		do
		{
			CXSharedArray<char> pDecodeBuffer;
			if (!::youmecommon::CCryptUtil::Base64Decoder(XStringToUTF8(strSecret), pDecodeBuffer))
			{
				YouMe_LOG_Error(__XT("parsed secret failed:%s"),strSecret.c_str());
				break;
			}
			if (131 != pDecodeBuffer.GetBufferLen())
			{
				YouMe_LOG_Error(__XT("base64 parsed failed,:%s len:%d"), strSecret.c_str (),pDecodeBuffer.GetBufferLen());
				break;
			}
			CXSharedArray<unsigned char> publicKey;
			CXSharedArray<unsigned char>  module;
        
			//模数
			module.Allocate(128);
			memcpy(module.Get(), pDecodeBuffer.Get(), 128);
        
			// publickey
			publicKey.Allocate(3);
			memcpy(publicKey.Get(), pDecodeBuffer.Get()+128, 3);
        
			bParseSuccess = rsa.SetPublicKey (publicKey, module);
		} while (0);
    
		return bParseSuccess;
	}

	CSDKValidate::CSDKValidate(IYouMeSystemProvider* pSystemProvier, CProfileDB* pProfileDB)
	{
		m_pSystemProvider = pSystemProvier;
		m_pProfileDB = pProfileDB;
	}

	SDKValidateErrorcode CSDKValidate::StartValidate(SDKInvalidateParam& invalidataParam, std::map<std::string, CXAny>&configurations, CXCondWait* sdkValidWait, const XString& strZone)
	{
		//DNS域名解析
		std::vector<XString> ipList;
		//读出备份的ip
		XString strBackupIP;
		if (NULL != m_pProfileDB)
		{
			//m_pProfileDB->getSetting(XStringToUTF8(invalidataParam.domain), strBackupIP);

		}
		/*
		if (!strBackupIP.empty())
		{
			ipList.push_back(strBackupIP);
		}
		*/
		CXDNSParse::ParseDomain2(invalidataParam.domain, ipList, 3000);
	
	
			
	
		ipList.insert(ipList.end(), invalidataParam.defaultIP.begin(), invalidataParam.defaultIP.end());
	
		CRSAUtil rsa;
		if (!InterParseSecret(m_pSystemProvider->getAppSecret(), rsa))
		{
			YouMe_LOG_Error(__XT("appSecurity resolve error:%s"), m_pSystemProvider->getAppSecret().c_str());
			return SDKValidateErrorcode_ILLEGAL_SDK;
		}
		//使用公钥对一个随机的数字进行rsa加密
		int iRand = rand();
		CXSharedArray<unsigned char>  rsaEncryptBuffer;
		if (!rsa.EncryptByPublicKey((const unsigned char *)&iRand, sizeof(int), rsaEncryptBuffer))
		{
			YouMe_LOG_Error(__XT("encrypt error"));
			return SDKValidateErrorcode_ILLEGAL_SDK;
		}

		YOUMEServiceProtocol::CommConfReq req;
		req.set_version(invalidataParam.protocolVersion);
		req.set_appkey(XStringToUTF8(m_pSystemProvider->getAppKey()));
		req.set_verify(rsaEncryptBuffer.Get(), rsaEncryptBuffer.GetBufferLen());
		req.set_service_type(invalidataParam.serviceType);
	#ifdef WIN32
		req.set_platform(YOUMECommonProtocol::Platform_Windows);
	#elif OS_OSX
		req.set_platform(YOUMECommonProtocol::Platform_OSX);
	#elif OS_IOS
		req.set_platform(YOUMECommonProtocol::Platform_IOS);
	#elif OS_IOSSIMULATOR
		req.set_platform(YOUMECommonProtocol::Platform_IOS);
	#elif OS_ANDROID
		req.set_platform(YOUMECommonProtocol::Platform_Android);
	#elif OS_LINUX
		req.set_platform(YOUMECommonProtocol::Platform_Linux);
	#endif
		req.set_brand(XStringToUTF8(m_pSystemProvider->getBrand()));
		req.set_sys_version(XStringToUTF8(m_pSystemProvider->getSystemVersion()));
		req.set_cpu_arch(XStringToUTF8(m_pSystemProvider->getCpuArchive()));
		req.set_pkg_name(XStringToUTF8(m_pSystemProvider->getPackageName()));
		req.set_device_token(XStringToUTF8(m_pSystemProvider->getUUID()));
		req.set_model(XStringToUTF8(m_pSystemProvider->getModel()));
		req.set_cpu_chip(XStringToUTF8(m_pSystemProvider->getCpuChip()));
		req.set_sdk_version(m_pSystemProvider->getSDKVersion());
		req.set_sdk_name(invalidataParam.sdkName);
		req.set_strzone(XStringToUTF8(strZone));
		SYoumeConnHead connHead;
		connHead.m_usCmd = 1;
		connHead.m_ullReqSeq = m_nMsgSerial++;
		connHead.m_uiServiceId = invalidataParam.serviceID;

		unsigned char szReqBuffer[4096] = { 0 };
		unsigned int iDataSize = 0;
		memcpy(szReqBuffer, &connHead, sizeof(connHead));
		iDataSize += sizeof(connHead);

		unsigned char* key = szReqBuffer + iDataSize;
		GenerateRandKey(key, PACKET_ENCRYT_KEY_LEN);
		iDataSize += PACKET_ENCRYT_KEY_LEN;

		req.SerializeToArray(szReqBuffer + iDataSize, 4096 - iDataSize);
		EncryDecryptPacketBody(szReqBuffer + iDataSize, req.ByteSize(), key, 16);
		iDataSize += req.ByteSize();

		((SYoumeConnHead*)(szReqBuffer))->ToNetOrder();
		((SYoumeConnHead*)(szReqBuffer))->SetTotalLength(szReqBuffer, iDataSize);

		sdkValidWait->Reset();
        
        for( std::vector<short>::size_type k = 0 ; k < invalidataParam.port.size(); ++k ){
            auto port = invalidataParam.port[k];
           
            for (std::vector<XString>::size_type i = 0; i < ipList.size(); ++i)
            {
                YouMe_LOG_Info(__XT("SDK validata IP:%s port:%d"), ipList.at(i).c_str(), port );
                ::youmecommon::CSyncTCP sdkTCP;
                if (!sdkTCP.Init(ipList.at(i), port))
                {
                    YouMe_LOG_Error(__XT("TCP init failed"));
                    return SDKValidateErrorcode_Fail;
                }
                if (!sdkTCP.Connect(15))
                {
                    YouMe_LOG_Error(__XT("connect error(%s)"), ipList.at(i).c_str());
                    continue;
                }
                sdkTCP.SendBufferData((char*)szReqBuffer, iDataSize);
                
                YouMe_LOG_Debug(__XT("send packet command:%d serial:%llu size:%d"), (int)connHead.m_usCmd, connHead.m_ullReqSeq, iDataSize);
                
                //判断是否要退出
                if (WaitResult_Timeout != sdkValidWait->WaitTime(0))
                {
                    YouMe_LOG_Info(__XT("SDK vaild exit"));
                    return SDKValidateErrorcode_Abort;
                }
    
        
                CXSharedArray<char> headBuffer;
                if (sdkTCP.RecvDataByLen(sizeof(SYoumeConnHead), headBuffer) != sizeof(SYoumeConnHead))
                {
                    YouMe_LOG_Error(__XT("SDK validate response recevie packet error"));
                    continue;
                }
                SYoumeConnHead *pConnHead = (SYoumeConnHead*)headBuffer.Get();
                pConnHead->Unpack((unsigned char*)headBuffer.Get());
                if (pConnHead->m_usLen <= sizeof(SYoumeConnHead) + PACKET_ENCRYT_KEY_LEN)
                {
                    YouMe_LOG_Error(__XT("SDK validate response packet size error size:%d"), (int)pConnHead->m_usLen);
                    continue;
                }
        
                CXSharedArray<char> packetBodyBuf;
                int nPackBodyLen = sdkTCP.RecvDataByLen(pConnHead->m_usLen - sizeof(SYoumeConnHead), packetBodyBuf);
                YouMe_LOG_Debug(__XT("recv packet command:%d serial:%llu size:%d"), (int)pConnHead->m_usCmd, pConnHead->m_ullReqSeq, nPackBodyLen);
                if (nPackBodyLen <= 0)
                {
                    YouMe_LOG_Error(__XT("SDK validate receive packet body error"));
                    continue;
                }
                else
                {
                    //更新ip
                    if (NULL != m_pProfileDB)
                    {
                        m_pProfileDB->setSetting(XStringToUTF8(invalidataParam.domain), ipList.at(i));
                    }
                    
                    unsigned char key[PACKET_ENCRYT_KEY_LEN] = { 0 };
                    memcpy(key, packetBodyBuf.Get(), PACKET_ENCRYT_KEY_LEN);
                    EncryDecryptPacketBody((unsigned char*)packetBodyBuf.Get() + PACKET_ENCRYT_KEY_LEN, packetBodyBuf.GetBufferLen() - PACKET_ENCRYT_KEY_LEN, key, PACKET_ENCRYT_KEY_LEN);
                    YOUMEServiceProtocol::CommConfRsp rsp;
                    if (!rsp.ParseFromArray(packetBodyBuf.Get() + PACKET_ENCRYT_KEY_LEN, packetBodyBuf.GetBufferLen() - PACKET_ENCRYT_KEY_LEN))
                    {
                        YouMe_LOG_Error(__XT("SDK validate unpack error serial:%llu size:%u"), pConnHead->m_ullReqSeq, packetBodyBuf.GetBufferLen());
                        return SDKValidateErrorcode_ProtoError;
                    }
                    
                    return OnSDKValidateRsp(rsp, configurations);
                }
            }
        }

		return SDKValidateErrorcode_Fail;
	}


	SDKValidateErrorcode CSDKValidate::OnSDKValidateRsp(YOUMEServiceProtocol::CommConfRsp& rsp, std::map<std::string, CXAny>& configurations)
	{
		YouMe_LOG_Info(__XT("SDK validate ret:%d svr_time:%llu appid:%d svr_addr:%s svr_port:%d config:%d"), rsp.ret(), rsp.svr_time(), rsp.appid(), UTF8TOXString(rsp.svr_addr()).c_str(), rsp.svr_port(), rsp.conf_size());
		int nRet = rsp.ret();
		if (nRet != 0)
		{
			YouMe_LOG_Error(__XT("SDK invalidate error(%d)"), rsp.ret());
			SDKValidateErrorcode errorcode = SDKValidateErrorcode_Fail;
			if (1 == nRet)
			{
				errorcode = SDKValidateErrorcode_InvalidAppkey;
			}
			else if (2 == nRet)
			{
				errorcode = SDKValidateErrorcode_InvalidFailed;
			}
			return errorcode;
		}

		configurations.insert(std::map<std::string, CXAny>::value_type("ACCESS_SERVER_ADDR", UTF8TOXString(rsp.svr_addr())));
		configurations.insert(std::map<std::string, CXAny>::value_type("ACCESS_SERVER_PORT", rsp.svr_port()));
		configurations.insert(std::map<std::string, CXAny>::value_type("APP_SERVICE_ID", rsp.appid()));
		configurations.insert(std::map<std::string, CXAny>::value_type("SERVER_TIME", rsp.svr_time()));
        
 
        std::stringstream ss;
        for( int k = 0 ; k < rsp.svr_list_size(); k++ ){
            
            ss<< rsp.svr_list(k).svr_addr() <<","<< rsp.svr_list( k ).svr_port();
            
            if( k != rsp.svr_list_size() - 1 ){
                ss<<";";
            }
        }
        
        configurations.insert( std::map<std::string, CXAny>::value_type( "ACCESS_SERVER_ADDR_PORT_ALL",
                                                                        UTF8TOXString( ss.str() )) );
        
		for (int i = 0; i < rsp.conf_size(); ++i)
		{
			YouMe_LOG_Info(__XT("type:%d key:%s value:%s"), rsp.conf(i).value_type(), UTF8TOXString(rsp.conf(i).name()).c_str(), UTF8TOXString(rsp.conf(i).value()).c_str());
			switch (rsp.conf(i).value_type()) 
			{
			case YOUMECommonProtocol::VALUE_BOOL:
			{
				configurations.insert(std::map<std::string, CXAny>::value_type(rsp.conf(i).name(), (CStringUtilT<char>::str_to_bool(rsp.conf(i).value().c_str()))));
			}
			break;
			case YOUMECommonProtocol::VALUE_INT32:
			{
				configurations.insert(std::map<std::string, CXAny>::value_type(rsp.conf(i).name(), CStringUtilT<char>::str_to_sint32(rsp.conf(i).value().c_str())));
			}
			break;
			case YOUMECommonProtocol::VALUE_INT64:
			{
				configurations.insert(std::map<std::string, CXAny>::value_type(rsp.conf(i).name(), CStringUtilT<char>::str_to_sint64(rsp.conf(i).value().c_str())));
			}
			break;
			case YOUMECommonProtocol::VALUE_UIN32:
			{
				configurations.insert(std::map<std::string, CXAny>::value_type(rsp.conf(i).name(), CStringUtilT<char>::str_to_uint32(rsp.conf(i).value().c_str())));
			}
			break;
			case YOUMECommonProtocol::VALUE_UINT64:
			{
				configurations.insert(std::map<std::string, CXAny>::value_type(rsp.conf(i).name(), CStringUtilT<char>::str_to_uint64(rsp.conf(i).value().c_str())));
			}
			break;
			case YOUMECommonProtocol::VALUE_STRING:
			{
				configurations.insert(std::map<std::string, CXAny>::value_type(rsp.conf(i).name(), UTF8TOXString(rsp.conf(i).value())));
			}
			break;
			default:
				YouMe_LOG_Warning(__XT("can not deal type：%d key:%s value:%s"), rsp.conf(i).value_type(), UTF8TOXString(rsp.conf(i).name()).c_str(), UTF8TOXString(rsp.conf(i).value()).c_str());
				break;
			}
		}
		return SDKValidateErrorcode_Success;
	}
	
	void CSDKValidate::EncryDecryptPacketBody(unsigned char* buffer, int bufferLen, unsigned char* key, int keyLen)
	{
		for (int i = 0; i < bufferLen; ++i)
		{
			buffer[i] ^= key[i % keyLen];
		}
	}

	void CSDKValidate::GenerateRandKey(unsigned char* key, int keyLen)
	{
		srand(time(NULL));
		for (int i = 0; i < keyLen; ++i)
		{
			key[i] = rand() % 26 + 97;
		}
	}
}

#include "AccessValidate.h"
#include <YouMeCommon/Log.h>
#include <YouMeCommon/RSAUtil.h>
#include <YouMeCommon/CryptUtil.h>
#include <YouMeCommon/StringUtil.hpp>
#include <YouMeCommon/DNSUtil.h>
#include <YouMeCommon/profiledb.h>
#include <YouMeCommon/pb/youme_comm.pb.h>
#include <YouMeCommon/pb/youme_service_head.h>
#include <YouMeCommon/pb/youme_comm_conf.pb.h>
#include <YouMeCommon/TimeUtil.h>
#include <YouMeCommon/XSharedArray.h>


#define PACKET_ENCRYT_KEY_LEN 16
#define SDK_VALIDATE_TIMEOUT 5000
#define SDK_VALIDATE_CONNECT_TIMEOUT 5000

namespace youmecommon
{

	XUINT64 AccessValidate::m_nMsgSerial = CTimeUtil::GetTimeOfDay_MS();

	AccessValidate::AccessValidate(IYouMeSystemProvider* pSystemProvier, CProfileDB* pProfileDB, SDKValidateCallback* callback) : m_pSystemProvider(pSystemProvier)
		, m_pProfileDB(pProfileDB)
		, m_pCallback(callback)
		, m_pTcpClient(NULL)
		, m_iCurrentIpIndex(-1)
		, m_iCurrentPortIndex(0)
		, m_bValidateRunning(false)
		, m_iServiceID(0)
		, m_validateStatus(VSTATUS_ERROR)
	{
		
	}

	AccessValidate::~AccessValidate()
	{
		m_bValidateRunning = false;
		m_pCallback = NULL;
		m_validateWait.SetSignal();

		if (m_pTcpClient != NULL)
		{
			m_pTcpClient->DisconnectAndWait();
			delete m_pTcpClient;
		}

		{
			std::lock_guard<std::mutex> lock(m_validateThreadMutex);
			if (m_validateThread.joinable())
			{
				m_validateThread.join();
			}
		}
	}

	bool AccessValidate::InterParseSecret(const XString &strSecret, CRSAUtil& rsa)
	{
		bool bParseSuccess = false;
		do
		{
			CXSharedArray<char> pDecodeBuffer;
			if (!::youmecommon::CCryptUtil::Base64Decoder(XStringToUTF8(strSecret), pDecodeBuffer))
			{
				YouMe_LOG_Error(__XT("parsed secret failed:%s"), strSecret.c_str());
				break;
			}
			if (131 != pDecodeBuffer.GetBufferLen())
			{
				YouMe_LOG_Error(__XT("base64 parsed failed,:%s len:%d"), strSecret.c_str(), pDecodeBuffer.GetBufferLen());
				break;
			}
			CXSharedArray<unsigned char> publicKey;
			CXSharedArray<unsigned char>  module;

			//模数
			module.Allocate(128);
			memcpy(module.Get(), pDecodeBuffer.Get(), 128);

			// publickey
			publicKey.Allocate(3);
			memcpy(publicKey.Get(), pDecodeBuffer.Get() + 128, 3);

			bParseSuccess = rsa.SetPublicKey(publicKey, module);
		} while (0);

		return bParseSuccess;
	}

	void AccessValidate::EncryDecryptPacketBody(unsigned char* buffer, int bufferLen, unsigned char* key, int keyLen)
	{
		for (int i = 0; i < bufferLen; ++i)
		{
			buffer[i] ^= key[i % keyLen];
		}
	}

	void AccessValidate::GenerateRandKey(unsigned char* key, int keyLen)
	{
		srand(time(NULL));
		for (int i = 0; i < keyLen; ++i)
		{
			key[i] = rand() % 26 + 97;
		}
	}

	SDKValidateErrorcode AccessValidate::StartValidate(SDKValidateParam& validataParam, ValidateReason reason)
	{
		if (validataParam.port.size() == 0)
		{
			YouMe_LOG_Error(__XT("port is empty"));
			return SDKValidateErrorcode_Fail;
		}

		if (m_bValidateRunning)
		{
			YouMe_LOG_Error(__XT("is validating"));
			return SDKValidateErrorcode_Success;
		}
		{
			std::lock_guard<std::mutex> lock(m_validateThreadMutex);
			if (m_validateThread.joinable())
			{
				m_validateThread.join();
			}

			m_bValidateRunning = true;
			m_reason = reason;
			m_iCurrentIpIndex = -1;
			m_iCurrentPortIndex = 0;
			XUINT64 startTime = CTimeUtil::GetTimeOfDay_MS();
			m_validateStatus = VSTATUS_ERROR;
			m_validateWait.Reset();
			m_validateThread = std::thread(&AccessValidate::ValidateThread, this, validataParam, startTime);
		}

		return SDKValidateErrorcode_Success;
	}

	void AccessValidate::Disconnect()
	{
		if (m_pTcpClient != NULL)
		{
			m_pTcpClient->DisconnectAndWait();
			delete m_pTcpClient;
			m_pTcpClient = NULL;
		}
		m_bValidateRunning = false;
		m_validateWait.SetSignal();
	}

	void AccessValidate::ValidateThread(const SDKValidateParam& validateParam, XUINT64 startTime)
	{
		YouMe_LOG_Debug(__XT("enter"));

		m_ipList.clear();
		m_portList = validateParam.port;
		m_iServiceID = validateParam.serviceID;
		if (NULL != m_pProfileDB)
		{
			//读出备份的ip
            /*
			XString strBackupIP;
			m_pProfileDB->getSetting(validateParam.domain, strBackupIP);
			if (!strBackupIP.empty())
			{
				m_ipList.push_back(XStringToUTF8(strBackupIP));
			}
             */
		}

		m_strDomain = validateParam.domain;
		std::vector<std::string> ipList;
		DNSUtil::Instance()->GetHostByNameAsync(validateParam.domain, ipList, 1000);
		for (std::vector<std::string>::const_iterator it = ipList.begin(); it != ipList.end(); ++it)
		{
			/*if (find(m_ipList.begin(), m_ipList.end(), *it) == m_ipList.end())
			{
				m_ipList.push_back(*it);
			}*/
			bool exist = false;
			for (std::vector<std::string>::const_iterator itInner = m_ipList.begin(); itInner != m_ipList.end(); ++itInner)
			{
				if (*itInner == *it)
				{
					exist = true;
					break;
				}
			}
			if (!exist)
			{
				m_ipList.push_back(*it);
			}
		}

		if (!m_bValidateRunning)
		{
			YouMe_LOG_Info(__XT("validate exit"));
			return;
		}

		m_ipList.insert(m_ipList.end(), validateParam.defaultIP.begin(), validateParam.defaultIP.end());

		SDKValidateErrorcode errorcode = SDKValidateErrorcode_Success;
		CXSharedArray<unsigned char> rsaEncryptBuffer;
		do
		{
			if (m_ipList.size() == 0 || m_portList.size() == 0)
			{
				YouMe_LOG_Error(__XT("ip or port empty"));
				errorcode = SDKValidateErrorcode_Fail;
				break;
			}

			CRSAUtil rsa;
			if (!InterParseSecret(m_pSystemProvider->getAppSecret(), rsa))
			{
				YouMe_LOG_Error(__XT("appSecurity resolve error:%s"), m_pSystemProvider->getAppSecret().c_str());
				errorcode = SDKValidateErrorcode_ILLEGAL_SDK;
				break;
			}
			//使用公钥对一个随机的数字进行rsa加密
			int iRand = rand();
			if (!rsa.EncryptByPublicKey((const unsigned char *)&iRand, sizeof(int), rsaEncryptBuffer))
			{
				YouMe_LOG_Error(__XT("encrypt error"));
				errorcode = SDKValidateErrorcode_ILLEGAL_SDK;
			}
		} while (0);

		if (errorcode != SDKValidateErrorcode_Success)
		{
			if (m_pCallback != NULL)
			{
				unsigned int costTime = static_cast<unsigned int>(CTimeUtil::GetTimeOfDay_MS() - startTime);
				std::string strIP;
				m_pCallback->OnSDKValidteComplete(errorcode, m_configurations, costTime, m_reason, strIP);
			}
			m_bValidateRunning = false;
			return;
		}

		m_requestData.set_version(validateParam.protocolVersion);
		m_requestData.set_appkey(XStringToUTF8(m_pSystemProvider->getAppKey()));
		m_requestData.set_verify(rsaEncryptBuffer.Get(), rsaEncryptBuffer.GetBufferLen());
		m_requestData.set_service_type(validateParam.serviceType);
#ifdef WIN32
		m_requestData.set_platform(YOUMECommonProtocol::Platform_Windows);
#elif OS_OSX
		m_requestData.set_platform(YOUMECommonProtocol::Platform_OSX);
#elif OS_IOS
		m_requestData.set_platform(YOUMECommonProtocol::Platform_IOS);
#elif OS_IOSSIMULATOR
		m_requestData.set_platform(YOUMECommonProtocol::Platform_IOS);
#elif OS_ANDROID
		m_requestData.set_platform(YOUMECommonProtocol::Platform_Android);
#elif OS_LINUX
		m_requestData.set_platform(YOUMECommonProtocol::Platform_Linux);
#endif
		m_requestData.set_brand(XStringToUTF8(m_pSystemProvider->getBrand()));
		m_requestData.set_sys_version(XStringToUTF8(m_pSystemProvider->getSystemVersion()));
		m_requestData.set_cpu_arch(XStringToUTF8(m_pSystemProvider->getCpuArchive()));
		m_requestData.set_pkg_name(XStringToUTF8(m_pSystemProvider->getPackageName()));
		m_requestData.set_device_token(XStringToUTF8(m_pSystemProvider->getUUID()));
		m_requestData.set_model(XStringToUTF8(m_pSystemProvider->getModel()));
		m_requestData.set_cpu_chip(XStringToUTF8(m_pSystemProvider->getCpuChip()));
		m_requestData.set_sdk_version(m_pSystemProvider->getSDKVersion());
		m_requestData.set_sdk_name(validateParam.sdkName);
		m_requestData.set_strzone(validateParam.serverZone);		

		int ret = StartConnect();
		if (ret != 0)
		{
			return;
		}

		while (true)
		{
			if (WaitResult_Timeout == m_validateWait.WaitTime(SDK_VALIDATE_TIMEOUT))
			{
				YouMe_LOG_Info(__XT("SDK validate timeout"));
				errorcode = SDKValidateErrorcode_Timeout;
				ret = StartConnect();
				if (ret != 0)
				{
					break;
				}
				continue;
			}

			if (!m_bValidateRunning)
			{
				break;
			}

			if (VSTATUS_SUCCESS == m_validateStatus)
			{
				errorcode = SDKValidateErrorcode_Success;
				m_pTcpClient->Disconnect();
				break;
			}
			else if (VSTATUS_TIMEOUT == m_validateStatus || VSTATUS_NETWORK_ERROR == m_validateStatus)
			{
				if (m_iCurrentPortIndex == m_portList.size() - 1 && m_iCurrentIpIndex == m_ipList.size() - 1)
				{
					errorcode = VSTATUS_TIMEOUT == m_validateStatus ? SDKValidateErrorcode_Timeout : SDKValidateErrorcode_Fail;
					break;
				}
				else
				{
					StartConnect();
				}
			}
			else
			{
				errorcode = SDKValidateErrorcode_Fail;
				break;
			}

			m_validateWait.Reset();
		}

		bool lastValidateStatus = m_bValidateRunning;
		m_bValidateRunning = false;
		std::string strIP;
		if (m_pTcpClient != NULL)
		{
			strIP = m_pTcpClient->GetIP();
		}
		if (m_pTcpClient != NULL)
		{
			m_pTcpClient->Disconnect();
			//delete m_pTcpClient;
			//m_pTcpClient = NULL;
		}

		if (lastValidateStatus && m_pCallback != NULL)
		{
			unsigned int costTime = static_cast<unsigned int>(CTimeUtil::GetTimeOfDay_MS() - startTime);
			m_pCallback->OnSDKValidteComplete(errorcode, m_configurations, costTime, m_reason, strIP);
		}

		YouMe_LOG_Debug(__XT("leave"));
	}

	int AccessValidate::StartConnect()
	{
		if (m_iCurrentPortIndex == m_portList.size() - 1 && m_iCurrentIpIndex == m_ipList.size() - 1)
		{
			return -1;
		}
		if (m_iCurrentIpIndex == m_ipList.size() - 1)
		{
			m_iCurrentIpIndex = 0;
			++m_iCurrentPortIndex;
		}
		else
		{
			++m_iCurrentIpIndex;
		}
		std::string ip = m_ipList.at(m_iCurrentIpIndex);
		unsigned port = m_portList.at(m_iCurrentPortIndex);

		if (m_pTcpClient != NULL)
		{
			m_pTcpClient->DisconnectAndWait();
			delete m_pTcpClient;
		}

		m_pTcpClient = new TcpClient(ip.c_str(), port, *this, SDK_VALIDATE_CONNECT_TIMEOUT);
		bool ret = m_pTcpClient->Connect();
		return !ret;
	}

	void AccessValidate::RequestValidateData()
	{
		SYoumeConnHead connHead;
		connHead.m_usCmd = 1;
		connHead.m_ullReqSeq = m_nMsgSerial++;
		connHead.m_uiServiceId = m_iServiceID;
		CXSharedArray<unsigned char> m_pRequestBuffer(sizeof(SYoumeConnHead) + PACKET_ENCRYT_KEY_LEN + m_requestData.ByteSize());
		memcpy(m_pRequestBuffer.Get(), &connHead, sizeof(connHead));

		unsigned int iDataSize = sizeof(connHead);
		unsigned char* key = m_pRequestBuffer.Get() + iDataSize;
		GenerateRandKey(key, PACKET_ENCRYT_KEY_LEN);
		iDataSize += PACKET_ENCRYT_KEY_LEN;

		m_requestData.SerializeToArray(m_pRequestBuffer.Get() + iDataSize, 4096 - iDataSize);
		EncryDecryptPacketBody(m_pRequestBuffer.Get() + iDataSize, m_requestData.ByteSize(), key, 16);
		iDataSize += m_requestData.ByteSize();

		((SYoumeConnHead*)(m_pRequestBuffer.Get()))->ToNetOrder();
		((SYoumeConnHead*)(m_pRequestBuffer.Get()))->SetTotalLength(m_pRequestBuffer.Get(), iDataSize);

		if (m_pTcpClient != NULL)
		{
			int ret = m_pTcpClient->Write(m_pRequestBuffer.Get(), m_pRequestBuffer.GetBufferLen());
			if (ret < 0)
			{
				YouMe_LOG_Error(__XT("send validate failed"));
				m_validateStatus = VSTATUS_ERROR;
				m_validateWait.SetSignal();
			}
		}
	}

	SDKValidateErrorcode AccessValidate::OnSDKValidateRsp(YOUMEServiceProtocol::CommConfRsp& rsp)
	{
		YouMe_LOG_Info(__XT("SDK validate ret:%d svr_time:%llu appid:%d config:%d"), rsp.ret(), rsp.svr_time(), rsp.appid(), rsp.conf_size());
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

		m_configurations.clear();

		m_configurations.insert(std::map<std::string, CXAny>::value_type("ACCESS_SERVER_ADDR", UTF8TOXString(rsp.svr_addr())));
		m_configurations.insert(std::map<std::string, CXAny>::value_type("ACCESS_SERVER_PORT", rsp.svr_port()));
		m_configurations.insert(std::map<std::string, CXAny>::value_type("APP_SERVICE_ID", rsp.appid()));
		m_configurations.insert(std::map<std::string, CXAny>::value_type("SERVER_TIME", rsp.svr_time()));

		std::stringstream ss;
		for (int k = 0; k < rsp.svr_list_size(); k++)
		{
			ss << rsp.svr_list(k).svr_addr() << "," << rsp.svr_list(k).svr_port();
			if (k != rsp.svr_list_size() - 1)
			{
				ss << ";";
			}
		}

		m_configurations.insert(std::map<std::string, CXAny>::value_type("ACCESS_SERVER_ADDR_PORT_ALL", UTF8TOXString(ss.str())));

		//YouMe_LOG_Debug(__XT("addr:%s"), UTF8TOXString(ss.str()).c_str());

		for (int i = 0; i < rsp.conf_size(); ++i)
		{
			YouMe_LOG_Info(__XT("type:%d key:%s value:%s"), rsp.conf(i).value_type(), UTF8TOXString(rsp.conf(i).name()).c_str(), UTF8TOXString(rsp.conf(i).value()).c_str());
			switch (rsp.conf(i).value_type())
			{
			case YOUMECommonProtocol::VALUE_BOOL:
			{
				m_configurations.insert(std::map<std::string, CXAny>::value_type(rsp.conf(i).name(), (CStringUtilT<char>::str_to_bool(rsp.conf(i).value().c_str()))));
			}
			break;
			case YOUMECommonProtocol::VALUE_INT32:
			{
				m_configurations.insert(std::map<std::string, CXAny>::value_type(rsp.conf(i).name(), CStringUtilT<char>::str_to_sint32(rsp.conf(i).value().c_str())));
			}
			break;
			case YOUMECommonProtocol::VALUE_INT64:
			{
				m_configurations.insert(std::map<std::string, CXAny>::value_type(rsp.conf(i).name(), CStringUtilT<char>::str_to_sint64(rsp.conf(i).value().c_str())));
			}
			break;
			case YOUMECommonProtocol::VALUE_UIN32:
			{
				m_configurations.insert(std::map<std::string, CXAny>::value_type(rsp.conf(i).name(), CStringUtilT<char>::str_to_uint32(rsp.conf(i).value().c_str())));
			}
			break;
			case YOUMECommonProtocol::VALUE_UINT64:
			{
				m_configurations.insert(std::map<std::string, CXAny>::value_type(rsp.conf(i).name(), CStringUtilT<char>::str_to_uint64(rsp.conf(i).value().c_str())));
			}
			break;
			case YOUMECommonProtocol::VALUE_STRING:
			{
				m_configurations.insert(std::map<std::string, CXAny>::value_type(rsp.conf(i).name(), UTF8TOXString(rsp.conf(i).value())));
			}
			break;
			default:
				YouMe_LOG_Warning(__XT("can not deal type：%d key:%s value:%s"), rsp.conf(i).value_type(), UTF8TOXString(rsp.conf(i).name()).c_str(), UTF8TOXString(rsp.conf(i).value()).c_str());
				break;
			}
		}
		return SDKValidateErrorcode_Success;
	}

	void AccessValidate::OnConnect()
	{
		YouMe_LOG_Debug(__XT("SDK validate connected"));
		RequestValidateData();
	}

	void AccessValidate::OnDisConnect(bool _isremote)
	{
		YouMe_LOG_Info(__XT("SDK validate disconnected"));
		m_validateWait.SetSignal();
	}

	void AccessValidate::OnError(int _status, int _errcode)
	{
		YouMe_LOG_Error(__XT("network error status:%d errrocode:%d"), _status, _errcode);
		m_validateStatus = SOCKET_ERRNO(ETIMEDOUT) ? VSTATUS_TIMEOUT : VSTATUS_NETWORK_ERROR;
		m_validateWait.SetSignal();
	}

	void AccessValidate::OnWrote(int _id, unsigned int _len)
	{
	}

	void AccessValidate::OnAllWrote()
	{
	}

	void AccessValidate::OnRead()
	{
		SDKValidateErrorcode errorcode = SDKValidateErrorcode_Fail;
		do 
		{
			if (m_pTcpClient == NULL)
			{
				break;
			}
			unsigned char headBuffer[PACKET_ENCRYT_KEY_LEN] = {0};
			if (m_pTcpClient->Read(headBuffer, sizeof(SYoumeConnHead)) != sizeof(SYoumeConnHead))
			{
				YouMe_LOG_Error(__XT("SDK validate response recevie packet error"));
				break;
			}
			SYoumeConnHead *pConnHead = (SYoumeConnHead*)headBuffer;
			pConnHead->Unpack((unsigned char*)headBuffer);
			if (pConnHead->m_usLen <= sizeof(SYoumeConnHead) + PACKET_ENCRYT_KEY_LEN)
			{
				YouMe_LOG_Error(__XT("SDK validate response packet size error size:%d"), (int)pConnHead->m_usLen);
				break;
			}

			int bodyLength = pConnHead->m_usLen - sizeof(SYoumeConnHead);
			CXSharedArray<char> packetBodyBuf(bodyLength);
			int nPackBodyLen = m_pTcpClient->Read(packetBodyBuf.Get(), bodyLength);
			if (nPackBodyLen < bodyLength)
			{
				YouMe_LOG_Error(__XT("SDK validate receive packet body error, command:%d serial:%llu size:%d"), (int)pConnHead->m_usCmd, pConnHead->m_ullReqSeq, nPackBodyLen);
				break;
			}
			else
			{
				//更新ip
				if (NULL != m_pProfileDB)
				{
					XString strIP = UTF8TOXString(m_ipList.at(m_iCurrentIpIndex));
					m_pProfileDB->setSetting(m_strDomain, strIP);
				}

				unsigned char key[PACKET_ENCRYT_KEY_LEN] = { 0 };
				memcpy(key, packetBodyBuf.Get(), PACKET_ENCRYT_KEY_LEN);
				EncryDecryptPacketBody((unsigned char*)packetBodyBuf.Get() + PACKET_ENCRYT_KEY_LEN, packetBodyBuf.GetBufferLen() - PACKET_ENCRYT_KEY_LEN, key, PACKET_ENCRYT_KEY_LEN);
				YOUMEServiceProtocol::CommConfRsp rsp;
				if (!rsp.ParseFromArray(packetBodyBuf.Get() + PACKET_ENCRYT_KEY_LEN, packetBodyBuf.GetBufferLen() - PACKET_ENCRYT_KEY_LEN))
				{
					YouMe_LOG_Error(__XT("SDK validate unpack error serial:%llu size:%u"), pConnHead->m_ullReqSeq, packetBodyBuf.GetBufferLen());
					errorcode = SDKValidateErrorcode_ProtoError;
					break;
				}

				errorcode = OnSDKValidateRsp(rsp);
			}
		} while (0);

		if (SDKValidateErrorcode_Fail == errorcode && NULL != m_pProfileDB)
		{
			m_pProfileDB->deleteByKey(m_strDomain);
		}

		m_validateStatus = errorcode == SDKValidateErrorcode_Success ? VSTATUS_SUCCESS : VSTATUS_ERROR;

		m_validateWait.SetSignal();
	}
}

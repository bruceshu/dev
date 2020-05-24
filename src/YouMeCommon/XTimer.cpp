#include "XTimer.h"
#include <assert.h>
namespace youmecommon
{

	struct TimerThreadParam
	{
		CXTimer* pThis;
		int iTimerID;
	};


	CXTimer::CXTimer()
	{
		m_bExit = false;
		m_iTimerID = 0;
	}


	CXTimer::~CXTimer()
	{
		UnInit();
	}

	CXTimer* CXTimer::GetInstance()
	{
		static CXTimer* s_CXTimer = new CXTimer();
		return s_CXTimer;
	}

	bool CXTimer::Init()
	{
		return true;
	}


	void CXTimer::UnInit()
	{
		//销毁主线程
		m_bExit = true;
		//销毁所有的定时器
		{
			std::lock_guard<std::mutex> lock(m_mapMutex);
			std::map<int, std::shared_ptr<TimerParam> >::iterator begin = m_timerMap.begin();
			std::map<int, std::shared_ptr<TimerParam> >::iterator end = m_timerMap.end();
			for (; begin != end; begin++)
			{
				begin->second->bCancle = true;
				begin->second->conWait.SetSignal();
			}
			m_timerMap.clear();
		}
	}

	int CXTimer::PostTimer(uint64_t ulTimeout, ITimeOutCallback* callback, void* pParam, bool bPostOnce)
	{
		if (nullptr == callback)
		{
			return -1;
		}
		std::lock_guard<std::mutex> lock(m_mapMutex);
		m_iTimerID++;
		std::shared_ptr<TimerParam> pPtr(new TimerParam);
		m_timerMap[m_iTimerID] = pPtr;
		pPtr->pCallback = callback;
		pPtr->pParam = pParam;
		pPtr->ulTimeout = ulTimeout;
		pPtr->bOnce = bPostOnce;
		//线程参数
		TimerThreadParam* pThreadParam = new TimerThreadParam;
		pThreadParam->pThis = this;
		pThreadParam->iTimerID = m_iTimerID;
		pPtr->waitThrad = std::thread(CXTimer::WaitThread, (void*)pThreadParam);

		return m_iTimerID;
	}

	bool CXTimer::CancleTimer(long iTimerID)
	{
		std::lock_guard<std::mutex> lock(m_mapMutex);
		std::map<int, std::shared_ptr<TimerParam> >::iterator it = m_timerMap.find(iTimerID);
		if (it == m_timerMap.end())
		{
			return false;
		}
		//标记一下，由线程自己退出，并且清理
		it->second->bCancle = true;
		return true;
	}

	void CXTimer::WaitThread(void * pParam)
	{
		TimerThreadParam* pThreadParam = (TimerThreadParam*)pParam;
		std::shared_ptr<TimerParam> sharedParam;
		{
			std::lock_guard<std::mutex> lock(pThreadParam->pThis->m_mapMutex);
			//此处不需要加锁，取消定时器的时候会等线程退出
			std::map<int, std::shared_ptr<TimerParam> >::iterator it = pThreadParam->pThis->m_timerMap.find(pThreadParam->iTimerID);
			if (it == pThreadParam->pThis->m_timerMap.end())
			{
				delete pThreadParam;
				return ;
			}
			sharedParam = it->second;
		}

		while (true)
		{
			sharedParam->conWait.WaitTime(sharedParam->ulTimeout);
			if (sharedParam->bCancle)
			{
				break;
			}
			//这个地方会存在风险，恰好取消定时器，然后回调，不过概率极低
			sharedParam->pCallback->OnTimeOut(sharedParam->pParam, pThreadParam->iTimerID);
			//一次性定时器需要删掉，然后退出线程
			if (sharedParam->bOnce) {
				break;
			}
		}
		if (sharedParam->waitThrad.joinable())
		{
			sharedParam->waitThrad.detach();
		}

		{
			std::lock_guard<std::mutex> lock(pThreadParam->pThis->m_mapMutex);
			std::map<int, std::shared_ptr<TimerParam> >::iterator it = pThreadParam->pThis->m_timerMap.find(pThreadParam->iTimerID);
			if (it != pThreadParam->pThis->m_timerMap.end()) {
				pThreadParam->pThis->m_timerMap.erase(it);
			}
		}

		delete pThreadParam;
	}
}
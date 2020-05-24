#include "MessageHandle.h"
namespace youmecommon
{
	CMessageHandle::CMessageHandle()
	{
	}


	CMessageHandle::~CMessageHandle()
	{
		UnInit();
	}

	void CMessageHandle::Post(const std::shared_ptr<IMessageRunable>& runable)
	{
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tasks.push_back(runable);
        m_semaphore.Increment();
	}

	int CMessageHandle::Init()
	{
        //已经启动线程初始化过了
        if (m_bInit)
        {
            return 0;
        }
        m_bInit = true;
        m_executeThread = std::thread(&CMessageHandle::HandleProc, this);
		return 0;
	}

	void CMessageHandle::UnInit()
	{
        if (!m_bInit)
        {
            return;
        }
        m_bInit = false;
        if (m_executeThread.joinable())
        {
            m_semaphore.Increment();
            m_executeThread.join();
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tasks.clear();
        }
	}

	void CMessageHandle::HandleProc()
	{
        while (m_semaphore.Decrement())
        {
            if (!m_bInit)
            {
                break;
            }
            std::shared_ptr<IMessageRunable> pRunable;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_tasks.empty())
                {
                    continue;
                }
                pRunable = m_tasks.front();
                m_tasks.pop_front();
            }
            if (pRunable == nullptr)
            {
                continue;
            }
            //执行
            pRunable->Run();
        }
	}

}

#include "XSemaphore.h"
#ifndef WIN32
#include <sys/time.h>
#include <assert.h>
#endif



namespace youmecommon {

CXSemaphore::CXSemaphore(int iInitValue)
{
#ifdef WIN32
	m_handle = CreateSemaphore(NULL, iInitValue, 0x7fffffff, NULL);
#else
#if (defined OS_IOS) || (defined OS_IOSSIMULATOR)
    struct timeval tv;
    gettimeofday(&tv,NULL);
    uint64_t ulEcho = (((uint64_t)tv.tv_sec)*(uint64_t)1000) + (((uint64_t)tv.tv_usec)/(uint64_t)1000);
    snprintf(m_semName, NAME_MAX, "%llu_%d",ulEcho,rand() ^ rand());
    m_handle = sem_open(m_semName, O_CREAT,S_IRUSR|S_IWUSR,iInitValue);
    sem_unlink(m_semName);
    assert(m_handle != SEM_FAILED);
#elif defined OS_OSX
    m_handle = dispatch_semaphore_create( 0 );
    for( int i = 0 ; i < iInitValue ; i++ )
    {
        dispatch_semaphore_signal( m_handle );
    }
#else
    m_handle = (SEMAPHORE_T)malloc(sizeof(sem_t));
    sem_init(m_handle, 0, iInitValue);
#endif
    
    
   
    
#endif // WIN32
}


CXSemaphore::~CXSemaphore()
{
   
#ifdef WIN32
	CloseHandle(m_handle);
#elif defined OS_OSX
    //把*handle信号量处理成当前值为0
    {
        while ( dispatch_semaphore_wait( m_handle, DISPATCH_TIME_NOW) == 0 ) {
        }
        while( dispatch_semaphore_signal(  m_handle ) != 0 ){
        }
    }
    dispatch_release(m_handle);
    m_handle = NULL;
#else
    assert(SEM_FAILED != m_handle);
    #if (defined OS_OSX) || (defined OS_IOS) || (defined OS_IOSSIMULATOR)
    if (SEM_FAILED != m_handle) {
        sem_close(m_handle);
        //sem_unlink(m_semName);
    }
    #else
      sem_destroy(m_handle);
    free(m_handle);
    #endif
#endif // WIN32

}

bool CXSemaphore::Increment()
{
#ifdef WIN32
	return (bool)ReleaseSemaphore(m_handle, 1, NULL);
#elif defined OS_OSX
    int ret =  dispatch_semaphore_signal( m_handle );
    if( ret == 0 )
    {
        return true;
    }
    else{
        return false;
    }
#else
	int ret = sem_post(m_handle);
	if (ret == 0)
	{
		return true;
	}
	return false;
#endif // WIN32
}

bool CXSemaphore::Decrement()
{
#ifdef WIN32
	if (WaitForSingleObject(m_handle, INFINITE) == WAIT_OBJECT_0)
	{
		return true;
	}
	return false;
#elif defined OS_OSX
    int ret = dispatch_semaphore_wait( m_handle, DISPATCH_TIME_FOREVER  );
    return ret == 0;
#else
	int	ret = sem_wait(m_handle);	
	return ret == 0;
#endif
}
}

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <string.h>
#include "get_sys_info.h"

#ifdef OS_ANDROID
#include <sys/stat.h>
#include <malloc.h>
#include <unistd.h>
#endif

#if (OS_IOS || OS_IOSSIMULATOR)
#import <mach/mach.h>
#include <sys/sysctl.h>
#include <malloc/malloc.h>
#endif
namespace youmecommon
{

	unsigned int getCountOfCores()
	{
		unsigned int ncpu=0;

#if (OS_IOS || OS_IOSSIMULATOR)
		size_t len = sizeof(ncpu);
		sysctlbyname("hw.ncpu", &ncpu, &len, NULL, 0);
#elif OS_ANDROID
		ncpu = sysconf(_SC_NPROCESSORS_ONLN);
#endif

		return ncpu;
	}

	float getCurrentProcessCPUUsed()
	{
#if (OS_IOS || OS_IOSSIMULATOR)
		kern_return_t kr;
		task_info_data_t tinfo;
		mach_msg_type_number_t task_info_count;

		task_info_count = TASK_INFO_MAX;
		kr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)tinfo, &task_info_count);
		if (kr != KERN_SUCCESS) {
			return 0;
		}

		task_basic_info_t      basic_info;
		thread_array_t         thread_list;
		mach_msg_type_number_t thread_count;

		thread_info_data_t     thinfo;
		mach_msg_type_number_t thread_info_count;

		thread_basic_info_t basic_info_th;
		uint32_t stat_thread = 0; // Mach threads

		basic_info = (task_basic_info_t)tinfo;

		// get threads in the task
		kr = task_threads(mach_task_self(), &thread_list, &thread_count);
		if (kr != KERN_SUCCESS) {
			return 0;
		}
		if (thread_count > 0)
			stat_thread += thread_count;

		long tot_sec = 0;
		long tot_usec = 0;
		float tot_cpu = 0;
		int j;

		for (j = 0; j < thread_count; j++)
		{
			thread_info_count = THREAD_INFO_MAX;
			kr = thread_info(thread_list[j], THREAD_BASIC_INFO,
				(thread_info_t)thinfo, &thread_info_count);
			if (kr != KERN_SUCCESS) {
				return 0;
			}

			basic_info_th = (thread_basic_info_t)thinfo;

			if (!(basic_info_th->flags & TH_FLAGS_IDLE)) {
				tot_sec = tot_sec + basic_info_th->user_time.seconds + basic_info_th->system_time.seconds;
				tot_usec = tot_usec + basic_info_th->system_time.microseconds + basic_info_th->system_time.microseconds;
				tot_cpu = tot_cpu + basic_info_th->cpu_usage / (float)TH_USAGE_SCALE * 100.0;
			}

		} // for each thread

		kr = vm_deallocate(mach_task_self(), (vm_offset_t)thread_list, thread_count * sizeof(thread_t));
		assert(kr == KERN_SUCCESS);

		return tot_cpu;
#endif

#if defined(OS_ANDROID)
		pid_t p = getpid();
		unsigned int totalcputime1, totalcputime2;
		unsigned int procputime1, procputime2;
		totalcputime1 = get_cpu_total_occupy();
		procputime1 = get_cpu_process_occupy(p);
		usleep(500000);//延迟500毫秒
		totalcputime2 = get_cpu_total_occupy();
		procputime2 = get_cpu_process_occupy(p);
		float pcpu = 100.0f*(procputime2 - procputime1) / (totalcputime2 - totalcputime1);
		return pcpu;

#endif
		return 0;
	}

	float getCurrentProcessMemoryUsed() {

		float result = 0.0f;

#if (OS_IOS || OS_IOSSIMULATOR)
		task_basic_info_data_t taskInfo;
		mach_msg_type_number_t infoCount = TASK_BASIC_INFO_COUNT;
		kern_return_t kernReturn = task_info(mach_task_self(),
			TASK_BASIC_INFO, (task_info_t)&taskInfo, &infoCount);
		if (kernReturn != KERN_SUCCESS) {
			return 0;
		}
		result = taskInfo.resident_size / 1024.0 / 1024.0;

#endif

#if defined(OS_ANDROID)
		struct mallinfo mi = mallinfo();
		//TSK_DEBUG_INFO("\theap_malloc_total=%lu heap_free_total=%lu heap_in_use=%lu\n\tmmap_total=%lu mmap_count=%lu\n", (long)mi.arena, (long)mi.fordblks, (long)mi.uordblks, (long)mi.hblkhd, (long)mi.hblks);
		result = mi.uordblks / 1024.0 / 1024.0;
#endif

		return result;
	}
}

#if defined(OS_ANDROID)

unsigned int get_cpu_process_occupy(const pid_t p)
{
    char file[64] = {0};//文件名
    process_cpu_occupy_t t;
    
    FILE *fd;         //定义文件指针fd
    char line_buff[1024] = {0};  //读取行的缓冲区
    sprintf(file,"/proc/%d/stat",p);//文件中第11行包含着
    
    fd = fopen (file, "r"); //以R读的方式打开文件再赋给指针fd
    fgets (line_buff, sizeof(line_buff), fd); //从fd文件中读取长度为buff的字符串再存到起始地址为buff这个空间里
    
    sscanf(line_buff,"%u",&t.pid);//取得第一项
    const char* q = get_items(line_buff,PROCESS_ITEM);//取得从第14项开始的起始指针
    sscanf(q,"%u %u %u %u",&t.utime,&t.stime,&t.cutime,&t.cstime);//格式化第14,15,16,17项
    
    fclose(fd);     //关闭文件fd
    return (t.utime + t.stime + t.cutime + t.cstime);
}


unsigned int get_cpu_total_occupy()
{
    FILE *fd;         //定义文件指针fd
    char buff[1024] = {0};  //定义局部变量buff数组为char类型大小为1024
    total_cpu_occupy_t t;
    
    fd = fopen ("/proc/stat", "r"); //以R读的方式打开stat文件再赋给指针fd
    fgets (buff, sizeof(buff), fd); //从fd文件中读取长度为buff的字符串再存到起始地址为buff这个空间里
    /*下面是将buff的字符串根据参数format后转换为数据的结果存入相应的结构体参数 */
    char name[16];//暂时用来存放字符串
    sscanf (buff, "%s %u %u %u %u", name, &t.user, &t.nice,&t.system, &t.idle);
    
    fclose(fd);     //关闭文件fd
    return (t.user + t.nice + t.system + t.idle);
}

const char* get_items(const char* buffer,int ie)
{
    assert(buffer);
    const char* p = buffer;//指向缓冲区
    int len = strlen(buffer);
    int count = 0;//统计空格数
    if (1 == ie || ie < 1)
    {
        return p;
    }
    int i;
    
    for (i=0; i<len; i++)
    {
        if (' ' == *p)
        {
            count++;
            if (count == ie-1)
            {
                p++;
                break;
            }
        }
        p++;
    }
    
    return p;
}

#endif

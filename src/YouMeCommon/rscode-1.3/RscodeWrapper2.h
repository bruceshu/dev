#pragma once
//#include "RscodeDefine.h"
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
	//rscode　基础封装
    //函数各种加2的原因，是为了避免与老版本符号冲突
	void rscode_init2();
	void* rscode_create_instance2( int npar  );
	void rscode_encode_data2(void* pInstance, unsigned char msg[], int nbytes, unsigned char dst[]);
	void rscode_decode_data2(void* pInstancce, unsigned char data[], int nbytes);
	int rscode_check_syndrome2(void * pInstance);
	int rscode_correct_errors_erasures2(void * pInstance,unsigned char codeword[],
		int csize,
		int nerasures,
		int erasures[]);
	void recode_destroy_instance2(void*);


	//高级一点封装,编码，传入的必须是
	//传入的是NPAR + 1个 rtp　需要编码的缓冲。NPAR　个冗余包。　真实数据排在数组的前面。冗余排在后面.会修改 buffer 的后面NPAR个数组的内容
	void rscode_encode_rtp_packet2(void* pInstance, unsigned char* buffer[], int iPerBufferLen[],int iNPAR);
	
	//解密RTP 包，传入一组RTP包进行解密
	void rscode_decode_rtp_packet2(void* pInstance, unsigned char* buffer[],int perBufferLen[] , int nerasures,int erasures[],int iNPAR);
#ifdef __cplusplus
}
#endif // __cplusplus

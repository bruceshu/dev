#include <stdio.h>
#include "Rscode.h"
#include "RscodeWrapper.h"
#include "XSharedArray.h"
void rscode_init()
{
	youmecommon::CRscode::initialize_ecc();
}

void* rscode_create_instance()
{
	return new youmecommon::CRscode;
}

void rscode_encode_data(void* pInstance, unsigned char msg[], int nbytes, unsigned char dst[])
{
	youmecommon::CRscode* pRsInstance = (youmecommon::CRscode*)pInstance;
	pRsInstance->encode_data(msg, nbytes, dst);
}

void rscode_decode_data(void* pInstancce, unsigned char data[], int nbytes)
{
	youmecommon::CRscode* pRsInstance = (youmecommon::CRscode*)pInstancce;
	pRsInstance->decode_data(data, nbytes);
}

int rscode_check_syndrome(void * pInstance)
{
	youmecommon::CRscode* pRsInstance = (youmecommon::CRscode*)pInstance;
	return pRsInstance->check_syndrome();
}

int rscode_correct_errors_erasures(void * pInstance, unsigned char codeword[], int csize, int nerasures, int erasures[])
{
	youmecommon::CRscode* pRsInstance = (youmecommon::CRscode*)pInstance;
	return pRsInstance->correct_errors_erasures(codeword, csize, nerasures, erasures);
}

void recode_destroy_instance(void* p)
{
	youmecommon::CRscode* pInstance = (youmecommon::CRscode*)p;
    delete pInstance;
}

void rscode_encode_rtp_packet(void* pInstance,  unsigned char* buffer[], int iPerBufferLen[],int iNPAR)
{
	youmecommon::CRscode* pRsInstance = (youmecommon::CRscode*)pInstance;
	//传入的是NPAR + 1个 rtp　需要编码的缓冲。NPAR　个冗余包。　真实数据排在数组的前面。冗余排在后面.会修改 buffer 的后面NPAR个数组的内容
	youmecommon::CXSharedArray<unsigned char>pSrc;
	pSrc.Allocate(iNPAR);
	youmecommon::CXSharedArray<unsigned char>pDest;
	pDest.Allocate(iNPAR + 1);
	//先判断一个最大长度的缓存
	int iMaxBufferLen = iPerBufferLen[0];
	for (int i = 1; i < iNPAR;i++)
	{
		if (iPerBufferLen[i] > iMaxBufferLen)
		{
			iMaxBufferLen = iPerBufferLen[i];
		}
	}


	for (int i = 0; i < iMaxBufferLen; i++)
	{
		//逐个编码
		for (int j = 0; j < iNPAR; j++)
		{
			//每一段数据的长度可能不一样，所以需要判断
			if (i < iPerBufferLen[j])
			{
				pSrc.Get()[j] = buffer[j][i];
			}
			else
			{
				pSrc.Get()[j] = 0;
			}
		}
		pRsInstance->encode_data(pSrc.Get(), iNPAR, pDest.Get());
		//赋值冗余,最后一个数据
        buffer[iNPAR][i] = pDest.Get()[iNPAR];
		
	}
}

void rscode_decode_rtp_packet(void* pInstance, unsigned char* buffer[], int iPerBufferLen[], int nerasures, int erasures1[],int iNPAR)
{
	//rscode　是倒序排列的，所以需要重新整理一下
    int *erasures = new int[nerasures];
	for (int i = 0; i < nerasures; i++)
	{
		erasures[i] = iNPAR - erasures1[i]; //倒序，从0 开始
	}
	youmecommon::CRscode* pRsInstance = (youmecommon::CRscode*)pInstance;
	youmecommon::CXSharedArray<unsigned char>pDest;
	pDest.Allocate(iNPAR + 1);

	//获取一个最大的缓存
	int iMaxBufferLen = iPerBufferLen[0];
	for (int i = 1; i < iNPAR+1; i++)
	{
		if (iPerBufferLen[i] > iMaxBufferLen)
		{
			iMaxBufferLen = iPerBufferLen[i];
		}
	}



	for (int i = 0; i < iMaxBufferLen; i++)
	{
		//逐个编码
		for (int j = 0; j < iNPAR + 1; j++)
		{
			if (i < iPerBufferLen[j])
			{
				pDest.Get()[j] = buffer[j][i];
			}
			else
			{
				pDest.Get()[j] = 0;
			}
		}
		pRsInstance->decode_data(pDest.Get(), iNPAR + 1);
		if (rscode_check_syndrome(pInstance) == 0)
		{
				continue;
		}
		pRsInstance->correct_errors_erasures(pDest.Get(),
			iNPAR + 1,
			nerasures,
			erasures);
		for (int j = 0; j < iNPAR + 1; j++)
		{
			if (i < iPerBufferLen[j])
			{
				buffer[j][i] = pDest.Get()[j];
			}			
		}
	}


    delete []erasures;
}

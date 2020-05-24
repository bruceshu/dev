#include <stdio.h>
#include "Rscode2.h"
#include "RscodeWrapper2.h"
#include "XSharedArray.h"
void rscode_init2()
{
	youmecommon::CRscode2::initialize_ecc();
}

void* rscode_create_instance2( int npar )
{
	return new youmecommon::CRscode2( npar );
}

void rscode_encode_data2(void* pInstance, unsigned char msg[], int nbytes, unsigned char dst[])
{
	youmecommon::CRscode2* pRsInstance = (youmecommon::CRscode2*)pInstance;
	pRsInstance->encode_data(msg, nbytes, dst);
}

void rscode_decode_data2(void* pInstancce, unsigned char data[], int nbytes)
{
	youmecommon::CRscode2* pRsInstance = (youmecommon::CRscode2*)pInstancce;
	pRsInstance->decode_data(data, nbytes);
}

int rscode_check_syndrome2(void * pInstance)
{
	youmecommon::CRscode2* pRsInstance = (youmecommon::CRscode2*)pInstance;
	return pRsInstance->check_syndrome();
}

int rscode_correct_errors_erasures2(void * pInstance, unsigned char codeword[], int csize, int nerasures, int erasures[])
{
	youmecommon::CRscode2* pRsInstance = (youmecommon::CRscode2*)pInstance;
	return pRsInstance->correct_errors_erasures(codeword, csize, nerasures, erasures);
}

void recode_destroy_instance2(void* p)
{
	youmecommon::CRscode2* pInstance = (youmecommon::CRscode2*)p;
    delete pInstance;
}

void rscode_encode_rtp_packet2(void* pInstance,  unsigned char* buffer[], int iPerBufferLen[],int iNPAR)
{
    //这里的npar其实是数据包的数量
	youmecommon::CRscode2* pRsInstance = (youmecommon::CRscode2*)pInstance;
	//传入的是NPAR + pRsInstance->getNPAR()个 rtp　需要编码的缓冲。pRsInstance->getNPAR()　个冗余包。　真实数据排在数组的前面。冗余排在后面.会修改 buffer 的后面NPAR个数组的内容
	youmecommon::CXSharedArray<unsigned char>pSrc;
	pSrc.Allocate(iNPAR);
	youmecommon::CXSharedArray<unsigned char>pDest;
	pDest.Allocate(iNPAR + pRsInstance->getNPAR() );
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
		//赋值冗余
        for( int j = 0 ; j < pRsInstance->getNPAR(); j++ ){
            buffer[iNPAR + j ][i] = pDest.Get()[iNPAR + j ];
        }
	}
}

#include <iostream>
using namespace std;
void rscode_decode_rtp_packet2(void* pInstance, unsigned char* buffer[], int iPerBufferLen[], int nerasures, int erasures1[],int iNPAR)
{
    youmecommon::CRscode2* pRsInstance = (youmecommon::CRscode2*)pInstance;
    
	//rscode　是倒序排列的，所以需要重新整理一下
    int *erasures = new int[nerasures];
	for (int i = 0; i < nerasures; i++)
	{
		erasures[i] = iNPAR + pRsInstance->getNPAR() - 1  - erasures1[i]; //倒序，从0 开始
	}
	
	youmecommon::CXSharedArray<unsigned char>pDest;
	pDest.Allocate(iNPAR + pRsInstance->getNPAR());

	//获取一个最大的缓存
	int iMaxBufferLen = iPerBufferLen[0];
	for (int i = 1; i < iNPAR+ pRsInstance->getNPAR() ; i++)
	{
		if (iPerBufferLen[i] > iMaxBufferLen)
		{
			iMaxBufferLen = iPerBufferLen[i];
		}
	}



	for (int i = 0; i < iMaxBufferLen; i++)
	{
		//逐个编码
		for (int j = 0; j < iNPAR + pRsInstance->getNPAR(); j++)
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
		pRsInstance->decode_data(pDest.Get(), iNPAR + pRsInstance->getNPAR() );
		if (rscode_check_syndrome2(pInstance) == 0)
		{
				continue;
		}
		int ret = pRsInstance->correct_errors_erasures(pDest.Get(),
			iNPAR + pRsInstance->getNPAR(),
			nerasures,
			erasures);
    
		for (int j = 0; j < iNPAR + pRsInstance->getNPAR(); j++)
		{
			if (i < iPerBufferLen[j])
			{
				buffer[j][i] = pDest.Get()[j];
			}			
		}
	}


    delete []erasures;
}

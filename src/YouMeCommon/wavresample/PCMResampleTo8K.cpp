#include "PCMResampleTo8K.h"
#include <YouMeCommon/amrFileCodec.h>
#include <YouMeCommon/wavresample/speex_resampler.h>
#include <YouMeCommon/XSharedArray.h>
namespace youmecommon
{
	//16000*0.02= 320
#define  PCM_FRAME_SIZE_16K 320



	bool CPCMResampleTo8K::PcmToWav(const XString& srcPath, unsigned short sampleBitSize, unsigned short channels, unsigned long sampleFrequency, int format, const XString desPath)
	{
		if (srcPath.empty())
		{
			return false;
		}
		XString strDesPath = desPath;
		if (strDesPath.empty())
		{
			XString::size_type stPos = srcPath.find_last_of(__XT("."));
			if (stPos == XString::npos)
			{
				return false;
			}
			strDesPath = srcPath.substr(0, stPos + 1);
			strDesPath.append(__XT("wav"));
		}

		FILE* pFileSrc = NULL;
		FILE* pFileDes = NULL;
#ifdef WIN32
		errno_t errCode = _wfopen_s(&pFileSrc, srcPath.c_str(), __XT("rb"));
		if (errCode != 0 || NULL == pFileSrc)
		{
			return false;
		}
		errCode = _wfopen_s(&pFileDes, strDesPath.c_str(), __XT("wb+"));
		if (errCode != 0 || NULL == pFileDes)
		{
			fclose(pFileSrc);
			return false;
		}
#else
		std::string strSrcTmp = XStringToUTF8(srcPath);
		pFileSrc = fopen(strSrcTmp.c_str(), __XT("rb"));
		if (NULL == pFileSrc)
		{
			return false;
		}
		std::string strDesTmp = XStringToUTF8(strDesPath);
		pFileDes = fopen(strDesTmp.c_str(), __XT("wb+"));
		if (NULL == pFileDes)
		{
			fclose(pFileSrc);
			return false;
		}
#endif

		fseek(pFileSrc, 0, SEEK_END);
		long srcFileSize = ftell(pFileSrc);
		if (0 == srcFileSize)
		{
			return false;
		}

		WAVHeadInfo headInfo;
		headInfo.riffID[0] = 'R';
		headInfo.riffID[1] = 'I';
		headInfo.riffID[2] = 'F';
		headInfo.riffID[3] = 'F';
		headInfo.fileSize = srcFileSize + sizeof(WAVHeadInfo) - 8;
		headInfo.riffFormat[0] = 'W';
		headInfo.riffFormat[1] = 'A';
		headInfo.riffFormat[2] = 'V';
		headInfo.riffFormat[3] = 'E';
		headInfo.fmtID[0] = 'f';
		headInfo.fmtID[1] = 'm';
		headInfo.fmtID[2] = 't';
		headInfo.fmtID[3] = 0X20;
		headInfo.fmtSize = 16;
		headInfo.formatTag = 0x0001;
		headInfo.channels = channels;
		headInfo.sampleFrequency = sampleFrequency;
		headInfo.sampleBitSize = sampleBitSize;
		headInfo.byteRate = headInfo.sampleFrequency * headInfo.channels * headInfo.sampleBitSize / 8;
		headInfo.blockAlign = headInfo.channels * headInfo.sampleBitSize / 8;
		headInfo.dataID[0] = 'd';
		headInfo.dataID[1] = 'a';
		headInfo.dataID[2] = 't';
		headInfo.dataID[3] = 'a';
		headInfo.dataChunkSize = srcFileSize;
		size_t stWrite = fwrite(&headInfo, 1, sizeof(headInfo), pFileDes);
		if (stWrite < sizeof(headInfo))
		{
			fclose(pFileSrc);
			fclose(pFileDes);
			return false;
		}
		if (format == 1)
		{
			fseek(pFileSrc, 4096, SEEK_SET);
		}
		else
		{
			fseek(pFileSrc, 0, SEEK_SET);
		}

		while (!feof(pFileSrc))
		{
			char szBuf[4096];
			size_t stRead = fread(szBuf, 1, 4096, pFileSrc);
			if (stRead > 0)
			{
				fwrite(szBuf, 1, stRead, pFileDes);
			}
		}

		fclose(pFileSrc);
		fclose(pFileDes);

		return true;
	}


	// 从WAVE文件读一个完整的PCM音频帧
	// 返回值: 0-错误 >0: 完整帧大小
	int CPCMResampleTo8K::ReadPCMFrame(short speech[], youmecommon::CXFile& fpwave, int nChannels, int nBitsPerSample)
	{
		int nRead = 0;
		int x = 0, y = 0;
	
        // 原始PCM音频帧数据
		unsigned char  pcmFrame_8b1[PCM_FRAME_SIZE_16K];
		unsigned char  pcmFrame_8b2[PCM_FRAME_SIZE_16K << 1];
		unsigned short pcmFrame_16b1[PCM_FRAME_SIZE_16K];
		unsigned short pcmFrame_16b2[PCM_FRAME_SIZE_16K << 1];

		if (nBitsPerSample == 8 && nChannels == 1)
		{
			nRead = fpwave.Read((byte*)pcmFrame_8b1, PCM_FRAME_SIZE_16K*nChannels);
			for (x = 0; x < PCM_FRAME_SIZE_16K; x++)
			{
				speech[x] = (short)((short)pcmFrame_8b1[x] << 7);
			}
		}
		else
			if (nBitsPerSample == 8 && nChannels == 2)
			{
				nRead = fpwave.Read((byte*)pcmFrame_8b2, PCM_FRAME_SIZE_16K*nChannels);
				for (x = 0, y = 0; y < PCM_FRAME_SIZE_16K; y++, x += 2)
				{
					// 1 - 取两个声道之左声道
					speech[y] = (short)((short)pcmFrame_8b2[x + 0] << 7);
					// 2 - 取两个声道之右声道
					//speech[y] =(short)((short)pcmFrame_8b2[x+1] << 7);
					// 3 - 取两个声道的平均值
					//ush1 = (short)pcmFrame_8b2[x+0];
					//ush2 = (short)pcmFrame_8b2[x+1];
					//ush = (ush1 + ush2) >> 1;
					//speech[y] = (short)((short)ush << 7);
				}
			}
			else
				if (nBitsPerSample == 16 && nChannels == 1)
				{
					nRead = fpwave.Read((byte*)pcmFrame_16b1, PCM_FRAME_SIZE_16K*nChannels *(nBitsPerSample / 8));
					for (x = 0; x < PCM_FRAME_SIZE_16K; x++)
					{
						speech[x] = (short)pcmFrame_16b1[x + 0];
					}
				}
				else
					if (nBitsPerSample == 16 && nChannels == 2)
					{
						nRead = fpwave.Read((byte*)pcmFrame_16b2, PCM_FRAME_SIZE_16K*nChannels *(nBitsPerSample / 8));
						for (x = 0, y = 0; y < PCM_FRAME_SIZE_16K; y++, x += 2)
						{
							//speech[y] = (short)pcmFrame_16b2[x+0];
							speech[y] = (short)((int)((int)pcmFrame_16b2[x + 0] + (int)pcmFrame_16b2[x + 1])) >> 1;
						}
					}

		// 如果读到的数据不是一个完整的PCM帧, 就返回0
		if (nRead < PCM_FRAME_SIZE_16K*nChannels) return 0;

		return nRead;
	}


	bool CPCMResampleTo8K::ConvertTo8K(const XString& strSrcPath, const XString& strDestPath)
	{
		XString strDestTmp = strDestPath + __XT(".tmp");
		youmecommon::CXFile srcFile;
		if (0 != srcFile.LoadFile(strSrcPath, youmecommon::CXFile::Mode_OpenExist_ReadOnly))
		{
			return false;
		}

		RIFFHEADER riff;
        FMTBLOCK fmt={{0}};
		XCHUNKHEADER chunk;
        WAVEFORMATX wfx={0};
	

		// 1. 读RIFF头
		srcFile.Read((byte*)&riff, sizeof(RIFFHEADER));
		// 2. 读FMT块 - 如果 fmt.nFmtSize>16 说明需要还有一个附属大小没有读
		srcFile.Read((byte*)&chunk, sizeof(XCHUNKHEADER));
		if (chunk.nChunkSize > 16)
		{
			srcFile.Read((byte*)&wfx, sizeof(WAVEFORMATX));
		}
		else
		{
			memcpy(fmt.chFmtID, chunk.chChunkID, 4);
			fmt.nFmtSize = chunk.nChunkSize;
			srcFile.Read((byte*)&fmt.wf, sizeof(WAVEFORMAT));
		}

		// 3.转到data块 - 有些还有fact块等。
		while (true)
		{
			srcFile.Read((byte*)&chunk, sizeof(XCHUNKHEADER));
			if (0 == memcmp(chunk.chChunkID, "data", 4))
			{
				
				break;
			}
			// 因为这个不是data块,就跳过块数据
			srcFile.Seek(chunk.nChunkSize, SEEK_CUR);
		}
		if (fmt.wf.nSamplesPerSec != 16000)
		{
			return false;
		}
		youmecommon::CXFile destFile;
		if (0 != destFile.LoadFile(strDestTmp, youmecommon::CXFile::Mode_CREATE_ALWAYS))
		{
			return false;
		}
		//开始转换
		SpeexResamplerState* pSampleState = speex_resampler_init(1, 16000, 8000, 3, nullptr);
		if (NULL == pSampleState)
		{
			return false;
		}
		int iFrameSize = 320;
		youmecommon::CXSharedArray<short> buffer(iFrameSize);
		youmecommon::CXSharedArray<short> outBuffer(iFrameSize);
		int iOutLen = iFrameSize;
		while (true)
		{
			if (!ReadPCMFrame(buffer.Get(), srcFile, 1, 16))
			{
				break;
			}
			speex_resampler_process_int(pSampleState, 0, (const spx_int16_t*)buffer.Get(), (spx_uint32_t *)&iFrameSize, (spx_int16_t*)outBuffer.Get(), (spx_uint32_t *)&iOutLen);
			destFile.Write((const byte*)outBuffer.Get(), iOutLen*sizeof(spx_int16_t));
		}
		speex_resampler_destroy(pSampleState);
		destFile.Close();
		bool bSuccess = PcmToWav(strDestTmp, 16, 1, 8000, 0, strDestPath);
		youmecommon::CXFile::remove_file(strDestTmp);
		return bSuccess;
	}
}
#ifndef opusFileCodec_hpp
#define opusFileCodec_hpp


#include <YouMeCommon/CrossPlatformDefine/PlatformDef.h>


#define MAX_PACKET (1500)
#define CODEC_SAMPLERATE (48000)
//#define CODEC_CHANNELS (2)

#pragma pack(push)
#pragma pack(1)  //1字节对齐
typedef struct
{
    char opusTag[4];   //"OPUS" 固定字符串，4字节
    int sampleRate;    //采样率
    short channels;    //声道数
    short bitSample;   //采样位数
    char version;   //版本号
    int remain;        //值为0，预留
}OPUSHEADER;

typedef struct
{
    int sampleRate;
    short channels;
    short bitSample;
}SampleConfigInfo;
#pragma pack(pop)

//将wav文件编码成opus文件
int EncodeWAVFileToOPUSFile(const XString &strWAVFileName, const XString &strOPUSFileName, int bitRate);

//将pcm文件编码成opus文件
int EncodePCMFileToOPUSFile(const XString &strPCMFileName, const XString &strOPUSFileName, int wav_sampleRate, short wav_channels, short wav_bitSample, int bitRate);

int DecodeOPUSFileToWAVFile(const XString &strOPUSFileName, const XString &strWAVFileName);

#endif /* opusFileCodec_hpp */

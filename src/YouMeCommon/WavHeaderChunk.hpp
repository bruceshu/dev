#ifndef WavHeaderChunk_hpp
#define WavHeaderChunk_hpp

#pragma pack(push)
#pragma pack(1)  //1字节对齐
typedef struct
{
    char chChunkID[4];
    int nChunkSize;
}XCHUNKHEADER;

typedef struct
{
    short nFormatTag;
    short nChannels;
    int nSamplesPerSec;
    int nAvgBytesPerSec;
    short nBlockAlign;
    short nBitsPerSample;
    
}WAVEFORMAT;

typedef struct
{
    short nFormatTag;
    short nChannels;
    int nSamplesPerSec;
    int nAvgBytesPerSec;
    short nBlockAlign;
    short nBitsPerSample;
    short nExSize;
}WAVEFORMATX;

typedef struct
{
    char chRiffID[4];
    int nRiffSize;
    char chRiffFormat[4];
}RIFFHEADER;

typedef struct
{
    char chFmtID[4];
    int nFmtSize;
    WAVEFORMAT wf;
}FMTBLOCK;
#pragma pack(pop)

#endif /* WavHeaderChunk_hpp */

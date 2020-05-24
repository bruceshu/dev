#include "opusFileCodec.hpp"
#include <YouMeCommon/XFile.h>
#include <YouMeCommon/Log.h>
#include <YouMeCommon/opus-1.1/include/opus.h>
#include <YouMeCommon/XSharedArray.h>
#include <YouMeCommon/WavHeaderChunk.hpp>

//整型转字符型
static void int_to_char(opus_uint32 i, unsigned char ch[4])
{
    ch[0] = i>>24;
    ch[1] = (i>>16)&0xFF;
    ch[2] = (i>>8)&0xFF;
    ch[3] = i&0xFF;
}

//字符型转整型
static opus_uint32 char_to_int(unsigned char ch[4])
{
    return ((opus_uint32)ch[0]<<24) | ((opus_uint32)ch[1]<<16)
    | ((opus_uint32)ch[2]<< 8) |  (opus_uint32)ch[3];
}

// 从WAVE文件中跳过WAVE文件头，直接到PCM音频数据并得到wav文件的采样信息
void SkipToPCMAudioData(youmecommon::CXFile& fpwave, SampleConfigInfo &sampleInfo)
{
    RIFFHEADER riff;
    FMTBLOCK fmt;
    XCHUNKHEADER chunk;
    WAVEFORMATX wfx;
    
    // 1. 读RIFF头
    fpwave.Read((byte*)&riff, sizeof(RIFFHEADER));
    // 2. 读FMT块 - 如果 fmt.nFmtSize>16 说明需要还有一个附属大小没有读
    fpwave.Read((byte*)&chunk, sizeof(XCHUNKHEADER));
    if ( chunk.nChunkSize>16 )
    {
        fpwave.Read((byte*)&wfx, sizeof(WAVEFORMATX));
        
        sampleInfo.sampleRate = wfx.nSamplesPerSec;
        sampleInfo.channels = wfx.nChannels;
        sampleInfo.bitSample = wfx.nBitsPerSample;
    }
    else
    {
        memcpy(fmt.chFmtID, chunk.chChunkID, 4);
        fmt.nFmtSize = chunk.nChunkSize;
        fpwave.Read((byte*)&fmt.wf, sizeof(WAVEFORMAT));
        
        sampleInfo.sampleRate = fmt.wf.nSamplesPerSec;
        sampleInfo.channels = fmt.wf.nChannels;
        sampleInfo.bitSample = fmt.wf.nBitsPerSample;
    }
    // 3.转到data块 - 有些还有fact块等。
    while(true)
    {
        fpwave.Read((byte*)&chunk, sizeof(XCHUNKHEADER));
        if ( !memcmp(chunk.chChunkID, "data", 4) )
        {
            break;
        }
        // 因为这个不是data块,就跳过块数据
        fpwave.Seek(chunk.nChunkSize, SEEK_CUR);
    }
}

void CreateOPUSFileHeader(int wav_sampleRate, short wav_channels, short wav_bitSample,OPUSHEADER &opusHeader)
{
    memcpy(opusHeader.opusTag, "OPUS", 4);
    opusHeader.sampleRate = wav_sampleRate;
    opusHeader.channels = wav_channels;
    opusHeader.bitSample = wav_bitSample;
    memcpy(&opusHeader.version, "1", 1);
    opusHeader.remain = 0;
}

void WriteWAVEFileHeader(youmecommon::CXFile& fpwave, int fileLength, short channels, int sampleRate, short bitPerSample)
{
    // 1. 写RIFF头
    RIFFHEADER riff;
    memcpy(riff.chRiffID, "RIFF", 4);
    riff.nRiffSize = 4                                 // WAVE
    + sizeof(XCHUNKHEADER)          // fmt
    + sizeof(WAVEFORMATX)           // WAVEFORMATX
    + sizeof(XCHUNKHEADER)          // DATA
    + fileLength;                   // 文件长度
    
    memcpy(riff.chRiffFormat, "WAVE", 4);
    fpwave.Write((const byte*)&riff, sizeof(RIFFHEADER));
    // 2. 写FMT块
    XCHUNKHEADER chunk;
    WAVEFORMATX wfx;
    memcpy(chunk.chChunkID, "fmt ", 4);
    chunk.nChunkSize = sizeof(WAVEFORMATX);
    
    fpwave.Write((const byte*)&chunk, sizeof(XCHUNKHEADER));
    memset(&wfx, 0, sizeof(WAVEFORMATX));
    wfx.nFormatTag = 1;
    wfx.nChannels = channels;
    wfx.nSamplesPerSec = sampleRate;
    wfx.nBitsPerSample = bitPerSample;
    wfx.nBlockAlign = (short)(wfx.nChannels * wfx.nBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
    fpwave.Write((const byte*)&wfx, sizeof(WAVEFORMATX));
    // 3. 写data块头
    memcpy(chunk.chChunkID, "data", 4);
    chunk.nChunkSize = fileLength;
    fpwave.Write((const byte*)&chunk, sizeof(XCHUNKHEADER));
}

int EncodeWAVFileToOPUSFile(const XString &strWAVFileName, const XString &strOPUSFileName, int bitRate)
{
    youmecommon::CXFile enc_fin;
    if (0 != enc_fin.LoadFile(strWAVFileName, youmecommon::CXFile::Mode_OpenExist_ReadOnly))
    {
        YouMe_LOG_Error(__XT("Could not open wav file: %s."),strWAVFileName.c_str());
        return -1;
    }
    youmecommon::CXFile enc_fout;
    if (0 != enc_fout.LoadFile(strOPUSFileName, youmecommon::CXFile::Mode_CREATE_ALWAYS))
    {
        YouMe_LOG_Error(__XT("Could not open opus file: %s."),strOPUSFileName.c_str());
        return -1;
    }
    SampleConfigInfo sampleInfo;
    SkipToPCMAudioData(enc_fin, sampleInfo);
    //创建编码器
    int en_err = 0; //创建编码器的返回值
    youmecommon::OpusEncoder *enc = youmecommon::opus_encoder_create(CODEC_SAMPLERATE, sampleInfo.channels, OPUS_APPLICATION_VOIP, &en_err);
    if (en_err != OPUS_OK || enc == NULL)
    {
        YouMe_LOG_Error(__XT("create encoder fail."));
        return -1;
    }

    opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitRate));
    YouMe_LOG_Info(__XT("encoder bitRate:%d"),bitRate);

    //写opus头
    OPUSHEADER opusHeader;
    CreateOPUSFileHeader(sampleInfo.sampleRate, sampleInfo.channels, sampleInfo.bitSample, opusHeader);
    XINT64 nwrite = 0;
    nwrite = enc_fout.Write((const byte*)&opusHeader, sizeof(OPUSHEADER));
    if (nwrite != sizeof(OPUSHEADER))
    {
        YouMe_LOG_Error(__XT("write opus header fail. Don't excute encode."));
        youmecommon::opus_encoder_destroy(enc);
        return -1;
    }
    
    int max_frame_size = 48000*2; //最高的采样率*声道数
    int max_payload_bytes = MAX_PACKET; //最大的包
    youmecommon::CXSharedArray<short> enc_in(max_frame_size*sampleInfo.channels);
    youmecommon::CXSharedArray<short> enc_out(max_frame_size*sampleInfo.channels);
    youmecommon::CXSharedArray<byte> enc_bytes(max_frame_size*sampleInfo.channels);
    youmecommon::CXSharedArray<byte> enc_data(max_payload_bytes);
    
    /* 每帧大小的公式
     2.5ms/one frame: frame_size = samplerate/400;   5ms/one frame: frame_size = samplerate/200
     10ms/one frame: frame_size = samplerate/100;    20ms/one frame: frame_size = samplerate/50
     40ms/one frame: frame_size = samplerate/25;     60ms/one frame: frame_size = 3*samplerate/50
     80ms/one frame: frame_size = 4*samplerate/50;   100ms/one frame: frame_size = 5*samplerate/50
     120ms/one frame: frame_size = 6*samplerate/50
     */
    int frame_size = CODEC_SAMPLERATE / 50; //一次编码的帧大小
    //XINT64 num_read; // 实际读到的文件字节数
    int enc_stop = 0;
    int curr_read = 0; //当前已读的文件字节数
    int enc_len = 0; //一次编码返回的字节数
    opus_uint32 enc_final_range = 0; //编码器的状态，编码一个有效负载后，编解码器的状态应该是相同的
    int remaining = 0; //一次编码实际的总字节数小于一次编码的帧大小部分
    int nb_encoded = 0; //一个opus包中，每帧的sample * 总帧数，即一次编码实际的总字节数
    while (!enc_stop)
    {
        XINT64 num_read = enc_fin.Read(enc_bytes.Get(), sizeof(short)*sampleInfo.channels*(frame_size-remaining));
        if (num_read <= 0)
        {
            break;
        }
        curr_read = (int)num_read;
        for (int i = 0; i < curr_read*sampleInfo.channels; i++)
        {
            opus_int32 s;
            s = enc_bytes.Get()[2*i+1] << 8 | enc_bytes.Get()[2*i];
            s = ((s&0xFFFF)^0x8000)-0x8000;
            enc_in.Get()[i+remaining*sampleInfo.channels] = s;
        }
        //实际读取+剩余的小于一次编码的帧大小时，将不足帧大小的部分补0，是该输入文件的最后一次编码
        if (curr_read + remaining < frame_size)
        {
            for (int i = (curr_read + remaining)*sampleInfo.channels; i < frame_size*sampleInfo.channels; i++)
            {
                enc_in.Get()[i] = 0;
            }
            enc_stop = 1;
        }
        
        enc_len = youmecommon::opus_encode(enc, enc_in.Get(), frame_size, enc_data.Get(), max_payload_bytes);
		nb_encoded = youmecommon::opus_packet_get_samples_per_frame(enc_data.Get(), CODEC_SAMPLERATE) * youmecommon::opus_packet_get_nb_frames(enc_data.Get(), enc_len);
        
        remaining = frame_size - nb_encoded;
        for (int i = 0; i < remaining*sampleInfo.channels; i++)
        {
            //移动到实际编码字节数长度的位置
            enc_in.Get()[i] = enc_in.Get()[nb_encoded*sampleInfo.channels+i];
        }
        // 编码器设置状态
		youmecommon::opus_encoder_ctl(enc, OPUS_GET_FINAL_RANGE(&enc_final_range));
        
        if (enc_len < 0)
        {
            YouMe_LOG_Error(__XT("Encode fail."));
			youmecommon::opus_encoder_destroy(enc);
            return -1;
        }
        unsigned char int_field[4];
        int_to_char(enc_len, int_field); //一次编码返回的字节数
        if (enc_fout.Write(int_field, 4) != 4)
        {
            YouMe_LOG_Error(__XT("Writing the length of encoding fail.Quit Encoding."));
			youmecommon::opus_encoder_destroy(enc);
            return -1;
        }
        int_to_char(enc_final_range, int_field); //一次编码的编码器状态
        if (enc_fout.Write(int_field, 4) != 4)
        {
            YouMe_LOG_Error(__XT("Writing the state of encoder fail.Quit Encoding."));
			youmecommon::opus_encoder_destroy(enc);
            return -1;
        }
        if (enc_fout.Write(enc_data.Get(), enc_len) != (unsigned)enc_len) //一次编码的数据
        {
            YouMe_LOG_Error(__XT("Writing the data of encoding fail.Quit Encoding."));
			youmecommon::opus_encoder_destroy(enc);
            return -1;
        }
    }
	youmecommon::opus_encoder_destroy(enc);
    return 0;
}

int EncodePCMFileToOPUSFile(const XString &strPCMFileName, const XString &strOPUSFileName, int wav_sampleRate, short wav_channels, short wav_bitSample, int bitRate)
{
    youmecommon::CXFile enc_fin;
    if (0 != enc_fin.LoadFile(strPCMFileName, youmecommon::CXFile::Mode_OpenExist_ReadOnly))
    {
        YouMe_LOG_Error(__XT("Could not open pcm file: %s."),strPCMFileName.c_str());
        return -1;
    }
    youmecommon::CXFile enc_fout;
    if (0 != enc_fout.LoadFile(strOPUSFileName, youmecommon::CXFile::Mode_CREATE_ALWAYS))
    {
        YouMe_LOG_Error(__XT("Could not open opus file: %s."),strOPUSFileName.c_str());
        return -1;
    }
    
    //创建编码器
    int en_err = 0; //创建编码器的返回值
	youmecommon::OpusEncoder *enc = youmecommon::opus_encoder_create(CODEC_SAMPLERATE, wav_channels, OPUS_APPLICATION_VOIP, &en_err);
    if (en_err != OPUS_OK || enc == NULL)
    {
        YouMe_LOG_Error(__XT("create encoder fail."));
        return -1;
    }
    opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitRate));
    YouMe_LOG_Info(__XT("encoder bitRate:%d"),bitRate);
    //写opus头
    OPUSHEADER opusHeader;
    CreateOPUSFileHeader(wav_sampleRate, wav_channels, wav_bitSample, opusHeader);
    XINT64 nwrite = 0;
    nwrite = enc_fout.Write((const byte*)&opusHeader, sizeof(OPUSHEADER));
    if (nwrite != sizeof(OPUSHEADER))
    {
        YouMe_LOG_Error(__XT("write opus header fail. Don't excute encode."));
		youmecommon::opus_encoder_destroy(enc);
        return -1;
    }
    
    int max_frame_size = 48000*2; //最高的采样率*声道数
    int max_payload_bytes = MAX_PACKET; //最大的包
    youmecommon::CXSharedArray<short> enc_in(max_frame_size*wav_channels);
    youmecommon::CXSharedArray<short> enc_out(max_frame_size*wav_channels);
    youmecommon::CXSharedArray<byte> enc_bytes(max_frame_size*wav_channels);
    youmecommon::CXSharedArray<byte> enc_data(max_payload_bytes);
    
    /* 每帧大小的公式
     2.5ms/one frame: frame_size = samplerate/400;   5ms/one frame: frame_size = samplerate/200
     10ms/one frame: frame_size = samplerate/100;    20ms/one frame: frame_size = samplerate/50
     40ms/one frame: frame_size = samplerate/25;     60ms/one frame: frame_size = 3*samplerate/50
     80ms/one frame: frame_size = 4*samplerate/50;   100ms/one frame: frame_size = 5*samplerate/50
     120ms/one frame: frame_size = 6*samplerate/50
     */
    int frame_size = CODEC_SAMPLERATE / 50; //一次编码的帧大小
    //XINT64 num_read; // 实际读到的文件字节数
    int enc_stop = 0;
    int curr_read = 0; //当前已读的文件字节数
    int enc_len = 0; //一次编码返回的字节数
    opus_uint32 enc_final_range = 0; //编码器的状态，编码一个有效负载后，编解码器的状态应该是相同的
    int remaining = 0; //一次编码实际的总字节数小于一次编码的帧大小部分
    int nb_encoded = 0; //一个opus包中，每帧的sample * 总帧数，即一次编码实际的总字节数
    while (!enc_stop)
    {
        XINT64 num_read = enc_fin.Read(enc_bytes.Get(), sizeof(short)*wav_channels*(frame_size-remaining));
        if (num_read <= 0)
        {
            break;
        }
        curr_read = (int)num_read;
        for (int i = 0; i < curr_read*wav_channels; i++)
        {
            opus_int32 s;
            s = enc_bytes.Get()[2*i+1] << 8 | enc_bytes.Get()[2*i];
            s = ((s&0xFFFF)^0x8000)-0x8000;
            enc_in.Get()[i+remaining*wav_channels] = s;
        }
        //实际读取+剩余的小于一次编码的帧大小时，将不足帧大小的部分补0，是该输入文件的最后一次编码
        if (curr_read + remaining < frame_size)
        {
            for (int i = (curr_read + remaining)*wav_channels; i < frame_size*wav_channels; i++)
            {
                enc_in.Get()[i] = 0;
            }
            enc_stop = 1;
        }
        
        enc_len = youmecommon::opus_encode(enc, enc_in.Get(), frame_size, enc_data.Get(), max_payload_bytes);
        nb_encoded = youmecommon::opus_packet_get_samples_per_frame(enc_data.Get(), CODEC_SAMPLERATE) * youmecommon::opus_packet_get_nb_frames(enc_data.Get(), enc_len);
        
        remaining = frame_size - nb_encoded;
        for (int i = 0; i < remaining*wav_channels; i++)
        {
            //移动到实际编码字节数长度的位置
            enc_in.Get()[i] = enc_in.Get()[nb_encoded*wav_channels+i];
        }
        // 编码器设置状态
        youmecommon::opus_encoder_ctl(enc, OPUS_GET_FINAL_RANGE(&enc_final_range));
        
        if (enc_len < 0)
        {
            YouMe_LOG_Error(__XT("Encode fail."));
            youmecommon::opus_encoder_destroy(enc);
            return -1;
        }
        unsigned char int_field[4];
        int_to_char(enc_len, int_field); //一次编码返回的字节数
        if (enc_fout.Write(int_field, 4) != 4)
        {
            YouMe_LOG_Error(__XT("Writing the length of encoding fail.Quit Encoding."));
            youmecommon::opus_encoder_destroy(enc);
            return -1;
        }
        int_to_char(enc_final_range, int_field); //一次编码的编码器状态
        if (enc_fout.Write(int_field, 4) != 4)
        {
            YouMe_LOG_Error(__XT("Writing the state of encoder fail.Quit Encoding."));
            youmecommon::opus_encoder_destroy(enc);
            return -1;
        }
        if (enc_fout.Write(enc_data.Get(), enc_len) != (unsigned)enc_len) //一次编码的数据
        {            
            YouMe_LOG_Error(__XT("Writing the data of encoding fail.Quit Encoding."));
            youmecommon::opus_encoder_destroy(enc);
            return -1;
        }
    }
    youmecommon::opus_encoder_destroy(enc);
    return 0;
}

int DecodeOPUSFileToWAVFile(const XString &strOPUSFileName, const XString &strWAVFileName)
{
    youmecommon::CXFile dec_fin;
    if (0 != dec_fin.LoadFile(strOPUSFileName, youmecommon::CXFile::Mode_OpenExist_ReadOnly))
    {
        YouMe_LOG_Error(__XT("Could not open input file: %s."),strOPUSFileName.c_str());
        return -1;
    }
    XINT64 num_read = 0; // 实际读到的文件字节数
    OPUSHEADER opusHeader;
    num_read = dec_fin.Read((byte*)&opusHeader, sizeof(OPUSHEADER)); //读取opus头
    if (num_read != sizeof(OPUSHEADER))
    {
        YouMe_LOG_Error(__XT("Reading the opus header fail.Don't excute decode."));
        return -1;
    }
    int ret_flag = memcmp(opusHeader.opusTag, "OPUS", 4);
    if (ret_flag != 0)
    {
        YouMe_LOG_Error(__XT("It isn't the header of opus.Don't excute decode."));
        return -1;
    }
    youmecommon::CXFile dec_fout;
    if (0 != dec_fout.LoadFile(strWAVFileName, youmecommon::CXFile::Mode_CREATE_ALWAYS))
    {
        YouMe_LOG_Error(__XT("Could not open output file: %s."),strWAVFileName.c_str());
        return -1;
    }
    int ret_version = memcmp(&opusHeader.version,"1", 1);
    if (ret_version > 0)
    {
        YouMe_LOG_Error(__XT("Doesn‘t support the version of decoding."));
        return -1;
    }
    int wav_sampleRate = opusHeader.sampleRate;
    short wav_channels = opusHeader.channels;
    short wav_bitSample = opusHeader.bitSample;
    int fileLength = 0; //pcm文件的长度
    //写wav头
    WriteWAVEFileHeader(dec_fout, fileLength, wav_channels, wav_sampleRate, wav_bitSample);
    
    //创建解码器
    int de_err; //创建解码器的返回值
    youmecommon::OpusDecoder *dec = youmecommon::opus_decoder_create(CODEC_SAMPLERATE, wav_channels, &de_err);
    if (de_err != OPUS_OK || dec == NULL)
    {
        YouMe_LOG_Error(__XT("create decoder fail."));
        return -1;
    }
    
    int max_frame_size = 48000*2; //最高的采样率*声道数
    int max_payload_bytes = MAX_PACKET;
    youmecommon::CXSharedArray<short> dec_in(max_frame_size*wav_channels);
    youmecommon::CXSharedArray<short> dec_out(max_frame_size*wav_channels);
    youmecommon::CXSharedArray<byte> dec_bytes(max_frame_size*wav_channels);
    youmecommon::CXSharedArray<byte> dec_data(max_payload_bytes);
    
    opus_uint32 dec_final_range; //解码器的状态
    int dec_len = 0;  //一次解码的字节数
    XINT64 num_write = 0; // 实际写入的文件字节数
    //ret_flag("OPUS")为0且ret_version<=0(版本号不大于最高版本)时才进行解码循环
    while (!ret_flag && (ret_version <= 0)) {
        unsigned char ch[4];
        num_read = dec_fin.Read(ch, 4); //读取一次解码的字节数
        if (num_read != 4)
        {
            break;
        }
        dec_len = char_to_int(ch);
        if (dec_len > max_payload_bytes || dec_len < 0)
        {
            YouMe_LOG_Error(__XT("Invalid payload length: %d. Don't excute decoding."),dec_len);
            youmecommon::opus_decoder_destroy(dec);
            return -1;
        }
        num_read = dec_fin.Read(ch, 4); //读取编码时写入的编码器的状态
        if (num_read != 4)
        {
            YouMe_LOG_Error(__XT("Reading the state of encoder fail.Don't excute decoding."));
            youmecommon::opus_decoder_destroy(dec);
            return -1;
        }
        dec_final_range = char_to_int(ch);
        num_read = dec_fin.Read(dec_data.Get(), dec_len); //读取一次解码的数据
        if (num_read != (size_t)dec_len)
        {
            YouMe_LOG_Error(__XT("Reading the length of encoding fail.Don't excute decoding."));
            youmecommon::opus_decoder_destroy(dec);
            return -1;
        }
        
        opus_int32 output_samples = max_frame_size;
        //output_samples: 解码一次实际返回的字节数
        output_samples = youmecommon::opus_decode(dec, dec_data.Get(), dec_len, dec_out.Get(), output_samples, 0);
        if (output_samples > 0)
        {
            for (int j = 0; j < output_samples*wav_channels; j++)
            {
                short s;
                s = dec_out.Get()[j];
                dec_bytes.Get()[2*j] = s & 0xFF;
                dec_bytes.Get()[2*j+1] = (s>>8)&0xFF;
            }
            num_write = dec_fout.Write(dec_bytes.Get(), sizeof(short)*wav_channels*output_samples);
            fileLength += num_write;
            if (num_write != sizeof(short)*wav_channels*output_samples)
            {
                YouMe_LOG_Error(__XT("Error writing wav file.Quit decoding."));
                youmecommon::opus_decoder_destroy(dec);
                return -1;
            }
        }
        else
        {
            YouMe_LOG_Error(__XT("Decodeing error."));
            youmecommon::opus_decoder_destroy(dec);
            return -1;
        }
        youmecommon::opus_decoder_ctl(dec, OPUS_GET_FINAL_RANGE(&dec_final_range));
    }
    //输出wav文件移到文件开始位置写wav头
    dec_fout.Seek(0, SEEK_SET);
    WriteWAVEFileHeader(dec_fout, fileLength, wav_channels, wav_sampleRate, wav_bitSample);
    
    youmecommon::opus_decoder_destroy(dec);
    return 0;
}

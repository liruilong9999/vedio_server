// Copyright (c) 1996-2017, Live Networks, Inc.  All rights reserved
// "RTSPServer" 的子类，根据指定的流名称是否作为文件存在，按需创建 "ServerMediaSession"
// 实现部分

#include "DynamicRTSPServer.hh"
#include <liveMedia.hh>
#include <string.h>
#include <iostream>

extern "C"
{
#include <libavformat\avformat.h>
#include <libavcodec\avcodec.h>
#include <libavutil\error.h>
#include <libavutil/imgutils.h>
}

DynamicRTSPServer *
DynamicRTSPServer::createNew(UsageEnvironment & env, Port ourPort, UserAuthenticationDatabase * authDatabase, unsigned reclamationTestSeconds)
{
    int ourSocket = setUpOurSocket(env, ourPort); // 设置我们的套接字
    if (ourSocket == -1)
        return NULL;

    return new DynamicRTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds); // 创建新的动态 RTSP 服务器
}

DynamicRTSPServer::DynamicRTSPServer(UsageEnvironment & env, int ourSocket, Port ourPort, UserAuthenticationDatabase * authDatabase, unsigned reclamationTestSeconds)
    : RTSPServerSupportingHTTPStreaming(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds)
{
}

DynamicRTSPServer::~DynamicRTSPServer()
{
}

static ServerMediaSession * createNewSMS(UsageEnvironment & env,
                                         char const *       fileName,
                                         FILE *             fid); // 向前声明

ServerMediaSession * DynamicRTSPServer::lookupServerMediaSession(char const * streamName, Boolean isFirstLookupInSession)
{
    // 首先，检查指定的 "streamName" 是否作为本地文件存在：
    FILE *  fid        = fopen(streamName, "rb");
    Boolean fileExists = fid != NULL;

    // 接下来，检查是否已经为该文件存在一个 "ServerMediaSession"：
    ServerMediaSession * sms       = RTSPServer::lookupServerMediaSession(streamName);
    Boolean              smsExists = sms != NULL;

    // 处理 "fileExists" 和 "smsExists" 四种可能的情况：
    if (!fileExists)
    {
        if (smsExists)
        {
            // "sms" 是为一个不再存在的文件创建的，移除它：
            removeServerMediaSession(sms);
            sms = NULL;
        }

        return NULL;
    }
    else
    {
        if (smsExists && isFirstLookupInSession)
        {
            // 如果文件发生了变化，移除现有的 "ServerMediaSession" 并创建一个新的：
            removeServerMediaSession(sms);
            sms = NULL;
        }

        if (sms == NULL)
        {
            sms = createNewSMS(envir(), streamName, fid); // 创建新的 ServerMediaSession
            addServerMediaSession(sms);                   // 添加到服务器的媒体会话中
        }

        fclose(fid); // 关闭文件
        return sms;
    }
}

// 处理 Matroska 文件的特殊代码：
struct MatroskaDemuxCreationState
{
    MatroskaFileServerDemux * demux;
    char                      watchVariable;
};
static void onMatroskaDemuxCreation(MatroskaFileServerDemux * newDemux, void * clientData)
{
    MatroskaDemuxCreationState * creationState = (MatroskaDemuxCreationState *)clientData;
    creationState->demux                       = newDemux;
    creationState->watchVariable               = 1;
}
// 结束处理 Matroska 文件的特殊代码：

// 处理 Ogg 文件的特殊代码：
struct OggDemuxCreationState
{
    OggFileServerDemux * demux;
    char                 watchVariable;
};
static void onOggDemuxCreation(OggFileServerDemux * newDemux, void * clientData)
{
    OggDemuxCreationState * creationState = (OggDemuxCreationState *)clientData;
    creationState->demux                  = newDemux;
    creationState->watchVariable          = 1;
}
// 结束处理 Ogg 文件的特殊代码：

#define NEW_SMS(description)                                                   \
    do                                                                         \
    {                                                                          \
        char const * descStr = description                                     \
            ", streamed by the LIVE555 Media Server";                          \
        sms = ServerMediaSession::createNew(env, fileName, fileName, descStr); \
    } while (0)

static ServerMediaSession * createNewSMS(UsageEnvironment & env,
                                         char const *       fileName,
                                         FILE * /*fid*/)
{
    // 使用文件名的扩展名来确定 "ServerMediaSession" 的类型：
    char const * extension = strrchr(fileName, '.');
    if (extension == NULL)
        return NULL;

    ServerMediaSession * sms         = NULL;
    Boolean const        reuseSource = False;
    if (strcmp(extension, ".aac") == 0)
    {
        // 假设为 AAC 音频（ADTS 格式）文件：
        NEW_SMS("AAC Audio");
        sms->addSubsession(ADTSAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    }
    else if (strcmp(extension, ".amr") == 0)
    {
        // 假设为 AMR 音频文件：
        NEW_SMS("AMR Audio");
        sms->addSubsession(AMRAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    }
    else if (strcmp(extension, ".ac3") == 0)
    {
        // 假设为 AC-3 音频文件：
        NEW_SMS("AC-3 Audio");
        sms->addSubsession(AC3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    }
    else if (strcmp(extension, ".m4e") == 0)
    {
        // 假设为 MPEG-4 视频元素流文件：
        NEW_SMS("MPEG-4 Video");
        sms->addSubsession(MPEG4VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    }
    else if (strcmp(extension, ".264") == 0)
    {
        // 假设为 H.264 视频元素流文件：
        NEW_SMS("H.264 Video");
        OutPacketBuffer::maxSize = 100000; // 允许较大的 H.264 帧
        sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    }
    else if (strcmp(extension, ".265") == 0)
    {
        // 假设为 H.265 视频元素流文件：
        NEW_SMS("H.265 Video");
        OutPacketBuffer::maxSize = 100000; // 允许较大的 H.265 帧
        sms->addSubsession(H265VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    }
    else if (strcmp(extension, ".mp3") == 0)
    {
        // 假设为 MPEG-1 或 MPEG-2 音频文件：
        NEW_SMS("MPEG-1 or 2 Audio");
        // 要使用 'ADUs' 流式传输而不是原始的 MP3 帧，请取消以下注释：
        // #define STREAM_USING_ADUS 1
        //  要在流式传输之前重新排序 ADU，请取消以下注释：
        // #define INTERLEAVE_ADUS 1
        //  （有关 ADUs 和重排的更多信息，
        //   请参见 <http://www.live555.com/rtp-mp3/>）
        Boolean        useADUs      = False;
        Interleaving * interleaving = NULL;
#ifdef STREAM_USING_ADUS
        useADUs = True;
#ifdef INTERLEAVE_ADUS
        unsigned char  interleaveCycle[]   = {0, 2, 1, 3}; // 或选择您自己的...
        unsigned const interleaveCycleSize = (sizeof interleaveCycle) / (sizeof(unsigned char));
        interleaving                       = new Interleaving(interleaveCycleSize, interleaveCycle);
#endif
#endif
        sms->addSubsession(MP3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, useADUs, interleaving));
    }
    else if (strcmp(extension, ".mpg") == 0)
    {
        // 假设为 MPEG-1 或 MPEG-2 程序流（音频+视频）文件：
        NEW_SMS("MPEG-1 or 2 Program Stream");
        MPEG1or2FileServerDemux * demux = MPEG1or2FileServerDemux::createNew(env, fileName, reuseSource);
        sms->addSubsession(demux->newVideoServerMediaSubsession());
        sms->addSubsession(demux->newAudioServerMediaSubsession());
    }
    else if (strcmp(extension, ".vob") == 0)
    {
        // 假设为 VOB（MPEG-2 程序流，包含 AC-3 音频）文件：
        NEW_SMS("VOB (MPEG-2 video with AC-3 audio)");
        MPEG1or2FileServerDemux * demux = MPEG1or2FileServerDemux::createNew(env, fileName, reuseSource);
        sms->addSubsession(demux->newVideoServerMediaSubsession());
        sms->addSubsession(demux->newAC3AudioServerMediaSubsession());
    }
    else if (strcmp(extension, ".ts") == 0)
    {
        // 假设为 MPEG 传输流文件：
        // 使用与 TS 文件名相同的索引文件名，只是扩展名为 ".tsx"：
        unsigned indexFileNameLen = strlen(fileName) + 2; // 留出尾部 "x\0"
        char *   indexFileName    = new char[indexFileNameLen];
        sprintf(indexFileName, "%sx", fileName);
        NEW_SMS("MPEG Transport Stream");
        sms->addSubsession(MPEG2TransportFileServerMediaSubsession::createNew(env, fileName, indexFileName, reuseSource));
        delete[] indexFileName;
    }
    else if (strcmp(extension, ".wav") == 0)
    {
        // 假设为 WAV 音频文件：
        NEW_SMS("WAV Audio Stream");
        // 在流式传输之前，将 16 位 PCM 数据转换为 8 位 u-law
        // 请将以下代码修改为 True：
        Boolean convertToULaw = False;
        sms->addSubsession(WAVAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, convertToULaw));
    }
    else if (strcmp(extension, ".dv") == 0)
    {
        // 假设为 DV 视频文件
        // 首先，确保 RTPSinks 的缓冲区足够大，以处理 DV 帧的巨大尺寸（最大可达 288000）。
        OutPacketBuffer::maxSize = 300000;

        NEW_SMS("DV Video");
        sms->addSubsession(DVVideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    }
    else if (strcmp(extension, ".mkv") == 0 || strcmp(extension, ".webm") == 0)
    {
        // 假设为 Matroska 文件（注意 WebM（`.webm`）文件也是 Matroska 文件）
        OutPacketBuffer::maxSize = 10000000; // 允许较大的 VP8 或 VP9 帧
        NEW_SMS("Matroska video+audio+(optional)subtitles");

        // 为指定的文件创建 Matroska 文件服务器解复用器。
        // （我们进入事件循环以等待此操作完成。）
        MatroskaDemuxCreationState creationState;
        creationState.watchVariable = 0;
        MatroskaFileServerDemux::createNew(env, fileName, onMatroskaDemuxCreation, &creationState);
        env.taskScheduler().doEventLoop(&creationState.watchVariable);

        ServerMediaSubsession * smss;
        while ((smss = creationState.demux->newServerMediaSubsession()) != NULL)
        {
            sms->addSubsession(smss);
        }
    }
    else if (strcmp(extension, ".ogg") == 0 || strcmp(extension, ".ogv") == 0 || strcmp(extension, ".opus") == 0)
    {
        // 假设为 Ogg 文件
        NEW_SMS("Ogg video and/or audio");

        // 为指定的文件创建 Ogg 文件服务器解复用器。
        // （我们进入事件循环以等待此操作完成。）
        OggDemuxCreationState creationState;
        creationState.watchVariable = 0;
        OggFileServerDemux::createNew(env, fileName, onOggDemuxCreation, &creationState);
        env.taskScheduler().doEventLoop(&creationState.watchVariable);

        ServerMediaSubsession * smss;
        while ((smss = creationState.demux->newServerMediaSubsession()) != NULL)
        {
            sms->addSubsession(smss);
        }
    }

    else if (strcmp(extension, ".mp4") == 0)
    {
       // // 初始化 FFmpeg
       // avformat_network_init();
       // AVFormatContext * fmt_ctx = nullptr;

       // // 打开 MP4 文件
       // if (avformat_open_input(&fmt_ctx, fileName, nullptr, nullptr) != 0)
       // {
       //     std::cerr << "Failed to open MP4 file: " << fileName << std::endl;
       //     return nullptr;
       // }

       // // 查找流信息
       // if (avformat_find_stream_info(fmt_ctx, nullptr) < 0)
       // {
       //     std::cerr << "Failed to retrieve stream info for: " << fileName << std::endl;
       //     avformat_close_input(&fmt_ctx);
       //     return nullptr;
       // }

       // // 查找视频流
       // int videoStreamIndex = -1;
       // for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++)
       // {
       //     if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
       //     {
       //         videoStreamIndex = i;
       //         break;
       //     }
       // }

       // if (videoStreamIndex == -1)
       // {
       //     std::cerr << "No video stream found in file: " << fileName << std::endl;
       //     avformat_close_input(&fmt_ctx);
       //     return nullptr;
       // }

       // AVStream *          videoStream = fmt_ctx->streams[videoStreamIndex];
       // AVCodecParameters * codecpar    = videoStream->codecpar;

       // // 打开解码器
       // const AVCodec * decoder = avcodec_find_decoder(codecpar->codec_id);
       // if (!decoder)
       // {
       //     std::cerr << "Failed to find decoder for codec id: " << codecpar->codec_id << std::endl;
       //     avformat_close_input(&fmt_ctx);
       //     return nullptr;
       // }

       // AVCodecContext * dec_ctx = avcodec_alloc_context3(decoder);
       // if (avcodec_parameters_to_context(dec_ctx, codecpar) < 0)
       // {
       //     std::cerr << "Failed to create decoder context" << std::endl;
       //     avcodec_free_context(&dec_ctx);
       //     avformat_close_input(&fmt_ctx);
       //     return nullptr;
       // }

       // if (avcodec_open2(dec_ctx, decoder, nullptr) < 0)
       // {
       //     std::cerr << "Failed to open decoder" << std::endl;
       //     avcodec_free_context(&dec_ctx);
       //     avformat_close_input(&fmt_ctx);
       //     return nullptr;
       // }

       // // 打开编码器（H.264）
       // const AVCodec * encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
       // if (!encoder)
       // {
       //     std::cerr << "Failed to find H.264 encoder" << std::endl;
       //     avcodec_free_context(&dec_ctx);
       //     avformat_close_input(&fmt_ctx);
       //     return nullptr;
       // }

       // AVCodecContext * enc_ctx = avcodec_alloc_context3(encoder);
       // enc_ctx->bit_rate        = 400000; // 设置比特率
       // enc_ctx->width           = codecpar->width;
       // enc_ctx->height          = codecpar->height;
       // enc_ctx->time_base       = videoStream->time_base;
       // enc_ctx->framerate       = av_guess_frame_rate(fmt_ctx, videoStream, nullptr);
       // enc_ctx->pix_fmt         = AV_PIX_FMT_YUV420P;

       // if (avcodec_open2(enc_ctx, encoder, nullptr) < 0)
       // {
       //     std::cerr << "Failed to open H.264 encoder" << std::endl;
       //     avcodec_free_context(&enc_ctx);
       //     avcodec_free_context(&dec_ctx);
       //     avformat_close_input(&fmt_ctx);
       //     return nullptr;
       // }

       // // 转码并推送到 Live555
       // OutPacketBuffer::maxSize = 1024000; // 允许较大的 H.264 帧
       // H264VideoFileServerMediaSubsession * subsession = H264VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource);
       // //   创建文件服务器子会话，用于推送 MP4 文件
       //// FileServerMediaSubsession * subsession = new FileServerMediaSubsession(env, "video.mp4", reuseSource);
       // if (subsession == nullptr)
       // {
       //     std::cerr << "Failed to create H.264 RTP subsession" << std::endl;
       //     avcodec_free_context(&enc_ctx);
       //     avcodec_free_context(&dec_ctx);
       //     avformat_close_input(&fmt_ctx);
       //     return nullptr;
       // }

       // sms->addSubsession(subsession);

       // // 清理资源
       // avcodec_free_context(&enc_ctx);
       // avcodec_free_context(&dec_ctx);
       // avformat_close_input(&fmt_ctx);
    }
    /*
    else if ((strcmp(extension, ".mp4") == 0) || (strcmp(extension, ".m4v") == 0))
    {
        NEW_SMS("MPEG-4 Video/Audio");
        // get the info about file
        char * command = (char *)malloc(150 * sizeof(char));
        memset(command, 0, 150 * sizeof(char));
        command = strcat(command, "mp4info  ");
        command = strcat(command, fileName);
        command = strcat(command, " >temp ");
        puts(command);
        system(command);
        FILE *fTemp = fopen("temp", "r"),
             *temp  = NULL;
        if (fTemp != NULL)
        {
            char c, ext[4];
            ext[0]      = '.';
            ext[1]      = 't';
            ext[3]      = '\0';
            char * Word = (char *)malloc(100 * sizeof(char));
            memset(Word, 0, 100 * sizeof(char));
            int flagLine = 0, lineCount = 0, flagWord = 0, wordCount = 0, i = 0, flagCodec = 0, streamCount = 1;
            while (!feof(fTemp))
            {
                if (lineCount != 3)
                {
                    c = getc(fTemp);
                }
                if (flagLine == 1)
                {
                    flagLine = 0;
                    if ((c > 48) && (c < 59))
                    { // get inside the stream numbers only ...
                        flagWord = 1;
                        while ((c != '\n') && (!feof(fTemp)))
                        {
                            c = getc(fTemp);
                            if (flagWord == 1)
                            {
                                i = 0;
                                while ((c != ' ') && (c != '\t') && (!feof(fTemp)))
                                {
                                    Word[i] = tolower(c);
                                    i++;
                                    c = getc(fTemp);
                                }
                                Word[i] = '\0';

                                if ((strcmp("video", Word) == 0) || (strcmp("audio", Word) == 0))
                                {
                                    flagCodec = 1;
                                    wordCount = 0;
                                    int i;
                                    for (i = 0; i < 100; i++)
                                    {
                                        Word[i] = '\n';
                                    }
                                    ext[2] = '0' + streamCount;
                                    strcpy(Word, "mp4creator -extract=");
                                    Word[20] = streamCount + 48;
                                    Word[21] = ' ';
                                    Word[22] = '\0';
                                    Word     = strcat(Word, fileName);
                                    puts(Word);

                                    command = strcpy(command, fileName);
                                    command = strcat(command, ext);
                                    temp    = fopen(command, "r");
                                    if (temp == NULL)
                                    {
                                        env << "creating files";
                                        system(Word);
                                    }
                                    puts(command);
                                    streamCount++;
                                }
                                if ((flagCodec == 1) && (wordCount == 1))
                                {
                                    if (strcmp("h264", Word) == 0)
                                    {
                                        //    printf("error cant play H.264 files.");
                                        //    return 0;
                                        NEW_SMS("H.264 Video");
                                        OutPacketBuffer::maxSize = 100000; // allow for some possibly large H.264 frames
                                        sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(env, command, reuseSource));
                                    }
                                }
                                if ((flagCodec == 1) && (wordCount == 2))
                                {
                                    flagCodec = 0;
                                    printf("Flagged - %s  line: %d\n", Word, lineCount);
                                    /// enter the code here
                                    if (strcmp("aac", Word) == 0)
                                    {
                                        puts("aac found");
                                        sms->addSubsession(ADTSAudioFileServerMediaSubsession ::createNew(env, command, reuseSource));
                                        puts(ext);
                                    }
                                    else if (strcmp("simple", Word) == 0)
                                    {
                                        puts("m4e found");
                                        sms->addSubsession(MPEG4VideoFileServerMediaSubsession ::createNew(env, command, reuseSource));
                                        puts(ext);
                                    }
                                    else if (strcmp("h264", Word) == 0)
                                    {
                                        puts("m4e found");
                                        puts(ext);
                                    }
                                    else if (strcmp("amr", Word) == 0)
                                    {
                                        puts("amr found");
                                        puts(ext);
                                    }
                                }
                                flagWord = 0; // the word flag is reset
                            }
                            if ((c == '\t') || (c == ' '))
                            {
                                wordCount++;
                                flagWord = 1; // the word flag set for getting next word.
                            }
                        }
                        flagWord  = 0;
                        wordCount = 0;
                        goto out;
                    }
                }
out:
                if (c == '\n')
                {
                    lineCount++;
                    if (lineCount > 2)
                    {
                        flagLine = 1;
                    }
                }
            }
        }
        else
        {
            printf("the file not found");
            return 0;
        }
    }
    */
    return sms;
}

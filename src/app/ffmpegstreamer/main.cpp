extern "C"
{
#include <libavformat/avformat.h> // 用于处理多媒体流的输入、输出、封装格式等
#include <libavcodec/avcodec.h>   // 用于音视频编解码操作
#include <libavutil/avutil.h>     // 提供一些工具函数
#include <libavdevice/avdevice.h> // 用于设备（如摄像头、音频输入等）操作
}

#include <QThread>

#include <iostream>

int main()
{
    const char * input_file = "./vedio/test4.mp4";                 // 输入文件路径
    const char * output_url = "rtsp://127.0.0.1:8554/live/stream"; // RTSP 推流地址

    // 初始化网络功能，支持RTSP推流等协议
    avformat_network_init();

    // 声明并初始化所需的上下文
    AVFormatContext * input_context  = nullptr; // 输入流上下文
    AVFormatContext * output_context = nullptr; // 输出流上下文
    AVCodecContext *  codec_context  = nullptr; // 解码器上下文
    const AVCodec *   codec          = nullptr; // 解码器指针

    // 打开输入文件并初始化输入流上下文
    if (avformat_open_input(&input_context, input_file, nullptr, nullptr) < 0)
    {
        std::cerr << "Could not open input file: " << input_file << std::endl;
        return -1; // 文件打开失败，返回错误
    }

    // 获取输入流信息，如音视频流的详细信息（码率、时长、流类型等）
    if (avformat_find_stream_info(input_context, nullptr) < 0)
    {
        std::cerr << "Failed to retrieve input stream info" << std::endl;
        return -1; // 获取流信息失败，返回错误
    }

    // 打印输入流的详细信息（音视频流的编码格式、分辨率等）
    av_dump_format(input_context, 0, input_file, 0);

    // 创建 RTSP 推流输出上下文
    if (avformat_alloc_output_context2(&output_context, nullptr, "rtsp", output_url) < 0)
    {
        std::cerr << "Could not create output context" << std::endl;
        return -1; // 创建输出上下文失败，返回错误
    }

    // 查找视频流的索引，通常视频流会在输入文件中首先出现
    int video_stream_index = -1;
    for (int i = 0; i < input_context->nb_streams; i++)
    {
        if (input_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_index = i; // 找到第一个视频流
            break;
        }
    }

    if (video_stream_index == -1)
    {
        std::cerr << "No video stream found in the input file" << std::endl;
        return -1; // 没有找到视频流，返回错误
    }

    // 获取视频流的相关参数和信息
    AVStream *          in_stream    = input_context->streams[video_stream_index];
    AVCodecParameters * codec_params = in_stream->codecpar;

    // 查找对应的视频解码器，使用流的编码ID
    codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec)
    {
        std::cerr << "Codec not found!" << std::endl;
        return -1; // 解码器未找到，返回错误
    }

    // 为解码器分配上下文
    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context)
    {
        std::cerr << "Failed to allocate codec context" << std::endl;
        return -1; // 分配解码器上下文失败，返回错误
    }

    // 将解码器的参数复制到解码器上下文中
    if (avcodec_parameters_to_context(codec_context, codec_params) < 0)
    {
        std::cerr << "Failed to copy codec parameters to codec context" << std::endl;
        return -1; // 复制参数失败，返回错误
    }

    // 打开解码器以准备解码
    if (avcodec_open2(codec_context, codec, nullptr) < 0)
    {
        std::cerr << "Failed to open codec" << std::endl;
        return -1; // 解码器打开失败，返回错误
    }

    // 创建输出流
    AVStream * out_stream = avformat_new_stream(output_context, nullptr);
    if (!out_stream)
    {
        std::cerr << "Failed to create output stream" << std::endl;
        return -1; // 输出流创建失败，返回错误
    }

    // 配置输出流的参数，复制输入流的编码参数到输出流
    if (avcodec_parameters_copy(out_stream->codecpar, codec_params) < 0)
    {
        std::cerr << "Failed to copy codec parameters to output stream" << std::endl;
        return -1; // 复制参数失败，返回错误
    }

    // 打开输出流，如果没有文件，则打开网络推流
    if (!(output_context->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&output_context->pb, output_url, AVIO_FLAG_WRITE) < 0)
        {
            std::cerr << "Failed to open output file" << std::endl;
            return -1; // 打开输出文件失败，返回错误
        }
    }

    // 写入输出流头部信息（格式、编码信息等）
    if (avformat_write_header(output_context, nullptr) < 0)
    {
        std::cerr << "Failed to write header to output file" << std::endl;
        return -1; // 写入头部失败，返回错误
    }

    // 推流循环：读取输入流的数据包，并写入输出流（RTSP）
    AVPacket packet;
    while (av_read_frame(input_context, &packet) >= 0)
    {
        if (packet.stream_index == video_stream_index)
        {
            // 如果是视频流，将数据包写入输出流
            if (av_interleaved_write_frame(output_context, &packet) < 0)
            {
                std::cerr << "Failed to write frame" << std::endl;
                break; // 写入数据包失败，跳出循环
            }
        }

        av_packet_unref(&packet); // 释放已处理的数据包
    }

    // 写入输出流尾部信息，完成推流
    av_write_trailer(output_context);

    // 清理资源：释放解码器上下文、输入输出上下文
    avcodec_free_context(&codec_context);  // 释放解码器上下文
    avformat_close_input(&input_context);  // 关闭输入文件
    avformat_free_context(output_context); // 释放输出上下文

    std::cout << "Stream finished" << std::endl;

    return 0; // 程序结束
}
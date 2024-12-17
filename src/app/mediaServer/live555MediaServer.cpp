#include <BasicUsageEnvironment.hh>
#include "DynamicRTSPServer.hh"
#include "version.hh"

int main(int argc, char ** argv)
{
    // 开始设置我们的使用环境：
    TaskScheduler *    scheduler = BasicTaskScheduler::createNew();              // 创建任务调度器
    UsageEnvironment * env       = BasicUsageEnvironment::createNew(*scheduler); // 创建使用环境

    UserAuthenticationDatabase * authDB = NULL; // 初始化用户认证数据库为空
#ifdef ACCESS_CONTROL
    // 如果需要实现客户端访问控制到 RTSP 服务器，执行以下操作：
    authDB = new UserAuthenticationDatabase;         // 创建一个新的用户认证数据库
    authDB->addUserRecord("username1", "password1"); // 添加用户记录，用户名为 "username1"，密码为 "password1"
                                                     // 重复上述操作以允许其他 <用户名> 和 <密码> 访问服务器
#endif

    // 创建 RTSP 服务器。首先尝试使用默认端口号（554），
    // 如果失败则使用备用端口号（8554）：
    RTSPServer * rtspServer;
    portNumBits  rtspServerPortNum = 554;                                                           // 默认端口号 554
    rtspServer                     = DynamicRTSPServer::createNew(*env, rtspServerPortNum, authDB); // 创建动态 RTSP 服务器
    if (rtspServer == NULL)
    {
        rtspServerPortNum = 8554;                                                          // 如果默认端口号创建失败，使用备用端口号 8554
        rtspServer        = DynamicRTSPServer::createNew(*env, rtspServerPortNum, authDB); // 再次尝试创建 RTSP 服务器
    }
    if (rtspServer == NULL)
    {
        *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n"; // 如果创建失败，输出错误信息
        exit(1);                                                                 // 退出程序
    }

    *env << "LIVE555 Media Server\n"; // 输出服务器信息
    *env << "\tversion " << MEDIA_SERVER_VERSION_STRING
         << " (LIVE555 Streaming Media library version "
         << LIVEMEDIA_LIBRARY_VERSION_STRING << ").\n"; // 输出媒体服务器版本信息

    char * urlPrefix = rtspServer->rtspURLPrefix(); // 获取 RTSP 服务器的 URL 前缀
    *env << "Play streams from this server using the URL\n\t"
         << urlPrefix << "<filename>\nwhere <filename> is a file present in the current directory.\n"; // 提供播放流媒体的 URL 路径
    *env << "Each file's type is inferred from its name suffix:\n";
    *env << "\t\".264\" => a H.264 Video Elementary Stream file\n";
    *env << "\t\".265\" => a H.265 Video Elementary Stream file\n";
    *env << "\t\".aac\" => an AAC Audio (ADTS format) file\n";
    *env << "\t\".ac3\" => an AC-3 Audio file\n";
    *env << "\t\".amr\" => an AMR Audio file\n";
    *env << "\t\".dv\" => a DV Video file\n";
    *env << "\t\".m4e\" => a MPEG-4 Video Elementary Stream file\n";
    *env << "\t\".mkv\" => a Matroska audio+video+(optional)subtitles file\n";
    *env << "\t\".mp3\" => a MPEG-1 or 2 Audio file\n";
    *env << "\t\".mpg\" => a MPEG-1 or 2 Program Stream (audio+video) file\n";
    *env << "\t\".ogg\" or \".ogv\" or \".opus\" => an Ogg audio and/or video file\n";
    *env << "\t\".ts\" => a MPEG Transport Stream file\n";
    *env << "\t\t(a \".tsx\" index file - if present - provides server 'trick play' support)\n";
    *env << "\t\".vob\" => a VOB (MPEG-2 video with AC-3 audio) file\n";
    *env << "\t\".wav\" => a WAV Audio file\n";
    *env << "\t\".webm\" => a WebM audio(Vorbis)+video(VP8) file\n";
    *env << "See http://www.live555.com/mediaServer/ for additional documentation.\n"; // 提供支持的文件类型以及文档链接

    // 另外，尝试创建一个 HTTP 服务器以支持 RTSP-over-HTTP 隧道传输。
    // 首先尝试使用默认 HTTP 端口（80），然后尝试备用的 HTTP
    // 端口号（8000 和 8080）。

    if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080))
    {
        *env << "(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling, or for HTTP live streaming (for indexed Transport Stream files only).)\n";
    }
    else
    {
        *env << "(RTSP-over-HTTP tunneling is not available.)\n"; // 如果 RTSP-over-HTTP 隧道传输不可用，输出提示信息
    }

    env->taskScheduler().doEventLoop(); // 进入事件循环，服务器开始工作

    return 0; // 仅为了避免编译器警告，实际代码永远不会到达这里
}

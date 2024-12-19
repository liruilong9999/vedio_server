/*
 * media_server.cpp
 *
 *  Created on: 2011-12-8
 *      Author: Liang Guangwei
 */

#include <BasicUsageEnvironment.hh>
#include "liveMedia.hh"
#include "ffmpeg_media.h"
#include "ffmpeg_rtsp_server.h"

int main(int argc, char *argv[])
{
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

    //create RTSP server
    /*RTSPServer* rtspServer = FfmpegRTSPServer::CreateNew(*env,554);  //554 port
    if(rtspServer == NULL)
    {
        rtspServer = FfmpegRTSPServer::CreateNew(*env,8554);  //if 554 port be used, use 8554 port
    }*/
	unsigned short port = 0;
	if(argc >= 2)
	{
		port = atoi(argv[1]);	
		if(port < 554)
			port = 8554;
	}
	else
		port = 8554;

    RTSPServer* rtspServer = FfmpegRTSPServer::CreateNew(*env, port);  //554 port
    if (rtspServer == NULL) 
    {
        *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
        exit(1);
    }

  //*env << "LIVE555 Media Server\n";
  //*env << "\tversion " //<< MEDIA_SERVER_VERSION_STRING
  //	  << " (LIVE555 Streaming Media library version \n";
       //<< LIVEMEDIA_LIBRARY_VERSION_STRING << ").\n";

  char* urlPrefix = rtspServer->rtspURLPrefix();
  *env << "Play streams from this server using the URL\n\t"
       << urlPrefix << "<filename>\nwhere <filename> is a file present in the current directory.\n";
#if 0
  *env << "Each file's type is inferred from its name suffix:\n";
  *env << "\t\".avi\" => a AVI file,Video:H264 decode and audio:PCMU Stream file\n";
  *env << "\t\".mp4\" => a MPEG-4 file, video:H264 decode and audio:AAC stream file\n";
  *env << "\t\".264\" => a H.264 Video Elementary Stream file\n";
  *env << "\t\".aac\" => an AAC Audio (ADTS format) file\n";
  *env << "\t\".ac3\" => an AC-3 Audio file\n";
  *env << "\t\".amr\" => an AMR Audio file\n";
  *env << "\t\".dv\" => a DV Video file\n";
  *env << "\t\".m4e\" => a MPEG-4 Video Elementary Stream file\n";
  *env << "\t\".mkv\" => a Matroska audio+video+(optional)subtitles file\n";
  *env << "\t\".mp3\" => a MPEG-1 or 2 Audio file\n";
  *env << "\t\".mpg\" => a MPEG-1 or 2 Program Stream (audio+video) file\n";
  *env << "\t\".ts\" => a MPEG Transport Stream file\n";
  *env << "\t\t(a \".tsx\" index file - if present - provides server 'trick play' support)\n";
  *env << "\t\".wav\" => a WAV Audio file\n";
  *env << "\t\".webm\" => a WebM audio(Vorbis)+video(VP8) file\n";
  *env << "See http://www.live555.com/mediaServer/ for additional documentation.\n";
#endif
  // Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
  // Try first with the default HTTP port (80), and then with the alternative HTTP
  // port numbers (8000 and 8080).

  /*if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080)) {
    *env << "(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling, or for HTTP live streaming (for indexed Transport Stream files only).)\n";
  } else {
    *env << "(RTSP-over-HTTP tunneling is not available.)\n";
  }*/

  env->taskScheduler().doEventLoop(); // does not return

    return 0;
}



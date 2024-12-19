/*
 * ffmpeg_PCM_server_media_subsession.h
 *
 *  Created on: 2011-12-8
 *      Author: Administrator
 */

#ifndef FFMPEG_PCM_SERVER_MEDIA_SUBSESSION_H_
#define FFMPEG_PCM_SERVER_MEDIA_SUBSESSION_H_

class FfmpegPCMServerMediaSubsession : public WAVAudioFileServerMediaSubsession{
public:
    static FfmpegPCMServerMediaSubsession* CreateNew(
            FfmpegServerDemux& demux,
            u_int8_t stream_id,
			//FramedSource *source,
            Boolean reuse_source);

private:
    FfmpegPCMServerMediaSubsession(
            FfmpegServerDemux& demux,
            u_int8_t stream_id,
			//FramedSource *source,
            Boolean reuse_source);
    virtual ~FfmpegPCMServerMediaSubsession();

protected:  //redefined virtual functions
    virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
                            unsigned& estBitrate);
    virtual void seekStreamSource(FramedSource* inputSource, double& seekNPT, double /*streamDuration*/, u_int64_t& /*numBytes*/);
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic,
		FramedSource* inputSource);
private:
    FfmpegServerDemux& ffmpeg_demux_;
    u_int8_t stream_id_;
	FramedSource *source;
};

#endif /* FFMPEG_PCM_SERVER_MEDIA_SUBSESSION_H_ */

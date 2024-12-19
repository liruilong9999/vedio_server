/*
 * ffmpeg_PCM_server_media_subsession.cpp
 *
 *  Created on: 2011-12-8
 *      Author: Liang Guangwei
 */

#include "BasicUsageEnvironment.hh"
#include "liveMedia.hh"
#include "ffmpeg_demux.h"
#include "ffmpeg_server_demux.h"
#include "ffmpeg_demuxed_elementary_stream.h"
#include "ffmpeg_pcm_server_media_subsession.h"

FfmpegPCMServerMediaSubsession *FfmpegPCMServerMediaSubsession::CreateNew(
	FfmpegServerDemux& demux, u_int8_t stream_id, Boolean reuse_source)
{
	return new FfmpegPCMServerMediaSubsession(demux, stream_id, reuse_source);
}

FfmpegPCMServerMediaSubsession::FfmpegPCMServerMediaSubsession(
	FfmpegServerDemux& demux, u_int8_t stream_id, Boolean reuse_source) :
		//VorbisAudioMatroskaFileServerMediaSubsession(demux.envir(), NULL,reuse_source, TRUE),
		WAVAudioFileServerMediaSubsession(demux.envir(), NULL, reuse_source, False),
        ffmpeg_demux_(demux),
        stream_id_(stream_id)
{

}

FfmpegPCMServerMediaSubsession::~FfmpegPCMServerMediaSubsession() {
    // TODO Auto-generated destructor stub
}

FramedSource *FfmpegPCMServerMediaSubsession::createNewStreamSource(
        unsigned  clientSessionId, unsigned  & estBitrate)
{
    FramedSource* source = ffmpeg_demux_.NewElementaryStream(clientSessionId, stream_id_);
	if (source == NULL)
	{
		printf("createNewStreamSource source: %p", source);
		return NULL;
	}
	//printf("audio SessionId:0x%x, estBitrate:%d, stream_id_:%d, FrameSrc:%p\n", clientSessionId, estBitrate, stream_id_, source);
	
	StreamInfo *info = (StreamInfo *)ffmpeg_demux_.GetStreamInfo(stream_id_);

	fAudioFormat = 7;
	fBitsPerSample = 16;
	fSamplingFrequency = info->sample_rate;
	//fNumChannels = info->channels;
	fFileDuration = info->total_duration;
//	fDurationInMicroseconds = info->totoa_duration * 1000;
	printf("fFileDuration:%d\n", info->duration);
	//printf("channels:%d codec_id:%d duration:%d min_type:%s sample_rate:%d\n", fNumChannels, info->codec_id, fFileDuration, info->mine_type, info->sample_rate);

	EndianSwap16 *temp = EndianSwap16::createNew(envir(), source);
	if (temp == NULL)
	{
		printf("uLawFromPCMAudioSource::createNew ERROR\n");
		return NULL;
	}

	return temp;
}

RTPSink *FfmpegPCMServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic,
		FramedSource* inputSource)
{
	do{
		char const *mimeType;
		unsigned char payloadFormatCode;

		mimeType = "PCMU";
		payloadFormatCode = 0;// a static RTP payload type
		return SimpleRTPSink::createNew(envir(), rtpGroupsock,
			payloadFormatCode, fSamplingFrequency,
			"audio", mimeType, fNumChannels);
	} while (0);
}

//we must override seekStreamSource function
void FfmpegPCMServerMediaSubsession::seekStreamSource(FramedSource* inputSource, 
                                                      double& seekNPT, 
                                                      double /*streamDuration*/, 
                                                      u_int64_t& /*numBytes*/)
{
    //TODO: now do nothing
}

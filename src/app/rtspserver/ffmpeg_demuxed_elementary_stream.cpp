/*
 * FfmpegDemuxedElementaryStream.cpp
 *
 *  Created on: 2011-12-8
 *      Author: Administrator
 */
#include "liveMedia.hh"
#include <GroupsockHelper.hh> 
#include "ffmpeg_demux.h"
#include "ffmpeg_demuxed_elementary_stream.h"

//#define EX_DEBUG

FfmpegDemuxedElementaryStream *FfmpegDemuxedElementaryStream::CreateNew(
        UsageEnvironment & env,
        u_int8_t stream_id,
        FfmpegDemux& demux, 
        char const* mine_type,
        unsigned duration
        )
{
    return new FfmpegDemuxedElementaryStream(env, stream_id, demux, mine_type, duration);
}

FfmpegDemuxedElementaryStream::~FfmpegDemuxedElementaryStream() {
    ffmpeg_demux_.NoteElementaryStreamDeletion();
}
FfmpegDemuxedElementaryStream::FfmpegDemuxedElementaryStream(
        UsageEnvironment& env,
        u_int8_t stream_id,
        FfmpegDemux& demux, 
        char const* mine_type,
        unsigned duration):
    FramedSource(env), ffmpeg_demux_(demux), stream_id_(stream_id), duration_(duration){

    mine_type_ = strdup(mine_type);

	fDurationInMicroseconds = duration_; //added liveStream 2016-05-07
	//printf("fDu:%d, dura:%d\n", fDurationInMicroseconds, duration_);
    // Use the current wallclock time as the base 'presentation time':
    gettimeofday(&fPresentationTime, NULL);  

}

void FfmpegDemuxedElementaryStream::doGetNextFrame()
{
	//printf("before ffmpeg_demux_.GetNextFrame fMaxSize:%d\n", fMaxSize);
    ffmpeg_demux_.GetNextFrame(stream_id_, fTo, fMaxSize,
            AfterGettingFrame, this, handleClosure, this);
}

void FfmpegDemuxedElementaryStream::AfterGettingFrame1(unsigned  frame_size,
        unsigned  num_truncated_bytes, struct timeval /*presentation_time*/,
        unsigned  /*duration_in_microseconds*/)
{
	//printf("IN FfmpegDemuxedElementaryStream::AfterGettingFrame1\n");
    fFrameSize = frame_size;
    fNumTruncatedBytes = num_truncated_bytes;
	//printf("mine_type:%s\n", MIMEtype());

	//if (strncmp(MIMEtype(), "audio/MPEG", 10) == 0) // ÒôÆµ
	//{
	//	//printf("mine_type:%s\n", MIMEtype());
	//	if (audio_sample_rate <= 0)
	//		duration_ = frame_size * 1000000 / 8000;
	//	else
	//		duration_ = frame_size * 1000000 / audio_sample_rate;
	//	//printf(".......duration:%d\n", duration_);
	//}
	//else
	//	;
    fPresentationTime.tv_usec += duration_;
    fPresentationTime.tv_sec += fPresentationTime.tv_usec/1000000;
    fPresentationTime.tv_usec %= 1000000;
    fDurationInMicroseconds = duration_;


#ifdef EX_DEBUG
//#if 1
    envir() << "stream " << stream_id_ << "  ";
        envir()<< "frame size " << frame_size << "  ";
        envir()<< "truncated bytes " << num_truncated_bytes << "  ";
        envir()<< "presentation time " << (double)(fPresentationTime.tv_sec) << "  " ;
        envir()<< (double)(fPresentationTime.tv_usec) << "  "
        << "duration" << fDurationInMicroseconds << "  "
        << "\n";
#endif

    FramedSource::afterGetting(this);
}

void FfmpegDemuxedElementaryStream::AfterGettingFrame(void *client_data,
        unsigned  frame_size, unsigned  num_truncated_bytes, struct timeval presentation_time,
        unsigned  duration_in_microseconds)
{
    FfmpegDemuxedElementaryStream* stream
      = (FfmpegDemuxedElementaryStream*)client_data;

    stream->AfterGettingFrame1(frame_size, num_truncated_bytes,
                   presentation_time, duration_in_microseconds);
}



void FfmpegDemuxedElementaryStream::doStopGettingFrames()
{
    //TODO
}

char const* FfmpegDemuxedElementaryStream::MIMEtype() const
{

    return mine_type_;
}





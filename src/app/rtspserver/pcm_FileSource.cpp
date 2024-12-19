/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2011 Live Networks, Inc.  All rights reserved.
// A WAV audio file source
// Implementation

#include "pcm_FileSource.hh"
#include "InputFile.hh"
#include "GroupsockHelper.hh"

////////// pcm_FileSource //////////

pcm_FileSource*
pcm_FileSource::createNew(UsageEnvironment& env, char const* fileName) {

  return NULL;
}

unsigned pcm_FileSource::numPCMBytes() const {
  if (fFileSize < fWAVHeaderSize) return 0;
  return fFileSize - fWAVHeaderSize;
}

void pcm_FileSource::setScaleFactor(int scale) {
  fScaleFactor = scale;

  if (fScaleFactor < 0 && TellFile64(fFid) > 0) {
    // Because we're reading backwards, seek back one sample, to ensure that
    // (i)  we start reading the last sample before the start point, and
    // (ii) we don't hit end-of-file on the first read.
    int bytesPerSample = (fNumChannels*fBitsPerSample)/8;
    if (bytesPerSample == 0) bytesPerSample = 1;
    SeekFile64(fFid, -bytesPerSample, SEEK_CUR);
  }
}

void pcm_FileSource::seekToPCMByte(unsigned byteNumber, unsigned numBytesToStream) {

}

unsigned char pcm_FileSource::getAudioFormat() {
  return 7;
}


#define nextc fgetc(fid)

static Boolean get4Bytes(FILE* fid, unsigned& result) { // little-endian
  int c0, c1, c2, c3;
  if ((c0 = nextc) == EOF || (c1 = nextc) == EOF ||
      (c2 = nextc) == EOF || (c3 = nextc) == EOF) return False;
  result = (c3<<24)|(c2<<16)|(c1<<8)|c0;
  return True;
}

static Boolean get2Bytes(FILE* fid, unsigned short& result) {//little-endian
  int c0, c1;
  if ((c0 = nextc) == EOF || (c1 = nextc) == EOF) return False;
  result = (c1<<8)|c0;
  return True;
}

static Boolean skipBytes(FILE* fid, int num) {
  while (num-- > 0) {
    if (nextc == EOF) return False;
  }
  return True;
}

pcm_FileSource::pcm_FileSource(UsageEnvironment& env, FILE* fid)
  : AudioInputDevice(env, 0, 0, 0, 0)/* set the real parameters later */,
    fFid(fid), fLastPlayTime(0), fWAVHeaderSize(0), fFileSize(0), fScaleFactor(1),
    fLimitNumBytesToStream(False), fNumBytesToStream(0), fAudioFormat(WA_UNKNOWN) {
  // Check the WAV file header for validity.
  // Note: The following web pages contain info about the WAV format:
  // http://www.ringthis.com/dev/wave_format.htm
  // http://www.lightlink.com/tjweber/StripWav/Canon.html
  // http://www.wotsit.org/list.asp?al=W

  Boolean success = False; // until we learn otherwise

  fSamplingFrequency  = 8000;

  fAudioFormat = WA_PCMU;

  fNumChannels = 1;

  fBitsPerSample = 16;

  fWAVHeaderSize = 0;

  fPlayTimePerSample = 1e6/(double)fSamplingFrequency;

  // Although PCM is a sample-based format, we group samples into
  // 'frames' for efficient delivery to clients.  Set up our preferred
  // frame size to be close to 20 ms, if possible, but always no greater
  // than 1400 bytes (to ensure that it will fit in a single RTP packet)
  unsigned maxSamplesPerFrame = (1400*8)/(fNumChannels*fBitsPerSample);
  unsigned desiredSamplesPerFrame = (unsigned)(0.02*fSamplingFrequency);
  unsigned samplesPerFrame = desiredSamplesPerFrame < maxSamplesPerFrame ? desiredSamplesPerFrame : maxSamplesPerFrame;
  fPreferredFrameSize = (samplesPerFrame*fNumChannels*fBitsPerSample)/8;
}

pcm_FileSource::~pcm_FileSource() {
  CloseInputFile(fFid);
}

// Note: We should change the following to use asynchronous file reading, #####
// as we now do with ByteStreamFileSource. #####
void pcm_FileSource::doGetNextFrame() {
  if (feof(fFid) || ferror(fFid) || (fLimitNumBytesToStream && fNumBytesToStream == 0)) {
    handleClosure(this);
    return;
  }

  // Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
  if (fLimitNumBytesToStream && fNumBytesToStream < fMaxSize) {
    fMaxSize = fNumBytesToStream;
  }
  if (fPreferredFrameSize < fMaxSize) {
    fMaxSize = fPreferredFrameSize;
  }
  unsigned bytesPerSample = (fNumChannels*fBitsPerSample)/8;
  if (bytesPerSample == 0) bytesPerSample = 1; // because we can't read less than a byte at a time
  unsigned bytesToRead = fMaxSize - fMaxSize%bytesPerSample;
  if (fScaleFactor == 1) {
    // Common case - read samples in bulk:
    fFrameSize = fread(fTo, 1, bytesToRead, fFid);
    fNumBytesToStream -= fFrameSize;
  } else {
    // We read every 'fScaleFactor'th sample:
    fFrameSize = 0;
    while (bytesToRead > 0) {
      size_t bytesRead = fread(fTo, 1, bytesPerSample, fFid);
      if (bytesRead <= 0) break;
      fTo += bytesRead;
      fFrameSize += bytesRead;
      fNumBytesToStream -= bytesRead;
      bytesToRead -= bytesRead;

      // Seek to the appropriate place for the next sample:
      SeekFile64(fFid, (fScaleFactor-1)*bytesPerSample, SEEK_CUR);
    }
  }

  // Set the 'presentation time' and 'duration' of this frame:
  if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
    // This is the first frame, so use the current time:
    gettimeofday(&fPresentationTime, NULL);
  } else {
    // Increment by the play time of the previous data:
    unsigned uSeconds	= fPresentationTime.tv_usec + fLastPlayTime;
    fPresentationTime.tv_sec += uSeconds/1000000;
    fPresentationTime.tv_usec = uSeconds%1000000;
  }

  // Remember the play time of this data:
  fDurationInMicroseconds = fLastPlayTime
    = (unsigned)((fPlayTimePerSample*fFrameSize)/bytesPerSample);

  // Switch to another task, and inform the reader that he has data:
#if defined(__WIN32__) || defined(_WIN32)
  // HACK: One of our applications that uses this source uses an
  // implementation of scheduleDelayedTask() that performs very badly
  // (chewing up lots of CPU time, apparently polling) on Windows.
  // Until this is fixed, we just call our "afterGetting()" function
  // directly.  This avoids infinite recursion, as long as our sink
  // is discontinuous, which is the case for the RTP sink that
  // this application uses. #####
  afterGetting(this);
#else
  nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
			(TaskFunc*)FramedSource::afterGetting, this);
#endif
}

Boolean pcm_FileSource::setInputPort(int /*portIndex*/) {
  return True;
}

double pcm_FileSource::getAverageLevel() const {
  return 0.0;//##### fix this later
}

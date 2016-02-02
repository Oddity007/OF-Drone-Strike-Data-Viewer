#include "AEAudio.h"
//Add known systems
/*#ifdef __APPLE_CC__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
//Most portable option, just add a header search path
#include "al.h"
#include "alc.h"
#endif*/
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/ext.hpp"


#include <vector>

struct AEAudioContext
{
	ALCdevice* alcdevice;
	ALCcontext* alccontext;
	
	std::vector<AEAudioStream> streams;
	std::vector<AEAudioHandle> sources;
};

struct AEAudioStream
{
	AEAudioHandle ID;
	
	stb_vorbis* stream;
	stb_vorbis_info info;
	
	AEAudioHandle buffers[2];
	AEAudioHandle source;
	ALenum format;
	
	size_t bufferSize;
	
	size_t totalSamplesLeft;
	
	bool shouldLoop;
};

void AEAudioStreamInit(AEAudioStream* self)
{
	memset(self, 0, sizeof(AEAudioStream));
	alGenSources(1, & self->source);
	alGenBuffers(2, self->buffers);
	self->bufferSize=4096*8;
	self->shouldLoop=true;//We loop by default
}

void AEAudioStreamDeinit(AEAudioStream* self)
{
	alDeleteSources(1, & self->source);
	alDeleteBuffers(2, self->buffers);
	stb_vorbis_close(self->stream);
	memset(self, 0, sizeof(AEAudioStream));
}

bool AEAudioStreamStream(AEAudioStream* self, AEAudioHandle buffer)
{
	//Uncomment this to avoid VLAs
	//#define BUFFER_SIZE 4096*32
	#ifndef BUFFER_SIZE//VLAs ftw
	#define BUFFER_SIZE (self->bufferSize)
	#endif
	ALshort pcm[BUFFER_SIZE];
	int size = 0;
	int result = 0;
	
	while(size < BUFFER_SIZE){
		result = stb_vorbis_get_samples_short_interleaved(self->stream, self->info.channels, pcm+size, BUFFER_SIZE-size);
		if(result > 0) size += result*self->info.channels;
		else break;
	}
	
	if(size == 0) return false;
	
	alBufferData(buffer, self->format, pcm, size*sizeof(ALshort), self->info.sample_rate);
	self->totalSamplesLeft-=size;
	#undef BUFFER_SIZE
	
	return true;
}

bool AEAudioStreamOpen(AEAudioStream* self, const char* filename)
{
	self->stream = stb_vorbis_open_filename((char*)filename, NULL, NULL);
	if(not self->stream) return false;
	// Get file info
	self->info = stb_vorbis_get_info(self->stream);
	if(self->info.channels == 2) self->format = AL_FORMAT_STEREO16;
	else self->format = AL_FORMAT_MONO16;
	
	if(not AEAudioStreamStream(self, self->buffers[0])) return false;
	if(not AEAudioStreamStream(self, self->buffers[1])) return false;
	alSourceQueueBuffers(self->source, 2, self->buffers);
	alSourcePlay(self->source);
	
	self->totalSamplesLeft = stb_vorbis_stream_length_in_samples(self->stream) * self->info.channels;
	
	return true;
}

bool AEAudioStreamUpdate(AEAudioStream* self)
{
	ALint processed=0;
	
    alGetSourcei(self->source, AL_BUFFERS_PROCESSED, &processed);

    while(processed--)
	{
        AEAudioHandle buffer=0;
        
        alSourceUnqueueBuffers(self->source, 1, &buffer);
		
		if(not AEAudioStreamStream(self, buffer))
		{
			bool shouldExit=true;
			
			if(self->shouldLoop)
			{
				stb_vorbis_seek_start(self->stream);
				self->totalSamplesLeft = stb_vorbis_stream_length_in_samples(self->stream) * self->info.channels;
				shouldExit = not AEAudioStreamStream(self, buffer);
			}
			
			if(shouldExit) return false;
		}
		
		alSourceQueueBuffers(self->source, 1, &buffer);
	}
	
	return true;
}

AEAudioContext* AEAudioContextNew(void)
{
	AEAudioContext* self = new AEAudioContext;
	self->alcdevice = alcOpenDevice(NULL);
	self->alccontext = alcCreateContext(self->alcdevice, NULL);
	alcMakeContextCurrent(self->alccontext);
	return self;
}

void AEAudioContextDelete(AEAudioContext* self)
{
	if(not self) return;
	if(self->sources.size()) alDeleteSources(self->sources.size(), & self->sources[0]);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(self->alccontext);
	alcCloseDevice(self->alcdevice);
	delete self;
}

void AEAudioContextSetPosition(AEAudioContext* self, float x, float y, float z)
{
	ALfloat v3[3] = {x, y, z};
	alListenerfv(AL_POSITION, v3);
}

void AEAudioContextSetRotation(AEAudioContext* self, float x, float y, float z, float w)
{
	glm::quat q(w, x, y, z);
	glm::vec3 at = q * glm::vec3(0, 0, -1);
	glm::vec3 up = q * glm::vec3(1, 0, 0);
	ALfloat orientation[] = {at.x, at.y, at.z , up.x, up.y, up.z};
	alListenerfv(AL_ORIENTATION, orientation);
}

void AEAudioContextSetVelocity(AEAudioContext* self, float x, float y, float z)
{
	ALfloat v3[3] = {x, y, z};
	alListenerfv(AL_VELOCITY, v3);
}

AEAudioHandle AEAudioContextBufferLoad(AEAudioContext* self, const char* filename)
{
	AEAudioHandle buffer=0;
	alGenBuffers(1, &buffer);
	
	stb_vorbis *stream = stb_vorbis_open_filename((char*)filename, NULL, NULL);
	if(not stream) return 0;

	stb_vorbis_info info = stb_vorbis_get_info(stream);
	ALenum format;
	if(info.channels == 2) format = AL_FORMAT_STEREO16;
	else format = AL_FORMAT_MONO16;

	size_t sampleCount = stb_vorbis_stream_length_in_samples(stream) * info.channels;
	void* data = malloc(sizeof(ALushort)*sampleCount);

//extern int stb_vorbis_get_samples_short_interleaved(stb_vorbis *f, int channels, short *buffer, int num_shorts);
	stb_vorbis_get_samples_short_interleaved(stream, info.channels, (short*)data, (int)sampleCount);
	stb_vorbis_close(stream);

	alBufferData(buffer, format, data, sampleCount * sizeof(ALushort), info.sample_rate);
	free(data);
	return buffer;
}

void AEAudioContextBufferDelete(AEAudioContext* self, AEAudioHandle buffer)
{
	alDeleteBuffers(1, &buffer);
}

AEAudioHandle AEAudioContextStreamLoad(AEAudioContext* self, const char* filename)
{
	AEAudioHandle streamID = 0;
	AEAudioStream stream;
	AEAudioStreamInit(& stream);
	if(not AEAudioStreamOpen(& stream, filename))
	{
		AEAudioStreamDeinit(& stream); 
		return 0;
	}
	
	for (size_t i = 0; i < self->streams.size(); i++)
		if(streamID < self->streams[i].ID)
			streamID = self->streams[i].ID;
	streamID++;
	if(streamID == 0) streamID++;
	stream.ID=streamID;
	self->streams.push_back(stream);
	return streamID;
}

AEAudioStream* AEAudioContextGetStreamForID(AEAudioContext* self, AEAudioHandle streamID)
{
	size_t length= self->streams.size();
	for (size_t i = 0; i < self->streams.size(); i++)
	{
		//printf("#%i: %i\n", (int)i, (int)AEArrayAsCArray(& self->streams)[i].ID);
		if(streamID == self->streams[i].ID) return & self->streams[i];
	}
	return NULL;
}

void AEAudioContextStreamDelete(AEAudioContext* self, AEAudioHandle streamID)
{
	AEAudioStream* stream=AEAudioContextGetStreamForID(self, streamID);
	AEAudioStreamDeinit(stream);
	//AEArrayRemoveBytes(& self->streams, stream);
}

AEAudioHandle AEAudioContextSourceNew(AEAudioContext* self, AEAudioHandle buffer)
{
	AEAudioHandle source=0;
	alGenSources(1, &source);
	if (alGetError() != AL_NO_ERROR) return 0;
	self->sources.push_back(source);
	
	alSourcei(source, AL_BUFFER, buffer);
	AEAudioContextSourceSetLooping(self, source, false);
	AEAudioContextSourceSetVelocity(self, source, 0, 0, 0);
	AEAudioContextSourceSetPosition(self, source, 0, 0, 0);
	AEAudioContextSourceSetPitch(self, source, 1);
	AEAudioContextSourceSetVolume(self, source, 1);
	AEAudioContextSourceSetPaused(self, source, false);
	return source;
}

void AEAudioContextSourceDelete(AEAudioContext* self, AEAudioHandle source)
{
//	AEArrayRemoveBytes(& self->sources, &source);
	alSourceStop(source);
	alDeleteSources(1, &source);
}

void AEAudioContextUpdateStreams(AEAudioContext* self)
{
	size_t length = self->streams.size();
	for (size_t i=0; i<length; i++)
	{
		AEAudioStream* stream = & self->streams[i];
		if(not AEAudioStreamUpdate(stream)) AEAudioContextStreamDelete(self, stream->ID);
	}
}

void AEAudioContextSourceSetPaused(AEAudioContext* self, AEAudioHandle source, bool to)
{
	if(not to) alSourcePlay(source);
	else alSourcePause(source);
}

void AEAudioContextSourceSetVolume(AEAudioContext* self, AEAudioHandle source, float amount)
{
	alSourcef(source, AL_GAIN, amount);
}

void AEAudioContextSourceSetPitch(AEAudioContext* self, AEAudioHandle source, float amount)
{
	alSourcef(source, AL_PITCH, amount);
}

void AEAudioContextSourceSetPosition(AEAudioContext* self, AEAudioHandle source, float x, float y, float z)
{
	ALfloat sourcePosition[] = {x, y, z};
	alSourcefv(source, AL_POSITION, sourcePosition);
}

void AEAudioContextSourceSetVelocity(AEAudioContext* self, AEAudioHandle source, float x, float y, float z)
{
	ALfloat sourceVelocity[] = {x, y, z};
	alSourcefv(source, AL_VELOCITY, sourceVelocity);
}

void AEAudioContextSourceSetLooping(AEAudioContext* self, AEAudioHandle source, bool loop)
{
	alSourcei(source, AL_LOOPING, loop);
}

bool AEAudioContextSourceGetStopped(AEAudioContext* self, AEAudioHandle source)
{
	ALint value = 0;
	alGetSourcei(source, AL_SOURCE_STATE, & value);
	return (value == AL_STOPPED) && (value == AL_INITIAL);
}



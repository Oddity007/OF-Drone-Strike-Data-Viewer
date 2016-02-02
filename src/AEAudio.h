#ifndef AEAudio_h
#define AEAudio_h


#ifdef __APPLE_CC__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
//Most portable option, just add a header search path
#include "al.h"
#include "alc.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef struct AEAudioStream AEAudioStream;
typedef struct AEAudioContext AEAudioContext;
typedef ALuint AEAudioHandle;

AEAudioContext* AEAudioContextNew(void);
void AEAudioContextDelete(AEAudioContext* self);

void AEAudioContextSetPosition(AEAudioContext* self, float x, float y, float z);
void AEAudioContextSetRotation(AEAudioContext* self, float x, float y, float z, float w);
void AEAudioContextSetVelocity(AEAudioContext* self, float x, float y, float z);

AEAudioHandle AEAudioContextBufferLoad(AEAudioContext* self, const char* filename);
void AEAudioContextBufferDelete(AEAudioContext* self, AEAudioHandle buffer);

AEAudioHandle AEAudioContextStreamLoad(AEAudioContext* self, const char* filename);
AEAudioStream* AEAudioContextGetStreamForID(AEAudioContext* self, AEAudioHandle streamID);
void AEAudioContextStreamDelete(AEAudioContext* self, AEAudioHandle streamID);

AEAudioHandle AEAudioContextSourceNew(AEAudioContext* self, AEAudioHandle buffer);
void AEAudioContextSourceDelete(AEAudioContext* self, AEAudioHandle source);

void AEAudioContextUpdateStreams(AEAudioContext* self);

void AEAudioContextSourceSetPaused(AEAudioContext* self, AEAudioHandle source, bool to);
void AEAudioContextSourceSetVolume(AEAudioContext* self, AEAudioHandle source, float amount);
void AEAudioContextSourceSetPitch(AEAudioContext* self, AEAudioHandle source, float amount);
void AEAudioContextSourceSetPosition(AEAudioContext* self, AEAudioHandle source, float x, float y, float z);
void AEAudioContextSourceSetVelocity(AEAudioContext* self, AEAudioHandle source, float x, float y, float z);
void AEAudioContextSourceSetLooping(AEAudioContext* self, AEAudioHandle source, bool loop);

bool AEAudioContextSourceGetStopped(AEAudioContext* self, AEAudioHandle source);

#ifdef __cplusplus
}
#endif

#endif
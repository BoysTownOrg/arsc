#ifndef ARSC_ARSC_ASIO_ARSC_ASIO_WRAPPERS_H_
#define ARSC_ARSC_ASIO_ARSC_ASIO_WRAPPERS_H_

#include "asio.h"
#include "../arsc_common.h"

#ifdef __cplusplus
#define EXTERN_CPP extern "C"
#else
#define EXTERN_CPP
#endif

extern bool (*SDKAsioSetSampleRate)(ASIOSampleRate aSampleRate);
extern bool (*SDKAsioGetBufferSize)(
	long* alngMinBufferSize,
	long* alngMaxBufferSize,
	long* aslngPreferredBufferSize,
	long* alngGranularity
);
extern bool (*SDKAsioOutputReady)();
extern bool (*SDKAsioCreateBuffers)(
	ASIOBufferInfo* bufferInfos,
	long numChannels,
	long bufferSize,
	ASIOCallbacks* callbacks
);
extern bool (*SDKAsioGetLatencies)(long* inputLatency, long* outputLatency);
extern bool (*SDKAsioStart)(void);

extern ASIOBufferInfo* global_asio_buffer_info;

EXTERN_CPP bool SDKLoadAsioDriver(char* name);
EXTERN_CPP bool SDKAsioInit(ASIODriverInfo* info);
EXTERN_CPP bool SDKAsioExit(void);
EXTERN_CPP bool SDKAsioGetChannels(long* alngInputChannels, long* alngOutputChannels);
EXTERN_CPP bool SDKAsioCanSampleRate(ASIOSampleRate aSampleRate);
EXTERN_CPP bool SDKAsioGetBufferSizeImpl(long* alngMinBufferSize,
	long* alngMaxBufferSize,
	long* aslngPreferredBufferSize,
	long* alngGranularity);
EXTERN_CPP bool SDKAsioGetChannelInfo(ASIOChannelInfo* info);
EXTERN_CPP bool SDKAsioCreateBuffersImpl(ASIOBufferInfo* bufferInfos,
	long numChannels,
	long bufferSize,
	ASIOCallbacks* callbacks);
EXTERN_CPP bool SDKAsioOutputReadyImpl();
EXTERN_CPP bool SDKAsioGetSamplePosition(ASIOSamples* sPos, ASIOTimeStamp* tStamp);
EXTERN_CPP bool SDKAsioGetLatenciesImpl(long* inputLatency, long* outputLatency);
EXTERN_CPP bool SDKAsioDisposeBuffers(void);
EXTERN_CPP bool SDKAsioStop(void);
EXTERN_CPP bool SDKAsioStartImpl(void);
EXTERN_CPP bool SDKAsioSetSampleRateImpl(ASIOSampleRate aSampleRate);

#endif

#ifndef ARSC_ARSC_ASIO_ARSC_ASIO_WRAPPERS_H_
#define ARSC_ARSC_ASIO_ARSC_ASIO_WRAPPERS_H_

#include "asiosys.h"
#include "asio.h"

#ifdef __cplusplus
#define EXTERN_CPP extern "C"
#else
#define EXTERN_CPP
#endif

extern int (*SDKAsioSetSampleRate)(ASIOSampleRate aSampleRate);
extern int (*SDKAsioGetBufferSize)(
	long* alngMinBufferSize,
	long* alngMaxBufferSize,
	long* aslngPreferredBufferSize,
	long* alngGranularity
);
extern int (*SDKAsioOutputReady)();
extern int (*SDKAsioCreateBuffers)(
	ASIOBufferInfo* bufferInfos,
	long numChannels,
	long bufferSize,
	ASIOCallbacks* callbacks
);
extern int (*SDKAsioGetLatencies)(long *inputLatency, long *outputLatency);
extern int (*SDKAsioStart)(void);

extern ASIOBufferInfo* global_asio_buffer_info;

EXTERN_CPP int SDKLoadAsioDriver(char *name);
EXTERN_CPP int SDKAsioInit(ASIODriverInfo *info);
EXTERN_CPP int SDKAsioExit(void);
EXTERN_CPP int SDKAsioGetChannels(
    long *alngInputChannels, long *alngOutputChannels);
EXTERN_CPP int SDKAsioCanSampleRate(ASIOSampleRate aSampleRate);
EXTERN_CPP int SDKAsioGetBufferSizeImpl(long *alngMinBufferSize,
	long* alngMaxBufferSize,
	long* aslngPreferredBufferSize,
	long* alngGranularity);
EXTERN_CPP int SDKAsioGetChannelInfo(ASIOChannelInfo *info);
EXTERN_CPP int SDKAsioCreateBuffersImpl(ASIOBufferInfo *bufferInfos,
	long numChannels,
	long bufferSize,
	ASIOCallbacks* callbacks);
EXTERN_CPP int SDKAsioOutputReadyImpl();
EXTERN_CPP int SDKAsioGetSamplePosition(
    ASIOSamples *sPos, ASIOTimeStamp *tStamp);
EXTERN_CPP int SDKAsioGetLatenciesImpl(
    long *inputLatency, long *outputLatency);
EXTERN_CPP int SDKAsioDisposeBuffers(void);
EXTERN_CPP int SDKAsioStop(void);
EXTERN_CPP int SDKAsioStartImpl(void);
EXTERN_CPP int SDKAsioSetSampleRateImpl(ASIOSampleRate aSampleRate);

#endif

#ifndef ARSC_ARSC_ASIO_ARSC_ASIO_WRAPPERS_H_
#define ARSC_ARSC_ASIO_ARSC_ASIO_WRAPPERS_H_

#include "asio.h"
#include "../arsc_common.h"

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

extern ASIOBufferInfo* bufferInfos;

#endif

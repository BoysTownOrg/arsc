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
extern ASIOBufferInfo* bufferInfos;

#endif

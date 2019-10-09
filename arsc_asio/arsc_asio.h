#ifndef ARSC_ARSC_ASIO_ARSC_ASIO
#define ARSC_ARSC_ASIO_ARSC_ASIO

#include "../arscdev.h"
#include "../arsc_common.h"

/*
Channels are handled differently in ASIO than MME, so made sense to
map the "segments" passed from the calling program in the format
Steve set up to an array of these structures.

32-bit integers is what all of the sound cards tested expect. 
*/
typedef struct {
	int32 Magic;					    // Just an identifier for this structure
	int32* data;				    // Pointer to segment
	int32 size;				    // # of samples, e.g. 2048, in this block
	int32 Index;					    // How many samples have already been played
	int32 channel;				    // 0, 1, 2, . . . 
	int32 segment;				    // Current segment for this channel
	bool OutputDone;				    // Says if output is done for this channel
} ArAsioSegment;

extern int32_t (*ar_asio_devices)();
extern char* (*ar_asio_device_name)(int32_t);
extern void (*ar_asio_io_stop)(int32_t);
extern void (*ar_asio_close)(int32_t);
extern int32_t(*ar_asio_open)(int32_t);
extern int32_t(*ar_asio_io_prepare)(int32_t);
extern void (*ar_asio_io_start)(int32_t);
extern int32_t(*ar_asio_transfer_segment)(int32_t, int32_t);
extern int32_t(*ar_asio_check_segment)(int32_t, int32_t);
extern int32_t(*ar_asio_latency)(int32_t, int32_t);
extern int32_t(*ar_asio_list_rates)(int32_t);
extern int32_t (*pLockAndLoad)(int32_t aintDevice);
extern ArAsioSegment* stimulusData;
extern ARDEV* ar_current_device;

int32_t _ar_asio_open(int32_t);
int32_t _ar_asio_io_prepare(int32_t);
int32 pSendStimulusData(int32* buffer, int32 aintBufferSize, ArAsioSegment* ptrStimulusData);

#endif

#ifndef ARSC_ARSC_ASIO_ARSC_ASIO
#define ARSC_ARSC_ASIO_ARSC_ASIO

#include "../arscdev.h"
#include "../arsc_common.h"

/*
Stimulus structure

Channels are handled differently in ASIO than MME, so made sense to
map the "segments" passed from the calling program in the format
Steve set up to an array of these structures.

A StimulusBlock is 32-bit integers, since that is what all of the sound
cards tested expect.  A pointer to the "next" sample to be sent is kept.
*/
typedef struct {
	int32		Magic;					    // Just an identifier for this structure
	int32* StimulusBlock;				    // Pointer to segment
	int32		Samples;				    // # of samples, e.g. 2048, in this block
	int32		Index;					    // How many samples have already been played
	int32		ChannelNumber;				    // 0, 1, 2, . . . 
	int32		SegmentNumber;				    // Current segment for this channel
	bool		OutputDone;				    // Says if output is done for this channel
} TStimulusData;

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
extern TStimulusData* stimulusData;

int32_t _ar_asio_open(int32_t);
int32_t _ar_asio_io_prepare(int32_t);

#endif

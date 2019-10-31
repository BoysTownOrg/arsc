#ifdef ASIO

#include "arsc_asio.h"
#include "asiosys.h"
#include "asio.h"
#include "arsc_asio_wrappers.h"

enum {
	max_registry_key_length = 255,
	max_drivers = 32, // PortAudio allows a max of 32, so we will too
	max_driver_name_length = 40
};

#define ASIO_PATH "software\\asio"

typedef struct {
	CLSID clsid;
	char name[max_driver_name_length];
	char description[max_driver_name_length];
	bool valid;
	int32_t	devices;
} TAsioDriver;

// ASIO loads and intializes only one valid driver at a time.  To
// make a full list of valid "devices" (in terms of waveopen functionality)
// I created this structure to hold possible devices.
typedef struct {
	char name[ARSC_NAMLEN];
	int32_t	driver; // index back to TAsioDriver struct
	int32_t	good_sampling_rates;
	bool IsOutput;
} TVirtualDevice;

ASIOBufferInfo* global_asio_buffer_info = NULL;  // Pointer to array of bufferInfos; one for each channel (input + output)
ArAsioOutputAudio* global_ar_asio_output_audio;   // pointer to the main stimulus block
ArAsioInputAudio* global_ar_asio_input_audio;   // pointer to the main response block
ARDEV* global_ar_asio_current_device;

// device identifier offset
// for multiple platforms such as when WIND and ASIO are both set in the 
// compilation.  The WIND devices are listed first, for example 0 to 5, then 
// the ASIO devices are listed, e.g. 6.
static int32_t device_identifier_offset = 0;
static TAsioDriver drivers[max_drivers];
static TVirtualDevice VirtualDevice[MAXDEV];  // To hold the device names and the driver to which they associate
static int32_t	initialized_driver = -1;  // Index into the AsioDriverList[] for the loaded ASIO driver
static int32_t	device_count = 0;   // Number of valid ASIO devices
static int32_t	max_input_channels = -1;   // Maximum as reported by the driver
static int32_t	max_output_channels = -1;   // Maximum as reported by the driver
static ASIOChannelInfo* channelInfos = NULL; // Pointer to array of channelInfos; one for each channel (input + output)
static ArAsioOutputAudio* first_output_audio_of_current_segment;   // Points to the current segment stimulus (channel 0)
static ArAsioInputAudio* first_input_audio_of_current_segment;   // Points to the current segment response (channel 0)
static long	preferred_buffer_size;   // Returned by the driver
static long	total_input_and_output_channels = 0;   // Need the total number of used input and output channels
static long	input_latency, output_latency;  // Latencies as polled from the card.
static bool	output_ready_optimization = false;  // flag - true if driver uses ASIOOutputReady optimization
static bool	buffers_have_been_created = false; // flag - true if buffers have been created
static bool	driver_has_started = false;  // flag - true if driver is started
static int32_t total_samples_processed = 0;  // Total samples processed
static long	latency_offset = 0;  // LatencyOffset specified by app
static int32_t segment_has_finished = 0; // semaphore-like variable to tell when segment is finished
static int32_t	good_sample_rates;

bool (*SDKAsioSetSampleRate)(ASIOSampleRate aSampleRate) = SDKAsioSetSampleRateImpl;
bool (*SDKAsioGetBufferSize)(
	long* alngMinBufferSize,
	long* alngMaxBufferSize,
	long* aslngPreferredBufferSize,
	long* alngGranularity
	) = SDKAsioGetBufferSizeImpl;
bool (*SDKAsioOutputReady)() = SDKAsioOutputReadyImpl;
bool (*SDKAsioCreateBuffers)(
	ASIOBufferInfo* bufferInfos,
	long numChannels,
	long bufferSize,
	ASIOCallbacks* callbacks
	) = SDKAsioCreateBuffersImpl;
bool (*SDKAsioGetLatencies)(long* inputLatency, long* outputLatency) = SDKAsioGetLatenciesImpl;
bool (*SDKAsioStart)(void) = SDKAsioStartImpl;

void bufferSwitch(long index, ASIOBool processNow);
ASIOTime* bufferSwitchTimeInfo(ASIOTime* timeInfo, long index, ASIOBool processNow);
void sampleRateChanged(ASIOSampleRate sRate);
long asioMessages(long selector, long value, void* message, double* opt);
static int32_t pPollAsioDrivers(void);
static int32_t pLockAndLoadImpl(int32_t aintDevice);
static int32_t pBuildVirtualDevices(int32_t aintDriver);

static int32_t
ar_asio_devices_impl()
{
	// Has the number of devices already been polled?
	if (device_count > 0) {
		
		return device_count;
	}
	else {
		// Need to load the ASIO driver and get some details.
		if (!pPollAsioDrivers())
			return 0;
	}
	return device_count;
}

int32_t(*ar_asio_devices)() = ar_asio_devices_impl;

/*
_ar_asio_dev_name - return name of I/O device

The original assumption was to just return the driver name.
This doesn't work for Echo ASIO WDM, which is the same driver
name for a number of Echo cards.
*/

static char*
_ar_asio_dev_name(int32_t di)
{
	// In case this is called before we are ready
	if (device_count == 0) {
		
		if (!pPollAsioDrivers())
			return NULL;
	}

	for (int32_t i = 0; i < MAXDEV; i++) {
		if (i == (di - device_identifier_offset)) {
			
			return VirtualDevice[i].name;
		}
	}

	

	return NULL;
}

char* (*ar_asio_device_name)(int32_t) = _ar_asio_dev_name;

/*
_ar_asio_list_rates - return good sampling rates
*/

static int32_t
_ar_asio_list_rates(int32_t di)
{
	// In case this is called before we are ready
	if (device_count == 0) {
		
		if (!pPollAsioDrivers())
			return 0;
	}

	for (int32_t i = 0; i < MAXDEV; i++) {
		if (i == (di - device_identifier_offset)) {
			
			return VirtualDevice[i].good_sampling_rates;
		}
	}

	

	return 0;
}

int32_t(*ar_asio_list_rates)(int32_t) = _ar_asio_list_rates;

static void
_ar_asio_io_stop(int32_t di)
{
	global_ar_asio_current_device = _ardev[di];

	if (driver_has_started) {
		
		SDKAsioStop();
		driver_has_started = false;
	}
}

void (*ar_asio_io_stop)(int32_t) = _ar_asio_io_stop;

static void
_ar_asio_close(int32_t di) {

	

	// Stop the driver if it is running
	if (driver_has_started) {
		_ar_asio_io_stop(di);
		driver_has_started = false;
		Sleep(1);	// 1-ms delay delay before freeing buffers
	}

	

	SDKAsioDisposeBuffers();
	buffers_have_been_created = false;

	// Clear the channels
	if (channelInfos != NULL) {
		free(channelInfos);
		channelInfos = NULL;
		
	}

	// Clear the buffers
	if (global_asio_buffer_info != NULL) {
		free(global_asio_buffer_info);
		global_asio_buffer_info = NULL;
		
	}

	// Clear stim blocks
	if (global_ar_asio_output_audio != NULL) {
		free(global_ar_asio_output_audio);
		global_ar_asio_output_audio = NULL;
	}

	// Clear resp blocks
	if (global_ar_asio_output_audio != NULL) {
		free(global_ar_asio_input_audio);
		global_ar_asio_input_audio = NULL;
	}

	// Unload the ASIO driver
	if (initialized_driver != -1) {
		
		SDKAsioExit();
	}

	initialized_driver = -1;
}

void (*ar_asio_close)(int32_t) = _ar_asio_close;

int32_t _ar_asio_open(int32_t di)
{
	bool		bolOutput;
	int32_t		intChannelOffset = 0;

	global_ar_asio_current_device = _ardev[di];
	// Loon test
	if (intChannelOffset + global_ar_asio_current_device->ncda > max_output_channels) {
		
		goto err;
	}
	if (intChannelOffset + global_ar_asio_current_device->ncad > max_input_channels) {
		
		goto err;
	}

	bolOutput = !(_arsc_find & ARSC_PREF_IN);

	// Load the driver and initialize
	if (!pLockAndLoad(di))
		goto err;

	// This is always true for ASIO.  This lets the API code use the 
	// segments as is because of the ef (effective) flag.
	global_ar_asio_current_device->nbps = 4;
	global_ar_asio_current_device->ntlv = 0;

	/*
	Ensure a good sample rate, then set it.
	gdsr is a bitwise combintation of our list of 27 sample rates.
	Once known, we don't have to look it up again.
	*/
	if (!global_ar_asio_current_device->gdsr)
		global_ar_asio_current_device->gdsr = ar_asio_list_rates(di);
	global_ar_asio_current_device->rate = _ar_adjust_rate(di, global_ar_asio_current_device->a_rate);
	if (!SDKAsioSetSampleRate((double)global_ar_asio_current_device->rate))
		goto err;

	// check whether the driver requires the ASIOOutputReady() optimization
	// (can be used by the driver to reduce output latency by one block)
	output_ready_optimization = SDKAsioOutputReady();
	

	/*
	Get the buffer size information from the driver.  This
	value is set in the ASIO control panel.  So far I haven't
	found a need to deviate from the Preferred . . . .
	*/
	long lngMaxBufferSize;
	long lngMinBufferSize;
	long lngGranularity;
	if (!SDKAsioGetBufferSize(&lngMinBufferSize,
		&lngMaxBufferSize,
		&preferred_buffer_size,
		&lngGranularity))
		goto err;
	else {
		
	}

	/*
	Allocate bufferInfos
	*/
	total_input_and_output_channels = global_ar_asio_current_device->a_ncad + global_ar_asio_current_device->a_ncda;
	if ((global_asio_buffer_info = (ASIOBufferInfo*)calloc(total_input_and_output_channels, sizeof(ASIOBufferInfo))) == NULL)
		goto err;

	// Figure out the channel offset in case there are multiple ASIO cards.
	// Count all "devices" up to the card in question.
	for (int32_t i = 0; i < initialized_driver; i++)
		if (drivers[i].valid)
			intChannelOffset += drivers[i].devices;
	intChannelOffset = (di - device_identifier_offset) - intChannelOffset;
	

	ASIOBufferInfo* bufferInfo_ = global_asio_buffer_info;
	for (int32_t i = 0; i < global_ar_asio_current_device->a_ncda; i++) {
		bufferInfo_->isInput = ASIOFalse;
		bufferInfo_->channelNum = i;
		bufferInfo_->buffers[0] = NULL;
		bufferInfo_->buffers[1] = NULL;
		bufferInfo_++;
	}

	for (int32_t i = 0; i < global_ar_asio_current_device->a_ncad; i++) {		// loop over output channels
		bufferInfo_->isInput = ASIOTrue;	// create an input buffer
		bufferInfo_->channelNum = i;		// (di - dio) handles channel offsets
		bufferInfo_->buffers[0] = NULL;	// clear buffer 1/2 channels
		bufferInfo_->buffers[1] = NULL;	// clear buffer 1/2 channels
		bufferInfo_++;
	}

	ASIOCallbacks asioCallbacks;
	// set up the asioCallback structure and create the ASIO data buffer
	asioCallbacks.bufferSwitch = &bufferSwitch;
	asioCallbacks.sampleRateDidChange = &sampleRateChanged;
	asioCallbacks.asioMessage = &asioMessages;
	asioCallbacks.bufferSwitchTimeInfo = &bufferSwitchTimeInfo;

	/*
	Create the ASIO buffers, both input and output.  Also set up the callbacks.
	*/
	buffers_have_been_created = SDKAsioCreateBuffers(
		global_asio_buffer_info,
		total_input_and_output_channels,
		preferred_buffer_size,
		&asioCallbacks
	);
	if (!buffers_have_been_created) {
		
		goto err;
	}

	// get the input and output latencies
	// Latencies often are only valid after ASIOCreateBuffers()
	// (input latency is the age of the first sample in the currently returned audio block)
	// (output latency is the time the first sample in the currently returned audio block requires to get to the output)
	if (!SDKAsioGetLatencies(&input_latency, &output_latency))
		goto err;
	

	// Clear the buffers because CardDeluxe has known issues
	bufferInfo_ = global_asio_buffer_info;
	for (int32_t i = 0; i < total_input_and_output_channels; i++) {
		memset(bufferInfo_->buffers[0], 0, preferred_buffer_size * sizeof(int32_t));
		memset(bufferInfo_->buffers[1], 0, preferred_buffer_size * sizeof(int32_t));
		bufferInfo_++;
	}

	total_samples_processed = 0;	// Total samples processed
	latency_offset = 0;	// LatencyOffset specified by app
	segment_has_finished = 0;	// semaphore-like variable to tell when segment is finished

	return (0);
err:
	return  120;		// ASIO open error
}

int32_t(*ar_asio_open)(int32_t) = _ar_asio_open;

int32_t
_ar_asio_io_prepare(int32_t di)
{
	
	global_ar_asio_current_device = _ardev[di];
	/*
	Set up stimulus (OUTPUT) blocks
	This is just a contiguous array of ArAsioChannelBuffers.  The bufferSwitch
	only loops over channels, so the contiguous array should (for stereo) look
	like this:
		SEGMENT		CHANNEL
		0		    0
		0		    1
		1		    0
		1		    1
		2		    0
			. . .
	The pointer "first_output_audio_of_current_segment" will point to the first (channel 0)
	global_output_audio for the current segment.
	*/
	size_t segments = global_ar_asio_current_device->segswp;
	if ((global_ar_asio_output_audio = calloc(global_ar_asio_current_device->ncda * segments, sizeof(ArAsioOutputAudio))) == NULL)
		return -1;

	if ((global_ar_asio_input_audio = calloc(global_ar_asio_current_device->ncad * segments, sizeof(ArAsioInputAudio))) == NULL)
		return -1;

	ArAsioOutputAudio* output_audio = global_ar_asio_output_audio;
	first_output_audio_of_current_segment = global_ar_asio_output_audio;
	for (int32_t i = 0; i < global_ar_asio_current_device->ncda * segments; i++) {
		output_audio->channel = i % global_ar_asio_current_device->ncda;
		output_audio->segment = i / global_ar_asio_current_device->ncda;
		output_audio->data = global_ar_asio_current_device->o_data[i];
		output_audio->Index = 0;
		output_audio->size = global_ar_asio_current_device->sizptr[i / global_ar_asio_current_device->ncda];
		output_audio++;
	}

	ArAsioInputAudio* ptrResponseData = global_ar_asio_input_audio;
	first_input_audio_of_current_segment = global_ar_asio_input_audio;
	for (int32_t i = 0; i < global_ar_asio_current_device->ncad * segments; i++) {
		ptrResponseData->channel = i % global_ar_asio_current_device->ncad;
		ptrResponseData->segment = i / global_ar_asio_current_device->ncad;
		ptrResponseData->data = global_ar_asio_current_device->i_data[i];
		ptrResponseData->Index = 0;
		ptrResponseData->size = global_ar_asio_current_device->sizptr[i / global_ar_asio_current_device->ncad];
		// Only segment 0 need be concerned with latency
		ptrResponseData->LatencyReached = (ptrResponseData->segment != 0);
		ptrResponseData++;
	}

	return (0);
}

int32_t(*ar_asio_io_prepare)(int32_t) = _ar_asio_io_prepare;

/* _ar_asio_xfer_seg - this segment is ready to go */

static int32_t
_ar_asio_xfer_seg(int32_t di, int32_t b)
{
	return 0;
}

int32_t(*ar_asio_transfer_segment)(int32_t, int32_t) = _ar_asio_xfer_seg;

int32_t _ar_asio_chk_seg(int32_t di, int32_t b)
{
	/*
	This function will tell the current segment that is running.
	Should be for both input and output.

	Polling sort of worked.  It was interesting in that printf() statements
	on the ASIO side and the calling program were combined onscreen.  Also,
	it ends one segment early.

	It's a threading synchronization thing.  My thought was to use
	Windows Messages to handle this, but then I am necessarily creating
	a windows application even if the Window is invisible.  PortAudio
	didn't require this, and it also means that the examples written
	for MME/WDM will need modification to use ASIO.

	The problem, then, is that the polling function ar_io_cur_seg() needs
	to go through the chk_seg() function in the API.  This is not only
	extra processing, but it doesn't really yield the result, i.e. the
	current segment that is needed.
	*/

	global_ar_asio_current_device = _ardev[di];

	if (!driver_has_started) {
		return -1;
	}

	switch (segment_has_finished) {
	case 0:
		break;
	case 1:
		// The segment has just finished.  If this routine is called frequently
		// enough, we should get this within moments of it happening.
		segment_has_finished--;

		
		return true;
		break;
	default:
		// Segment overrun
		// This can happen when clicking to another window while the app is running.
		
		global_ar_asio_current_device->xrun++;
		return true;
	}

	return false;			// false means the API xfer function isn't run
}

int32_t(*ar_asio_check_segment)(int32_t, int32_t) = _ar_asio_chk_seg;

void
_ar_asio_io_start(int32_t di)
{
	total_samples_processed = 0;
	
	driver_has_started = SDKAsioStart();
}

void (*ar_asio_io_start)(int32_t) = _ar_asio_io_start;

/* _ar_asio_latency - set and get latency */

static int32_t
_ar_asio_latency(int32_t di, int32_t nsmp)
{
	long max_latency;
	global_ar_asio_current_device = _ardev[di];			// get access to application parameters
	if (nsmp != ARSC_GET_LATENCY) {
		max_latency = input_latency + output_latency; // sum of driver input/output latencies
		max_latency -= max_latency % 256;		    // subtract impulse latency, if any
		if (nsmp > max_latency)
			nsmp = max_latency;
		latency_offset = nsmp;
	}
	return latency_offset;
}

int32_t(*ar_asio_latency)(int32_t, int32_t) = _ar_asio_latency;

static ARDVT* device_type(int32_t n) {
	return &_ardvt[n];
}

int32_t
_ar_asio_bind(int32_t ndt, int32_t tnd)
{
	
	// Get the number of ASIO devices.  This is not the same as the 
	// number of ASIO Drivers in the registry.  This will either be
	// 1 or 0.
	int32_t devices = ar_asio_devices();

	if (devices > 0) {
		device_type(ndt)->num_dev = ar_asio_devices;
		device_type(ndt)->dev_name = ar_asio_device_name;
		device_type(ndt)->io_stop = ar_asio_io_stop;
		device_type(ndt)->close = ar_asio_close;
		device_type(ndt)->open = ar_asio_open;
		device_type(ndt)->io_prepare = ar_asio_io_prepare;
		device_type(ndt)->io_start = ar_asio_io_start;
		device_type(ndt)->xfer_seg = ar_asio_transfer_segment;
		device_type(ndt)->chk_seg = ar_asio_check_segment;
		device_type(ndt)->latency = ar_asio_latency;
		device_type(ndt)->list_rates = ar_asio_list_rates;

		device_identifier_offset = tnd;
	}

	return devices;
}

static int is_last_output_segment(ArAsioOutputAudio* audio) {
	return audio->segment + 1 == global_ar_asio_current_device->segswp;
}

static int is_last_output_channel(ArAsioOutputAudio* audio) {
	int32_t output_channels = global_ar_asio_current_device->a_ncda;
	int32_t last_channel = output_channels - 1;
	return audio->channel == last_channel;
}

static int is_last_input_channel(ArAsioInputAudio* audio) {
	int32_t input_channels = global_ar_asio_current_device->a_ncad;
	int32_t last_channel = input_channels - 1;
	return audio->channel == last_channel;
}

static int output_is_exhausted(ArAsioOutputAudio* audio) {
	return audio->Index == audio->size;
}

static int input_is_exhausted(ArAsioInputAudio* audio) {
	return audio->Index == audio->size;
}

static int is_last_input_segment(ArAsioInputAudio* audio) {
	return audio->segment + 1 == global_ar_asio_current_device->segswp;
}

static int32_t minimum(int32_t a, int32_t b) {
	return a < b ? a : b;
}

static void copy(int32_t* destination, int32_t* source, int32_t count) {
	memcpy(destination, source, count * sizeof(int32_t));
}

static void update_if_last_output_channel(ArAsioOutputAudio* audio) {
	if (is_last_output_channel(audio)) {
		// If there are no input channels, the out channel determines the segment end
		if (!global_ar_asio_current_device->a_ncad)
			segment_has_finished++;
		first_output_audio_of_current_segment = is_last_output_segment(audio)
			? global_ar_asio_output_audio
			: audio + 1;
	}
}

int32_t ar_asio_write_device_buffer(int32_t* destination, int32_t size, ArAsioOutputAudio* audio) {
	int32_t copied = 0;
	while (1) {
		int32_t* source = audio->data + audio->Index;
		int32_t to_copy = minimum(size - copied, audio->size - audio->Index);
		copy(destination + copied, source, to_copy);
		copied += to_copy;
		audio->Index += to_copy;
		if (output_is_exhausted(audio)) {
			update_if_last_output_channel(audio);
			int32_t output_channels = global_ar_asio_current_device->a_ncda;
			audio += is_last_output_segment(audio)
				? -output_channels * (global_ar_asio_current_device->segswp - 1)
				: output_channels;
			audio->Index = 0;
		}
		else
			return 1;
	}
}

/*
Any samples due to device latency are skipped.
*/
int32_t ar_asio_read_device_buffer(int32_t* source, int32_t size, ArAsioInputAudio* audio) {
	int32_t copied = 0;
	/*
	Check to see that the latency between ouput and input has been reached.
	Each channel has the same latency (one would hope), and we only check
	the latency once for each channel of segment 0.  This was accomplished
	by filling the LatencyReached flag to TRUE for segments 1..N
	*/
	if (!audio->LatencyReached) {
		// Latency not yet reached
		//
		// An E-mail to the ASIO listserv suggested the calculation for the
		// offset to be Input + Output latency.  These values are usually quite
		// close.
		//intLoopbackLatency = slngInputLatency + slngOutputLatency + slngLatencyOffset;    // Tone's version
		// Changed default LoopbackLatecy to match WDM latency [STN:Jun-2007]
		int32_t intLoopbackLatency = input_latency + output_latency;	    // sum of driver input/output latencies
		int32_t intImpulseLatency = intLoopbackLatency % 256;			    // extract impulse latency, if any
		intLoopbackLatency -= latency_offset + intImpulseLatency;    // subtract app & impulse latencies
		// With the passed buffer, are we past the latency?
		if (audio->SkippedSamples + size >= intLoopbackLatency) {
			// Partial buffer needs to be sent
			int32_t intDifference = intLoopbackLatency - audio->SkippedSamples;
			if (intDifference < 0)
				intDifference = 0;
			int32_t* source_ = source;
			source_ += intDifference;
			audio->LatencyReached = true;

			// Check for abnormally large buffers
			int32_t intRemainder = size - intDifference;
			if (intRemainder < audio->size) {
				int32_t* destination = audio->data + audio->Index;
				copy(destination, source_, intRemainder);
				audio->Index += intRemainder;
			}
			else
				if (!ar_asio_read_device_buffer(source_, intRemainder, audio))
					return 0L;
		}
		else
			// Entire buffer is worthless.  Still need to track # of skipped samples until LatencyReached.
			audio->SkippedSamples += size;
	}
	while (1) {
		int32_t* destination = audio->data + audio->Index;
		int32_t to_copy = minimum(size - copied, audio->size - audio->Index);
		copy(destination, source + copied, to_copy);
		copied += to_copy;
		audio->Index += to_copy;
		if (input_is_exhausted(audio)) {
			if (is_last_input_channel(audio)) {
				segment_has_finished++;
				first_input_audio_of_current_segment = is_last_input_segment(audio)
					? global_ar_asio_input_audio
					: audio + 1;
			}
			int32_t input_channels = global_ar_asio_current_device->a_ncad;
			audio += is_last_input_segment(audio)
				? -input_channels * (global_ar_asio_current_device->segswp - 1)
				: input_channels;
			audio->Index = 0;
		}
		else
			return 1;
	}
}

static long
check_rates(void) {

	int i;

	/*
	Get a list of sample rates the card can handle.  The list will be
	used later to ensure the closest valid sample rate to the requested
	rate is used.
	*/
	good_sample_rates = 0;
	for (i = 0; i < SRLSTSZ; i++) {
		if (SDKAsioCanSampleRate(_ar_SRlist[i]) == true) {
			good_sample_rates |= 1 << i;
			
		}
	}
	
	return (good_sample_rates);
}

// The sound card name is contained within the channel information.  
// This is opposed to the old method of just using the driver name.
// The SYNC routine has "devices" which more closely resemble these
// ASIO channels.
//
// The channel information is more useful than the driver name, even
// though only one driver can be used at a time, because all Echo cards
// have the same driver name of "Echo ASIO WDM."  This is a way to
// differentiate between cards.
//
// The SYNC code only gets the OUT devices, unless only input is given.
int32_t pGetChannelDetails(int32_t aintDriver) {

	int32_t		    i;
	ASIOChannelInfo* ptrChannelInfo;
	bool		    bolOutput;


	// Are we interested in input or output channels?  Usually output.
	bolOutput = !(_arsc_find & ARSC_PREF_IN);

	// Hold onto the number of "devices" for this driver.  This is used 
	// later in calculating the channel offset from the device number.
	if (bolOutput)
		drivers[aintDriver].devices = max_output_channels;
	else
		drivers[aintDriver].devices = max_input_channels;;

	// Channel details
	ptrChannelInfo = channelInfos;
	for (i = 0; i < max_output_channels; i++) {
		ptrChannelInfo->isInput = ASIOFalse;
		ptrChannelInfo->channel = i;

		// Gets channel information from the driver
		if (!SDKAsioGetChannelInfo(ptrChannelInfo))
			return -1;
		

		if (bolOutput) {
			// Set the VirtualDevice information
			VirtualDevice[device_count].driver = aintDriver;
			sprintf(VirtualDevice[device_count].name, "%s ASIO", ptrChannelInfo->name);
			VirtualDevice[device_count].IsOutput = true;
			VirtualDevice[device_count].good_sampling_rates = check_rates();
			device_count++;
		}

		ptrChannelInfo++;
	}

	for (i = 0; i < max_input_channels; i++) {
		ptrChannelInfo->isInput = ASIOTrue;
		ptrChannelInfo->channel = i;

		// Gets channel information from the driver
		if (!SDKAsioGetChannelInfo(ptrChannelInfo))
			return -1;
		

		if (!bolOutput) {
			// Set the VirtualDevice information
			VirtualDevice[device_count].driver = aintDriver;
			sprintf(VirtualDevice[device_count].name, "%s (%s) ASIO %s", ptrChannelInfo->name, "In", ptrChannelInfo->isActive == ASIOTrue ? "*" : "");
			VirtualDevice[device_count].IsOutput = false;
			VirtualDevice[device_count].good_sampling_rates = check_rates();
			device_count++;
		}

		ptrChannelInfo++;
	}

	return 1;
}

// This loads the ASIO driver and acquires some details.
int32_t pPollAsioDrivers(void) {

	long		lRet;
	HKEY		hkEnum = 0;
	HKEY		hkDriver = 0;
	char		keyname[max_registry_key_length];
	DWORD		index = 0;
	DWORD		type, size;
	char		strFullKeyPath[max_registry_key_length];
	char		value[max_registry_key_length];		// Arbitrary value type
	int32_t		intDriver;			// looping variable
	ASIODriverInfo	asioDriverInfo;			// needed for ASIOInit()

	// Open the main key to the ASIO drivers
	// http://msdn.microsoft.com/library/en-us/sysinfo/base/regopenkey.asp
	lRet = RegOpenKey(HKEY_LOCAL_MACHINE, ASIO_PATH, &hkEnum);
	if (lRet != ERROR_SUCCESS) {
		
		return 0;
	}

	while (lRet == ERROR_SUCCESS) {
		// http://msdn.microsoft.com/library/en-us/sysinfo/base/regenumkey.asp
		if ((lRet = RegEnumKey(hkEnum, index, (LPTSTR)keyname, max_registry_key_length)) == ERROR_SUCCESS) {
			// Print out the ASIO card name
			

			// Get the subkey that this keyname represents
			sprintf(strFullKeyPath, "%s\\%s", ASIO_PATH, keyname);
			lRet = RegOpenKey(HKEY_LOCAL_MACHINE, strFullKeyPath, &hkDriver);
			if (lRet != ERROR_SUCCESS) {
				
				return 0;
			}

			// name
			// The name is the key in the registry.
			strcpy(drivers[index].name, keyname);

			// CLSID
			size = max_registry_key_length;
			lRet = RegQueryValueEx(hkDriver, "CLSID", 0, &type, (LPBYTE)value, &size);
			if (lRet == ERROR_SUCCESS) {
				char* ptrClsid = (char*)value;	// Shorthand
				WORD	wData[100];

				// Convert to CLSID
				if (strlen(ptrClsid) == 38) {		// Must be 38 long, including { }
					MultiByteToWideChar(CP_ACP, 0, (LPCSTR)ptrClsid, -1, (LPWSTR)wData, 100);

					if (CLSIDFromString((LPOLESTR)wData, (LPCLSID) & (drivers[index].clsid)) != S_OK) {
						
					}
				}
			}
			else {
				// All ASIO drivers should have a CLSID
				
				return 0;
			}

			// Description
			// The description is not necessarily the same as the name of the driver.
			size = max_registry_key_length;
			if (RegQueryValueEx(hkDriver, "Description", 0, &type, (LPBYTE)value, &size) == ERROR_SUCCESS) {
				strcpy(drivers[index].description, value);
			}
			else {
				// Some ASIO drivers don't have a description
				strcpy(drivers[index].description, keyname);
			}

			index++;
		} // fi RegEnumKey
	} // elihw

	if (hkEnum)
		RegCloseKey(hkEnum);

	

	/*
	The number of available drivers is not the same as the available ASIO devices.
	Need to test each one out to see if it opens.
	*/
	device_count = 0;
	for (intDriver = 0; intDriver < max_drivers; intDriver++) {
		if (strlen(drivers[intDriver].name) > 0) {

			// In case multiple ASIO sound cards exist, terminate the AudioStreamIO.
			// This may not be necessary.
			if (initialized_driver > 0)
				SDKAsioExit();

			if (SDKLoadAsioDriver(drivers[intDriver].name)) {
				

				// But loading isn't enough.  The Echo Gina will load even if it isn't installed.
				// Initialize the AudioStreamIO.
				asioDriverInfo.driverVersion = 2;			// ASIO 2.0
				asioDriverInfo.sysRef = GetDesktopWindow();		// Application main window handle
				if (SDKAsioInit(&asioDriverInfo) == true) {
					// This ASIO driver can be used.  Although we can use only one
					// ASIO driver at a time, there may be multiple sound cards in the same
					// box, so we aren't done yet.
					drivers[intDriver].valid = true;
					

					// For now, this sets the last ASIO driver as the one used.
					initialized_driver = intDriver;

					// Get the device information
					if (!pBuildVirtualDevices(intDriver))
						return 0;

				}
				else {
					
				}
			}
			else {
				
			} // fi SDKLoadAsioDriver
		}
		else {
			// Get out when there are no more named drivers
			break;
		} // fi .name
	} // rof intDriver

	// Terminate AudioStreamIO.  The open function will reload the correct driver
	// based upon the application's device choice.
	if (initialized_driver > 0) {
		SDKAsioExit();
		initialized_driver = -1;
		
	}

	return 1;
}

/*
Run only after a driver is loaded -and- initialized.  This function gets
channel information to derive virtual device ids (for the SYNC analog).

The total possible channels is used to get full channel information from the card.
*/
int32_t pBuildVirtualDevices(int32_t aintDriver) {

	// Gets the number of channels for this card
	if (!SDKAsioGetChannels(&max_input_channels, &max_output_channels)) {
		
		return 0;
	}
	long slngTotalPossibleChannels = max_input_channels + max_output_channels;


	// Allocate enough memory for every channel info, but only until we get the device information
	if ((channelInfos = (ASIOChannelInfo*)calloc(slngTotalPossibleChannels, sizeof(ASIOChannelInfo))) == NULL)
		return 0;

	// Get the channel details.  This should be done after the buffers are created
	// otherwise it is not known which channels are active.
	if (!pGetChannelDetails(aintDriver))
		return 0;

	// Get rid of the channelInfos allocated in this function
	if (channelInfos != NULL) {
		free(channelInfos);
		channelInfos = NULL;
		
	}

	return 1;
}

/*
The driver is now known from the application's request.  Load
the appropriate driver and initialize.
*/
int32_t pLockAndLoadImpl(int32_t aintDevice) {
	int32_t		intDriver = VirtualDevice[aintDevice - device_identifier_offset].driver;
	ASIODriverInfo	asioDriverInfo;			// needed for ASIOInit()
	ASIOChannelInfo* ptrChannelInfo;		// handy pointer
	int32_t		i;


	

	if (SDKLoadAsioDriver(drivers[intDriver].name)) {
		

		asioDriverInfo.driverVersion = 2;				// ASIO 2.0
		asioDriverInfo.sysRef = GetDesktopWindow();			// Application main window handle
		if (SDKAsioInit(&asioDriverInfo) == true) {
			

			// Hold for later
			initialized_driver = intDriver;
		}
		else {
			
			return 0;
		}
	}
	else {
		
		return 0;
	} // fi SDKLoadAsioDriver

	// Gets the number of channels for this card
	if (!SDKAsioGetChannels(&max_input_channels, &max_output_channels)) {
		
	}
	else {
		
	}
	long slngTotalPossibleChannels = max_input_channels + max_output_channels;

	// Allocate enough memory for every channel info
	if ((channelInfos = (ASIOChannelInfo*)calloc(slngTotalPossibleChannels, sizeof(ASIOChannelInfo))) == NULL)
		return 0;

	// Initialize channels
	ptrChannelInfo = channelInfos;
	for (i = 0; i < max_output_channels; i++) {
		ptrChannelInfo->isInput = ASIOFalse;
		ptrChannelInfo->channel = i;

		// Gets channel information from the driver
		if (!SDKAsioGetChannelInfo(ptrChannelInfo))
			return -1;

		ptrChannelInfo++;
	}
	for (i = 0; i < max_input_channels; i++) {
		ptrChannelInfo->isInput = ASIOTrue;
		ptrChannelInfo->channel = i;

		// Gets channel information from the driver
		if (!SDKAsioGetChannelInfo(ptrChannelInfo))
			return -1;

		ptrChannelInfo++;
	}

	return 1;
}

int32_t(*pLockAndLoad)(int32_t aintDevice) = pLockAndLoadImpl;

/*
SDK Note:
	Beware that this is normally in a seperate thread, hence be sure that you
	take care about thread synchronization. This is omitted here for simplicity.
*/
ASIOTime* bufferSwitchTimeInfo(ASIOTime* timeInfo, long index, ASIOBool processNow) {
	long lngAsioBufferSize = preferred_buffer_size;    // shorthand to buffer size in samples
	ArAsioOutputAudio* output_audio = first_output_audio_of_current_segment;	    // pointer to channel 0 of current segment stimulus
	ArAsioInputAudio* input_audio = first_input_audio_of_current_segment;	    // pointer to channel 0 of current segment response

	if (!driver_has_started)
		return 0L;

	for (int32_t i = 0; i < total_input_and_output_channels; i++) {
		if (global_asio_buffer_info[i].isInput == false) {
			switch (channelInfos[i].type) {
			case ASIOSTInt32LSB:
				/*
				Tone - all of the cards tested here are Int32LSB, including:
				CardDeluxe, Gina24, Layla24, and M-Audio Delta Audiophile 2496
				*/
				if (!ar_asio_write_device_buffer(global_asio_buffer_info[i].buffers[index], lngAsioBufferSize, output_audio))
					break;
				break;
			default:
				break;
			}
			output_audio++;
		}
		else {
			switch (channelInfos[i].type) {
			case ASIOSTInt32LSB:
				if (!ar_asio_read_device_buffer(global_asio_buffer_info[i].buffers[index], lngAsioBufferSize, input_audio))
					break;
				break;
			default:
				break;
			}
			input_audio++;
		}
	}

	// finally if the driver supports the ASIOOutputReady() optimization, do it here, all data are in place
	if (output_ready_optimization)
		SDKAsioOutputReadyImpl();

	// The total samples is really the count of buffer dimensions.
	// Because it is outside the channels loop, it is independent of
	// channel.  It is as though one channel was tallied.
	total_samples_processed += lngAsioBufferSize;

	return 0L;
}

void bufferSwitch(long index, ASIOBool processNow) {
	// as this is a "back door" into the bufferSwitchTimeInfo a timeInfo needs to be created
	// though it will only set the timeInfo.samplePosition and timeInfo.systemTime fields and the according flags
	ASIOTime  timeInfo;
	memset(&timeInfo, 0, sizeof(timeInfo));
	if (SDKAsioGetSamplePosition(&timeInfo.timeInfo.samplePosition, &timeInfo.timeInfo.systemTime))
		timeInfo.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;

	bufferSwitchTimeInfo(&timeInfo, index, processNow);
}


void sampleRateChanged(ASIOSampleRate sRate)
{
}

long asioMessages(long selector, long value, void* message, double* opt) {
	long ret = 0;
	switch (selector)
	{
	case kAsioSelectorSupported:
		if (value == kAsioResetRequest
			|| value == kAsioEngineVersion
			|| value == kAsioResyncRequest
			|| value == kAsioLatenciesChanged
			// the following three were added for ASIO 2.0, you don't necessarily have to support them
			|| value == kAsioSupportsTimeInfo
			|| value == kAsioSupportsTimeCode
			|| value == kAsioSupportsInputMonitor)
			ret = 1L;
		break;
	case kAsioResetRequest:
		// defer the task and perform the reset of the driver during the next "safe" situation
		// You cannot reset the driver right now, as this code is called from the driver.
		// Reset the driver is done by completely destruct is. I.e. ASIOStop(), ASIODisposeBuffers(), Destruction
		// Afterwards you initialize the driver again.
		//asioDriverInfo.stopped;  // In this sample the processing will just stop
		ret = 1L;
		break;
	case kAsioResyncRequest:
		// This informs the application, that the driver encountered some non fatal data loss.
		// It is used for synchronization purposes of different media.
		// Added mainly to work around the Win16Mutex problems in Windows 95/98 with the
		// Windows Multimedia system, which could loose data because the Mutex was hold too long
		// by another thread.
		// However a driver can issue it in other situations, too.
		ret = 1L;
		break;
	case kAsioLatenciesChanged:
		// This will inform the host application that the drivers were latencies changed.
		// Beware, it this does not mean that the buffer sizes have changed!
		// You might need to update internal delay data.
		ret = 1L;
		break;
	case kAsioEngineVersion:
		// return the supported ASIO version of the host application
		// If a host applications does not implement this selector, ASIO 1.0 is assumed
		// by the driver
		ret = 2L;
		break;
	case kAsioSupportsTimeInfo:
		// informs the driver wether the asioCallbacks.bufferSwitchTimeInfo() callback
		// is supported.
		// For compatibility with ASIO 1.0 drivers the host application should always support
		// the "old" bufferSwitch method, too.
		ret = 1;
		break;
	case kAsioSupportsTimeCode:
		// informs the driver wether application is interested in time code info.
		// If an application does not need to know about time code, the driver has less work
		// to do.
		ret = 0;
		break;
	}
	return ret;
}

#endif // ASIO

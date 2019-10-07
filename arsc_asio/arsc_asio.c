/* arsc_asio.c - soudcard functions for ASIO devices */

#ifdef ASIO

#include "arsc_asio.h"						    // 
#include "../arsc_common.h"					    // 
#include "asiosys.h"						    // platform definition from Steinberg SDK
#include "asio.h"						    // from Steinberg SDK

#define MAX_KEY_LENGTH		255				    // Maximum registry key length
#define ASIO_PATH		"software\\asio"		    // For my example
#define MAX_ASIO_DRIVERS	32				    // PortAudio allows a max of 32, so we will too
#define MAXDRVNAMELEN		40
//#define NO_LATENCY_CORRECTION


/*
This was added for thread synchronization between the polling (chk_seg) and the filling of the response 
buffer.  It turned out that it wasn't actually needed.  I am leaving it here for awhile just in case.
*/
#define	xxUSE_MUTEX

typedef struct {
	CLSID		clsid;					    // Direct from the registry
	char		name[MAXDRVNAMELEN];			    // driver name
	char		description[MAXDRVNAMELEN];		    // drivers sometimes give a description in the registry
	bool		valid;					    // ASIO loaded and initializes and so is valid.
	int32		devices;				    // Number of ASIO output devices (or input if ARSC_PREF_IN )
} TAsioDriver;

// ASIO loads and intializes only one valid driver at a time.  To
// make a full list of valid "devices" (in terms of waveopen functionality)
// I created this structure to hold possible devices.
typedef struct {
	char		name[ARSC_NAMLEN];
	int32		driver;					    // index back to TAsioDriver struct
	int32		good_rates;				    // good sampling rates
	bool		IsOutput;
} TVirtualDevice;

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
	int32		*StimulusBlock;				    // Pointer to segment
	int32		Samples;				    // # of samples, e.g. 2048, in this block
	int32		Index;					    // How many samples have already been played
	int32		ChannelNumber;				    // 0, 1, 2, . . . 
	int32		SegmentNumber;				    // Current segment for this channel
	bool		OutputDone;				    // Says if output is done for this channel
} TStimulusData;

/*
Response structure
*/
typedef struct {
	int32		Magic;					    // Just an identifier for this structure
	int32		*ResponseBlock;				    // Pointer to a segment defined in calling program.
	int32		Samples;				    // # of samples, e.g. 2048, in this segment
	int32		Index;					    // Current position of this ResponseBlock
	int32		ChannelNumber;				    // 0, 1, 2, . . . 
	int32		SegmentNumber;				    // Current segment for this channel
	int32		SkippedSamples;				    // Skipped samples before Latency kicks in
	bool		LatencyReached;				    // For each channel, but only segment 0
} TResponseData;

static int32		dio = 0;				    // device identifier offset
								    // for multiple platforms such as when WIND and ASIO are both set in the 
								    // compilation.  The WIND devices are listed first, for example 0 to 5, then 
								    // the ASIO devices are listed, e.g. 6.
static TAsioDriver	AsioDriverList[MAX_ASIO_DRIVERS];
static TVirtualDevice	VirtualDevice[MAXDEV];			    // To hold the device names and the driver to which they associate
static int32		sintAsioIntializedDriver = -1;		    // Index into the AsioDriverList[] for the loaded ASIO driver
static int32		sintNumDevices = 0;			    // Number of valid ASIO devices
static int32		sintMaxInputChannels = -1;		    // Maximum as reported by the driver
static int32		sintMaxOutputChannels = -1;		    // Maximum as reported by the driver
static ASIOBufferInfo	*bufferInfos = NULL;			    // Pointer to array of bufferInfos; one for each channel (input + output)
static ASIOChannelInfo	*channelInfos = NULL;			    // Pointer to array of channelInfos; one for each channel (input + output)
static ASIOCallbacks	asioCallbacks;				    // structure that holds the pointers to the callback functions
static TStimulusData	*stimulusData;				    // pointer to the main stimulus block
static TStimulusData	*sptrCurSegmentStimulus;		    // Points to the current segment stimulus (channel 0)
static TResponseData	*responseData;				    // pointer to the main response block
static TResponseData	*sptrCurSegmentResponse;		    // Points to the current segment response (channel 0)
static long		slngPreferredBufferSize;		    // Returned by the driver
static long		slngTotalUsedChannels = 0;		    // Need the total number of used input and output channels
static long		slngTotalPossibleChannels = 0;		    // Total possible channels
static long		slngInputLatency, slngOutputLatency;	    // Latencies as polled from the card.
static bool		sbolPostOutput = false;			    // flag - true if driver uses ASIOOutputReady optimization
static bool		sbolBuffersCreated = false;		    // flag - true if buffers have been created
static bool		sbolIsStarted = false;			    // flag - true if driver is started
static int32		sintTotalSamples = 0;			    // Total samples processed
static long		slngLatencyOffset = 0;			    // LatencyOffset specified by app
static int32		sintSegmentFinished = 0;		    // semaphore-like variable to tell when segment is finished
static FILE		*fhResponse0 = NULL;
static FILE		*fhStimulus0 = NULL;
static FILE		*fhResponse1 = NULL;
static FILE		*fhStimulus1 = NULL;
static FILE		*fhBD = NULL;
static int32		gdsr;					    // Good sample rates
static ARDEV		*a;

#ifdef USE_MUTEX
static HANDLE			hMutex;				    // mutex for synchronizaton between threads
#endif // USE_MUTEX

/*
External prototypes to CPP functions
*/
bool SDKLoadAsioDriver ( char *name );
bool SDKAsioInit ( ASIODriverInfo *info );
bool SDKAsioExit ( void );
bool SDKAsioGetChannels ( long *alngInputChannels, long *alngOutputChannels );
bool SDKAsioCanSampleRate ( ASIOSampleRate aSampleRate );
bool SDKAsioGetBufferSize ( long *alngMinBufferSize,
			long *alngMaxBufferSize,
			long *aslngPreferredBufferSize,
			long *alngGranularity );
bool SDKAsioSetSampleRate ( ASIOSampleRate aSampleRate );
bool SDKAsioGetChannelInfo ( ASIOChannelInfo *info );
bool SDKAsioCreateBuffers ( ASIOBufferInfo *bufferInfos,
			long numChannels,
			long bufferSize,
			ASIOCallbacks *callbacks );
bool SDKAsioOutputReady ( );
bool SDKAsioGetSamplePosition ( ASIOSamples *sPos, ASIOTimeStamp *tStamp );
bool SDKAsioGetLatencies ( long *inputLatency, long *outputLatency );
bool SDKAsioDisposeBuffers ( void );
bool SDKAsioStop ( void );
bool SDKAsioStart ( void );

/*
Callback prototypes
*/
void bufferSwitch(long index, ASIOBool processNow);
ASIOTime *bufferSwitchTimeInfo(ASIOTime *timeInfo, long index, ASIOBool processNow);
void sampleRateChanged(ASIOSampleRate sRate);
long asioMessages(long selector, long value, void* message, double* opt);

/*
Internal prototypes
*/
static int32_t ar_asio_devices_impl();
static char *_ar_asio_dev_name(int32_t di);
static void _ar_asio_close(int32_t di);
static int32_t _ar_asio_open(int32_t di);
static int32_t _ar_asio_io_prepare(int32_t di);
static int32_t _ar_asio_xfer_seg(int32_t di, int32_t b);
static int32_t _ar_asio_chk_seg(int32_t di, int32_t b);
static void _ar_asio_io_start(int32_t di);
static void _ar_asio_io_stop(int32_t di) ;
static int32 pSendStimulusData ( int32 *buffer, int32 aintBufferSize, TStimulusData *ptrStimulusData );
static int32 pFillResponseBlock ( int32 *buffer, int32 aintBufferSize, TResponseData *ptrResponseData );
static int32 pWriteBufferDemarcation ( int32 aintChunkSize, int32 aintAbsAmplitude );
static int32 pPollAsioDrivers ( void );
static int32 pLockAndLoad ( int32 aintDevice );
static int32 pBuildVirtualDevices ( int32 aintDriver );
#ifdef USE_MUTEX
bool pGetMutex ( void );
#endif // USE_MUTEX

/***************************************************************************/

/* 
_ar_asio_num_dev_impl - return number of ASIO devices
*/

static int32_t
ar_asio_devices_impl()
{

    // Has the number of devices already been polled?
    if ( sintNumDevices > 0 ) {
	FDBUG ( ( _arS, "_ar_asio_dev_name(): Already have number of devices [%d]\n", sintNumDevices ) );
	return ( sintNumDevices );
    } else {
	// Need to load the ASIO driver and get some details.
	if ( ! pPollAsioDrivers ( ) )
	    return 0;
    }

    // Now we should have the number of devices
    return ( sintNumDevices );
}

int32_t(*ar_asio_devices)() = ar_asio_devices_impl;

/* 
_ar_asio_dev_name - return name of I/O device 

The original assumption was to just return the driver name.
This doesn't work for Echo ASIO WDM, which is the same driver
name for a number of Echo cards.

*/

static char   *
_ar_asio_dev_name(int32_t di)
{
    int32		i;

    // In case this is called before we are ready
    if ( sintNumDevices == 0 ) {
	FDBUG ( ( _arS, "_ar_asio_dev_name(): Repoll\n" ) );
	if ( ! pPollAsioDrivers ( ) )
	    return NULL;
    }

    for (i = 0; i < MAXDEV; i++) {
	if ( i == ( di - dio ) ) {
	    FDBUG ( ( _arS, "_ar_asio_dev_name(): Found name [%s]\n", VirtualDevice[i].name ) );
	    return VirtualDevice[i].name;
	}
    }

    FDBUG ( ( _arS, "_ar_asio_dev_name(): Could not find channel information for di [%d] dio [%d]\n", di, dio ) );

    return NULL;
}

char* (*ar_asio_device_name)(int32_t) = _ar_asio_dev_name;

/* 
_ar_asio_list_rates - return good sampling rates
*/

static int32_t
_ar_asio_list_rates(int32_t di)
{
    int32		i;

    // In case this is called before we are ready
    if ( sintNumDevices == 0 ) {
	FDBUG ( ( _arS, "_ar_asio_dev_name(): Repoll\n" ) );
	if ( ! pPollAsioDrivers ( ) )
	    return 0;
    }

    for (i = 0; i < MAXDEV; i++) {
	if ( i == ( di - dio ) ) {
	    FDBUG ( ( _arS, "_ar_asio_list_rates(): good_rates=%0X\n", VirtualDevice[i].good_rates ) );
	    return VirtualDevice[i].good_rates;
	}
    }

    FDBUG ( ( _arS, "Could not find channel information for di [%d] dio [%d]\n", di, dio ) );

    return 0;
}

/* 
_ar_asio_close - close I/O device 
*/
static void
_ar_asio_close(int32_t di) {

    FDBUG ( ( _arS, "_ar_asio_close(): sintAsioIntializedDriver is [%d] and di is [%d].\n", sintAsioIntializedDriver, di ) );

    if ( fhResponse0 )
	fclose ( fhResponse0 );
    if ( fhStimulus0 )
	fclose ( fhStimulus0 );
    if ( fhResponse1 )
	fclose ( fhResponse1 );
    if ( fhStimulus1 )
	fclose ( fhStimulus1 );
    if ( fhBD )
	fclose ( fhBD );

    // Stop the driver if it is running
    if ( sbolIsStarted ) {
	_ar_asio_io_stop( di );
	sbolIsStarted = false;
	Sleep (1);	// 1-ms delay delay before freeing buffers
    }

    FDBUG ( ( _arS, "_ar_asio_close(): Disposing of buffers\n" ) );

    SDKAsioDisposeBuffers ();
    sbolBuffersCreated = false;

    // Clear the channels
    if ( channelInfos != NULL ) {
	free ( channelInfos );
	channelInfos = NULL;
	FDBUG ( ( _arS, "_ar_asio_close(): Freed channelInfos\n" ) );
    }

    // Clear the buffers
    if ( bufferInfos != NULL ) {
	free ( bufferInfos );
	bufferInfos = NULL;
	FDBUG ( ( _arS, "_ar_asio_close(): Freed bufferInfos\n" ) );
    }
    
    // Clear stim blocks
    if ( stimulusData != NULL ) {
	stimulusData->Magic = 0;	// Indentification for debugging
	free ( stimulusData );
	stimulusData = NULL;
    }
    
    // Clear resp blocks
    if ( stimulusData != NULL ) {
	responseData->Magic = 0;	// Indentification for debugging
	free ( responseData );
	responseData = NULL;
    }

#ifdef USE_MUTEX
    // The mutex is no longer needed
    if ( hMutex != NULL ) {
	if ( ! CloseHandle ( hMutex ) ) {
	    FDBUG ( ( _arS, "Error closing mutex\n" ) );
	}
	hMutex = NULL;
    }
#endif // USE_MUTEX


    // Unload the ASIO driver
    if ( sintAsioIntializedDriver != -1 ) {
	FDBUG ( ( _arS, "_ar_asio_close(): Calling SDKAsioExit().\n" ) );
	SDKAsioExit ();
    }

    sintAsioIntializedDriver = -1;										// Let 'em know driver is unloaded
}

/* _ar_asio_open - open I/O device */

static int32_t
_ar_asio_open(int32_t di)
{
    long		lngMinBufferSize;
    long		lngMaxBufferSize;
    long		lngGranularity;
    ASIOBufferInfo	*ptrBufferInfo;			// handy pointer
    int32		i;
    bool		bolOutput;
    int32		intChannelOffset = 0;

    a = _ardev[di];
    // Loon test
    if (intChannelOffset + a->ncda > sintMaxOutputChannels ) {
	FDBUG ( ( _arS, "_ar_asio_open(): too many output channels requested for device\n" ) );
	goto err;
    }
    if (intChannelOffset + a->ncad > sintMaxInputChannels ) {
	FDBUG ( ( _arS, "_ar_asio_open(): too many input channels requested for device\n" ) );
	goto err;
    }

    bolOutput = ! ( _arsc_find & ARSC_PREF_IN );

    // Load the driver and initialize
    if ( ! pLockAndLoad ( di ) )
	goto err;

    // This is always true for ASIO.  This lets the API code use the 
    // segments as is because of the ef (effective) flag.
    a->nbps = 4;
    a->ntlv = 0;

    /* 
    Ensure a good sample rate, then set it.
    gdsr is a bitwise combintation of our list of 27 sample rates.
    Once known, we don't have to look it up again.
    */
    if (!a->gdsr)
	a->gdsr = _ar_asio_list_rates(di);
    a->rate = _ar_adjust_rate ( di, a->a_rate );
    if ( ! SDKAsioSetSampleRate ( (double) a->rate ) )
	goto err;

    // check whether the driver requires the ASIOOutputReady() optimization
    // (can be used by the driver to reduce output latency by one block)
    sbolPostOutput = SDKAsioOutputReady ();
    FDBUG ( ( _arS, "ASIOOutputReady(); - %s\n", sbolPostOutput ? "Supported" : "Not supported" ) );

    /* 
    Get the buffer size information from the driver.  This 
    value is set in the ASIO control panel.  So far I haven't
    found a need to deviate from the Preferred . . . .
    */
    if ( ! SDKAsioGetBufferSize (	&lngMinBufferSize, 
					&lngMaxBufferSize, 
					&slngPreferredBufferSize, 
					&lngGranularity ) )
	goto err;
    else {
	FDBUG ( ( _arS, "Preferred buffer size [%d]\n", slngPreferredBufferSize ) );
    }

    /* 
    Allocate bufferInfos
    */
    slngTotalUsedChannels = a->a_ncad + a->a_ncda;
    if ( ( bufferInfos = (ASIOBufferInfo *) calloc ( slngTotalUsedChannels, sizeof ( ASIOBufferInfo ) ) ) == NULL )
	goto err;

    // Figure out the channel offset in case there are multiple ASIO cards.
    // Count all "devices" up to the card in question.
    for ( i = 0; i < sintAsioIntializedDriver; i++ ) {
	if ( AsioDriverList[i].valid  ) {
	    intChannelOffset += AsioDriverList[i].devices;
	}
    }
    intChannelOffset = ( di - dio ) - intChannelOffset;
    FDBUG ( ( _arS, "Channel Offset [%d]\n", intChannelOffset ) );

    // Allocate buffers
    ptrBufferInfo = bufferInfos;		// Set a pointer
    for (i = 0; i < a->a_ncda; i++) {		// loop over output channels
    	ptrBufferInfo->isInput = ASIOFalse;	// create an output buffer
	ptrBufferInfo->channelNum = i;		// (di - dio) handles channel offsets
	ptrBufferInfo->buffers[0] = NULL;	// clear buffer 1/2 channels
	ptrBufferInfo->buffers[1] = NULL;	// clear buffer 1/2 channels

	ptrBufferInfo++;
    }

    for (i = 0; i < a->a_ncad; i++) {		// loop over output channels
	ptrBufferInfo->isInput = ASIOTrue;	// create an input buffer
	ptrBufferInfo->channelNum = i;		// (di - dio) handles channel offsets
	ptrBufferInfo->buffers[0] = NULL;	// clear buffer 1/2 channels
	ptrBufferInfo->buffers[1] = NULL;	// clear buffer 1/2 channels

	ptrBufferInfo++;
    }

    // set up the asioCallback structure and create the ASIO data buffer
    asioCallbacks.bufferSwitch = &bufferSwitch;
    asioCallbacks.sampleRateDidChange = &sampleRateChanged;
    asioCallbacks.asioMessage = &asioMessages;
    asioCallbacks.bufferSwitchTimeInfo = &bufferSwitchTimeInfo;


    /*
    Create the ASIO buffers, both input and output.  Also set up the callbacks.
    */
    sbolBuffersCreated = SDKAsioCreateBuffers ( bufferInfos, slngTotalUsedChannels, slngPreferredBufferSize, &asioCallbacks );
    if ( ! sbolBuffersCreated ) {
	FDBUG ( ( _arS, "_ar_asio_open(): unable to create buffers\n" ) );
	goto err;
    }

    // get the input and output latencies
    // Latencies often are only valid after ASIOCreateBuffers()
    // (input latency is the age of the first sample in the currently returned audio block)
    // (output latency is the time the first sample in the currently returned audio block requires to get to the output)
    if ( ! SDKAsioGetLatencies ( &slngInputLatency, &slngOutputLatency ) )
	goto err;
    FDBUG ( ( _arS, "Latencies: input (%d) output (%d)\n", slngInputLatency, slngOutputLatency  ) );

    // Clear the buffers because CardDeluxe has known issues
    ptrBufferInfo = bufferInfos;
    for ( i = 0; i < slngTotalUsedChannels; i++ ) {
	memset ( ptrBufferInfo->buffers[0], 0, slngPreferredBufferSize * sizeof(int32) );
	memset ( ptrBufferInfo->buffers[1], 0, slngPreferredBufferSize * sizeof(int32) );
	ptrBufferInfo++;
    }

#ifdef USE_MUTEX
    // Create a mutex with no initial owner.  The mutex is used for thread synchronization.
    hMutex = CreateMutex( 
	NULL,			// default security attributes
	FALSE,			// initially not owned
	NULL);			// unnamed mutex

    if ( hMutex == NULL ) {
	FDBUG ( ( _arS, "CreateMutex error: %d\n", GetLastError() ) );
    }
#endif // USE_MUTEX

    sintTotalSamples = 0;	// Total samples processed
    slngLatencyOffset = 0;	// LatencyOffset specified by app
    sintSegmentFinished = 0;	// semaphore-like variable to tell when segment is finished

    return (0);
err:
    return  120;		// ASIO open error
}

/* _ar_asio_io_prepare - prepare device and buffers for I/O */

static int32_t
_ar_asio_io_prepare(int32_t di)
{
    int32		intNumberSegments;	    // Same for in and out
    int32		**out;
    int32		**in;
    TStimulusData	*ptrStimulusData;
    TResponseData	*ptrResponseData;
    int32		i;

    FDBUG ( (_arS, "asio_io_prepare:\n") );

    a = _ardev[di];				    // get access to application parameters

    intNumberSegments = a->segswp;		    // shorthand
    out = (int32 **) a->o_data;			    // shorthand
    in = (int32 **) a->i_data;			    // shorthand

    /*
    Set up stimulus (OUTPUT) blocks
    This is just a contiguous array of stimulusData structures.  The bufferSwitch
    only loops over channels, so the contiguous array should (for stereo) look 
    like this:
    	SEGMENT		CHANNEL
	    0		    0
	    0		    1
	    1		    0
	    1		    1
	    2		    0
			. . .
    The pointer "sptrCurSegmentStimulus" will point to the first (channel 0)
    stimulusData for the current segment.
    */
    if ( ( stimulusData = (TStimulusData *) calloc ( a->ncda * intNumberSegments, sizeof ( TStimulusData ) ) ) == NULL )
	return -1;

    /*
    Set up response (INPUT) blocks
    (see note above for details)
    */
    if ( ( responseData = (TResponseData *) calloc ( a->ncad * intNumberSegments, sizeof ( TResponseData ) ) ) == NULL )
	return -1;

    // Fill stimulusData (OUTPUT) blocks
    ptrStimulusData = stimulusData;										// Initialize
    sptrCurSegmentStimulus = stimulusData;			// Set to SEG0 CH0 to start.
    for ( i = 0; i < a->ncda * intNumberSegments; i++ ) {

	ptrStimulusData->Magic = 0xBEEF;			// Indentification for debugging
	ptrStimulusData->ChannelNumber = i % a->ncda;		// e.g. 0, 1, 0, 1, . . . 
	ptrStimulusData->SegmentNumber = i / a->ncda;		// segment number
	ptrStimulusData->StimulusBlock = out[i];
	ptrStimulusData->Index = 0;										// initialize to the first sample
	ptrStimulusData->Samples = a->sizptr[i / a->ncda];

	FDBUG ( ( _arS, "STM --> Magic [%2d] ch [%d] seg [%d] out [%p] samples [%d] end [%p]\n", 
	    i, 
	    ptrStimulusData->ChannelNumber, 
	    ptrStimulusData->SegmentNumber, 
	    out[i],
	    ptrStimulusData->Samples,
	    out[i] + ptrStimulusData->Samples ) );

	    ptrStimulusData++;
    }

    // Fill responseData (INPUT) blocks
    ptrResponseData = responseData;				// Initialize
    sptrCurSegmentResponse = responseData;			// Set to SEG0 CH0 to start.
    for ( i = 0; i < a->ncad * intNumberSegments; i++ ) {

	ptrResponseData->Magic = 0xBEEF;			// Indentification number for debugging
	ptrResponseData->ChannelNumber = i % a->ncad;		// e.g. 0, 1, 0, 1, . . . 
	ptrResponseData->SegmentNumber = i / a->ncad;		// segment number
	ptrResponseData->ResponseBlock = in[i];
	ptrResponseData->Index = 0;				// initialize to the first sample
	ptrResponseData->Samples = a->sizptr[i / a->ncad];

	FDBUG ( ( _arS, "RSP --> Magic [%2d] ch [%d] seg [%d] in [%p] samples [%d] end [%p]\n", 
	    i, 
	    ptrResponseData->ChannelNumber, 
	    ptrResponseData->SegmentNumber, 
	    in[i],
	    ptrResponseData->Samples,
	    in[i] + ptrResponseData->Samples ) );

	// Only segment 0 need be concerned with latency
	ptrResponseData->LatencyReached = ( ptrResponseData->SegmentNumber != 0 );

	ptrResponseData++;
    }

    return (0);
}

/* _ar_asio_xfer_seg - this segment is ready to go */

static int32_t
_ar_asio_xfer_seg(int32_t di, int32_t b)
{
	//FDBUG ( ( _arS, "ASIO XFER SEG\n" ) );
	return ( 0 );
}

/* _ar_asio_chk_seg - check for segment completion */

static int32_t
_ar_asio_chk_seg( int32_t di, int32_t b )
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

    static int32	sintHoldSegment = -1;
    bool		bolGotMutex = false;

    a = _ardev[di];			// get access to application parameters

    if ( ! sbolIsStarted ) {
	FDBUG ( ( _arS, "sbolIsStarted is FALSE\n" ) );
	return -1;
    }

#ifdef NEVER			//  no need to check escape here [STN: Jul-2007]
    /* Allows the user to hit the ESC key to exit the loop */
    if ( GetAsyncKeyState (VK_ESCAPE) & 0x8001 ) {
	//_ar_asio_io_stop(0);
	//a->seg_oc = a->seg_ic = -1;	// Should force an exit in the calling thread
	return ( 0 );
    }
#endif

    switch ( sintSegmentFinished ) {
    case 0:
	// Nothing to do
	break;
    case 1:
	// The segment has just finished.  If this routine is called frequently
	// enough, we should get this within moments of it happening.
#ifdef USE_MUTEX
	// Request ownership of mutex.
	if ( ( bolGotMutex = pGetMutex ( ) ) == true ) {
#endif // USE_MUTEX
	    sintSegmentFinished--;
#ifdef USE_MUTEX
	    // Release the mutex
	    if ( ! ReleaseMutex(hMutex)) { 
		FDBUG ( ( _arS, "*** Could not release mutex\n" ) );
		return false;
	    } 
	}
#endif // USE_MUTEX

	FDBUG ( ( _arS, "Got semaphore for segment [%d] [%d]\n", a->seg_ic, a->seg_oc ) );
	return true;
	break;
    default:
	// Segment overrun
	// This can happen when clicking to another window while the app is running.
	FDBUG ( ( _arS, "._._._ S E G M E N T  O V E R R U N _._._.\n" ) );
#ifdef USE_MUTEX
	// Request ownership of mutex.
	if ( ( bolGotMutex = pGetMutex ( ) ) == true ) {
#endif // USE_MUTEX
	    //sintSegmentFinished = 0;	    // Need to handle overruns better [STN, Jan-2008]
	    a->xrun++;			    // increment xrun count
#ifdef USE_MUTEX
	    // Release the mutex
	    if ( ! ReleaseMutex(hMutex)) { 
		FDBUG ( ( _arS, "*** Could not release mutex\n" ) );
		return false;
	    } 
	}
#endif // USE_MUTEX
	return true;
    }

    return ( false );			// false means the API xfer function isn't run
}


/* _ar_asio_io_start - start I/O */

static void
_ar_asio_io_start(int32_t di)
{

    sintTotalSamples = 0;		// Reset the total samples

    FDBUG ( ( _arS, "_ar_asio_io_start(): entered.\n" ) );

    sbolIsStarted = SDKAsioStart();

}

/* _ar_asio_io_stop - stop I/O */

static void
_ar_asio_io_stop(int32_t di) 
{
    a = _ardev[di];			// get access to application parameters

    if ( sbolIsStarted ) {
	FDBUG ( ( _arS, "_ar_asio_io_stop(): Calling SDKAsioStop().\n" ) );
	SDKAsioStop();
	sbolIsStarted = false;
    }

    // Let the calling window know that we are done.
    if ( _arsc_wind )
	PostMessage ( (HWND) _arsc_wind, WM_ARSC, AM_Stopped, dio );
}

void (*ar_asio_io_stop)(int32_t) = _ar_asio_io_stop;

/* _ar_asio_latency - set and get latency */

static int32_t
_ar_asio_latency(int32_t di, int32_t nsmp) 
{
    long max_latency;

    a = _ardev[di];			// get access to application parameters
    if (nsmp != ARSC_GET_LATENCY) {
	max_latency = slngInputLatency + slngOutputLatency; // sum of driver input/output latencies
	max_latency -= max_latency % 256;		    // subtract impulse latency, if any
	if (nsmp > max_latency)
	    nsmp = max_latency;
        slngLatencyOffset = nsmp;
    }
    return (slngLatencyOffset);
}

/* _ar_asio_bind - bind ASIO functions */

int32_t
_ar_asio_bind(int32_t ndt, int32_t tnd)
{
    FDBUG ( ( _arS, "asio_bind\n" ) );
    // Get the number of ASIO devices.  This is not the same as the 
    // number of ASIO Drivers in the registry.  This will either be
    // 1 or 0.
	int32_t devices = ar_asio_devices();

    if (devices > 0) {
		_ardvt[ndt].num_dev = ar_asio_devices;
		_ardvt[ndt].dev_name = ar_asio_device_name;
		_ardvt[ndt].io_stop = ar_asio_io_stop;
		_ardvt[ndt].close = _ar_asio_close;
		_ardvt[ndt].open = _ar_asio_open;
		_ardvt[ndt].io_prepare = _ar_asio_io_prepare;
		_ardvt[ndt].io_start = _ar_asio_io_start;
		_ardvt[ndt].xfer_seg = _ar_asio_xfer_seg;
		_ardvt[ndt].chk_seg = _ar_asio_chk_seg;
		_ardvt[ndt].latency = _ar_asio_latency;
		_ardvt[ndt].list_rates = _ar_asio_list_rates;
	
		dio = tnd;
    }

    return devices;
}

/*
Sends a stimulus file, piece-meal, by filling the passed ASIO buffer with
stimulus data.

Each channel has its own StimulusData block, so this function does not worry
about channels. A pointer to the correct StimulusData structure is passed.
*/
int32 pSendStimulusData ( int32 *buffer, int32 aintBufferSize, TStimulusData *ptrStimulusData ) {

    int32	k;
    int32	*ptrBuffer;
    int32	*ptrCurStimSample;
    int32	intCurOutputSegment;
    int32	intOutputChannels;
    int32	intNumWritten = 0;
    bool	bolGotMutex = false;

    intOutputChannels = a->a_ncda;			// shorthand
    if ( a->a_ncad )
    	intCurOutputSegment = a->seg_oc;		// If there are output channels, use the correct counter.
    else						// Otherwise, use seg_ic because that is passed back to calling program.
    	intCurOutputSegment = a->seg_ic;

    // Loon test
    if ( ptrStimulusData->Magic != 0xBEEF )
	return 0;
    if ( ptrStimulusData->Samples < 0 || ptrStimulusData->Samples > 999000 )
	return 0;
    if ( ptrStimulusData->ChannelNumber < 0 || ptrStimulusData->ChannelNumber >= intOutputChannels )
	return 0;

    ptrBuffer = buffer;					// Set a pointer to passed 1/2 buffer
    ptrCurStimSample = ptrStimulusData->StimulusBlock;	// Get pointer to stimulus data
    ptrCurStimSample += ptrStimulusData->Index;		// Move pointer to current sample

    DBUG_S ( ( "send: m/seg/ch [%d]/[%d]/[%d] index [%d] of [%d] TotalSamples [%d].\n", 
    	ptrStimulusData->Magic,	
	ptrStimulusData->SegmentNumber, 
	ptrStimulusData->ChannelNumber,
	ptrStimulusData->Index,
	ptrStimulusData->Samples,
	sintTotalSamples ) );

    // Loop over buffer size samples
    for ( k = 0; k < aintBufferSize; k++ ) {

	// -----------------------------------------------------------------------------------
	// Fixed a problem that occurred when an ASIO buffer boundary and a segment boundary
	// were the same.  For example, if there are 4410 samples total (index 0 - 4409), it 
	// was possible for the pointer to point one beyond, i.e. 4410, without causing a 
	// switch to the next ptrStimulusData before exiting this function.  Then the 
	// BufferSwitchTimeInfo() function returns you here causing an immediate switch, 
	// but the global pointer sptrCurSegmentStimulus gets munged.
	//
	// The fix was to move these next three lines from the end of the for(k) loop to the
	// top.
	*ptrBuffer = *ptrCurStimSample;			// Set this sample value
	ptrCurStimSample++;				// Move the sample pointer to the next sample
	ptrStimulusData->Index++;			// Increment the index for this channel

	// -----------------------------------------------------------------------------------

	// Input done?
	if ( ptrStimulusData->OutputDone ) {
	    *ptrBuffer = 0;
	    ptrBuffer++;
	    continue;
	}

	// SEGMENT DONE
	// Is the current index of this segment beyond this channel's segment size?
	if ( ptrStimulusData->Index >= ptrStimulusData->Samples ) {
	    DBUG_S ( ( "m/seg/ch [%d]/[%d]/[%d] finished.\n", ptrStimulusData->Magic, ptrStimulusData->SegmentNumber, ptrStimulusData->ChannelNumber ) );

	    // LAST CHANNEL
	    // If last channel of finished segment, increment the global segment count
	    if ( ptrStimulusData->ChannelNumber == intOutputChannels - 1 ) {

		// send a message to let 'em know . . . .
		// Windows messages aren't currently enacted in this implementation
		if ( _arsc_wind )
		    PostMessage ( (HWND) _arsc_wind, WM_ARSC, AM_StimulusSent, dio );

#ifdef USE_MUTEX
		// Request ownership of mutex.
		if ( ( bolGotMutex = pGetMutex ( ) ) == false ) {
		    FDBUG ( ( _arS, "Couldn't get mutex\n" ) );
		    return 0L;
		}
#endif // USE_MUTEX

		// If there are no input channels, the out channel determines the segment end
		if ( ! a->a_ncad ) {
		    sintSegmentFinished++;
		    FDBUG ( ( _arS, "S %d\n", sintSegmentFinished ) );
		}

// -------------------------------------------------
		/*
		Global pointer code
		*/
		if ( ( ( intCurOutputSegment + 1 ) % a->segswp) != 0 ) {

		    // Move the global stim block pointer 1
		    if ( ptrStimulusData - stimulusData <= a->segswp + 1 ) {
			sptrCurSegmentStimulus = ptrStimulusData + 1;		// On end channel, so move to channel 0 of next . . .
			DBUG ( ( "all channels done . . . sptrCurSegmentStimulus incremented to [%p].\n", ptrStimulusData ) );
		    } else {
			// Ouch!  Some sort of overrun caused this.  Maybe by clicking on
			// an external window or some such thing.  Just set back to zero.
			PRINT ( ( "O U C H\n" ) );
			sptrCurSegmentStimulus = stimulusData;
			ptrStimulusData = stimulusData;
			sintSegmentFinished = 0;
		    }
		} else {
		    // All segments have been completed in this sweep.
		    DBUG ( ( "*** All segments have been completed in this sweep.  Moving sptrCurSegmentStimulus to start. ***\n") );
		    sptrCurSegmentStimulus = stimulusData;						// Just sets it back to the beginning in case of infinite sweep
		} /* fi */

// -------------------------------------------------
#ifdef USE_MUTEX
		// Release ownership of the mutex object.
		if ( bolGotMutex ) {
		    if (! ReleaseMutex(hMutex)) { 
			FDBUG ( ( _arS, "*** Could not release mutex\n" ) );
			return 0L;
		    } 
		} 
#endif // USE_MUTEX

	    } /* fi last channel */

	    // Any more segments to play for this (current) channel?
	    if ( ptrStimulusData->SegmentNumber + 1 == a->segswp ) {
		// No more segments to play for this channel
		DBUG_S ( ( "no more segments to play for channel [%d].\n", ptrStimulusData->ChannelNumber ) );
		// Wrap back in case of sweeping
		ptrStimulusData -= (intOutputChannels) * ( a->segswp - 1 );

	    } else {
		/*
		More segments to play . . . 
		Move to the next stimulus structure for the rest of this bufferswitch.
		For example, if just finishing SEG0 CH1, need to jump to SEG1 CH1.
		*/
		DBUG_S ( ( "incrementing ptrStimulusData [%p] to [%p].\n", ptrStimulusData, ( ptrStimulusData + intOutputChannels ) ) );

		ptrStimulusData += intOutputChannels;		// Now pointing at next segment, same channel
	    } /* fi last segment */

	    ptrCurStimSample = ptrStimulusData->StimulusBlock;	// Point to the appropriate Stim sample
	    ptrStimulusData->Index = 0;				// Reset to beginning of block

	} /* fi segment done */

	ptrBuffer++;														// Move the buffer pointer

    } /* rof k */

    return 1;
}

/*
The response buffer is filled by the sound card.

Line up the input (response) waveform to the output (stimulus) wave form.  
Any samples due to device latency are skipped.

The BYPASS compiler directive will send the raw INPUT data.  This is
only for testing, only.
*/
int32 pFillResponseBlock ( int32 *buffer, int32 aintBufferSize, TResponseData *ptrResponseData ) {

    int32	*ptrBuffer;
    int32	intDifference;
    int32	intRemainder;
//    int32	intNumWritten;
    int32	intImpulseLatency;
    int32	intLoopbackLatency;
    int32	*ptrResponseSample;
    int32	intCurInputSegment;
    int32	intInputChannels;
    int32	intActualSegment;			// after mod
    bool	bolGotMutex = false;


    intInputChannels = a->a_ncad;			// shorthand
    intCurInputSegment = a->seg_ic;			// shorthand
    intActualSegment = intCurInputSegment % a->segswp;	// shorthand

    // Loon test
    if ( ptrResponseData->Magic != 0xBEEF )
	return 0;
    if ( ptrResponseData->ChannelNumber < 0 || ptrResponseData->ChannelNumber >= intInputChannels )
	return 0;

/*
The NO_LATENCY_CORRECTION compiler directive will send the raw INPUT data rather than
correct for latency between the response and the stimulus.  This 
latency is based upon the ASIO BufferSize and the sampling rate.

STN: Latency correction should enabled for consistency between the SYNC and ASIO code. [Nov-2006]
*/
#ifdef NO_LATENCY_CORRECTION
    /*
    Response is not adjusted for latency.  The input (response) will start a little more than
    two buffers past the stimulus start of time zero.
    */
    ptrResponseData->LatencyReached = true;
#endif // NO_LATENCY_CORRECTION

    ptrBuffer = buffer;						// Set a pointer to passed 1/2 buffer
    ptrResponseSample = ptrResponseData->ResponseBlock;		// Get reference to Response Block that is currently being filled
    ptrResponseSample += ptrResponseData->Index;		// Point to correct sample in Response Block

    DBUG_R ( ( "fill: m/seg/ch [%d]/[%d]/[%d] index [%d] of [%d] TotalSamples [%d] skipped [%d] bolLatency [%d].\n", 
    	ptrResponseData->Magic,	
    	ptrResponseData->SegmentNumber, 
    	ptrResponseData->ChannelNumber,
    	ptrResponseData->Index,
    	ptrResponseData->Samples,
    	sintTotalSamples,
    	ptrResponseData->SkippedSamples,
    	ptrResponseData->LatencyReached ) );
    /*
    Check to see that the latency between ouput and input has been reached.
    Each channel has the same latency (one would hope), and we only check
    the latency once for each channel of segment 0.  This was accomplished
    by filling the LatencyReached flag to TRUE for segments 1..N
    */
    if ( ! ptrResponseData->LatencyReached ) {

	// Latency not yet reached
	//
	// An E-mail to the ASIO listserv suggested the calculation for the
	// offset to be Input + Output latency.  These values are usually quite
	// close.
	//intLoopbackLatency = slngInputLatency + slngOutputLatency + slngLatencyOffset;    // Tone's version
	// Changed default LoopbackLatecy to match WDM latency [STN:Jun-2007]
	intLoopbackLatency = slngInputLatency + slngOutputLatency;	    // sum of driver input/output latencies
	intImpulseLatency = intLoopbackLatency % 256;			    // extract impulse latency, if any
	intLoopbackLatency -= slngLatencyOffset + intImpulseLatency;    // subtract app & impulse latencies
	FDBUG ( ( _arS, "Loopback latency: input [%d] + output [%d] + offset [%d]\n", slngInputLatency, slngOutputLatency, slngLatencyOffset ) );
	// With the passed buffer, are we past the latency?
	if ( ptrResponseData->SkippedSamples + aintBufferSize >= intLoopbackLatency ) {

	    // Partial buffer needs to be sent
	    intDifference = intLoopbackLatency - ptrResponseData->SkippedSamples;
	    if (intDifference < 0)
		intDifference = 0;
	    ptrBuffer += intDifference;
	    ptrResponseData->LatencyReached = true;

	    // Check for abnormally large buffers
	    intRemainder = aintBufferSize - intDifference;
	    if ( intRemainder < ptrResponseData->Samples ) {
		memcpy ( ptrResponseSample, ptrBuffer, intRemainder * sizeof ( int32 ) );
		ptrResponseData->Index += intRemainder;

	    } else {
		// This recursively calls this same function to handle the
		// remainder of this oversized buffer.
		if ( ! pFillResponseBlock ( ptrBuffer, intRemainder, ptrResponseData ) )
		    return 0L;
	    }
	} else {
	    // Entire buffer is worthless.  Still need to track # of skipped samples until LatencyReached.
	    ptrResponseData->SkippedSamples += aintBufferSize;
	}
    } else {
	// Latency has been reached.
	// This is true most of the time.  We are filling the segment.

	// Whole buffer or partial buffer?
	// 2005 Apr 19 -- changed from 1 to 0
	if ( ptrResponseData->Index + aintBufferSize  < ptrResponseData->Samples + 0 ) {

	    // Whole buffer will fit in block
	    memcpy ( ptrResponseSample, ptrBuffer, aintBufferSize * sizeof ( int32 ) );
	    ptrResponseData->Index += aintBufferSize;

	} else {
	    // Partial buffer will fit in block.
			
	    // Calculate what it will take to fill rest of block from end of segment, then put it in
	    intDifference = ptrResponseData->Samples - ptrResponseData->Index - 0;
	    memcpy ( ptrResponseSample, ptrBuffer, intDifference * sizeof ( int32 ) );
	    ptrResponseData->Index += intDifference;		// Brings us to the end
	    ptrBuffer += intDifference;				// Move the pointer

	    /*
	    P O S T - This segment is full at this point
	    */
#ifdef POSTRESPONSE
	    if ( ptrResponseData->ChannelNumber == 0 )
	        BlockWrite ( fhPostResponse, ptrResponseData->ReadBlock^, ptrResponseData->Samples * sizeof ( Integer ), intNumWritten );
#endif // POSTRESPONSE

#ifdef BLOCKDEMARCATION
	    // Send demarcation
	    if ( ptrResponseData->ChannelNumber == 0 ) {
	        // Send modified datablock to indicate buffer demarcation
	        pWriteBlockDemarcation ( ptrResponseData->Samples, SCALE_32BIT );
	    }
#endif // BLOCKDEMARCATION

	    // if this is the last channel, increment the global segment count
	    if ( ptrResponseData->ChannelNumber == intInputChannels - 1 ) {

		DBUG_R ( ( "Posting message to AM_ResponseFull for channel [%d]\n", ptrResponseData->ChannelNumber ) );
		if ( _arsc_wind )
		    PostMessage ( (HWND) _arsc_wind, WM_ARSC, AM_ResponseFull, dio );

		// Increment a semaphore-like variable
		sintSegmentFinished++;
		FDBUG ( ( _arS, "F %d\n", sintSegmentFinished ) );

		// Are sweeps done?
		if ( ( ( intCurInputSegment + 1 ) % a->segswp ) != 0 ) {

		    // Move the global response block pointer 1 to get to the zeroeth channel of next segment.
		    // (ptrResponseData already points to the last channel of the previous segment.)
		    DBUG_R ( ( "all channels done . . moving sptrCurSegmentResponse to next pointer [%p].\n", (ptrResponseData + 1) ) );
		    sptrCurSegmentResponse = ptrResponseData + 1;
		} else {
		    // All segments have been completed in this sweep.
		    DBUG_R ( ( "*** All segments have been completed in this sweep. Moving sptrCurSegmentResponse to start. *** [%d] [%d] [%d]\n",
			intCurInputSegment + 1,
			a->segswp,
			intCurInputSegment + 1 % a->segswp ) );
		    sptrCurSegmentResponse = responseData;			// Just sets it back to the beginning
		}
	    } // fi last channel

	    // Any more segments to play for this (current) channel?
	    if ( ptrResponseData->SegmentNumber + 1 == a->segswp ) {
		// No more
		DBUG_R ( ( "no more segments for m/seg/ch [%d]/[%d]/[%d].\n", 
		    ptrResponseData->Magic,	
		    intCurInputSegment, 
		    ptrResponseData->ChannelNumber ) );
		// Wrap back in case of sweeping
		ptrResponseData -= (intInputChannels) * ( a->segswp - 1 );
	    } else {
		/*
		There are more segments.
		Move to the next response structure for the rest of this bufferswitch.
		For example, if just finishing SEG0 CH1, need to jump to SEG1 CH1.
		*/
		DBUG_R ( ( "incrementing ptrResponseData [%p] [%d] channels.\n", 
		    ptrResponseData, 
		    intInputChannels ) );
		ptrResponseData += intInputChannels;		    // Now pointing at next segment, same channel
	    } // fi any more segments

	    ptrResponseSample = ptrResponseData->ResponseBlock;	    // Point to the appropriate Response sample
	    ptrResponseData->Index = 0;				    // Reset to beginning of block

	    // Remainder of RESPONSE data from sound card goes into next block

	    intDifference = aintBufferSize - intDifference;

	    // This should, at least, not cause a memory fault.  It does
	    // mean that data is thrown away.
	    if ( intDifference < ptrResponseData->Samples ) {

		memcpy ( ptrResponseSample, ptrBuffer, intDifference * sizeof ( int32 ) );
		ptrResponseData->Index = intDifference;
	    } else {
		// This recursively calls this same function to handle the
		// remainder of this oversized buffer.
		if ( ! pFillResponseBlock ( ptrBuffer, intDifference, ptrResponseData ) )
		    return 0L;
	    }
	} // fi whole/partial
    } // fi Latency reached

    return 1L;
}


/*
This writes a file with a single sample of amplitude aintAbsAmplitude
whenever the buffer switches.  This is used for reference in MATLAB plotting
of input and output waveforms.

Typically aintAbsAmplitude is SCALE_32BIT, but sometimes needs to be dropped
down to SCALE_24BIT depending upon the input/output waveform amplitude(s).
*/
int32 pWriteBufferDemarcation ( int32 aintChunkSize, int32 aintAbsAmplitude ) {

	int32		*ptrBlock;
	int32		*ptrInteger;
	int32		intNumWritten;

	// Allocate memory for exactly one buffer-width chunk.
	if ( ( ptrBlock = (int32 *) calloc ( aintChunkSize, sizeof ( int32 ) ) ) == NULL )
		return -1;

	// Fill this memory with zeros
	memset ( ptrBlock, 0, aintChunkSize * sizeof ( int32 ) );

	// Give the first one a large value
	ptrInteger = ptrBlock;
	*ptrInteger = aintAbsAmplitude;

	// Give the last one a negative large value
	//Inc ( ptrInteger, aintChunkSize - 1 );
	//ptrInteger^ := - aintAbsAmplitude;

	intNumWritten = (int32) fwrite ( ptrBlock, sizeof ( int32 ), aintChunkSize, fhBD );

	free ( ptrBlock );

	return 1;
}

#ifdef USE_MUTEX

bool pGetMutex ( void ) {

    DWORD		dwWaitResult;					// for mutex use
    bool		bolResult = false;

    dwWaitResult = WaitForSingleObject( 
	hMutex,   // handle to mutex
	1000L);   // one-second time-out interval

    switch (dwWaitResult) {
    case WAIT_OBJECT_0: 
	// The thread got mutex ownership.
	bolResult =  true;
	break; 

    // Cannot get mutex ownership due to time-out.
    case WAIT_TIMEOUT:
	FDBUG ( ( _arS, "mutex: WAIT_TIMEOUT\n" ) );
	break;

    // Got ownership of the abandoned mutex object.
    case WAIT_ABANDONED:
	FDBUG ( ( _arS, "mutex: WAIT_ABANDONED\n" ) );
	break;
    }

    return ( bolResult );
}

#endif // USE_MUTEX

static long 
check_rates(void) {

    int i;

    /*
    Get a list of sample rates the card can handle.  The list will be
    used later to ensure the closest valid sample rate to the requested 
    rate is used.
    */
    gdsr = 0;
    for ( i = 0; i < SRLSTSZ; i++ ) {
	if ( SDKAsioCanSampleRate ( _ar_SRlist[i] ) == true ) {
	    gdsr |= 1 << i;
	    FDBUG ( ( _arS, "Sample Rate [%ld] is supported.\n", _ar_SRlist[i] ) );
	}
    }
    FDBUG ( ( _arS, "gdsr is [%x]\n", gdsr ) );
    return ( gdsr );
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
int32 pGetChannelDetails ( int32 aintDriver ) {

    int32		    i;
    ASIOChannelInfo	    *ptrChannelInfo;
    bool		    bolOutput;


    // Are we interested in input or output channels?  Usually output.
    bolOutput = ! ( _arsc_find & ARSC_PREF_IN );

    // Hold onto the number of "devices" for this driver.  This is used 
    // later in calculating the channel offset from the device number.
    if ( bolOutput ) 
	AsioDriverList[aintDriver].devices = sintMaxOutputChannels;
    else
	AsioDriverList[aintDriver].devices = sintMaxInputChannels;;

    // Channel details
    ptrChannelInfo = channelInfos;
    for (i = 0; i < sintMaxOutputChannels; i++) {
	ptrChannelInfo->isInput = ASIOFalse;
	ptrChannelInfo->channel = i;

	// Gets channel information from the driver
	if ( ! SDKAsioGetChannelInfo ( ptrChannelInfo ) )
	    return -1;
	FDBUG ( ( _arS, "--- %s: Output Channel #%d\n", ptrChannelInfo->name, i) );

	if ( bolOutput ) {
	    // Set the VirtualDevice information
	    VirtualDevice[sintNumDevices].driver = aintDriver;
	    sprintf ( VirtualDevice[sintNumDevices].name, "%s ASIO", ptrChannelInfo->name);
	    VirtualDevice[sintNumDevices].IsOutput = true;
	    VirtualDevice[sintNumDevices].good_rates = check_rates();
	    sintNumDevices++;
	}

	ptrChannelInfo++;
    }

    for (i = 0; i < sintMaxInputChannels; i++) {
	ptrChannelInfo->isInput = ASIOTrue;
	ptrChannelInfo->channel = i;

	// Gets channel information from the driver
	if ( ! SDKAsioGetChannelInfo ( ptrChannelInfo ) )
	    return -1;
	FDBUG ( ( _arS, "--- %s: Input Channel #%d\n", ptrChannelInfo->name, i) );

	if ( ! bolOutput ) {
	    // Set the VirtualDevice information
	    VirtualDevice[sintNumDevices].driver = aintDriver;
	    sprintf ( VirtualDevice[sintNumDevices].name, "%s (%s) ASIO %s", ptrChannelInfo->name, "In", ptrChannelInfo->isActive == ASIOTrue ? "*" : "" );
	    VirtualDevice[sintNumDevices].IsOutput = false;
	    VirtualDevice[sintNumDevices].good_rates = check_rates();
	    sintNumDevices++;
	}

	ptrChannelInfo++;
    }

    return 1;
}

// This loads the ASIO driver and acquires some details.
int32 pPollAsioDrivers ( void ) {

    long		lRet;
    HKEY		hkEnum = 0;
    HKEY		hkDriver = 0;
    char		keyname[MAX_KEY_LENGTH];
    DWORD		index = 0;
    DWORD		type, size;
    char		strFullKeyPath[MAX_KEY_LENGTH];
    char		value[MAX_KEY_LENGTH];		// Arbitrary value type
    int32		intDriver;			// looping variable
    ASIODriverInfo	asioDriverInfo;			// needed for ASIOInit()

    // Open the main key to the ASIO drivers
    // http://msdn.microsoft.com/library/en-us/sysinfo/base/regopenkey.asp
    lRet = RegOpenKey ( HKEY_LOCAL_MACHINE, ASIO_PATH, &hkEnum );
    if ( lRet != ERROR_SUCCESS ) {
	FDBUG ( ( _arS, "pPollAsioDrivers(): Couldn't open key\n" ) );
	return 0;
    }	

    while ( lRet == ERROR_SUCCESS ) {
        // http://msdn.microsoft.com/library/en-us/sysinfo/base/regenumkey.asp
	if ( ( lRet = RegEnumKey ( hkEnum, index, (LPTSTR) keyname, MAX_KEY_LENGTH ) ) == ERROR_SUCCESS) {
	    // Print out the ASIO card name
	    FDBUG ( ( _arS, "[%d] [%s]\n", index, keyname ) );

	    // Get the subkey that this keyname represents
	    sprintf ( strFullKeyPath, "%s\\%s", ASIO_PATH, keyname );
	    lRet = RegOpenKey ( HKEY_LOCAL_MACHINE, strFullKeyPath, &hkDriver );
	    if (lRet != ERROR_SUCCESS) {
		FDBUG ( ( _arS, "pPollAsioDrivers(): Couldn't open key [%s]\n", strFullKeyPath ) );
		return 0;
	    }

	    // name
	    // The name is the key in the registry.
	    strcpy ( AsioDriverList[index].name, keyname );

	    // CLSID
	    size = MAX_KEY_LENGTH;
	    lRet = RegQueryValueEx ( hkDriver, "CLSID", 0, &type, (LPBYTE) value, &size);
	    if ( lRet == ERROR_SUCCESS ) {
	    	char	*ptrClsid = (char *) value;	// Shorthand
		WORD	wData[100];

		// Convert to CLSID
		if ( strlen(ptrClsid) == 38 ) {		// Must be 38 long, including { }
		    MultiByteToWideChar(CP_ACP,0,(LPCSTR) ptrClsid, -1 ,(LPWSTR)wData, 100);

		    if ( CLSIDFromString( (LPOLESTR) wData, (LPCLSID) &(AsioDriverList[index].clsid) ) != S_OK ) {
			FDBUG ( ( _arS, "CLSIDFromString() was not able to convert [%s]\n", ptrClsid ) );
		    }
		}
	    } else {
		// All ASIO drivers should have a CLSID
		FDBUG ( ( _arS, "ReqQueryValueEx(): returned [%ld] for key [%s]\n", lRet, strFullKeyPath ) );
		return 0;
	    }

	    // Description
	    // The description is not necessarily the same as the name of the driver.
	    size = MAX_KEY_LENGTH;
	    if ( RegQueryValueEx ( hkDriver, "Description", 0, &type, (LPBYTE) value, &size) == ERROR_SUCCESS ) {
		strcpy ( AsioDriverList[index].description, value );
	    } else {
		// Some ASIO drivers don't have a description
		strcpy ( AsioDriverList[index].description, keyname );
	    }

	    index++;
	} // fi RegEnumKey
    } // elihw

    if (hkEnum)
	RegCloseKey(hkEnum);

    FDBUG ( ( _arS, "pPollAsioDrivers(): Found [%d] ASIO drivers\n", index ) );

    /*
    The number of available drivers is not the same as the available ASIO devices.
    Need to test each one out to see if it opens.
    */
    sintNumDevices = 0;
    for ( intDriver = 0; intDriver < MAX_ASIO_DRIVERS; intDriver++ ) {
	if ( strlen ( AsioDriverList[intDriver].name ) > 0 ) {

	    // In case multiple ASIO sound cards exist, terminate the AudioStreamIO.
	    // This may not be necessary.
	    if ( sintAsioIntializedDriver > 0 )
		SDKAsioExit ();

	    if ( SDKLoadAsioDriver ( AsioDriverList[intDriver].name ) ) {
		FDBUG ( ( _arS, "Success: Driver [%s] loaded fine\n", AsioDriverList[intDriver].name ) );

		// But loading isn't enough.  The Echo Gina will load even if it isn't installed.
		// Initialize the AudioStreamIO.
		asioDriverInfo.driverVersion = 2;			// ASIO 2.0
		asioDriverInfo.sysRef = GetDesktopWindow();		// Application main window handle
		if ( SDKAsioInit ( &asioDriverInfo ) == true ) {
		    // This ASIO driver can be used.  Although we can use only one
		    // ASIO driver at a time, there may be multiple sound cards in the same
		    // box, so we aren't done yet.
		    AsioDriverList[intDriver].valid = true;
		    FDBUG ( ( _arS, "INIT Valid! Driver [%s] is ok to use\n", AsioDriverList[intDriver].name ) );

		    // For now, this sets the last ASIO driver as the one used.
		    sintAsioIntializedDriver = intDriver;

		    // Get the device information
		    if ( ! pBuildVirtualDevices ( intDriver ) )
			return 0;

		} else {
		    FDBUG ( ( _arS, "INIT Failed: Driver [%s] did not init\n", AsioDriverList[intDriver].name ) );
		}
	    } else {
		FDBUG ( ( _arS, "Failed: Driver [%s] did not load\n", AsioDriverList[intDriver].name ) );
	    } // fi SDKLoadAsioDriver
	} else {
	    // Get out when there are no more named drivers
	    break;
	} // fi .name
    } // rof intDriver

    // Terminate AudioStreamIO.  The open function will reload the correct driver
    // based upon the application's device choice.
    if ( sintAsioIntializedDriver > 0 ) {
	SDKAsioExit ();
	sintAsioIntializedDriver = -1;
	FDBUG ( ( _arS, "There should be no driver initialized at this point.\n" ) );
    }

    return 1;
}

/*
Run only after a driver is loaded -and- initialized.  This function gets
channel information to derive virtual device ids (for the SYNC analog).

The total possible channels is used to get full channel information from the card.
*/
int32 pBuildVirtualDevices ( int32 aintDriver ) {

    // Gets the number of channels for this card
    if ( ! SDKAsioGetChannels ( &sintMaxInputChannels, &sintMaxOutputChannels ) ) {
	FDBUG ( ( _arS, "GetChannels failed\n" ) );
	return 0;
    }
    slngTotalPossibleChannels = sintMaxInputChannels + sintMaxOutputChannels;


    // Allocate enough memory for every channel info, but only until we get the device information
    if ( ( channelInfos = (ASIOChannelInfo *) calloc ( slngTotalPossibleChannels, sizeof ( ASIOChannelInfo ) ) ) == NULL )
	return 0;

    // Get the channel details.  This should be done after the buffers are created
    // otherwise it is not known which channels are active.
    if ( ! pGetChannelDetails ( aintDriver ) )
    	return 0;

    // Get rid of the channelInfos allocated in this function
    if ( channelInfos != NULL ) {
	free ( channelInfos );
	channelInfos = NULL;
	FDBUG ( ( _arS, "pBuildVirtualDevices(): Freed channelInfos\n" ) );
    }

    return 1;
}

/*
The driver is now known from the application's request.  Load
the appropriate driver and initialize.
*/
int32 pLockAndLoad ( int32 aintDevice ) {
    int32		intDriver = VirtualDevice[aintDevice - dio].driver;
    ASIODriverInfo	asioDriverInfo;			// needed for ASIOInit()
    ASIOChannelInfo	*ptrChannelInfo;		// handy pointer
    int32		i;


    FDBUG ( ( _arS, "pLockAndLoad: Device [%d] dio [%d] driver [%d]\n", aintDevice, dio, intDriver ) );

    if ( SDKLoadAsioDriver ( AsioDriverList[intDriver].name ) ) {
	FDBUG ( ( _arS, "pLockAndLoad: Driver [%s] loaded fine\n", AsioDriverList[intDriver].name ) );

	asioDriverInfo.driverVersion = 2;				// ASIO 2.0
	asioDriverInfo.sysRef = GetDesktopWindow();			// Application main window handle
	if ( SDKAsioInit ( &asioDriverInfo ) == true ) {
	    FDBUG ( ( _arS, "pLockAndLoad: Driver [%s] is intialized\n", AsioDriverList[intDriver].name ) );

	    // Hold for later
	    sintAsioIntializedDriver = intDriver;
	} else {
	    FDBUG ( ( _arS, "pLockAndLoad: Driver [%s] did not init\n", AsioDriverList[intDriver].name ) );
	    return 0;
	}
    } else {
	FDBUG ( ( _arS, "pLockAndLoad: Driver [%s] did not load\n", AsioDriverList[intDriver].name ) );
	return 0;
    } // fi SDKLoadAsioDriver

    // Gets the number of channels for this card
    if ( ! SDKAsioGetChannels ( &sintMaxInputChannels, &sintMaxOutputChannels ) ) {
	FDBUG ( ( _arS, "GetChannels failed\n" ) );
    } else {
	FDBUG ( ( _arS, "MaxInputChannels [%d] MaxOutputChannels [%d]\n", sintMaxInputChannels, sintMaxOutputChannels ) );
    }
    slngTotalPossibleChannels = sintMaxInputChannels + sintMaxOutputChannels;

    // Allocate enough memory for every channel info
    if ( ( channelInfos = (ASIOChannelInfo *) calloc ( slngTotalPossibleChannels, sizeof ( ASIOChannelInfo ) ) ) == NULL )
	return 0;

    // Initialize channels
    ptrChannelInfo = channelInfos;
    for (i = 0; i < sintMaxOutputChannels; i++) {
    	ptrChannelInfo->isInput = ASIOFalse;
	ptrChannelInfo->channel = i;

    // Gets channel information from the driver
    if ( ! SDKAsioGetChannelInfo ( ptrChannelInfo ) )
	return -1;

	ptrChannelInfo++;
    }
    for (i = 0; i < sintMaxInputChannels; i++) {
	ptrChannelInfo->isInput = ASIOTrue;
	ptrChannelInfo->channel = i;

	// Gets channel information from the driver
	if ( ! SDKAsioGetChannelInfo ( ptrChannelInfo ) )
	    return -1;

	ptrChannelInfo++;
    }

    return 1;
}

/*
--------------------------------------- CALLBACKS ------------------------------------------

These were originally snarfed from Steinberg's SDK, but modified.

*/

/*
The actual processing callback; the real McCoy
This is where processing of samples occurs.

SDK Note:
	Beware that this is normally in a seperate thread, hence be sure that you 
	take care about thread synchronization. This is omitted here for simplicity.
*/
ASIOTime *bufferSwitchTimeInfo ( ASIOTime *timeInfo, long index, ASIOBool processNow ) {
    int32	    i;
    long	    lngAsioBufferSize = slngPreferredBufferSize;    // shorthand to buffer size in samples
    TStimulusData   *ptrStimulusData = sptrCurSegmentStimulus;	    // pointer to channel 0 of current segment stimulus
    TResponseData   *ptrResponseData = sptrCurSegmentResponse;	    // pointer to channel 0 of current segment response

    if (!sbolIsStarted)
	return 0L;

    // perform the processing
    for ( i = 0; i < slngTotalUsedChannels; i++) {
	if ( bufferInfos[i].isInput == false ) {
	    // OUTPUT (STIMULUS) buffer

	    switch ( channelInfos[i].type ) {
	    case ASIOSTInt32LSB:
		/*
		Tone - all of the cards tested here are Int32LSB, including:
		CardDeluxe, Gina24, Layla24, and M-Audio Delta Audiophile 2496
		*/
		if ( ! pSendStimulusData ( bufferInfos[i].buffers[index], lngAsioBufferSize, ptrStimulusData ) )
		    break;	// ???
//		    return 0L;
		break;
	    default:
		FDBUG ( ( _arS, "Channel type is [%ld], and no provisions were made for this type.\n", channelInfos[i].type ) );
		break;
	    } /* hciws */

		ptrStimulusData++;			// point to next channel's output/stimulus structure

	} else {
	    // INPUT (RESPONSE) buffer

	    switch ( channelInfos[i].type ) {
	    case ASIOSTInt32LSB:
		if ( ! pFillResponseBlock ( bufferInfos[i].buffers[index], lngAsioBufferSize, ptrResponseData ) )
		    break;	// ???
//		    return 0L;
		break;
	    default:
		FDBUG ( ( _arS, "Channel type is [%ld], and no provisions were made for this type.\n", channelInfos[i].type ) );
		break;
	    } /* hciws */

	    ptrResponseData++;				// point to next channel's input/response structure

	} /* fi isInput */
    } /* rof */

    // finally if the driver supports the ASIOOutputReady() optimization, do it here, all data are in place
    if ( sbolPostOutput )
	SDKAsioOutputReady ( );

    // The total samples is really the count of buffer dimensions.
    // Because it is outside the channels loop, it is independent of
    // channel.  It is as though one channel was tallied.
    sintTotalSamples += lngAsioBufferSize;

    return 0L;
}

//----------------------------------------------------------------------------------
void bufferSwitch(long index, ASIOBool processNow)
{	// the actual processing callback.
	// Beware that this is normally in a seperate thread, hence be sure that you take care
	// about thread synchronization. This is omitted here for simplicity.

	// as this is a "back door" into the bufferSwitchTimeInfo a timeInfo needs to be created
	// though it will only set the timeInfo.samplePosition and timeInfo.systemTime fields and the according flags
	ASIOTime  timeInfo;
	memset (&timeInfo, 0, sizeof (timeInfo));

	// get the time stamp of the buffer, not necessary if no
	// synchronization to other media is required
	// tone - it is kind of handy to see the counters . . . .
	if ( SDKAsioGetSamplePosition ( &timeInfo.timeInfo.samplePosition, &timeInfo.timeInfo.systemTime ) )
		timeInfo.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;

	bufferSwitchTimeInfo (&timeInfo, index, processNow);
}


//----------------------------------------------------------------------------------
void sampleRateChanged(ASIOSampleRate sRate)
{
	// do whatever you need to do if the sample rate changed
	// usually this only happens during external sync.
	// Audio processing is not stopped by the driver, actual sample rate
	// might not have even changed, maybe only the sample rate status of an
	// AES/EBU or S/PDIF digital input at the audio device.
	// You might have to update time/sample related conversion routines, etc.
}

//----------------------------------------------------------------------------------
long asioMessages(long selector, long value, void* message, double* opt)
{
    // currently the parameters "value", "message" and "opt" are not used.
    long ret = 0;
    switch(selector)
    {
    case kAsioSelectorSupported:
	if(value == kAsioResetRequest
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

/**************************************************************************/

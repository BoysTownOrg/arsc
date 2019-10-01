/* arsc_helper.c

General notes:

[1] These "helper" functions intended to assist ARSC.DLL users
and are specific to Windows.

[2] Hitting the ESC key will stop the sound engine.

*/

#include <windows.h>
#include "arsc_common.h"	/* Common to .dll, .exe, and/or .lib */
#include "arscdev.h"		/* ARSC internal data */
#include "arsclib.h"		/* ARSC internal data */
#include "arsc_dll.h"		/* raw function prototypes */
#include "version.h"		/* Version information */

#define WIN32DLL_API	__declspec(dllexport)
#define STDCALL		_stdcall

/* These are essentially global to the DLL because they have to be freed */

static int32 **in_data_array = NULL;											/* dimension # channels * # segments */
static int32 **out_data_array = NULL;											/* dimension # channels * # segments */

/*
The DLL functions are all STDCALL, so it is likely that the 
xfer functions will also be STDCALL.  The API functions, however,
are cdecl.  These are declared here, and special functions in the
DLL will act as a go-between for the xfer functions.

I am writing this with C# in mind.  See the function ar_set_xfer_stdcall()
for more details.
*/

static void (_stdcall *in_xfer_stdcall)(long);
static void (_stdcall *out_xfer_stdcall)(long);

long WINAPI DllEntryPoint(HINSTANCE hinst, unsigned int reason, void* lpReserved)
{
	return 1;
}


/*
Below are the wrappers to the _ar_* functions in Steve's
arsc library.  The arguments are the same as the _ar_*
counterpart.

There are a few extra routines for special cases, e.g.
	ar_io_prepare_vb()
takes a contiguous block of data and calculates the necessary
pointers for Steve's _ar_io_prepare() function.  
*/

#ifdef WRAP

WIN32DLL_API int32_t STDCALL 
ar_find_dev(		
    int32_t flags							
    ) {									

    FDBUG ( ( _arS, "ar_find_dev(): calling _ar_find_dev ( %d )\n", flags ) );

    return _ar_find_dev ( flags );
}

WIN32DLL_API int32_t STDCALL 
ar_find_dev_name(	
    char *name							
    ) {									

    return _ar_find_dev_name ( name );
}

WIN32DLL_API int32_t STDCALL 
ar_xruns(	    // Get xrun count
    int32_t dev				    // - device identifier
    ) {

    return _ar_xruns( dev );
}

/*
Get the number of devices
*/
WIN32DLL_API int32_t STDCALL 
ar_num_devs ( void ) {
    return _ar_num_devs();
}

/*
Opens both input and output channels.  To open just input or
output, use the following functions (for readibility):
	ar_in_open()
	ar_out_open()
Or, just pass a 0 in whichever channel type is not used.
*/
WIN32DLL_API int32_t STDCALL 
ar_io_open(	
	int32_t dev,						
	double rate,					
	int32_t in_chan,				 	
	int32_t out_chan				 	
    ) {								

    FDBUG ( ( _arS, "ar_io_open(): dev [%d] rate [%lf] in_chan [%d] out_chan [%d]\n", dev, rate, in_chan, out_chan ) );

    return _ar_io_open ( dev, rate, in_chan, out_chan );
}

/*
Opens both input and output channels with channel offset.
*/
WIN32DLL_API int32_t STDCALL 
ar_io_open_off(	
	int32_t dev,						
	double rate,					
	int32_t in_chan,				 	
	int32_t out_chan,
	int32_t chnoff_i,				 	
	int32_t chnoff_o				 	
    ) {								

    FDBUG ( ( _arS, "ar_io_open_off(): dev [%d] rate [%lf] in_chan [%d] out_chan [%d] chnoff_i [%d] chnoff_o [%d]\n", dev, rate, in_chan, out_chan, chnoff_i, chnoff_o ) );

    return _ar_io_open_off ( dev, rate, in_chan, out_chan, chnoff_i, chnoff_o );
}

/*
Just calls _ar_io_open() setting input channels to zero.
Don't even need to go to Steve's C function for this case.
*/
WIN32DLL_API int32_t STDCALL 
ar_out_open(	
    int32_t dev,						
    double rate,					
    int32_t out_chan				 	
    ) {								

    return _ar_io_open ( dev, rate, 0, out_chan );
}

/*
Just calls _ar_io_open() setting output channels to zero.
*/
WIN32DLL_API long STDCALL 
ar_in_open(	
    long dev,						
    double rate,					
    long in_chan				 	
    ) {								

    return _ar_io_open ( dev, rate, in_chan, 0 );
}

/*
Wipes out the DLL internal pointers, then calls the 
close function for the chosen protocol ( WDM/ASIO )
*/
WIN32DLL_API int32_t STDCALL 
ar_io_close(	
    int32_t dev 						
    ) {								

    if ( in_data_array != NULL ) {
	FDBUG ( ( _arS, "ar_io_close(): freeing in_data_array array.\n" ) );
	free ( in_data_array );
	in_data_array = NULL;
    }
    if ( out_data_array != NULL ) {
	FDBUG ( ( _arS, "ar_io_close(): freeing out_data_array array.\n" ) );
	free ( out_data_array );
	out_data_array = NULL;
    }

    return _ar_io_close ( dev );
}

WIN32DLL_API int32_t STDCALL 
ar_io_prep (	
    int32_t dev,
    int32_t *in_data[],					
    int32_t *out_data[],					
    int32_t size[], 				 		
    int32_t nseg,					 		
    int32_t tseg					 		
    ) {									

    return _ar_io_prep ( dev, in_data, out_data, size, nseg, tseg );
}

WIN32DLL_API int32_t STDCALL 
ar_io_prepare (	
    int32_t dev,
    int32_t *in_data[],					
    int32_t *out_data[],					
    int32_t size[], 				 		
    int32_t nseg,					 		
    int32_t nswp					 		
    ) {									

    return _ar_io_prep ( dev, in_data, out_data, size, nseg, nswp * nseg );
}

/*
Just calls _ar_io_prepare() setting in_data to null.
Don't even need to go to Steve's C function for this case.
*/
WIN32DLL_API int32_t STDCALL 
ar_out_prepare (	
	int32_t dev,
	int32_t *out_data[],					
	int32_t size[], 				 		
	int32_t nseg,					 		
	int32_t nswp					 		
	) {									

    return _ar_io_prep ( dev, NULL, out_data, size, nseg, nswp * nseg );
}

WIN32DLL_API int32_t STDCALL 
ar_io_start(	
    int32_t dev 					 	
    ) {								

    FDBUG ( ( _arS, "ar_io_start(): dev [%d]\n", dev ) );

    return _ar_io_start ( dev );
}

WIN32DLL_API int32_t STDCALL 
ar_io_stop(	
    int32_t dev 					 	
    ) {								

    FDBUG ( ( _arS, "ar_io_stop(): dev [%d]\n", dev ) );

    return _ar_io_stop ( dev );
}

WIN32DLL_API int32_t STDCALL 
ar_set_fmt(	
    int32_t dev, 					 	
    int32_t *fmt 					 	
    ) {								

    FDBUG ( ( _arS, "ar_set_fmt(): dev [%d] nbps [%d] ntlv [%d]\n", dev, fmt[0], fmt[1] ) );

    return _ar_set_fmt ( dev, fmt );
}

WIN32DLL_API int32_t STDCALL 
ar_get_fmt(	
    int32_t dev, 					 	
    int32_t *fmt 					 	
    ) {								

    return _ar_get_fmt ( dev, fmt );
}

WIN32DLL_API long STDCALL 
ar_get_gdsr(
    long dev
    ) {								

    return _ar_get_gdsr ( dev );
}

WIN32DLL_API int32_t STDCALL 
ar_set_xfer(		
    int32_t dev, 							
    void (*in_xfer)(int32_t),				
    void (*out_xfer)(int32_t)			 	
    ) {									

    return _ar_set_xfer ( dev, in_xfer, out_xfer );
}

WIN32DLL_API int32_t STDCALL 
ar_io_cur_seg(	
    int32_t dev 							
    ) {

    return _ar_io_cur_seg ( dev );
}

WIN32DLL_API int32_t STDCALL 
ar_set_latency(	
    int32_t dev,
    int32_t nsmp							
    ) {

    return _ar_set_latency ( dev, nsmp );
}

/*
Ordinarily the calling program polls to see when a segment 
has finished.  This function provides a different methodology 
where the looping is done internal to the DLL, then control is
released to the calling program only after the segment is 
finished.

Because this function resides within a windows DLL, an escape
mechanism has been provided.  Specifically, hit the ESC key
any time the sound engine is running to stop.
*/
WIN32DLL_API int32_t STDCALL 
ar_io_wait_seg(	
    int32_t dev 							
    ) {									

    long		intCurSegment;

    /* 
    Get whatever segment we are currently on.  Assuming the granularity 
    isn't too great, i.e. it wouldn't have changed between returning and 
    coming back.

    I thought I had this as a static variable once.  Not sure
    why it isn't now, other than having to reset it.
    */
    long intSegmentHold = _ar_io_cur_seg ( dev );
    FDBUG ( ( _arS, "ar_io_wait_seg(): intSegmentHold [%d]\n", intSegmentHold ) );

    while ( 1 ) {
	intCurSegment = _ar_io_cur_seg ( dev );
	if ( intCurSegment != intSegmentHold || intCurSegment < 0 ) {
	    FDBUG ( ( _arS, "ar_io_wait_seg(): intCurSegment [%d] <> intSegmentHold [%d]\n", intCurSegment, intSegmentHold ) );
	    break;
	}

#ifdef _WINDOWS_
	/* Allows the user to hit the ESC key to exit the loop */
	if ( GetAsyncKeyState (VK_ESCAPE) & 0x8001 )
	    return -1;
#endif
    }

    return intCurSegment;
}


WIN32DLL_API void STDCALL 
ar_err_msg(	
    int32_t code,			// error code
    char *msg,			// message array
    int32_t len			// array length
    ) {								

    _ar_err_msg ( code, msg, len );
}

WIN32DLL_API int32_t STDCALL 
ar_dev_name(
    int32_t dev,			// - device identifier
    char *name,			// name array
    int32_t len								
    ) {									

    return _ar_dev_name ( dev, name, len );
}

WIN32DLL_API double STDCALL 
ar_get_rate(
    int32_t dev
    ) {					

    return _ar_get_rate ( dev );
}

WIN32DLL_API double STDCALL 
ar_adjust_rate(
    int32_t dev,
    double rate
    ) {									

    return (double) _ar_adjust_rate ( dev, rate );
}

WIN32DLL_API void STDCALL 
ar_get_sfs(
    int32_t dev,
    double *i_sfs,					
    double *o_sfs					
    ) {

    _ar_get_sfs ( dev, i_sfs, o_sfs );
}

WIN32DLL_API void STDCALL 
ar_set_sfs(
    int32_t dev,
    double *i_sfs,					
    double *o_sfs					
    ) {

    _ar_set_sfs ( dev, i_sfs, o_sfs );
}

WIN32DLL_API int32_t STDCALL
ar_get_cardinfo(int32_t di, CARDINFO * ci)
{
    return (_ar_get_cardinfo(di, ci));
}

WIN32DLL_API void STDCALL 
ar_get_vfs(
    int32_t dev,
    double *da_vfs,					
    double *ad_vfs					
    ) {

    _ar_get_vfs ( dev, da_vfs, ad_vfs );
}

WIN32DLL_API void STDCALL 
ar_set_vfs(
    int32_t dev,
    double *da_vfs, 				
    double *ad_vfs					
    ) {

    _ar_set_vfs ( dev, da_vfs, ad_vfs );
}

WIN32DLL_API void STDCALL 
ar_close_all(		    // Close all devices
    ) {

    _ar_close_all();
}			    //

WIN32DLL_API void STDCALL 
ar_wind(		    // Specify a window to receive messages
    int32_t wind		    // - handle to window
    ) {

    _ar_wind( wind );
}			    //

WIN32DLL_API int32_t STDCALL 
ar_out_seg_fill(
    int32_t dev									// - device identifier
    ) {

    return _ar_out_seg_fill ( dev );
}

/*
Returns the version of the underlying ARSC code.
*/
WIN32DLL_API char * STDCALL 
ar_version ( void ) {									

    return _ar_version();
}

#endif //WRAP

/**************************************************************************/

/*
VB.NET, and perhaps other languages, doesn't seem able to send 
a pointer to an array or a pointer to a pointer.  I tried using 
Tom's VarPtr trick, but this didn't work since VarPtr is different 
in VB.NET than in prior releases of VB.  The pinning memory model 
method didn't seem to give valid pointers.

This wrapper assumes a pointer to a single block of memory
then re-defines it to the ar_io_prepare() arguments.  This
has been tested and works well with VB.NET, MatLab, and 
even C#.

A visual interpretation may help to clarify how this works.
Consider a scenario of 3 segments (0,1,2) and 2 channels (0,1)
and that we are considering output only.  The first segment
is 1024; the second is 2048; the third is 1024 samples.

		  seg 0               seg 1               seg2
		--------		----------------		--------
		| ch 0 |		|     ch 0     |		| ch 0 |
		--------		----------------		--------
		| ch 1 |		|     ch 1     |		| ch 1 |
		--------		----------------		--------
		0     1024      0             2048      0     1024

** FIGURE 1 **

Figure 1 shows the segments as separate entities.  The underlying
C library only needs pointers, so this layout shows the segments
in a logical train of left to right.

Channels are interleaved in the underlying code.  It is useful
to think of channels as being layers of the segment.

This next figure shows how this function takes the data as 
one contiguous block.

-------------------------------------------------
| seg 0 | seg 0 | seg 1 | seg 1 | seg 2 | seg 2 |
| ch 0  | ch 1  | ch 0  | ch 1  | ch 0  | ch 1  |
-------------------------------------------------

** FIGURE 2 **

The sizes array is populated with segment lengths.  This is 
independent of the number of channels.  That is, if you have
1 or 1000 channels, the segment length must be the same.

For this example, sizes[3] = { 1024, 2048, 1024 }
*/
WIN32DLL_API long STDCALL 
ar_io_prepare_vb (	
    long dev,
    void *in_data,					
    void *out_data,					
    long size[], 				 		
    long nseg,					 		
    long nswp					 		
    ) {									
	
    long    rvalue;
    long    intChannel;
    long    intSegment;
    long    *ptrIn = (long *) in_data;
    long    *ptrOut = (long *) out_data;
    long    intOutputChannels = _ardev[dev]->ncda;	/* Output channels */
    long    intInputChannels  = _ardev[dev]->ncad;	/* Input channels */
    bool    bolInterleaved = (bool) _arfmt[dev].a_ntlv;	/* To interleave or not to interleave */
    long    i;

    FDBUG ( ( _arS, "ar_io_prepare_vb(): dev [%d] in_data [%p] out_data [%p] size [%p] nseg [%d] nswp [%d] ntlv [%d]\n", 
    	dev, in_data, out_data, size, nseg, nswp, bolInterleaved ) );

    // Show the sizes of the segments
    for ( i = 0; i < nseg; i++ ) {
    	FDBUG ( ( _arS, "ar_io_prepare_vb(): seg [%d] size [%d] samples . . . [%d] bytes\n", 
	    i, 
	    size[i], 
	    size[i] * _arfmt[dev].a_nbps * intOutputChannels ) );
    }

    /* Allocate memory for input pointers */
    if ( in_data != NULL ) {
	FDBUG ( ( _arS, "ar_io_prepare_vb(): allocation of in_data_array.  In channels [%d] nseg [%d] allocating [%d] bytes\n", 
	    intInputChannels, 
	    nseg,
	    (bolInterleaved ? nseg : intInputChannels * nseg) * sizeof (long *)) );

	in_data_array = (long **) calloc ( bolInterleaved ? nseg : intInputChannels * nseg, sizeof (long *) );
    } else
	in_data_array = NULL;

    /* Allocate memory for output pointers */
    if ( out_data != NULL ) { 
	FDBUG ( ( _arS, "ar_io_prepare_vb(): allocation of out_data_array . . . \n" ) );
	FDBUG ( ( _arS, ". . . Out channels [%d] nseg [%d] ntlv [%d] long [%d] . . . allocating [%d] bytes\n", 
	    intOutputChannels, 
	    nseg,
	    bolInterleaved,
	    sizeof( long),
	    (bolInterleaved ? nseg : intOutputChannels * nseg) * sizeof (long *)) );

	out_data_array = (long **) calloc ( bolInterleaved ? nseg : intOutputChannels * nseg, sizeof (long *) );
    } else {
	out_data_array = NULL;
    }

    /* 
    There will be equal number of segments for input & output, regardless of channel count.
    If the data is passed interleaved, then channels are not considered, i.e. segment 
    boundaries are as is.
    */
    for ( intSegment = 0; intSegment < nseg; intSegment++ ) {
	/* Input channels */
	if ( in_data != NULL ) {
	    if ( bolInterleaved ) {
		FDBUG ( ( _arS, "IN[%d]> %p (seg [%d])\n", 
		    intSegment, 
		    ptrIn,
		    intSegment ) );
		in_data_array[intSegment]  = (long *) ( ptrIn );
		ptrIn += size[intSegment] * intInputChannels;
	    } else {
		for ( intChannel = 0; intChannel < intInputChannels; intChannel++ ) {
		    FDBUG ( ( _arS, "IN[%d]> %p (seg [%d] / ch[%d])\n", 
			intSegment * intInputChannels + intChannel, 
			ptrIn,
			intSegment,
			intChannel ) );
		    in_data_array[intSegment * intInputChannels + intChannel]  = (long *) ( ptrIn );
		    ptrIn += size[intSegment];
		} // rof
	    } // fi
	} // fi

	/* Output channels */
	if ( out_data != NULL ) {
	    if ( bolInterleaved ) {
		FDBUG ( ( _arS, "OUT[%d]> %p (seg [%d])\n", 
		    intSegment, 
		    ptrOut,
		    intSegment ) );
		out_data_array[intSegment]  = (long *) ( ptrOut );
		ptrOut += size[intSegment] * intOutputChannels;
	    } else {
		// Non-interleaved
		for ( intChannel = 0; intChannel < intOutputChannels; intChannel++ ) {
		    FDBUG ( ( _arS, "OUT[%d]> %p (seg [%d] / ch[%d])\n", 
			intSegment * intOutputChannels + intChannel, 
			ptrOut,
			intSegment,
			intChannel ) );
		    out_data_array[intSegment * intOutputChannels + intChannel] = (long *) ( ptrOut );
		    ptrOut += size[intSegment];
		} // rof
	    } // fi
	}
    }

    FDBUG ( ( _arS, "ar_io_prepare_vb(): About to call _ar_io_prepare()\n" ) );
    //return _ar_io_prepare ( dev, in_data_array, out_data_array, size, nseg, nswp );
    rvalue =  _ar_io_prep ( dev, in_data_array, out_data_array, size, nseg, nswp * nseg );
    FDBUG ( ( _arS, "ar_io_prepare_vb(): CALLED _ar_io_prepare()\n" ) );
    return rvalue;
}

/*
Wrapper to ar_io_prepare_vb() to skip input channels.
*/
WIN32DLL_API long STDCALL 
ar_out_prepare_vb (	
    long dev,				// device identifier
    void *out_data,			// pointer to output data
    long size[],			// array of segment sizes
    long nseg,				// number of segments
    long nswp				// number of sweeps
    ) {

    return ar_io_prepare_vb ( dev, NULL, out_data, size, nseg, nswp );
}

/*
These two functions used as a go-between for stdcall and cdecl.
*/
static void 
in_xfer_cdecl (long aintInSegment) {

    //FDBUG ( ( _arS, "in_xfer_cdecl(): aintInSegment [%d]\n", aintInSegment ) );

    // Call the stdcall version
    (*in_xfer_stdcall)(aintInSegment);
}

static void 
out_xfer_cdecl (long aintOutSegment) {

    //FDBUG ( ( _arS, "out_xfer_cdecl(): aintOutSegment [%d]\n", aintOutSegment ) );

    // Call the stdcall version
    (*out_xfer_stdcall)(aintOutSegment);
}

/*
Special function for stdcall transfer (xfer) functions.
*/
WIN32DLL_API long STDCALL 
ar_set_xfer_stdcall (
    long dev, 							
    void (STDCALL *in_xfer)(long),				
    void (STDCALL *out_xfer)(long)			 	
    ) {									

    void *ptrInCdecl = &in_xfer_cdecl;
    void *ptrOutCdecl = &out_xfer_cdecl;

    FDBUG ( ( _arS, "ar_set_xfer_stdcall(): dev [%d] [%p] [%p]\n", dev, in_xfer, out_xfer ) );

    // Set the stdcall Xfer functions that are defined above in the DLL
    in_xfer_stdcall = in_xfer;
    out_xfer_stdcall = out_xfer;

    if ( ! in_xfer ) ptrInCdecl = NULL;
    if ( ! out_xfer ) ptrOutCdecl = NULL;

    // Then set the functions in the API code with functions defined in the DLL
    FDBUG ( ( _arS, "ar_set_xfer_stdcall(): returning [%p] [%p]\n", ptrInCdecl, ptrOutCdecl ) );
    return _ar_set_xfer ( dev, ptrInCdecl, ptrOutCdecl );
}

/*
MatLab cannot pass anything by reference, so this
function will return the error message for a given code.

This function will work for other languages, but we 
recommend using ar_err_msg() when possible.
*/
WIN32DLL_API char * STDCALL 
ar_err_msg_matlab (	
    long code			// error code
    ) {								

    static char	strErrorMessage[ARSC_MSGLEN];

    _ar_err_msg ( code, strErrorMessage, sizeof(strErrorMessage) );

    return strErrorMessage;
}

/*
MatLab cannot pass anything by reference, so this
function will return the device name.
*/
WIN32DLL_API char * STDCALL 
ar_dev_name_matlab (	
    long dev			// device identifier
    ) {								

    static char	strDeviceName[ARSC_NAMLEN];

    _ar_dev_name ( dev, strDeviceName, sizeof(strDeviceName) );

    return strDeviceName;
}

/*
Returns the version of the DLL and underlying ARSC code.
*/
WIN32DLL_API long STDCALL 
ar_version_dll (
    char *astrVersionInfo,	// pointer to string that holds version information
    long len			// Length of string
    ) {
    static char *vs1 = "ARSC Version Information:\n\n    ";
    static char *vs2 = "\n    ";
    static char *vs3 = "\n    ";
    static char *vs4 = "\n  \n";
    static int vslen = sizeof(vs1) + sizeof(vs2) + sizeof(vs3) + sizeof(vs4)
	+ sizeof(VER) + sizeof(NOTICE) + sizeof(RIGHTS);

    if ( len > vslen ) {
	strcpy ( astrVersionInfo, vs1);
	strcat ( astrVersionInfo, VER );
	strcat ( astrVersionInfo, vs2);
	strcat ( astrVersionInfo, NOTICE);
	strcat ( astrVersionInfo, vs3);
	strcat ( astrVersionInfo, RIGHTS);
	strcat ( astrVersionInfo, vs4);
	return ( (long) strlen ( astrVersionInfo ) );
    } else {
        strncpy(astrVersionInfo, VER, len);	// Short version
	return 0;
    }
}

/* 
Fill a memory block with a tone of frequency.
*/
WIN32DLL_API long STDCALL 
ar_fill_tone (	
    long *ptrBlock,				// Output
    long aintBlockSize,				// Size of the memory block in bytes
    double adblDesiredFrequency,		// Frequency in Hz
    double adblSampleRate,			// e.g. 44100.
    double adblAmplitude			// sine wave peak amplitude, e.g. 2^31 - 1
    ) {			

    long		intSample;
    double		dblActualFrequency = (long) ( adblDesiredFrequency * (double) aintBlockSize / adblSampleRate );

    DBUG ( ( "fill_tone(): block size [%d] desired f [%f] actual f [%f] sample rate [%f] amplitude [%f]\n",	aintBlockSize, 
	adblDesiredFrequency,
	dblActualFrequency,
	adblSampleRate,
	adblAmplitude ) );

    for ( intSample = 0; intSample < aintBlockSize; intSample++ )
	ptrBlock[intSample] = (long) ( adblAmplitude * sin ( 2.0 * M_PI * dblActualFrequency * (double) intSample / (double) aintBlockSize ) );

    return 1;
}

/*
This is a dummy function that I can use to pass a pointer
and return something based upon it.
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/cpguide/html/cpconpinvokelibdll.asp?frame=true
*/
WIN32DLL_API long STDCALL 
ar_pass_pointer (
    long (STDCALL *in_xfer)( long )
    ) {

    // Call it to see if breakpoint is reached.
    long res = (*in_xfer)(7);

    // This sets a global
    //fnX = in_xfer;

    return res;
}

/**************************************************************************/

/*
Some internal helper functions.
These may one day be exported.
*/


/*
Checks an error code.
arsc API returns non-zero results if an error occurs.
*/
int32 
pArscError ( int32 aintArscError ) {
    char    strErrorMessage[1024];

    if ( aintArscError != 0 ) {
	// Nice to see the error
	ar_err_msg ( aintArscError, strErrorMessage, sizeof(strErrorMessage) );
	PRINT ( ( "*** ERROR ***\n%s\n*** ERROR ***\n", strErrorMessage ) );

	// I suppose I could just die here
	//exit ( aintArscError );
    }

    // Or return the code
    return aintArscError;
}

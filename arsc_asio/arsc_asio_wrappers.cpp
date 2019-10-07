/*
2005 Mar 17
This code needs to be CPP to interface to the SDK.

*/

#ifdef ASIO

#include <objbase.h>										/* For working w/ CLSIDs */
#include "asiosys.h"										/* platform definition from Steinberg SDK */
#include "asio.h"											/* from Steinberg SDK */
#include "..\arsc_common.h"									/* Common to .dll, .exe, and/or .lib */

#ifndef __GNUC__
#define EXTERN_CPP extern "C"
#else
#define EXTERN_CPP
#endif // __GNUC__

// External prototypes
bool loadAsioDriver ( char *name );

/*
This function is in CPP because it calls functions in the
Steinberg SDK which are written in CPP.
*/
EXTERN_CPP bool SDKLoadAsioDriver ( char *name ) {
	CoInitialize(0);
	return loadAsioDriver(name);
}

EXTERN_CPP bool SDKAsioExit ( ) {
	ASIOError		aseError = ASE_OK;

	if ( ( aseError = ASIOExit ( ) ) == ASE_OK )
		return true;
	else {
		DBUG ( ( "ASIOExit() returned [%ld]\n", aseError ) );
		return false;
	}
}

EXTERN_CPP bool SDKAsioInit ( ASIODriverInfo *info ) {
	ASIOError		aseError = ASE_OK;

	if ( ( aseError = ASIOInit ( info ) ) == ASE_OK )
		return true;
	else {
		DBUG ( ( "ASIOInit() returned [%ld]\n", aseError ) );
		return false;
	}
}

EXTERN_CPP bool SDKAsioGetChannels ( long *alngInputChannels, long *alngOutputChannels ) {
	ASIOError		aseError = ASE_OK;

	if ( ( aseError = ASIOGetChannels ( alngInputChannels, alngOutputChannels ) ) == ASE_OK )
		return true;
	else {
		DBUG ( ( "AsioGetChannels() returned [%ld]\n", aseError ) );
		return false;
	}
}

EXTERN_CPP bool SDKAsioCanSampleRate ( ASIOSampleRate aSampleRate ) {

	ASIOError aseError = ASIOCanSampleRate ( aSampleRate );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			break;
		case ASE_NoClock:
            //DBUG ( ( "Sample rate [%f] not supported\n", aSampleRate ) );
			bolReturn = false;
			break;
		case ASE_NotPresent:
			//DBUG ( ( "Sample rate [%f] not supported\n", aSampleRate ) );
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}


EXTERN_CPP bool SDKAsioSetSampleRateImpl ( ASIOSampleRate aSampleRate ) {
	ASIOError aseError = ASIOSetSampleRate ( aSampleRate );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			break;
		case ASE_NoClock:
            DBUG ( ( "Sample rate [%f] is unknown\n", aSampleRate ) );
			bolReturn = false;
			break;
		case ASE_InvalidMode:
			DBUG ( ( "Sample rate [%f] is non-zero, but the clock is set to external.\n", aSampleRate ) );
			bolReturn = false;
			break;
		case ASE_NotPresent:
			DBUG ( ( "No input/output is present.\n" ) );
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

EXTERN_CPP bool SDKAsioGetBufferSize (	long *alngMinBufferSize,
										long *alngMaxBufferSize,
										long *aslngPreferredBufferSize,
										long *alngGranularity ) {

	ASIOError aseError = ASIOGetBufferSize (alngMinBufferSize, 
											alngMaxBufferSize, 
											aslngPreferredBufferSize, 
											alngGranularity );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			// This might be useful information to the developer
			DBUG ( ( "min [%ld] max [%ld] pref [%ld] gran [%ld].\n",	*alngMinBufferSize, 
																		*alngMaxBufferSize, 
																		*aslngPreferredBufferSize, 
																		*alngGranularity ) );
			break;
		case ASE_NotPresent:
			DBUG ( ( "No input/output is present.\n" ) );
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;

}

EXTERN_CPP bool SDKAsioGetChannelInfo ( ASIOChannelInfo *info ) {
	ASIOError aseError = ASIOGetChannelInfo ( info );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			break;
		case ASE_NotPresent:
			DBUG ( ( "No input/output is present.\n" ) );
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

EXTERN_CPP bool SDKAsioCreateBuffers ( ASIOBufferInfo *bufferInfos,
										long numChannels,
										long bufferSize,
										ASIOCallbacks *callbacks ) {

	ASIOError aseError = ASIOCreateBuffers (bufferInfos,
											numChannels,
											bufferSize,
											callbacks );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			break;
		case ASE_NotPresent:
			DBUG ( ( "No input/output is present.\n" ) );
			bolReturn = false;
			break;
		case ASE_NoMemory:
			DBUG ( ( "Not enough memory available.\n" ) );
			bolReturn = false;
			break;
		case ASE_InvalidMode:					// Documentation says this
		case ASE_InvalidParameter:				// I think it may be this.
			DBUG ( ( "Either bufferSize is not supported, or one or more of the bufferInfos elements contain invalid settings.\n" ) );
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

EXTERN_CPP bool SDKAsioOutputReady ( ) {

	if ( ASIOOutputReady() == ASE_OK )
		return true;
	else
		return false;
}

/*
Inquires the sample position/time stamp pair.
*/
EXTERN_CPP bool SDKAsioGetSamplePosition ( ASIOSamples *sPos, ASIOTimeStamp *tStamp ) {
	
	ASIOError aseError = ASIOGetSamplePosition ( sPos, tStamp );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			break;
		case ASE_NotPresent:
			DBUG ( ( "No input/output is present.\n" ) );
			bolReturn = false;
			break;
		case ASE_SPNotAdvancing:
			DBUG ( ( "No clock; sample position not advancing.\n" ) );
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

/*
Returns the input and output latencies.  This includes device specific
delays like FIFOs, etc.
*/
EXTERN_CPP bool SDKAsioGetLatencies ( long *inputLatency, long *outputLatency ) {

	ASIOError aseError = ASIOGetLatencies ( inputLatency, outputLatency );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			// Nice to let the developer know
			DBUG ( ( "SDKAsioGetLatencies(): (input: %d, output: %d)\n", *inputLatency, *outputLatency ) );
			break;
		case ASE_NotPresent:
			DBUG ( ( "No input/output is present.\n" ) );
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

/*
Clear buffers
*/
EXTERN_CPP bool SDKAsioDisposeBuffers ( void ) {

	ASIOError aseError = ASIODisposeBuffers ( );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			break;
		case ASE_NotPresent:
			DBUG ( ( "No input/output is present.\n" ) );
			bolReturn = false;
			break;
		case ASE_InvalidMode:
			DBUG ( ( "No buffers were ever prepared.\n" ) );
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

/*
Stop the driver
*/
EXTERN_CPP bool SDKAsioStop ( void ) {

	ASIOError aseError = ASIOStop ( );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			break;
		case ASE_NotPresent:
			DBUG ( ( "No input/output is present.\n" ) );
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

/*
Start input and output processing synchronously.

Note: There is no restriction on the time that ASIOStart() takes to 
perform ( that is, it is not considered a real-time trigger ).
*/
EXTERN_CPP bool SDKAsioStart ( void ) {

	ASIOError aseError = ASIOStart ( );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			break;
		case ASE_NotPresent:
			DBUG ( ( "No input/output is present.\n" ) );
			bolReturn = false;
			break;
		case ASE_HWMalfunction:
			DBUG ( ( "The hardware failed to start.\n" ) );
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

#endif /* ASIO */
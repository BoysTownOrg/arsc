/*
2005 Mar 17
This code needs to be CPP to interface to the SDK.

*/

#ifdef ASIO

#include "arsc_asio_wrappers.h"
#include <objbase.h>

// External prototypes
bool loadAsioDriver ( char *name );

/*
This function is in CPP because it calls functions in the
Steinberg SDK which are written in CPP.
*/
int SDKLoadAsioDriver ( char *name ) {
	CoInitialize(0);
	return loadAsioDriver(name);
}

int SDKAsioExit ( ) {
	ASIOError		aseError = ASE_OK;

	if ( ( aseError = ASIOExit ( ) ) == ASE_OK )
		return true;
	else {
		return false;
	}
}

int SDKAsioInit ( ASIODriverInfo *info ) {
	ASIOError		aseError = ASE_OK;

	if ( ( aseError = ASIOInit ( info ) ) == ASE_OK )
		return true;
	else {
		return false;
	}
}

int SDKAsioGetChannels ( long *alngInputChannels, long *alngOutputChannels ) {
	ASIOError		aseError = ASE_OK;

	if ( ( aseError = ASIOGetChannels ( alngInputChannels, alngOutputChannels ) ) == ASE_OK )
		return true;
	else {
		return false;
	}
}

int SDKAsioCanSampleRate ( double aSampleRate ) {

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


int SDKAsioSetSampleRateImpl ( double aSampleRate ) {
	ASIOError aseError = ASIOSetSampleRate ( aSampleRate );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			break;
		case ASE_NoClock:
			bolReturn = false;
			break;
		case ASE_InvalidMode:
			bolReturn = false;
			break;
		case ASE_NotPresent:
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

int SDKAsioGetBufferSizeImpl (	long *alngMinBufferSize,
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
			break;
		case ASE_NotPresent:
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;

}

int SDKAsioGetChannelInfo ( ASIOChannelInfo *info ) {
	ASIOError aseError = ASIOGetChannelInfo ( info );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			break;
		case ASE_NotPresent:
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

int SDKAsioCreateBuffersImpl ( ASIOBufferInfo *bufferInfos,
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
			bolReturn = false;
			break;
		case ASE_NoMemory:
			bolReturn = false;
			break;
		case ASE_InvalidMode:					// Documentation says this
		case ASE_InvalidParameter:				// I think it may be this.
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

int SDKAsioOutputReadyImpl ( ) {

	if ( ASIOOutputReady() == ASE_OK )
		return true;
	else
		return false;
}

/*
Inquires the sample position/time stamp pair.
*/
int SDKAsioGetSamplePosition ( ASIOSamples *sPos, ASIOTimeStamp *tStamp ) {
	
	ASIOError aseError = ASIOGetSamplePosition ( sPos, tStamp );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			break;
		case ASE_NotPresent:
			bolReturn = false;
			break;
		case ASE_SPNotAdvancing:
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

/*
Returns the input and output latencies.  This includes device specific
delays like FIFOs, etc.
*/
int SDKAsioGetLatenciesImpl ( long *inputLatency, long *outputLatency ) {

	ASIOError aseError = ASIOGetLatencies ( inputLatency, outputLatency );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			// Nice to let the developer know
			break;
		case ASE_NotPresent:
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

/*
Clear buffers
*/
int SDKAsioDisposeBuffers ( void ) {

	ASIOError aseError = ASIODisposeBuffers ( );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			break;
		case ASE_NotPresent:
			bolReturn = false;
			break;
		case ASE_InvalidMode:
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

/*
Stop the driver
*/
int SDKAsioStop ( void ) {

	ASIOError aseError = ASIOStop ( );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			break;
		case ASE_NotPresent:
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
int SDKAsioStartImpl ( void ) {

	ASIOError aseError = ASIOStart ( );
	bool bolReturn = false;

	switch ( aseError ) {
		case ASE_OK:
			bolReturn = true;
			break;
		case ASE_NotPresent:
			bolReturn = false;
			break;
		case ASE_HWMalfunction:
			bolReturn = false;
			break;
	} /* hctiws */

	return bolReturn;
}

#endif /* ASIO */
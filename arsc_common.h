/*
arsc_common.h

This file has common definitions and structures that the .exe calling and the .dll 
can use for C programming.  For VB or other languages, there are other means of
connecting to the DLL.

copy H:\arsc\arsc_dll\arsc_dll\Debug\*.dll H:\arsc\arsc_dll\arsc_vb_net_test\bin
copy H:\arsc\arsc_dll\arsc_dll\Debug\*.dll H:\arsc\arsc_dll\arsc_dll_csharp_test\bin\Debug
copy H:\arsc\arsc_dll\arsc_dll\Debug\*.dll H:\arsc\arsc_dll\arsc_dll_csharp_mixing\bin\Debug
copy H:\arsc\arsc_dll\arsc_dll\Debug\*.dll H:\arsc\arsc_dll\arsc_dll_c_mixing_2\Debug
copy H:\arsc\arsc_dll\arsc_dll\Debug\*.dll H:\arsc\arsc_dll\arsc_dll_c_test\Debug
copy H:\arsc\arsc_dll\arsc_dll\Debug\*.dll H:\arsc\arsc_dll\stress_test\Debug

*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#ifndef __GNUC__
#include <windows.h>
#endif // __GNUC__
#include "arsclib.h"

/*
Debugging constants
*/
#define PRINT(x)	{ printf x; fflush(stdout); }
#define DBUG(x)		//PRINT(x)			/* Comment out the PRINT(x) to turn off debugging */
#define DBUG_S(x)	//PRINT(x)			/* Stimulus Detail */
#define DBUG_R(x)	//PRINT(x)			/* Response Detail */

// Used in conjunction with the SYNC_DEBUG_FLAGS environment variable
#define DB_TEXT					1
#define DB_DUMP_CH0				2
#define DB_DUMP_CH1				4
#define DB_DUMP_BUFFER_SWITCH	8

// asio message(s), as wParam for PM_ASIO.	The lParam is always 0 (device 0)
#define AM_ResponseFull			1						// Response block is full
#define AM_StimulusSent			2						// Full stimulus file has been sent
#define AM_BufferSwitch			100						// new buffer index in lParam
#define AM_BufferSwitchTimeInfo	101						// new buffer index in lParam
#define AM_Stop					102						// stop the processing
#define AM_Stopped				103						// processing has stopped

#define	SCALE_32BIT				2147483647				/* 2^31 - 1 (it swings -/+ for 2^32) */
#define	SCALE_24BIT				8388608					/* 2^23 - 1 (it swings -/+ for 2^24) */
#define DLL_VERSION_SIZE		256

/*
Use like sprintf, i.e. send a global "s" and the other stuff, 
e.g.
		FDBUG ( ( _arS, "__________", a, b, c ) );
*/
#define FDBUG(x)	{		\
    if ( _ar_debug ) {			\
        if ( sprintf x > 0 ) {		\
	    PRINT ( ( "%s", _arS ) );	\
        }				\
    }					\
}
#ifndef DEBUG
#undef FDEBUG
#endif
// For FDBUG debugging
extern char _arS[];
extern int  _ar_debug;


/*
This looks messy, but makes getting a single keystroke very clean below.
The relevant code is run after the keyboard buffer is cleared, so there
may be a slight time delay.

I love macros.  This assumes a defined integer:
	int32	intKeyDown = 0;
*/
#define KEYDOWN(vk_code)	((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEYUP(vk_code)		((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)
#define KEYSTROKE(vk_code)				\
    if KEYDOWN ( vk_code )	intKeyDown = vk_code;	\
    if ( intKeyDown == vk_code && KEYUP ( vk_code ) ) {	\
	intKeyDown = 0;

/*
Booleans
*/
#define	true	1
#define false	0

/*
These are handy typedefs to ensure we are talking apples and apples
*/
typedef	int			int32;											/* 32 bit int - -2,147,483,648 < X < 2,147,483,648 */
typedef	double			real64;											/* 64 bit real64 - 15-digit precision */
#ifndef __cplusplus
//typedef	unsigned short int	bool;											/* boolean of 0 or 1 */
typedef	unsigned char	bool;											/* boolean of 0 or 1 */
#endif // _cplusplus


/* 
Helper Function Prototypes
*/
int32 pArscError ( int32 aintArscError );
int32 OpenDebugFile ( FILE **fh, int32 aintChannelNumber, char achrType );

/* siodll.c - sio DLL routines */

#include <stdio.h>
#include "sio.h"
#include "siodll.h"

static float *obp[2];	// array of pointers to each output buffer
static float *ibp[2];	// array of pointers to each input buffer

// ssio_open - Initializes soundcard and internal SIO variables.

FUNC(int) ssio_open(	    // returns non-zero if successful
)
{
    return (sio_open());
}

// ssio_close - Terminate I/O and free any allocated resources.

FUNC(void) ssio_close(    // returns card number
)
{
    sio_close();
}

// ssio_get_device - Obtain short description of I/O device.

FUNC(void) ssio_get_device(
    char *s		// Device description returned here
)
{
    sio_get_device(s);
}

// ssio_get_info - Obtain one-line of information about I/O device.

FUNC(void) ssio_get_info(
    char *s		// Device information returned here
)
{
    sio_get_info(s);
}

// ssio_get_vfs - Obtain full-scale voltage of ADCs and DACs.

FUNC(void) ssio_get_vfs(
    double *ad_vfs, 	// volts-full-scale for each ADC channel returned 
    double *da_vfs 	// volts-full-scale for each DAC channel returned
)
{
    sio_get_vfs(da_vfs, ad_vfs);
}

// ssio_io - Initiate i/o.

FUNC(void) ssio_io(
    int nskip, 		// number of samples to skip before averaging
    int nswps 		// maximum number of sweeps while averaging
)
{
    sio_io(nskip, nswps, 0, 0);
}

// ssio_set_device - select I/O device.

FUNC(int) ssio_set_device(
    int n		// card number
)
{
    return (sio_set_device(n));
}

// ssio_set_device - select I/O device.

FUNC(int) ssio_set_latency(
    int n		// number of samples
)
{
    return (sio_set_latency(n));
}

// ssio_set_rate - Specify sample rate.

FUNC(double) ssio_set_rate(	// returns actual sample rate
    double r		// desired sample rate (samples/second)
)
{
    return (sio_set_rate(r));
}

// ssio_set_io - Specify i/o buffers simply.

FUNC(void) ssio_set_io(
    float *out1,	// channel 1 out
    float *out2,	// channel 2 out
    float *in1,		// channel 1 in
    float *in2,		// channel 2 in
    int nsamp 		// size of I/O buffers (samples)
)
{
    static int inc[2] = {1,1};	// array of increments for each output buffer

    obp[0] = out1;
    obp[1] = out2;
    ibp[0] = in1;
    ibp[1] = in2;
    sio_set_output(2, 1, inc, obp);
    sio_set_input(2, 1, inc, NULL);
    sio_set_average(ibp, NULL);
    sio_set_size(nsamp, 0, 0);
}

// ssio_set_att_out - Specify attenuation on output all channels.

FUNC(double) ssio_set_att_out(	// returns actual output attenuation (dB)
    double a			// desired output attenutation (dB)
)
{
    return(sio_set_att_out(a));
}

// ssio_set_vfs - Specify full-scale voltage on ADCs and DACs

FUNC(void) ssio_set_vfs(
    double *ad_vfs, 	// array set to volts-full-scale for each ADC
    double *da_vfs	// array set to volts-full-scale for each DAC
)
{
    sio_set_vfs(da_vfs, ad_vfs);
}

/**************************************************************************/

#ifdef WIN32

/* LibMain - entry point for the DLL */

int _stdcall
LibMain(void *hinstDll, unsigned long dwReason, void *reserved)
{
	return(1);
}

#endif

/**************************************************************************/

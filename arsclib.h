/* arsclib.h */
#ifndef ARSCLIB_H
#define ARSCLIB_H

#define ARSC_MSGLEN	    80
#define ARSC_NAMLEN	    40

#define ARSC_PREF_NONE	    0
#define ARSC_PREF_SYNC	    1
#define ARSC_PREF_ASIO	    2
#define ARSC_PREF_OS	    3
#define ARSC_PREF_OUT	    16
#define ARSC_PREF_IN	    32
#define ARSC_PREF_IO	    64

#define ARSC_PREF_ALSA	    ARSC_PREF_OS
#define ARSC_PREF_WIND	    ARSC_PREF_OS

#define ARSC_DATA_UNKNOWN   0
#define ARSC_DATA_U1	    1
#define ARSC_DATA_I2	    2
#define ARSC_DATA_P3	    3
#define ARSC_DATA_I4	    4
#define ARSC_DATA_X3	    5
#define ARSC_DATA_F4	    6
#define ARSC_DATA_M1	    7
#define ARSC_DATA_F8	    8

#define ARSC_GET_LATENCY    9999

#ifndef ARSC_LPTR
#define ARSC_LPTR
#include <stdint.h>
typedef int16_t     SINT2;
typedef int32_t     SINT4;
typedef uint32_t    UINT4;
#endif /* ARSC_LPTR */

#ifdef WIN32
#define WM_ARSC		    (WM_USER+555)
#endif /* WIN32 */

/* CARDINFO */

#define MAX_CT_NAME 40		/* max length of cardType name   */
#define MAXNCT      20		/* max number of soundcard types */
#define MAXNCH      8		/* max number of I/O channels    */
#ifndef M_PI
#define M_PI 3.141592653589793238462643
#endif

typedef struct {
    char    name[MAX_CT_NAME];
    int     bits;
    int     left;
    int     nbps;
    int     ncad;
    int     ncda;
    int     gdsr;
    double  ad_vfs[MAXNCH];
    double  da_vfs[MAXNCH];
} CARDINFO;

/* ARSC API function prototypes */

#ifdef WRAP
#define _API(type)  __declspec(dllexport) type _stdcall
#else	/* WRAP */
#define _API(type)  type
#endif /* WRAP */

_API(char *) ar_version (       /* Get ARSC version string */
    );                          /* > version string */

_API(SINT4) ar_find_dev(        /* Find device given preferences */
    SINT4 flags                 /* - hints about desired device */
    );                          /* > device identifier */

_API(SINT4) ar_find_dev_name(   /* Find device given name */
    char *name                  /* - name string */
    );                          /* > device identifier */

_API(SINT4) ar_io_open(         /* Open device for i/o */
    SINT4 dev,                  /* - device identifier */
    double rate,                /* - desired sampling rate */
    SINT4 in_chan,              /* - desired input channels */
    SINT4 out_chan              /* - desired output channels */
    );                          /* > error code */

_API(SINT4) ar_io_open_off(     /* Open device for i/o */
    SINT4 dev,                  /* - device identifier */
    double rate,                /* - desired sampling rate */
    SINT4 in_chan,              /* - desired input channels */
    SINT4 out_chan,             /* - desired output channels */
    SINT4 chnoff_in,            /* - input channel offset */
    SINT4 chnoff_out            /* - output channel offset */
    );                          /* > error code */

_API(SINT4) ar_io_close(        /* Close device */
    SINT4 dev                   /* - device identifier */
    );                          /* > error code */

_API(SINT4) ar_io_prep(         /* Prepare device for i/o */
    SINT4 dev,                  /* - device identifier */
    void *in_data[],            /* - input data for each segment */
    void *out_data[],           /* - output data for each segment */
    SINT4 size[],               /* - size of each segment */
    SINT4 nseg,                 /* - number of segments buffered */
    SINT4 tseg                  /* - total number of segments */
    );                          /* > error code */

_API(SINT4) ar_io_prepare(      /* Prepare device for i/o */
    SINT4 dev,                  /* - device identifier */
    void *in_data[],            /* - input data for each segment */
    void *out_data[],           /* - output data for each segment */
    SINT4 size[],               /* - size of each segment */
    SINT4 nseg,                 /* - number of segments buffered */
    SINT4 nswp                  /* - number of sweeps */
    );                          /* > error code */

_API(SINT4) ar_io_start(        /* Start i/o */
    SINT4 dev                   /* - device identifier */
    );                          /* > error code */

_API(SINT4) ar_io_stop(         /* Stop i/o */
    SINT4 dev                   /* - device identifier */
    );                          /* > error code */

_API(SINT4) ar_set_fmt(         /* Specify app's data format */
    SINT4 dev,                  /* - device identifier */
    SINT4 *fmt                  /* - data format */
    );                          /* > error code */

_API(SINT4) ar_get_fmt(         /* Get dev's data format */
    SINT4 dev,                  /* - device identifier */
    SINT4 *fmt                  /* - data format */
    );                          /* > error code */

_API(SINT4) ar_set_xfer(        /* Specify app data transfer functions */
    SINT4 dev,                  /* - device identifier */
    void (*in_xfer)(SINT4),     /* - input transfer function */
    void (*out_xfer)(SINT4)     /* - output tranfer function */
    );                          /* > error code */

_API(SINT4) ar_io_cur_seg(      /* Get current segment */
    SINT4 dev                   /* - device identifier */
    );                          /* > unwrapped segment number */

_API(SINT4) ar_out_seg_fill(    /* Refill output segments */
    SINT4 dev                   /* - device identifier */
    );                          /* > error code */

_API(SINT4) ar_io_wait_seg(     /* Wait for segment change */
    SINT4 dev                   /* - device identifier */
    );                          /* > unwrapped segment number */

_API(SINT4) ar_dev_name(        /* Get device name */
    SINT4 dev,                  /* - device identifier */
    char *name,                 /* - name string */
    SINT4 len                   /* - array length */
    );                          /* > error code */

_API(SINT4) ar_xruns(           /* Get xrun count */
    SINT4 dev                   /* - device identifier */
    );                          /* > number of xruns */

_API(SINT4) ar_num_devs(        /* Get number of devices */
    );                          /* > device count */

_API(SINT4) ar_set_latency(     /* Set device latency */
    SINT4 dev,                  /* - device identifier */
    SINT4 nsmp                  /* - desired latency (samples) */
    );                          /* > current latency (samples) */

_API(void) ar_err_msg(          /* Get error message */
    SINT4 err,                  /* - error code */
    char *msg,                  /* - message string */
    SINT4 len                   /* - array length */
    );			    

_API(double) ar_get_rate(       /* Get sampling rate */
    SINT4 dev                   /* - device identifier */
    );                          /* > sampling rate */

_API(double) ar_adjust_rate(    /* Adjust sampling rate */
    SINT4 dev,                  /* - device identifier */
    double rate                 /* > desired rate */
    );                          /* > nearest rate */

/* float-sample functions */

_API(void) ar_get_sfs(          /* get i/o sample-full-scale */
    SINT4 dev,                  /* - device identifier */
    double *i_sfs,              /* - input sample-full-scale */
    double *o_sfs               /* - output sample-full-scale */
    );			    

_API(void) ar_set_sfs(          /* set i/o sample-full-scale */
    SINT4 dev,                  /* - device identifier */
    double *i_sfs,              /* - input sample-full-scale */
    double *o_sfs               /* - output sample-full-scale */
    );			    

/* vfs functions */

_API(void) ar_get_vfs(          /* get convertor volts-full-scale */
    SINT4 dev,                  /* - device identifier */
    double *da_vfs,             /* - DAC volts-full-scale array */
    double *ad_vfs              /* - ADC volts-full-scale array */
    );			    

_API(void) ar_set_vfs(          /* set convertor volts-full-scale */
    SINT4 dev,                  /* - device identifier */
    double *da_vfs,             /* - DAC volts-full-scale array  */
    double *ad_vfs              /* - ADC volts-full-scale array */
    );			    

/* misc functions */

_API(void) ar_close_all(        /* Close all devices */
    );			    

_API(void) ar_wind(             /* Specify a window to receive messages */
    SINT4 wind                  /* - handle to window */
    );			    

/* output (only) functions */

_API(SINT4) ar_out_open(        /* Open device for output */
    SINT4 dev,                  /* - device identifier */
    double rate,                /* - desired sampling rate */
    SINT4 out_chan              /* - desired output channels */
    );                          /* > error code */

_API(SINT4) ar_out_prepare(     /* Prepare device for output */
    SINT4 dev,                  /* - device identifier */
    void *out_data[],           /* - output data for each segment */
    SINT4 size[],               /* - size of each segment */
    SINT4 nseg,                 /* - number of segments */
    SINT4 nswp                  /* - number of sweeps */
    );                          /* > error code */

/* CARDINFO function */

_API(SINT4) ar_get_cardinfo(    /* Get card info */
    SINT4 dev,                  /* - device identifier */
    CARDINFO *ci                /* - pointer to CARDINFO structure */
    );                          /* > card type */

#endif /* ARSCLIB_H */

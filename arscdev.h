/* arscdev.h - ARSC internal data */

#define MAXDEV	    40		/* maximum number of I/O devices */
#define MAXNDT	    2		/* maximum number of device types */
#define MAXNDC	    8		/* maximum number of device channels */
#define MAXNAM	    80		/* maximum device name length */

#define SRLSTSZ	    27		/* sample rate list size */

#ifndef ARSC_LPTR
#define ARSC_LPTR
#include <stdint.h>
typedef int16_t     SINT2;
typedef int32_t     SINT4;
typedef uint32_t    UINT4;
#endif /* ARSC_LPTR */

typedef struct{
    double a_rate;		/* desired sampling rate */
    double ad_vfs[MAXNDC];	/* A/D volts full scale */
    double da_vfs[MAXNDC];	/* D/A volts full scale */
    SINT4 a_chnmsk_i;		/* desired input channel mask */
    SINT4 a_chnmsk_o;		/* desired output channel mask */
    SINT4 a_chnoff_i;		/* desired input channel offset */
    SINT4 a_chnoff_o;		/* desired output channel offset */
    SINT4 a_ncad;		/* desired channels of A/D  */
    SINT4 a_ncda;		/* desired channels of D/A */
    SINT4 chnmsk_i;		/* device input channel mask */
    SINT4 chnmsk_o;		/* device output channel mask */
    SINT4 dev_id;		/* device identifier */
    SINT4 bits;			/* number of converter bits */
    SINT4 left;			/* number of left-shift bits */
    SINT4 nbps;			/* number of bytes per sample */
    SINT4 ntlv;			/* samples interleaved ? */
    SINT4 ncad;			/* device channels of A/D  */
    SINT4 ncda;			/* device channels of D/A */
    SINT4 nibp;			/* number of input buffer pointers */
    SINT4 nobp;			/* number of output buffer pointers */
    SINT4 segswp;		/* segments per sweep */
    SINT4 opened;		/* was i/o opened ? */
    SINT4 prepped;		/* was i/o prepared ? */
    SINT4 started;		/* was i/o started ? */
    SINT4 seg_ic;		/* input segment counter */
    SINT4 seg_oc;		/* output segment counter */
    SINT4 xrun;			/* number of underruns/overruns */
    SINT4 segtot;		/* segments total (all sweeps) */
    SINT4 gdsr;			/* dev's good samplin rates */
    SINT4 smpswp;		/* samples per sweep */
    SINT4 swptot;		/* total number of sweeps */
    SINT4 smptot;		/* total number of samples */
    SINT4 *sizptr;		/* list of segment sizes */
    UINT4 rate;			/* dev's sampling rate */
    void **i_data;		/* dev's input data buffer */
    void **o_data;		/* dev's output data buffer */
    void **ibptr;		/* list of app's input data buffers */
    void **obptr;		/* list of app's output data buffers */
    void (*a_xfer_in)(SINT4);	/* app's input data transfer function */
    void (*a_xfer_out)(SINT4);	/* app's output data transfer function */
} ARDEV;

typedef struct{
    SINT4 a_dfmt;		/* app's data format */
    SINT4 a_xfer;		/* app's data transfer function */
    SINT4 a_nbps;		/* app's number of bytes per sample */
    SINT4 a_ntlv;		/* app's samples interleaved ? */
    double mxfli;		/* maximum float in */
    double mxflo;		/* maximum float out */
} ARFMT;

typedef struct {
    char *(* dev_name)(SINT4);
    SINT4 (* num_dev)();
    void (* io_stop)(SINT4);
    void (* close)(SINT4);
    SINT4 (* open)(SINT4);
    SINT4 (* io_prepare)(SINT4);
    void (* io_start)(SINT4);
    SINT4 (* xfer_seg)(SINT4,SINT4);
    SINT4 (* chk_seg)(SINT4,SINT4);
    SINT4 (* latency)(SINT4,SINT4);
    SINT4 (* list_rates)(SINT4);
    SINT4 (* find_dev)(SINT4);
} ARDVT;

typedef struct {
    void (*xfer_in)(ARDEV*,int);		/* transfer data in */
    void (*xfer_out)(ARDEV*,int,int);		/* transfer data out */
} ARXFR;

extern SINT4   _arsc_wind;
extern SINT4   _arsc_find;
extern ARDEV *_ardev[MAXDEV];
extern ARFMT  _arfmt[MAXDEV];
extern ARDVT  _ardvt[MAXNDT];
extern ARXFR  _arxfr[MAXDEV];

SINT4  _ar_os_bind();
#ifdef ASIO
SINT4  _ar_asio_bind();
#endif /* ASIO */
SINT4 _ar_xfer_bind();
void _ar_chk_seg();

/* For sampling rates. */
extern UINT4 _ar_SRlist[];
SINT4 _ar_adjust_rate(SINT4 di, double r);


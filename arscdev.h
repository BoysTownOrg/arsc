/* arscdev.h - ARSC internal data */

#define MAXDEV	    40		/* maximum number of I/O devices */
#define MAXNDT	    2		/* maximum number of device types */
#define MAXNDC	    8		/* maximum number of device channels */
#define MAXNAM	    80		/* maximum device name length */

#define SRLSTSZ	    27		/* sample rate list size */

#if !defined(_MSC_VER) || (_MSC_VER > 1500)
#include <stdint.h>
#else
typedef short int16_t;
typedef long int32_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
#endif

typedef struct{
    double a_rate;		/* desired sampling rate */
    double ad_vfs[MAXNDC];	/* A/D volts full scale */
    double da_vfs[MAXNDC];	/* D/A volts full scale */
    int32_t a_chnmsk_i;		/* desired input channel mask */
    int32_t a_chnmsk_o;		/* desired output channel mask */
    int32_t a_chnoff_i;		/* desired input channel offset */
    int32_t a_chnoff_o;		/* desired output channel offset */
    int32_t a_ncad;		/* desired channels of A/D  */
    int32_t a_ncda;		/* desired channels of D/A */
    int32_t chnmsk_i;		/* device input channel mask */
    int32_t chnmsk_o;		/* device output channel mask */
    int32_t dev_id;		/* device identifier */
    int32_t bits;			/* number of converter bits */
    int32_t left;			/* number of left-shift bits */
    int32_t nbps;			/* number of bytes per sample */
    int32_t ntlv;			/* samples interleaved ? */
    int32_t ncad;			/* device channels of A/D  */
    int32_t ncda;			/* device channels of D/A */
    int32_t nibp;			/* number of input buffer pointers */
    int32_t nobp;			/* number of output buffer pointers */
    int32_t segswp;		/* segments per sweep */
    int32_t opened;		/* was i/o opened ? */
    int32_t prepped;		/* was i/o prepared ? */
    int32_t started;		/* was i/o started ? */
    int32_t seg_ic;		/* input segment counter */
    int32_t seg_oc;		/* output segment counter */
    int32_t xrun;			/* number of underruns/overruns */
    int32_t segtot;		/* segments total (all sweeps) */
    int32_t gdsr;			/* dev's good samplin rates */
    int32_t smpswp;		/* samples per sweep */
    int32_t swptot;		/* total number of sweeps */
    int32_t smptot;		/* total number of samples */
    int32_t *sizptr;		/* list of segment sizes */
    uint32_t rate;			/* dev's sampling rate */
    void **i_data;		/* dev's input data buffer */
    void **o_data;		/* dev's output data buffer */
    void **ibptr;		/* list of app's input data buffers */
    void **obptr;		/* list of app's output data buffers */
    void (*a_xfer_in)(int32_t);	/* app's input data transfer function */
    void (*a_xfer_out)(int32_t);	/* app's output data transfer function */
} ARDEV;

typedef struct{
    int32_t a_dfmt;		/* app's data format */
    int32_t a_xfer;		/* app's data transfer function */
    int32_t a_nbps;		/* app's number of bytes per sample */
    int32_t a_ntlv;		/* app's samples interleaved ? */
    double mxfli;		/* maximum float in */
    double mxflo;		/* maximum float out */
} ARFMT;

typedef struct {
    char *(* dev_name)(int32_t);
    int32_t (* num_dev)();
    void (* io_stop)(int32_t);
    void (* close)(int32_t);
    int32_t (* open)(int32_t);
    int32_t (* io_prepare)(int32_t);
    void (* io_start)(int32_t);
    int32_t (* xfer_seg)(int32_t,int32_t);
    int32_t (* chk_seg)(int32_t,int32_t);
    int32_t (* latency)(int32_t,int32_t);
    int32_t (* list_rates)(int32_t);
    int32_t (* find_dev)(int32_t);
} ARDVT;

typedef struct {
    void (*xfer_in)(ARDEV*,int);		/* transfer data in */
    void (*xfer_out)(ARDEV*,int,int);		/* transfer data out */
} ARXFR;

extern int32_t   _arsc_wind;
extern int32_t   _arsc_find;
extern ARDEV *_ardev[MAXDEV];
extern ARFMT  _arfmt[MAXDEV];
extern ARDVT  _ardvt[MAXNDT];
extern ARXFR  _arxfr[MAXDEV];

int32_t  _ar_os_bind();
#ifdef ASIO
int32_t  _ar_asio_bind();
#endif /* ASIO */
int32_t _ar_xfer_bind();
void _ar_chk_seg();

/* For sampling rates. */
extern uint32_t _ar_SRlist[];
int32_t _ar_adjust_rate(int32_t di, double r);


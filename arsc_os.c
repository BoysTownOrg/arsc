/* arsc_os.c - soudcard functions for OS devices [DUMMY VERSION] */

#include <stdio.h>
#include <stdlib.h>
#include "arscdev.h"

static int dio = 0;			// device identifier offset
static int debug = 0;			// debug print ?

/***************************************************************************/

/* _ar_os_num_dev - return number of OS devices */

static long
_ar_os_num_dev()
{
    int nd = 0;

    return (nd);
}

/* _ar_os_dev_name - return name of I/O device */

static char   *
_ar_os_dev_name(long di)
{
    static char *name = "no device";

    if (debug) {
	fprintf(stderr, "_dev_name\n");
    }
    return (name);
}

/* _ar_os_close - close I/O device */

static void
_ar_os_close(long di)
{
    if (debug) {
	fprintf(stderr, "_close\n");
    }
}

/* _ar_os_open - open I/O device */

static long
_ar_os_open(long di)
{
    long err = 0;
    ARDEV *a;

    if (debug) {
	fprintf(stderr, "_open: di=%ld\n", di);
    }
    a = _ardev[di];

    return (err);
}

/* _ar_os_io_prepare - prepare device and buffers for I/O */

static long
_ar_os_io_prepare(long di)
{
    ARDEV  *a;

    if (debug) {
	fprintf(stderr, "_io_prepare\n");
    }
    a = _ardev[di];

    return (0);
}

/* _ar_os_xfer_seg - this segment is ready to go */

static long
_ar_os_xfer_seg(long di, long b)
{
    return (0);
}

/* _ar_os_chk_seg - check for segment completion */

static long
_ar_os_chk_seg(long di, long b)
{
    return (0);
}

/* _ar_os_io_start - start I/O */

static void
_ar_os_io_start(long di)
{
    if (debug) {
	fprintf(stderr, "_io_start\n");
    }
}

/* _ar_os_io_stop - stop I/O */

static void
_ar_os_io_stop(long di)
{
    if (debug) {
	fprintf(stderr, "_io_stop\n");
    }
}

/* _ar_win_list_rates - create a list of available sample rates */

static long
_ar_os_list_rates(long di)
{
    if (debug) {
	fprintf(stderr, "_list_rates\n");
    }
    return (0);
}

/* _ar_os_bind - bind OS functions */

long
_ar_os_bind(long ndt, long tnd)
{
    char *e;
    long nd;

    e = getenv("ARSC_DEBUG");
    if (e){
	debug = atoi(e);
    }
    nd = _ar_os_num_dev();
    if (debug) {
	fprintf(stderr, "ARSC: nd=%ld\n", nd);
    }
    if (nd > 0) {
	_ardvt[ndt].num_dev = _ar_os_num_dev;
	_ardvt[ndt].dev_name = _ar_os_dev_name;
	_ardvt[ndt].io_stop = _ar_os_io_stop;
	_ardvt[ndt].close = _ar_os_close;
	_ardvt[ndt].open = _ar_os_open;
	_ardvt[ndt].io_prepare = _ar_os_io_prepare;
	_ardvt[ndt].io_start = _ar_os_io_start;
	_ardvt[ndt].xfer_seg = _ar_os_xfer_seg;
	_ardvt[ndt].chk_seg = _ar_os_chk_seg;
	_ardvt[ndt].list_rates = _ar_os_list_rates;
	dio = tnd;
    }

    return (nd);
}

/**************************************************************************/

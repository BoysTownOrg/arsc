/* arsc_asio.c - soudcard functions for ASIO devices [DUMMY VERSION] */

#ifdef ASIO

#include "arscdev.h"

static int dio = 0;			// device identifier offset

/***************************************************************************/

/* _ar_asio_num_dev - return number of ASIO devices */

static long
_ar_asio_num_dev()
{
    int nd = 0;

    return (nd);
}

/* _ar_asio_dev_name - return name of I/O device */

static char   *
_ar_asio_dev_name(long di)
{
    char *name = 0;

    return (name);
}

/* _ar_asio_close - close I/O device */

static void
_ar_asio_close(long di)
{
}

/* _ar_asio_open - open I/O device */

static long
_ar_asio_open(long di)
{
    long err = 0;
    ARDEV *a;

    a = _ardev[di];

    return (err);
}

/* _ar_asio_io_prepare - prepare device and buffers for I/O */

static long
_ar_asio_io_prepare(long di)
{
    ARDEV  *a;

    a = _ardev[di];

    return (0);
}

/* _ar_asio_xfer_seg - this segment is ready to go */

static long
_ar_asio_xfer_seg(long di, long b)
{
    return (0);
}

/* _ar_asio_chk_seg - check for segment completion */

static long
_ar_asio_chk_seg(long di, long b)
{
    return (0);
}

/* _ar_asio_io_start - start I/O */

static void
_ar_asio_io_start(long di)
{
}

/* _ar_asio_io_stop - stop I/O */

static void
_ar_asio_io_stop(long di)
{
}

/* _ar_asio_bind - bind ASIO functions */

long
_ar_asio_bind(long ndt, long tnd)
{
    long nd;

    nd = _ar_asio_num_dev();
    if (nd > 0) {
        _ardvt[ndt].num_dev = _ar_asio_num_dev;
	_ardvt[ndt].dev_name = _ar_asio_dev_name;
	_ardvt[ndt].io_stop = _ar_asio_io_stop;
	_ardvt[ndt].close = _ar_asio_close;
	_ardvt[ndt].open = _ar_asio_open;
	_ardvt[ndt].io_prepare = _ar_asio_io_prepare;
	_ardvt[ndt].io_start = _ar_asio_io_start;
	_ardvt[ndt].xfer_seg = _ar_asio_xfer_seg;
	_ardvt[ndt].chk_seg = _ar_asio_chk_seg;
	dio = tnd;
    }

    return (nd);
}

#endif // ASIO

/**************************************************************************/

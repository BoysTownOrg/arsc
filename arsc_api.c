/* arsc_api.c - soudcard functions for ARSC API */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "arsclib.h"
#include "arscdev.h"
#include "arscerr.h"
#include "version.h"
#include "arsc_common.h"

#define free_null(p)    if(p){free(p);p=NULL;}
#ifdef WIN32
#define strncasecmp	_strnicmp
#endif


char   _arS[100];	
int    _ar_debug = 0;
SINT4   _arsc_wind = 0;
SINT4   _arsc_find = 0;
ARDEV *_ardev[MAXDEV] = {0};
ARFMT  _arfmt[MAXDEV] = {{0}};
ARDVT  _ardvt[MAXNDT] = {{0}};
ARXFR  _arxfr[MAXDEV] = {{0}};

/**************************************************************************/
static CARDINFO card_info[MAXNCT] = {
    { "Default",	     16, 0, 2, 2, 2, 0, { 2.660}, { 2.560} },
    { "M-Audio Delta",       24, 8, 4, 2, 2, 0, { 1.826}, { 1.767} },
    { "CardDeluxe Analog",   24, 8, 4, 2, 2, 0, {10.200}, { 4.875} }, //  +4
    { "Indigo io",           24, 8, 4, 2, 2, 0, { 2.357}, { 2.190} },
    { "Gina24",              24, 8, 4, 2, 8, 0, {10.370}, {10.250} },
    { "Gina3G",              24, 8, 4, 2, 6, 0, {-8.183}, { 7.849} },
    { "Layla3G",             24, 8, 4, 8, 8, 0, { 4.292}, { 7.995} },
    { "LynxTWO-B",           24, 8, 4, 2, 6, 0, {11.060}, {11.009} },
    { "MOTU UltraLite mk3",  24, 8, 4, 6, 8, 0, {10.000}, {10.000} },
    { "Analog 1 (1) ASIO",   24, 8, 4, 2, 2, 0, {12.734}, { 4.454} },
    { "5-Waveterminal",      24, 8, 4, 2, 6, 0, { 9.655}, { 4.351} },
};
static int nct = MAXNCT;        // number of card_types with info
static int ndt = 0;		// number of device types
static int tnd = 0;		// total number of devices
static SINT4 bound = 0;		// have ARDEV functions been bound ?
static SINT4 prf[MAXNDT] = {0};	// preference flag for each device type
static SINT4 dvt[MAXDEV] = {0};	// device-type for each device identifier


/* card_type - return type of "card" from structure */

static int
card_type(SINT4 di)
{
    char   *dn;
    int     i, j, n, c = 0;

    if (tnd == 0) {
	return (0);
    }
    for (i = 0; i < MAXNCT; i++) {
	if(card_info[i].name[0] == 0) {
	    nct = i;
	    break;
	}
	if (card_info[i].ad_vfs[0] != 0) {
	    n = (card_info[i].ncad < MAXNCH) ? card_info[i].ncad : MAXNCH;
	    for (j = 1; j < n; j++) {
		if (card_info[i].ad_vfs[j] == 0)
		    card_info[i].ad_vfs[j] = card_info[i].ad_vfs[0];
	    }
	}
	if (card_info[i].da_vfs[0] != 0) {
	    n = (card_info[i].ncda < MAXNCH) ? card_info[i].ncda : MAXNCH;
	    for (j = 1; j < n; j++) {
		if (card_info[i].da_vfs[j] == 0)
		    card_info[i].da_vfs[j] = card_info[i].da_vfs[0];
	    }
	}
    }
    dn = _ardvt[dvt[di]].dev_name(di);
    for (i = 0; i < nct; i++) {
	if (card_info[i].name[0]) {
	    n = (int) strlen(card_info[i].name);
	    if (n && strncasecmp(dn, card_info[i].name, n) == 0) {
		c = i;
		break;
	    }
	}
    }

    return (c);
}

// fun_init - bind functions to handle device i/o

static void
fun_init()
{
    int    i, n;

    if (!bound) {
        card_type(0);   // initialize cardinfo

        ndt = tnd = 0;

	if (ndt < MAXNDT && tnd < MAXDEV) {
	    n = _ar_os_bind(ndt, tnd);
	    if (n) {
		prf[ndt] = ARSC_PREF_OS;
		for (i = 0; i < n && tnd < MAXDEV; i++) {
		    dvt[tnd] = ndt;
		    tnd++;
		}
		ndt++;
	    }
            //FDBUG( (_arS, "fun_init: ndt=%d tnd=%d\n", ndt, tnd) );
	}
#ifdef ASIO
	if (ndt < MAXNDT && tnd < MAXDEV) {
	    n = _ar_asio_bind(ndt, tnd);
	    if (n) {
		prf[ndt] = ARSC_PREF_ASIO;
		for (i = 0; i < n && tnd < MAXDEV; i++) {
		    dvt[tnd] = ndt;
		    tnd++;
		}
		ndt++;
	    }
            //FDBUG( (_arS, "fun_init: ndt=%d tnd=%d\n", ndt, tnd) );
	}
#endif // ASIO
	i = n = 0;	// avoid compiler error: "unreferenced"
	bound++;	// all available ARDEV functions have now been bound
    }
}

// xfer_seg - tranfer i/o segments between app's buffer and dev's buffer
//
// Processing on an output segment needs to be done -before- sending it
// to the hardware.  Thus, the output xfer function will be called once
// before the segment has been played.
static SINT4
xfer_seg(SINT4 di, SINT4 get, SINT4 queue)
{
    int     b, d, ss, st;
    ARDEV  *a;
    ARXFR  *x;

    a = _ardev[di];
    x = &_arxfr[di];

    ss = a->segswp;
    st = a->segtot;

    if (a->seg_ic < 0) {
        return ( 0 );
    }
    if (get) {				    // GET segment
        b = a->seg_ic % ss;
        d = (st && a->seg_ic >= st) || !a->ibptr;

        if (x->xfer_in && !d) {
            x->xfer_in(a, b);		    // copy data to app's input buffer
        }
        if (a->a_xfer_in && !d) {
            a->a_xfer_in(a->seg_ic);	    // let app retrieve input data
        }
		
        a->seg_ic++;
	if (st && a->seg_ic >= st) {
	    queue = 0;
	}
    }

    while ( a->seg_oc < (a->seg_ic + ss)) {  // PUT segment(s)
	b = a->seg_oc % ss;
        d = (st && a->seg_oc >= st) || !a->obptr;

	if (a->a_xfer_out && !d) {
            a->a_xfer_out(a->seg_oc);        // let app supply output data
        }
		
        if (x->xfer_out) {
            x->xfer_out(a, b, d);           // copy data from app's output buffer
        }

        a->seg_oc++;

        if (queue) {                        // QUEUE segment  
	    if ( _ardvt[dvt[di]].xfer_seg(di, b) < 0 ) {
                return -1;
	    }
        } // fi
    }

    return ( 0 );
}


// channel_mask - create channel mask from number & offset

static int
channel_mask(int n, int o)
{
    int i, m = 0;

    for (i = 0; i < n; i++) {
	m |= 1 << (i + o);
    }

    return (m);
}

static void
free_data(int di, int xf)
{
    int i;
    ARDEV  *a;
    ARXFR  *x;

    a = _ardev[di];
    x = &_arxfr[di];
    if (a->nibp) {
        for (i = 0; i < a->nibp; i++) {
            free_null(a->i_data[i]);
        }
	if (xf || x->xfer_in)
            free_null(a->i_data);
        a->nibp = 0;
    }
    if (a->nobp) {
        for (i = 0; i < a->nobp; i++) {
            free_null(a->o_data[i]);
        }
	if (xf || x->xfer_out)
	    free_null(a->o_data);
        a->nobp = 0;
    }
}

static int
chk_fmt(int di)
{
    int     i, j, ss, nb, nc, ef, sz;
    ARDEV  *a;
    ARFMT  *f;
    ARXFR  *x;

    //FDBUG( (_arS, "chk_fmt:\n") );
    a = _ardev[di];
    f = &_arfmt[di];
    x = &_arxfr[di];

    free_data(di, 0);
    ss = a->segswp;	
    // check for null pointers
    for (i = 0; i < ss; i++) {
        if (a->ncad && a->ibptr && !a->ibptr[i])
          break;
        if (a->ncda && a->obptr && !a->obptr[i])
          break;
    }
    // check for equivalent data format
    ef = (f->a_dfmt == a->nbps) && (f->a_ntlv == a->ntlv);

    // assign dev data pointers
    if ((i == ss) && ef) {
        a->i_data = a->ibptr;
        a->o_data = a->obptr;
        x->xfer_in = NULL;
        x->xfer_out = NULL;
    } else {
        a->nibp = a->ncad ? (a->ntlv ? a->segswp : (a->segswp * a->ncad)) : 0;
        a->nobp = a->ncda ? (a->ntlv ? a->segswp : (a->segswp * a->ncda)) : 0;
        a->i_data = a->ncad ? calloc(a->nibp, sizeof(void *)) : NULL;
        a->o_data = a->ncda ? calloc(a->nobp, sizeof(void *)) : NULL;

        for (i = 0; i < ss; i++) {
            sz = a->sizptr[i];
	    if (a->ncad) {
                if (a->ntlv) {
                    nb = a->nbps * a->ncad;
                    a->i_data[i] = calloc(sz, nb);
                } else {
                    nb = a->nbps;
                    nc = a->ncad;
                    for (j = 0; j < nc; j++) {
                        a->i_data[j + i * nc] = calloc(sz, nb);
                    }
                }
            }
            if (a->ncda) {
                if (a->ntlv) {
                    nb = a->nbps * a->ncda;
                    a->o_data[i] = calloc(sz, nb);
                } else {
                    nb = a->nbps;
                    nc = a->ncda;
                    for (j = 0; j < nc; j++) {
                        a->o_data[j + i * nc] = calloc(sz, nb);
                    }
                }
            }
        }

        if ( _ar_xfer_bind(di) ) {
            free_data(di, 1);
            return (1);
        }
    }

    //FDBUG( (_arS, ":chk_fmt\n") );
    return (0);
}

/**************************************************************************/

/* _arsc_get_cardinfo - get card info */

CARDINFO
_arsc_get_cardinfo(int ct)
{
    CARDINFO ci = {{0}};

    if (ct >= 0 && ct < nct)
        ci = card_info[ct];

    return (ci);
}

/* _arsc_set_cardinfo - set card info */

void
_arsc_set_cardinfo(CARDINFO ci, int ct)
{
    card_info[ct] = ci;
}

/* _arsc_get_cardtype - get card type */

SINT4
_arsc_get_cardtype(SINT4 di)
{
    return (card_type(di));
}

/*****************************************************************************/

UINT4 _ar_SRlist[SRLSTSZ] = {
    4000, 5512, 6000, 8000, 8269, 10000, 11025, 12000, 16000, 16538,
    20000, 22050, 24000, 25000, 32000, 33075, 44100, 48000, 50000,
    64000, 88200, 96000, 100000, 128000, 176400, 192000, 200000
};

SINT4
_ar_adjust_rate(SINT4 di, double r)
{
    double mindiff, diff;
    int     i, bestindex, gdsr;
    UINT4 rt;
    ARDEV  *a;

    if (di < 0 || di >= tnd)
        return (0);
    a = _ardev[di];
    gdsr = (a && a->gdsr) ? _ardev[di]->gdsr : 0xFFFFFFFF;
    bestindex = 16;		// default rate = 44100
    mindiff = fabs(log10(r / _ar_SRlist[bestindex]));
    for (i = 0; i < SRLSTSZ; i++) {
	if (gdsr & (1 << i)) {
	    diff = fabs(log10(r / _ar_SRlist[i]));
	    if (mindiff > diff) {
		mindiff = diff;
		bestindex = i;
	    }
	}
    }
    rt = _ar_SRlist[bestindex];

    return (rt);
}

/*****************************************************************************/

#define MAXSEGSWP   1024	/* max segments per sweep */
#define MAXSEGTOT   1<<30	/* max total segments */

SINT4
_ar_find_dev(
    SINT4 flags		    // hints about desired device
    )
{
    char   *cn, *dn;
    int     i, j, k, nc, nd, bits = 24;
    SINT4    di = 0, o = 0;

    fun_init();
    //FDBUG( (_arS, "find_dev:\n") );
    if (tnd == 0)
        return (-1);
    _arsc_find = flags;	    // modifies num_dev & dev_name
    flags &= 0xFF;	    // remove i/o pref flags

    // Loop through all possible device types
    for (k = 0; k < ndt; k++) {
        // Number of devices for this type
        nd = _ardvt[k].num_dev();
	if (nd <= 0) {
	    continue;
	}
	// check requested devices
	if ( flags == prf[k] || flags == ARSC_PREF_SYNC ) {
	    for (i = 0; i < nd; i++) {
		// Get device name to match to card_info
		dn = _ardvt[k].dev_name( i + o );
		if (dn == NULL) {
		    continue;
		}
		// loop through list of card_types
		for (j = 0; j < nct && card_info[j].bits; j++) {
		    if (card_info[j].bits >= bits) {
		        cn = card_info[j].name;
		        nc = (int) strlen(cn);
    			if (strncasecmp(dn, cn, nc) == 0) {
			    di = i + o;
			    return ( di );
			} //fi strcmp
		    } //fi bits
		} //rof j
	    } //rof i
	}
        // check default for device type
	if (_ardvt[k].find_dev) {
	    di = _ardvt[k].find_dev(flags);
	}
	o += nd;
    }

    //FDBUG( (_arS, ":find_dev\n") );
    return (di);
}

SINT4
_ar_find_dev_name(
    char *name		    // name of desired device
    )
{
    char   *dn;
    int     i, j, k, nc, nd, nn;
    SINT4    di = -1;

    fun_init();
    //FDBUG( (_arS, "find_dev_name:\n") );
    if (tnd == 0)
        return (-1);
    if (name == NULL)
	return (0);
    nc = (int) strlen(name);
    if (nc <= 0)
	return (0);
    for (k = 0; k < ndt; k++) {
        nd = _ardvt[k].num_dev();
	for (i = 0; i < nd; i++) {
	    dn = _ardvt[k].dev_name(i);
	    if (dn) {
		nn = (int) strlen(dn) - nc;
		for (j = 0; j < nn; j++) {
		    if (strncasecmp(dn + j, name, nc) == 0) {
			di = i;
			break;
		    }
		}
		if (di == i)
		    break;
	    }
	}
        if (i < nd) {
	    break;
	}
    }

    //FDBUG( (_arS, "find_dev_name:\n") );
    return (di);
}

SINT4
_ar_io_stop(
    SINT4 di 		    // device identifier
    )
{
    ARDEV  *a;

    fun_init();
    //FDBUG( (_arS, "io_stop:\n") );
    if (di < 0 || di >= tnd)
	return (1);
    a = _ardev[di];
    if (a && (a->opened == 1)) {
	if (a->started == 1) {
	    a->started = 0;
	}
	if (a->prepped == 1) {
	    _ardvt[dvt[di]].io_stop(di);
	    free_data(di, 0);
	    a->prepped = 0;
	}
    }

    //FDBUG( (_arS, ":io_stop\n") );
    return (0);		    // error code
}

SINT4
_ar_io_close(
    SINT4 di 		    // device identifier
    )
{
    SINT4    err = 0;
    ARDEV  *a;

    fun_init();
    //FDBUG( (_arS, "io_close:\n") );
    if (di < 0 || di >= tnd)
	return (1);
    a = _ardev[di];
    if (a && (a->opened == 1)) {
        _ar_io_stop(di);
	_ardvt[dvt[di]].close(di);
	free_null(_ardev[di]);
    }

    //FDBUG( (_arS, ":io_close\n") );
    return (err);	    // error code
}

SINT4
_ar_io_open_off(
    SINT4 di,		    // device identifier
    double rate,	    // desired sampling rate
    SINT4 chan_in,	    // desired number of input channels
    SINT4 chan_out,	    // desired number of output channels
    SINT4 chnoff_in,	    // desired input channel mask
    SINT4 chnoff_out	    // desired output channel mask
    )
{
    int  j, ct;
    SINT4 err;
    ARDEV *a;

    fun_init();
    //FDBUG( (_arS, "io_open_off: di=%ld\n", di) );
    if (di < 0 || di >= tnd)
	return (1);
    if (_ardev[di]) {
	_ar_io_close(di);
    }
    a = _ardev[di] = (ARDEV *) calloc(1, sizeof(ARDEV));
    a->dev_id = di;
    a->a_rate = (rate > 1000) ? rate : 44100;
    a->a_chnmsk_i = channel_mask(chan_in, chnoff_in);   
    a->a_chnmsk_o = channel_mask(chan_out, chnoff_out);
    a->a_chnoff_i = chnoff_in;              // desired input channels offset
    a->a_chnoff_o = chnoff_out;             // desired output channels offset
    a->a_ncad = chan_in;                    // desired number of input channels
    a->a_ncda = chan_out;                   // desired number of output channels
    a->seg_oc = -1;			    // invalidate output counter
    a->seg_ic = -1;			    // incalidate input counter
    a->opened = 1;			    // device opened
    a->prepped = 0;			    // device not yet prepared
 
    // initialize card-specific values

    ct = card_type(di);
    a->bits = card_info[ct].bits;	// I/O bits 
    a->left = card_info[ct].left;	// left-shift bits within sample
    a->nbps = card_info[ct].nbps;	// number of bytes per sample
    for (j = 0; j < MAXNDC; j++) {
	a->ad_vfs[j] = card_info[ct].ad_vfs[j];
        a->da_vfs[j] = card_info[ct].da_vfs[j];
    }

    // open i/o device

    a->chnmsk_i = a->a_chnmsk_i;	// set input channel mask
    a->chnmsk_o = a->a_chnmsk_o;	// set output channel mask
    a->ncad = a->a_ncad;		// set number of input channels
    a->ncda = a->a_ncda;		// set number output channels
    a->i_data = NULL;
    a->o_data = NULL;
    a->nibp = 0;
    a->nobp = 0;

    err = _ardvt[dvt[di]].open(di);
    if (err) {
#ifdef ALSA
	err = 130;			// ALSA open error
#else
	if (prf[dvt[di]] == ARSC_PREF_WIND) {
	    // WIND open errors
	    if (err >= 1 && err <= 7) {
		err = 100 + err;	// MMSYSERR
	    } else if (err == 32) {
		err = 108;		// WAVERR
	    } else if (err == -1) {
		err = 109;		// unknown device identifier
	    } else {
		err = 110;		// WIND open error
	    }
	} else if (prf[dvt[di]] == ARSC_PREF_ASIO) {
	    err = 120;			// ASIO open error
	}
#endif
	free_null(_ardev[di]);
    }

    //FDBUG( (_arS, ":io_open_off\n") );
    return (err);		    // error code
}

SINT4
_ar_io_open(
    SINT4 di,		    // device identifier
    double rate,	    // desired sampling rate
    SINT4 chan_in,	    // desired number of input channels
    SINT4 chan_out	    // desired number of output channels
    )
{
    return (_ar_io_open_off(di, rate, chan_in, chan_out, 0, 0));
}

SINT4
_ar_io_prep(
    SINT4 di,                // device identifier
    void **in_data,         // input data for each segment
    void **out_data,        // output data for each segment
    SINT4 *size,             // size of each segment
    SINT4 nseg,              // number of segments buffered
    SINT4 tseg               // total number of segments
    )
{

    SINT4 i, err = 0;
    ARDEV *a;
	
    fun_init();
    //FDBUG( (_arS, "io_prep:\n") );
    if (di < 0 || di >= tnd)
        return (1);
    a = _ardev[di];
    if (!a) {
        err = 201;          // device not opened
    } else if (size == NULL) {
        err = 203;          // size matters
    } else if (nseg > MAXSEGSWP) {
        err = 204;          // too many segments per sweep
    } else if (tseg > MAXSEGTOT) {
        err = 205;          // too many sweeps
    } else {
        _ar_io_stop(di);
        a->segswp = nseg;
        a->segtot = tseg;
        a->swptot = (tseg + nseg - 1) / nseg;		// unneeded?
        a->ibptr = a->a_ncad ? in_data : NULL;
        a->obptr = a->a_ncda ? out_data : NULL;
        a->sizptr = size;
        a->smptot = 0;

        for (i = 0; i < nseg; i++) {
            a->smptot += size[i];
        }
        if (chk_fmt(di)) {
            err = 202;      // unsupported format conversion
        }
    }
    if (!err) {
        if ( _ardvt[dvt[di]].io_prepare(di) < 0 ) {
	    a->opened = 0;  // device not opened
            return ( 206 ); // failed low level prepare
        }
        a->seg_oc = 0;
        a->seg_ic = 0;

	if ( xfer_seg(di, 0, 1) < 0 )
	    return -1;

        a->prepped = 1;
        a->started = 0;
    }

    //FDBUG( (_arS, "io_prep\n") );
    return (err);           // error code
}

SINT4
_ar_io_start(
    SINT4 di 		    // device identifier
    )
{
    SINT4 err;
    ARDEV *a;

    if (di < 0 || di >= tnd)
	return (1);	    // device identifier out of range;
    a = _ardev[di];
    if (!a)
	return (2);	    // device not opened

    if ((a->prepped == 1) && (a->started == 0)) {
	    _ardvt[dvt[di]].io_start(di);
        a->started = 1;
        a->xrun = 0;
        err = 0;
    } else {
        err = 301;
    }

    return (err);	    // error code
}

SINT4
_ar_set_fmt(
    SINT4 di, 		    // device identifier
    SINT4 *fmt 		    // data format
    )
{
    SINT4 err = 0;
    ARDEV *a;
    ARFMT *f;
    static int nbps[] = {
	0, 0, 2, 0, 4, 0, 4, 0, 8
    };
    static int xfer[] = {
	0, 0, 1, 0, 2, 0, 3, 0, 4
    };
    static int ndfmt = sizeof(nbps)/sizeof(int);

    fun_init();
    //FDBUG( (_arS, "set_fmt:\n") );
    if (di < 0 || di >= tnd)
        return (1);		    // device identifier out of range;
    f = &_arfmt[di];
    if (fmt == NULL) {
        err = 401;		    // NULL format
    } else if ((fmt[0] < 0) || (fmt[0] >= ndfmt) || (nbps[fmt[0]] == 0)) {
        err = 402;		    // unsupported data format
    } else if ((fmt[1] != 0) && (fmt[1] != 1)) {
        err = 403;		    // unsupported interleave

    } else if ((fmt[0] != f->a_dfmt) || (fmt[1] != f->a_ntlv)) {

        f->a_dfmt = fmt[0];	    // data format
        f->a_xfer = xfer[fmt[0]];   // data transfer function
        f->a_nbps = nbps[fmt[0]];   // bytes per sample
        f->a_ntlv = fmt[1];	    // interleave ?

        a = _ardev[di];
        if (a && (a->prepped == 1)) {
            if (chk_fmt(di)) {
                err = 404;	    // unsupported format conversion
            }
        }
    }
    if (err) {
        _ar_io_stop(di);
    }

    //FDBUG( (_arS, ":set_fmt\n") );
    return (err);		    // error code
}

SINT4
_ar_get_fmt(
    SINT4 di, 			    // device identifier
    SINT4 *fmt 			    // data format
    )
{
    ARDEV *a;

    fun_init();
    //FDBUG( (_arS, "get_fmt:\n") );
    if (di < 0 || di >= tnd)
	return (1);		    // device identifier out of range;
    a = _ardev[di];
    if (!a)
	return (2);		    // device not opened
    fmt[0] = a->nbps;		    // bytes per sample
    fmt[1] = a->ntlv;		    // interleave

    //FDBUG( (_arS, ":get_fmt\n") );
    return (0);			    // error code
}

SINT4
_ar_get_gdsr(
    SINT4 di 			    // device identifier
    )
{
    ARDEV *a;

    fun_init();
    //FDBUG( (_arS, "get_gdsr:\n") );
    if (di < 0 || di >= tnd)
	return (0);		    // device identifier out of range;
    a = _ardev[di];
    //FDBUG( (_arS, ":get_gdsr\n") );
    return (a->gdsr);		    // good sampling rates
}

SINT4
_ar_set_xfer(
    SINT4 di,                    // device identifier
    void (*in_xfer)(SINT4),	    // in_xfer function
    void (*out_xfer)(SINT4)	    // out_xfer function
    )
{
    fun_init();
    //FDBUG( (_arS, "set_xfer:\n") );
    if (di < 0 || di >= tnd)
        return (1);
    _ardev[di]->a_xfer_in = in_xfer;
    _ardev[di]->a_xfer_out = out_xfer;

    //FDBUG( (_arS, ":set_xfer\n") );
    return (0);			    // error code
}

// _ar_chk_seg - check segment and transfer data if DONE

void
_ar_chk_seg(
    SINT4 di 			    // device identifier
    )
{
    int    i, ss;
    ARDEV *a;

    fun_init();
    //FDBUG( (_arS, "chk_seg:\n") );
    if (di < 0 || di >= tnd)
        return;
    a = _ardev[di];
    if (!a)
        return;
    if (a->segtot && (a->segtot == a->seg_ic))    // done ?
        return;

    ss= a->segswp;
    for (i = 0; i < ss; i++) {

        if ( _ardvt[dvt[di]].chk_seg(di, a->seg_ic % ss) ) {
            xfer_seg(di, 1, 1);	    // transfer segment
        } else {
            break;
        }
    }

    if (i == ss) {		    // if all segments are DONE...
        a->xrun++;		    // increment xrun count
    }
    //FDBUG( (_arS, ":chk_seg\n") );
}

SINT4
_ar_io_cur_seg(
    SINT4 di 			    // device identifier
    )
{
    ARDEV *a;

    fun_init();
    //FDBUG( (_arS, "io_cur_seg:\n") );
    if (di < 0 || di >= tnd)
        return (-2);
    a = _ardev[di];
    if (!a || !a->started)
        return (-1);

    _ar_chk_seg(di);

    //FDBUG( (_arS, ":io_cur_seg\n") );
    return (a->seg_ic);    // current segment
}

SINT4
_ar_io_wait_seg(SINT4 di)
{
    int m, n, st;
    
    m = n = _ar_io_cur_seg(di);
    if (n >= 0) {
	st = _ardev[di]->segswp;    // segments total
	if ((st == 0) || (st > n)) {
	    while (m == n) {	    // wait for seg change
		n = _ar_io_cur_seg(di);
	    }
	}
    }

    return (n);
}

SINT4
_ar_set_latency(SINT4 di, SINT4 nsmp)
{
    fun_init();
    //FDBUG( (_arS, "set_latency:\n") );
    if (di < 0 || di >= tnd)
        return (1);
    if (_ardvt[dvt[di]].latency) {
	return(_ardvt[dvt[di]].latency(di,nsmp));
    }
    //FDBUG( (_arS, ":set_latency\n") );
    return (0);
}

SINT4
_ar_out_seg_fill(
    SINT4 di 			    // device identifier
    )
{
    ARDEV *a;

    fun_init();
    //FDBUG( (_arS, "out_seg_fill:\n") );
    if (di < 0 || di >= tnd)
	return (1);		    // device identifier out of range;
    a = _ardev[di];
    if (!a)
	return (2);		    // device not opened
    if (a->segswp > 2) {
        a->seg_oc = a->seg_ic + 2;
	xfer_seg(di, 0, 0);
    }
    //FDBUG( (_arS, ":out_seg_fill\n") );
    return (0);
}

SINT4
_ar_dev_name(
    SINT4 di,			    // device identifier
    char *name,			    // pointer to name array
    SINT4 len			    // length of array
    )
{
    fun_init();
    //FDBUG( (_arS, "dev_name:\n") );
    if (di < 0 || di >= tnd)
	return (1);
    strncpy(name, _ardvt[dvt[di]].dev_name(di), len);

    //FDBUG( (_arS, ":dev_name\n") );
    return (0);
}

SINT4
_ar_xruns(
    SINT4 di			    // device identifier
    )
{
    ARDEV *a;

    fun_init();
    //FDBUG( (_arS, "xruns:\n") );
    if (di < 0 || di >= tnd)
	return (-1);		    // device identifier out of range;
    a = _ardev[di];
    if (!a)
	return (-2);		    // device not opened

    //FDBUG( (_arS, ":xruns\n") );
    return (a->xrun);
}

SINT4
_ar_num_devs(
    )
{
    fun_init();
    //FDBUG( (_arS, "num_devs=%ld\n", tnd) );
    return (tnd);
}

void
_ar_err_msg(
    SINT4 err,			    // error code
    char *msg,			    // pointer to name array
    SINT4 len			    // length of array
    )
{
    char *e;
    int   i, n;

    e = _arerr[0].mesg;
    n = sizeof(_arerr) / sizeof(ARERR);
    for (i = 1; i < n; i++) {
	if (err == _arerr[i].code) {
	    e = _arerr[i].mesg;
	    break;
	}
    }
    strncpy(msg, e, len);	    // error message
}

double
_ar_get_rate(
    SINT4 di			    // device identifier
    )
{
    fun_init();
    //FDBUG( (_arS, "get_rate\n") );
    if (di < 0 || di >= tnd)
	return (0);
    return (_ardev[di]->rate);	    // sampling rate
}

void
_ar_get_sfs(		    // get i/o sample-full-scale
    SINT4 di,		    // - device identifier
    double *i_sfs,	    // - input sample-full-scale
    double *o_sfs	    // - output sample-full-scale
    )
{
    ARFMT  *f;

    fun_init();
    //FDBUG( (_arS, "get_sfs\n") );
    if (di < 0 || di >= tnd)
	return;
    f = &_arfmt[di];
    *i_sfs = f->mxfli;	    // - input sample-full-scale
    *o_sfs = f->mxflo;	    // - output sample-full-scale
}

void
_ar_set_sfs(		    // set i/o sample-full-scale
    SINT4 di,		    // - device identifier
    double *i_sfs,	    // - input sample-full-scale
    double *o_sfs	    // - output sample-full-scale
    )
{
    ARFMT  *f;

    fun_init();
    //FDBUG( (_arS, "set_sfs\n") );
    if (di < 0 || di >= tnd)
	return;
    f = &_arfmt[di];
    f->mxfli = *i_sfs;	    // - input sample-full-scale
    f->mxflo = *o_sfs;	    // - output sample-full-scale
}

SINT4 
_ar_get_cardinfo(           // Get card info
    SINT4 di,                // - device identifier
    CARDINFO *ci            // - pointer to CARDINFO structure
    )
{
    SINT4 ct;

    fun_init();
    ct = card_type(di);
    *ci = _arsc_get_cardinfo(ct);

    return (ct);
}

void
_ar_get_vfs(
    SINT4 di,			    // device identifier
    double *da_vfs,		    // DAC volts full scale
    double *ad_vfs		    // ADC volts full scale
    )
{
    int     i, ct;
    CARDINFO ci;

    fun_init();
    //FDBUG( (_arS, "get_vfs\n") );
    if (di < 0 || di >= tnd)
	return;
    ct = card_type(di);
    ci = _arsc_get_cardinfo(ct);
    for (i = 0; i < MAXNDC; i++) {
	ad_vfs[i] = ci.ad_vfs[i];
	da_vfs[i] = ci.da_vfs[i];
    }
}

void
_ar_set_vfs(
    SINT4 di,			    // device identifier
    double *da_vfs, 		    // DAC volts full scale
    double *ad_vfs		    // ADC volts full scale
    )
{
    int     i, ct;
    CARDINFO ci;

    fun_init();
    //FDBUG( (_arS, "set_vfs\n") );
    if (di < 0 || di >= tnd)
	return;
    ct = card_type(di);
    ci = _arsc_get_cardinfo(ct);
    for (i = 0; i < MAXNDC; i++) {
	ci.ad_vfs[i] = ad_vfs[i];
	ci.da_vfs[i] = da_vfs[i];
    }
    _arsc_set_cardinfo(ci, ct);
}

void
_ar_close_all()
{
    int di;

    for (di = 0; di < tnd; di++) {
        if (_ardev[di]) {
	    _ar_io_stop(di);
    	    _ardvt[dvt[di]].close(di);
	    free_null(_ardev[di]);
	}
    }
}

void
_ar_wind(
    SINT4 wind
)
{
    _arsc_wind = wind;
}

char *
_ar_version(
)
{
    return (VER);
}

/*****************************************************************************/

#ifndef WRAP

_API(SINT4)
ar_find_dev(SINT4 flags)
{
    return (_ar_find_dev(flags));
}

_API(SINT4)
ar_find_dev_name(char *name)
{
    return (_ar_find_dev_name(name));
}

_API(SINT4)
ar_io_open(SINT4 di, double rate, SINT4 chn_i, SINT4 chn_o)
{

    return(_ar_io_open(di, rate, chn_i, chn_o));
}

_API(SINT4)
ar_io_open_off(SINT4 di, double rate, SINT4 chn_i, SINT4 chn_o,
    SINT4 chnoff_i, SINT4 chnoff_o)
{

    return(_ar_io_open_off(di, rate, chn_i, chn_o, chnoff_i, chnoff_o));
}

_API(SINT4)
ar_io_prep(SINT4 di, void **in_data, void **out_data, SINT4 *size, SINT4 nseg, SINT4 tseg)
{
    return (_ar_io_prep(di, in_data, out_data, size, nseg, tseg));
}

_API(SINT4)
ar_io_prepare(SINT4 di, void **in_data, void **out_data, SINT4 *size, SINT4 nseg, SINT4 nswp)
{
    return (_ar_io_prep(di, in_data, out_data, size, nseg, nswp * nseg));
}

_API(SINT4)
ar_io_start(SINT4 di)
{
    return (_ar_io_start(di));
}

_API(SINT4)
ar_io_stop(SINT4 di)
{
    return (_ar_io_stop(di));
}

_API(SINT4)
ar_io_close(SINT4 di)
{
    return (_ar_io_close(di));
}

_API(SINT4)
ar_set_fmt(SINT4 di, SINT4 *fmt)
{
    return (_ar_set_fmt(di, fmt));
}

_API(SINT4)
ar_get_fmt(SINT4 di, SINT4 *fmt)
{
    return (_ar_get_fmt(di, fmt));
}

_API(SINT4)
ar_get_gdsr(SINT4 di)
{
    return (_ar_get_gdsr(di));
}

_API(SINT4)
ar_set_xfer(SINT4 di, void (*in_xfer)(SINT4), void (*out_xfer)(SINT4))
{
    return (_ar_set_xfer(di, in_xfer, out_xfer));
}

_API(SINT4)
ar_io_cur_seg(SINT4 di)
{
    return (_ar_io_cur_seg(di));
}

_API(SINT4)
ar_io_wait_seg(SINT4 di)
{
    return (_ar_io_wait_seg(di));
}

_API(SINT4)
ar_out_seg_fill(SINT4 di)
{
    return (_ar_out_seg_fill(di));
}

_API(SINT4)
ar_dev_name(SINT4 di, char *name, SINT4 len)
{
    return (_ar_dev_name(di, name, len));
}

_API(SINT4)
ar_xruns(SINT4 di)
{
    return (_ar_xruns(di));
}

_API(SINT4)
ar_num_devs()
{
    return (_ar_num_devs());
}

_API(void)
ar_err_msg(SINT4 err, char *msg, SINT4 len)
{
    _ar_err_msg(err, msg, len);
}

_API(double)
ar_get_rate(SINT4 di)
{
    return (_ar_get_rate(di));
}

_API(double)
ar_adjust_rate(SINT4 di, double rate)
{
    return ((double) _ar_adjust_rate(di, rate));
}

_API(void)
ar_get_sfs(SINT4 di, double *i_sfs, double *o_sfs)
{
    _ar_get_sfs(di, i_sfs, o_sfs);
}

_API(void)
ar_set_sfs(SINT4 di, double *i_sfs, double *o_sfs)
{
    _ar_set_sfs(di, i_sfs, o_sfs);
}

_API(SINT4) 
ar_get_cardinfo(SINT4 di, CARDINFO *ci)
{
    return (_ar_get_cardinfo(di, ci));
}

_API(void)
ar_get_vfs(SINT4 di, double *da_vfs, double *ad_vfs)
{
    _ar_get_vfs(di, da_vfs, ad_vfs);
}

_API(void)
ar_set_vfs(SINT4 di, double *da_vfs, double *ad_vfs)
{
    _ar_set_vfs(di, da_vfs, ad_vfs);
}

_API(void)
ar_close_all()
{
    _ar_close_all();
}

_API(void)
ar_wind(SINT4 wind)
{
    _ar_wind(wind);
}

_API(char *)
ar_version()
{
    return (_ar_version());
}

/*****************************************************************************/

_API(SINT4)
ar_set_latency(SINT4 di, SINT4 nsmp)
{
    return (_ar_set_latency(di, nsmp));
}

/*****************************************************************************/

_API(SINT4)
ar_out_open(SINT4 di, double rate, SINT4 chan_out)
{

    return(_ar_io_open(di, rate, 0, chan_out));
}

_API(SINT4)
ar_out_prepare(SINT4 di, void **out_data, SINT4 *size, SINT4 nseg, SINT4 nswp)
{
    return (_ar_io_prep(di, NULL, out_data, size, nseg, nswp * nseg));
}

#endif // WRAP

/*****************************************************************************/

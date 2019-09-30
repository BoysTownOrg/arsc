/* sio_arsc.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "arsclib.h"
#include "sio.h"

#define MAXNDC	    8		/* maximum number of I/O channels */
#define NBNK	    2		/* max banks */
#define limit(mn,aa,mx) (((aa)<(mn))?(mn):((aa)>(mx))?(mx):(aa))

SINT4     _arsc_get_cardtype(SINT4);
void     _arsc_set_cardinfo(CARDINFO, int);
CARDINFO _arsc_get_cardinfo(int);

/**************************************************************************/

typedef struct {
    SINT4 accept;
    SINT4 avmode[MAXNDC];
    SINT4 resp_i[MAXNDC];
    SINT4 stim_i[MAXNDC];
    SINT4 *bank_b;
    SINT4 *resp_a[MAXNDC];
    SINT4 *resp_b[MAXNDC];
    SINT4 *stim_b[MAXNDC];
} BIO;

BIO bio_;

/**************************************************************************/

static double mxsmp[2] = {0, 0};
static double rate_set = 0;
static SINT4 iodev = -1;
static int bnktot = 0;
static int cho[2] = {0, 0};
static int debug = 0;
static int ndc[2] = {2, 2};
static void *vbanki[NBNK];
static void *vbanko[NBNK];

/**************************************************************************/

static void
bank_put(int bo, int is, int ns, int doit, int os)
{
    int     c, i, j, d, n;
    SINT4   *s, z = 0;
    SINT4   *lbuf;

    n = ns * ndc[1];
    lbuf = (SINT4 *) vbanko[bo] + is * ndc[1];
    for (c = 0; c < ndc[1]; c++) {
        if (bio_.stim_b[c] && doit) {
	    d = bio_.stim_i[c];
	    s = bio_.stim_b[c] + os * d;
	} else {
	    d = 0;
	    s = &z;
        }
	for (i = c, j = 0; i < n; i += ndc[0], j += d) {
	    lbuf[i] = s[j] << 8;
	}
    }
}

static void
bank_get(int bo, int is, int ns)
{
    int     i, n;
    SINT4   *lbuf, *r;

    n = ns * ndc[0];
    r = bio_.bank_b;
    lbuf = (SINT4 *) vbanki[bo] + is * ndc[0];
    for (i = 0; i < n; i++)
	r[i] = lbuf[i] >> 8;
}

static void
bank_avg(int ns, int os)
{
    int c, i, j, d, n, nc;
    SINT4 *b;
    SINT4 *r, *a;

    nc = ndc[0];    /* number of input channels */
    n = ns * nc;
    b = bio_.bank_b;
    for (c = 0; c < nc; c++) {
	d = bio_.resp_i[c];
	switch(bio_.avmode[c]) {
	case 1:
	    r = bio_.resp_b[c] + os * d;
	    for (i = c, j = 0; i < n; i += nc, j += d) {
		r[j] = b[i];
	    }
	    break;
	case 2:
	    a = bio_.resp_a[c] + os * d;
	    for (i = c, j = 0; i < n; i += nc, j += d) {
		a[j] += b[i];
	    }
	    break;
	case 3:
	    r = bio_.resp_b[c] + os * d;
	    a = bio_.resp_a[c] + os * d;
	    for (i = c, j = 0; i < n; i += nc, j += d) {
		r[j] = b[i];
		a[j] += b[i];
	    }
	    break;
	}
    }
}

/**************************************************************************/

static float **abptr = NULL;
static float ad_vpc[MAXNDC] = {1,1};
static float da_vpc[MAXNDC] = {1,1};
static float **ibptr = NULL;
static float **obptr = NULL;
static int avcnt = 0;
static int *avgcp = NULL;
static int bnksiz = 1024;
static int gapflg = 0;
static int *ibinc = NULL;
static int nbpc = 2;
static int nrbc = 0;
static int nsbc = 0;
static int *obinc = NULL;
static int ofst0 = 0;
static int rspbuf = 0;
static int smpskp = 0;
static int smpstm = 0;
static int smpswp = 0;
static int smptot = 0;
static int swpcnt = 0;
static int swptot = 0;

static void
set_stim_buff(int b)
{
    int c, k;

    for (c = 0; c < ndc[1]; c++) {
	k = c * nsbc + b;
	bio_.stim_i[c] = obinc ? obinc[k] : 1;
	bio_.stim_b[c] = obptr ? (SINT4 *) obptr[k] : NULL;
    }
}

static void
set_resp_buff(int b)
{
    int c, k;
    SINT4 *bp, *ap;

    for (c = 0; c < ndc[0]; c++) {
	k = c * nrbc + b;
	bio_.resp_i[c] = ibinc ? ibinc[k] : 1;
	bio_.resp_b[c] = bp = ibptr ? (SINT4 *) ibptr[k] : NULL;
	bio_.resp_a[c] = ap = abptr ? (SINT4 *) abptr[k] : NULL;
 	bio_.avmode[c] = ap ? (bp ? 3 : 2) : (bp ? 1 : 0);
    }
    rspbuf = b;
}

static void
put_bank(SINT4 bnk_oc)
{
    int     bnko, chnk, ibnk, doit, ofst, nrem, nsmp, smpo;

    bnko = bnk_oc % nbpc;
    for (chnk = bnksiz; chnk > 0; chnk -= nsmp) {
	ibnk = bnksiz - chnk;
	smpo = bnk_oc * bnksiz + ibnk;
	ofst = (smpo + ofst0) % smpswp;
	if ((ofst == 0 && nsbc > 0) || smpo == 0)
	    set_stim_buff((smpo / smpswp) % nsbc);
	nrem = (ofst < smpstm) ? (smpstm - ofst) : (smpswp - ofst);
	nsmp = (chnk < nrem) ? chnk : nrem;
	doit = (smpo < smptot && ofst < smpstm);
	bank_put(bnko, ibnk, nsmp, doit, ofst);
    }
}

static void
get_bank(SINT4 bnk_ic)
{
    int     bnko, chnk, ibnk, doit, ofst, nrem, nsmp, smpi;

    bnko = bnk_ic % nbpc;
    for (chnk = bnksiz; chnk > 0; chnk -= nsmp) {
	ibnk = bnksiz - chnk;
	smpi = bnk_ic * bnksiz + ibnk;
	ofst = (smpi +ofst0) % smpswp;
	if ((ofst == 0 && nrbc > 0) || smpi == 0)
	    set_resp_buff((smpi / smpswp) % nrbc);
	nrem = (ofst < smpstm) ? (smpstm - ofst) : (smpswp - ofst);
	nsmp = (chnk < nrem) ? chnk : nrem;
	doit = (smpi >= smpskp && ofst < smpstm && swpcnt < swptot);
	if (doit) {
	    bank_get(bnko, ibnk, nsmp);
	    bank_avg(nsmp, ofst);
	}
	ofst += nsmp;
	if (ofst == smpstm) {
	    swpcnt++;
	    if (doit && (rspbuf == (nrbc - 1))) {
		avcnt++;
		if (avgcp)
		    *avgcp = avcnt;
	    }
	}
	gapflg = (ofst >= smpstm);
    }
}

/**************************************************************************/

static void
reset_io(int bs)
{
    int b, nseg;
    static int maxbnksiz = 16384;
    static int nbps = 4;
    static SINT4 size[2];
    static SINT4 fmt[2] = {ARSC_DATA_I4, 1};

    bnksiz = limit(1, bs, maxbnksiz);
    for (b = 0; b < NBNK; b++) {
        // Allocate memory for the input and output buffers
        vbanki[b] = (void *) calloc(bnksiz, ndc[0] * nbps); /* input bank */
        vbanko[b] = (void *) calloc(bnksiz, ndc[1] * nbps); /* output bank */
        if (vbanki[b] == NULL || vbanko[b] == NULL) {
	    bnksiz = 0;
	    return;
	}
    }
    bio_.bank_b = (SINT4 *) calloc(bnksiz * ndc[0], sizeof(SINT4));
    bnktot = (smptot + bnksiz - 1) / bnksiz;    
    size[0] = bnksiz;
    size[1] = bnksiz;
    nseg = (bs > bnksiz) ? NBNK : 1;

    (void) ar_set_fmt(iodev, fmt);
    (void) ar_set_xfer(iodev, get_bank, put_bank);
    (void) ar_io_prep(iodev, vbanki, vbanko, size, nseg, bnktot);
}

/**************************************************************************/

static int (*escape)() = NULL;
static int escflg = 0;

static void
setup_io(SINT4 nstm, SINT4 ngap, int nskp, int nswp, double pregap)
{
    time_t t = 0;

    if (nswp <= 0 || nstm <= 0) {
	escflg = 1;
	return;
    }
    smpstm = nstm;
    smpswp = nstm + ngap;
    smpskp = nskp;
    smptot = smpswp * nswp - ngap + smpskp;
    swptot = nswp + (smpskp + smpswp - 1) / smpswp;
    ofst0 = swptot * smpswp - smptot;
    swpcnt = 0;
    escflg = 0;
    if (pregap > 0) {
	t = clock() + (time_t)(pregap * CLOCKS_PER_SEC);
	while(clock() < t)
	    continue;
    }
    if (escape)
        if (escape())
            return;
    reset_io(smptot);
}

static void
perform_io(int navg, int nrej)
{
    if (debug) {
	fprintf(stderr, "perform_io: navg=%d bnktot=%d gapflg=%d\n",
	    navg, bnktot, gapflg);
    }
    ar_io_start(iodev);
    while (ar_io_cur_seg(iodev) < bnktot) {
	if (gapflg) {
	    if (navg && avcnt >= navg)
		break;
	if (escape)
	    if (escape())
		escflg++;
	if (escflg > 1 || (escflg && gapflg))
	    break;
	if (swpcnt >= swptot)
	    break;
	}
    }
}

static void
stop_io()
{
    int b;

    (void) ar_io_stop(iodev);

    for (b = 0; b < NBNK; b++) {
        free(vbanki[b]);
	free(vbanko[b]);
    }
    if (bio_.bank_b) {
	free(bio_.bank_b);
	bio_.bank_b = NULL;
    }
}

/**************************************************************************/

static double prgap = 0;
static int nsgap = 0;
static int nsstm = 0;

static void
init_vpc()
{
    double  da_vfs[MAXNDC], ad_vfs[MAXNDC];
    int     c, nbps = 3;

    mxsmp[0] = mxsmp[1] = pow(2, nbps * 8 - 1) - 1;
    ar_get_vfs(iodev, da_vfs, ad_vfs);
    for (c = 0; c < ndc[1]; c++) {
	da_vpc[c] = (float) ((da_vfs[c] / mxsmp[1]));
    }
    for (c = 0; c < ndc[0]; c++) {
	ad_vpc[c] = (float) ((ad_vfs[c] / mxsmp[0]));
    }
}


static void
stim_fix()
{
    double  sc, s, m;
    float  *ob;
    int     c, b, i, j, k, dj;
    SINT4   *sl;

    m = (float) mxsmp[1];
    for (c = 0; c < ndc[1]; c++) {
	for (b = 0; b < nsbc; b++) {
	    k = c * nsbc + b;
	    if (obptr && obptr[k]) {
		ob = obptr[k];
		dj = obinc[k];
		sl = (SINT4 *) ob;
		sc = 1 / da_vpc[c];
		for (i = j = 0; i < nsstm; i++, j += dj) {
		    s = ob[j] * sc + 0.5;
		    sl[j] = (SINT4) limit(-m, s, m);
		}
	    }
	}
    }
}

static void
stim_float()
{
    float  *ob, sc;
    int     c, b, i, j, k, dj;
    SINT4   *sl;

    for (c = 0; c < ndc[1]; c++) {
	for (b = 0; b < nsbc; b++) {
	    k = c * nsbc + b;
	    if (obptr && obptr[k]) {
		ob = obptr[k];
		dj = obinc[k];
		sl = (SINT4 *) ob;
		sc = da_vpc[c];
		for (i = j = 0; i < nsstm; i++, j += dj) {
		    ob[j] = sl[j] * sc;
		}
	    }
	}
    }
}

static void
resp_zero()
{
    float  *ib;
    int     c, b, i, j, k, dj;
    SINT4   *rl;

    for (c = 0; c < ndc[0]; c++) {
	for (b = 0; b < nrbc; b++) {
	    k = c * nrbc + b;
	    if (abptr && abptr[k]) {
		ib = abptr[k];
		dj = ibinc[k];
		rl = (SINT4 *) ib;
		for (i = j = 0; i < nsstm; i++, j += dj) {
		    rl[j] = 0;
		}
	    }
	}
    }
    if (avgcp)
	*avgcp = 0;
    escflg = bio_.accept = avcnt = 0;
}

static void
resp_scale_average()
{
    float  *ab, sc;
    int     c, b, i, j, k, dj, n;
    SINT4   *rb;

    for (c = 0; c < ndc[0]; c++) {
	for (b = 0; b < nrbc; b++) {
	    n = avcnt;
	    k = c * nrbc + b;
	    if (abptr && abptr[k]) {
		ab = abptr[k];
		dj = ibinc[k];
		rb = (SINT4 *) ab;
		sc = (n > 0) ? (float) (ad_vpc[c] / n) : 0;
		for (i = j = 0; i < nsstm; i++, j += dj) {
		    ab[j] = rb[j] * sc;
		}
	    }
	}
    }
}

static void
resp_scale_input()
{
    float  *ib, sc;
    int     c, b, i, j, k, dj;
    SINT4   *rb;

    for (c = 0; c < ndc[0]; c++) {
	for (b = 0; b < nrbc; b++) {
	    k = c * nrbc + b;
	    if (ibptr && ibptr[k]) {
		ib = ibptr[k];
		dj = ibinc[k];
		rb = (SINT4 *) ib;
		sc = ad_vpc[c];
		for (i = j = 0; i < nsstm; i++, j += dj) {
		    ib[j] = rb[j] * sc;
		}
	    }
	}
    }
}

/**************************************************************************/
/* SIO library routines */
/**************************************************************************/

// sio_open - Initializes soundcard and internal SIO variables.

int sio_open(	// returns non-zero if successful
)
{
    if (iodev < 0) {
        iodev = ar_find_dev(ARSC_PREF_ASIO);
    }
    if (ar_io_open_off(iodev, rate_set, ndc[0], ndc[1], cho[0], cho[1]))
	return (0);
    rate_set = ar_get_rate(iodev);
    nsstm = 0;
    nsgap = 0;
    prgap = 0;
    nsbc = 0;
    nrbc = 0;
    obptr = NULL;
    obinc = NULL;
    abptr = NULL;
    ibptr = NULL;
    ibinc = NULL;
    escape = NULL;
    bio_.bank_b = NULL;

    if (debug) {
	fprintf(stderr, "sio_open\n");
    }
    return (1);
}

// sio_close - Terminate I/O and free any allocated resources.

void sio_close(
)
{
    (void) ar_io_stop(iodev);
    (void) ar_io_close(iodev);
    iodev = -1;
    if (debug) {
	fprintf(stderr, "sio_close\n");
    }
}

// sio_get_nioch - Obtain number of I/O channels.

int sio_get_nioch(
    int *ni, 		// Number of input channels returned here
    int *no		// Number of output channels returned here
)
{
    *ni = ndc[0];
    *no = ndc[1];

    if (debug) {
	fprintf(stderr, "sio_close\n");
    }
    return (iodev >= 0);
}

// sio_get_device - Obtain brief description of I/O device.

int sio_get_device(
    char *s		// Device description returned here
)
{
    if (ar_dev_name(iodev, s, ARSC_NAMLEN))
	strcpy(s, "invalid");

    return (iodev >= 0);
}

// sio_set_device - select I/O device.

int sio_set_device(
    int n		// card number
)	    		// returns card number
{
    if (n < 0) {
        iodev = -1;
    } else if (n > ar_num_devs()) {
	iodev = ar_num_devs() - 1;
    } else if (n > 0) {
        iodev = n - 1;
    }
    if (iodev < 0) {
        iodev = ar_find_dev(ARSC_PREF_ASIO);
    }

    return (iodev + 1);
}

// sio_get_info - Obtain one-line of information about I/O device.

int sio_get_info(
    char *s		// Device information returned here
)
{
    int e, i, n;
    char nam[ARSC_NAMLEN];

    n = (int) ar_num_devs();
    if ((n > 0) && (iodev >= 0)) {
	e = (int) ar_dev_name(iodev, nam, ARSC_NAMLEN);
	i = (int) iodev + 1;
	sprintf(s, "%s device: %s, %d/%d.", e ? "Invalid" : "I/O", nam, i, n);
    } else {
	sprintf(s, "No I/O device.");
    }

    return (iodev >= 0);
}

// sio_get_vfs - Obtain full-scale voltage of ADCs and DACs.

int sio_get_vfs(
    double *ad_vfs, 	// volts-full-scale for each ADC channel returned 
    double *da_vfs 	// volts-full-scale for each DAC channel returned
)
{
    ar_get_vfs(iodev, da_vfs, ad_vfs);

    return (iodev >= 0);
}

// sio_set_size - Specify size of buffers and gap between buffers.

void sio_set_size(
    int ns, 		// size of I/O buffers (samples)
    int ng,		// size of gap between I/O buffers (samples)
    double pg		// size of gap before I/O (seconds)
)
{
    nsstm = ns;
    nsgap = ng;
    prgap = pg;
}

// sio_set_output - Specify output buffers.

void sio_set_output(
    int nch, 		// number of output channels to be used
    int bpc, 		// number of buffers per output channel
    int *inc, 		// array of increments for each output buffer
    float **bpt		// array of pointers to each output buffer
)
{
    ndc[1] = nch;
    nsbc = bpc;
    obinc = inc;
    obptr = bpt;
    while (nch < ndc[1]) {
	inc[nch] = 0;
	bpt[nch] = NULL;
	nch++;
    }
}

// sio_set_input - Specify input buffers.

void sio_set_input(
    int nch, 		// number of input channels to be used
    int bpc, 		// number of buffers per input channel
    int *inc, 		// array of increments for each input buffer
    float **bpt 	// array of pointers to each input buffer
)
{
    ndc[0] = nch;
    nrbc = bpc;
    ibinc = inc;
    ibptr = bpt;
    while (nch < ndc[0]) {
	inc[nch] = 0;
	bpt[nch] = NULL;
	nch++;
    }
}

// sio_set_average - Specify averaging.

void sio_set_average(
    float **apt, 	// array of pointers to accumulate buffers
    int *acp		// array set to num of swps averaged in each buffer
)
{
    abptr = apt;
    avgcp = acp;
}

// sio_set_rate - Specify sampling rate.

double sio_set_rate(
    double r		// desired rate (samples/second)
)			// returns actual rate (samples/second)
{
    if (r != rate_set) {
        ar_io_close(iodev);
	ar_io_open_off(iodev, r, ndc[0], ndc[1], cho[0], cho[1]);
	rate_set = ar_get_rate(iodev);
    }

    if (debug) {
	fprintf(stderr, "sio_set_rate: rate=%.0f\n", rate_set);
    }
    return (rate_set);
}

// sio_set_att_in - Specify attenuation on input all channels.

double sio_set_att_in(
    double a		// desired input attenutation (dB)
)			// returns actual input attenuation (dB)
{
    return (0);
}

// sio_set_att_out - Specify attenuation on output all channels.

double sio_set_att_out(
    double a		// desired output attenutation (dB)
)			// returns actual output attenuation (dB)
{
    return (0);
}

// sio_set_vfs - Specify full-scale voltage on ADCs and DACs

void sio_set_vfs(
    double *ad_vfs, 	// volts-full-scale for each ADC
    double *da_vfs	// volts-full-scale for each DAC
)
{
    ar_set_vfs(iodev, da_vfs, ad_vfs);
}

// sio_set_escape - Specify callback function for early I/O termination.

void sio_set_escape(
    int (*esc)()	// escape callback function
)
{
    escape = esc;
}

// sio_io - Perform input/output/averaging.

void sio_io(
    int nskip, 		// samples to skip before averaging
    int nswps, 		// maximum total sweeps while averaging
    int navgs, 		// maximum sweeps averaged
    int nrejs		// maximum sweeps rejected
)
{
    init_vpc();
    stim_fix();
    resp_zero();
    setup_io(nsstm, nsgap, nskip, nswps, prgap);
    perform_io(navgs, nrejs);
    stop_io();
    resp_scale_average();
    stim_float();
    if (debug) {
	fprintf(stderr, "sio_io\n");
    }
}

// sio_io_chk - Perform input/output/averaging with callback.

void sio_io_chk(
    void (*resp_check)(int *)	// sweep callback function
)
{
    init_vpc();
    stim_fix();
    resp_zero();
    while (!escflg) {
        setup_io(nsstm, 0, 0, 1, prgap);
	perform_io(0, 0);
	if (resp_check) {
	    resp_scale_input();
	    resp_check(&escflg);
	}
        stop_io();
    }
    resp_scale_average();
    stim_float();
    if (debug) {
	fprintf(stderr, "sio_io_chk\n");
    }
}

/**************************************************************************/

// sio_set_latency - Set ASIO internal latency

int sio_set_latency(
    int nsmp
)
{
    return (ar_set_latency(iodev, nsmp));
}

// sio_set_channel_offset - Set input & output channel offsets

void sio_set_channel_offset(
    int chnoff_i,
    int chnoff_o
)
{
    cho[0] = chnoff_i;
    cho[1] = chnoff_o;
}

/**************************************************************************/

// sio_init_cardinfo - get initialize info

void sio_init_cardinfo(
)
{
}

// sio_get_cardinfo - get card info

CARDINFO sio_get_cardinfo(
    int ct       // card type
)
{
    if (ct < 0)
        ct = _arsc_get_cardtype(iodev);
    return (_arsc_get_cardinfo(ct));
}

// sio_set_cardinfo - set card info

void sio_set_cardinfo(
    CARDINFO ci, // card info
    int ct       // card type
)
{
    _arsc_set_cardinfo(ci, ct);
}

/**************************************************************************/
// sio_version - ARSC version

char *sio_version(void)
{
    return (ar_version());
}

/**************************************************************************/

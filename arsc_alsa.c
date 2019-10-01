/* arsc_alsa.c - soundcard functions for Linux/ALSA devices */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <alsa/asoundlib.h>
#include "arsclib.h"
#include "arscdev.h"

#define free_null(p)    if (p){free(p);p=NULL;}

static double *td[MAXDEV] = {NULL};
static int ndv = -1;			// number of devices
static int bsz = 0;			// device buffer size
static int psz = 0;			// device period size
static int dio = 0;			// device identifier offset
static int debug = 0;
static int linked[MAXDEV] = {0};
static int opened[MAXDEV] = {0};
static long *qi[MAXDEV] = {NULL};
static long *qo[MAXDEV] = {NULL};
static void *bridge = NULL;
static snd_ctl_card_info_t *card_info = NULL;
static snd_pcm_t *hwi[MAXDEV] = {NULL};
static snd_pcm_t *hwo[MAXDEV] = {NULL};
static struct {
    char dnm[64];
    int dev, crd, ich, och;
} dcs[MAXDEV];
static struct timeval tv0;

/***************************************************************************/

static void
test(int err, int i)
{
    if (err == -11 && debug == 0) {
	fprintf(stderr, "ALSA error: %s.\n", snd_strerror(err));
	exit(1);
    } else if (err < 0 && debug > 0) {
	fprintf(stderr, "At test=%3d err=%3d : %s.\n", i, err, snd_strerror(err));
    } else if (debug > 1) {
	fprintf(stderr, "No error at test=%3d.\n", i);
    }
}

static void
set_time_out(long di, long b)
{
    double tn, tq, sr;
    struct timeval tv;
    snd_pcm_sframes_t nf;
    static double delay_reduction = 0.9; // fudge factor ???

    gettimeofday(&tv, NULL);
    tn = (tv.tv_sec - tv0.tv_sec) * 1e6 + (tv.tv_usec - tv0.tv_usec);
    sr =  _ardev[di]->a_rate;
    snd_pcm_delay(hwo[di], &nf);
    tq = nf * 1e6 / sr * delay_reduction;
    td[di][b] =  tn + tq;
}

static int
chk_time_out(long di, long b)
{
    double tn;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    tn = (tv.tv_sec - tv0.tv_sec) * 1e6 + (tv.tv_usec - tv0.tv_usec);

    return (tn >= td[di][b]);
}

static long
chk_done_i(long di, long b)	// interleaved version
{
    int rc, avail, nb, b2, n1, n2, nn, sz, qio;
    void *pd;
    ARDEV *a;

    a = _ardev[di];
    sz = a->sizptr[b];
    qio = 0;

    if (qi[di] && qi[di][b]) {
	nb = a->ncad * a->nbps;
	pd = a->i_data[b] + (sz - qi[di][b]) * nb;
	rc = snd_pcm_readi(hwi[di], pd, qi[di][b]);
	if (rc > 0) {
	    qi[di][b] -= rc;
	}
	if (qi[di][b] > 0 && linked[di]) {
	    qio += qi[di][b];
	}
    }

    if (qo[di] && qo[di][b]) {
	avail = snd_pcm_avail_update(hwo[di]);
	if (avail < 0) {
	    snd_pcm_prepare(hwo[di]);
	} else if (avail >= bsz / 2) {
	    if (avail > qo[di][b]) {
	        avail = qo[di][b];
	    }
	    nb = a->ncda * a->nbps;
	    pd = a->o_data[b] + (sz - qo[di][b]) * nb;
	    rc = snd_pcm_writei(hwo[di], pd, avail);
	    if (rc < 0) {
		snd_pcm_prepare(hwo[di]);
	    }
	    if (rc > 0) {
	        qo[di][b] -= rc;
		n1 = qo[di][b];
		n2 = 0;
		nn = psz;
		if (n1 < nn) {
		    bridge = calloc(psz, nb);
	            pd = a->o_data[b] + (sz - n1) * nb;
		    memcpy(bridge, pd, n1 * nb);
		    b2 = (b + 1) % a->segswp;
		    if (b < a->segtot) {
	                pd = a->o_data[b2];
	                sz = a->sizptr[b2];
		        n2 = (sz < (psz - n1)) ? sz : (psz - n1);
		        memcpy(bridge + n1 * nb, pd, n2 * nb);
		    }
	    	    rc = snd_pcm_writei(hwo[di], bridge, n1 + n2);
		    if (rc > 0) {
	                qo[di][b2] -= n2;
	                qo[di][b] = 0;
		    }
		    free_null(bridge);
		}
	    }
	}
	if (qo[di][b] > 0) {
	    qio += qo[di][b];
	} else {
	    set_time_out(di, b);
	}
    }

    return (qio == 0);
}

static long
chk_done_n(long di, long b)	// non-interleaved version
{
    int c, j, rc, avail, nb, b2, n1, n2, nn, sz, qio;
    void **pd, *p1, *p2;
    ARDEV *a;

    a = _ardev[di];
    sz = a->sizptr[b];
    qio = 0;

    if (qi[di] && qi[di][b]) {
	c = a->ncad;
	nb = a->nbps;
        pd = calloc(c, sizeof(void *));
	for (j = 0; j < c; j++)
	    pd[j] = a->i_data[j + b * c] + (sz - qi[di][b]) * nb;
	rc = snd_pcm_readn(hwi[di], pd, qi[di][b]);
	if (rc > 0) {
	    qi[di][b] -= rc;
	}
	free_null(pd);
	if (qi[di][b] > 0 && linked[di]) {
	    qio += qi[di][b];
	}
    }

    if (qo[di] && qo[di][b]) {
	avail = snd_pcm_avail_update(hwo[di]);
	if (avail < 0) {
	    snd_pcm_prepare(hwo[di]);
	} else if (avail >= bsz / 2) {
	    if (avail > qo[di][b]) {
	        avail = qo[di][b];
	    }
	    c = a->ncda;
	    nb = a->nbps;
            pd = calloc(c, sizeof(void *));
	    for (j = 0; j < c; j++) {
	        pd[j] = a->o_data[j + b * c] + (sz - qo[di][b]) * nb;
	    }
	    rc = snd_pcm_writen(hwo[di], pd, avail);
	    if (rc < 0) {
		snd_pcm_prepare(hwo[di]);
	    }
	    if (rc > 0) {
	        qo[di][b] -= rc;
		n1 = qo[di][b];
		n2 = 0;
		nn = psz;
		if (n1 < nn) {
		    bridge = calloc(psz, nb * c);
		    b2 = (b + 1) % a->nobp;
		    if (b < a->segtot) {
	                sz = a->sizptr[b2];
		        n2 = (sz < (psz - n1)) ? sz : (psz - n1);
		    }
		    for (j = 0; j < c; j++) {
		        pd[j] = bridge + j * (n1 + n2);
	                p1 = a->o_data[j + b * c] + (sz - n1) * nb;
		        memcpy(pd[j], p1, n1 * nb);
	                p2 = a->o_data[j + b2 * c];
		        memcpy(pd[j] + n1 * nb, p2, n2 * nb);
		    }
	    	    rc = snd_pcm_writen(hwo[di], pd, n1 + n2);
		    if (rc > 0) {
	                qo[di][b2] -= n2;
	                qo[di][b] = 0;
		    }
		    free_null(bridge);
		}
	    }
	    free_null(pd);
	}
	if (qo[di][b] > 0) {
	    qio += qo[di][b];
	} else {
	    set_time_out(di, b);
	}
    }

    return (qio == 0);
}

static void 
reset_params(long di, snd_pcm_t *hw)
{
    snd_pcm_hw_params_t *hwp;
    snd_pcm_sw_params_t *swp;

    hwp = calloc(1, snd_pcm_hw_params_sizeof());
    swp = calloc(1, snd_pcm_sw_params_sizeof());
    
    // reset hardware params

    snd_pcm_hw_params(hw, hwp);
    
    // reset software params
 
    snd_pcm_sw_params_current(hw, swp);

    free_null(hwp);
    free_null(swp);
} 

static int 
set_params(long di, snd_pcm_t *hw, int nchan)
{
    int mode;
    long err = 0;
    unsigned int r;
    snd_pcm_hw_params_t *hwp;
    snd_pcm_sw_params_t *swp;
    snd_pcm_uframes_t bufsiz = 0;
    snd_pcm_uframes_t persiz = 0;
    ARDEV *a;

    a = _ardev[di];

    hwp = calloc(1, snd_pcm_hw_params_sizeof());
    swp = calloc(1, snd_pcm_sw_params_sizeof());
    
    // set hardware params

    err = snd_pcm_hw_params_any(hw, hwp);
    test(err, 3);
    if (err) goto error_return;
    if (a->ntlv) {
	mode = SND_PCM_ACCESS_RW_INTERLEAVED;
    } else {
	mode = SND_PCM_ACCESS_RW_NONINTERLEAVED;
    }
    err = snd_pcm_hw_params_set_access(hw, hwp, mode);
    test(err, 4);
    if (err) goto error_return;
    if (a->nbps == 4) {
	mode = SND_PCM_FORMAT_S32_LE;
    } else {
	mode = SND_PCM_FORMAT_S16_LE;
    }
    err = snd_pcm_hw_params_set_format(hw, hwp, mode);
    test(err, 5);
    if (err) goto error_return;
    r = (unsigned int) a->a_rate;
    err = snd_pcm_hw_params_set_rate_near(hw, hwp, &r, 0);
    test(err, 6);
    err =  snd_pcm_hw_params_get_rate(hwp, &r, NULL);
    test(err, 7);
    a->rate = r;
    err = snd_pcm_hw_params_set_channels(hw, hwp, nchan);
    test(err, 8);
    err = snd_pcm_hw_params(hw, hwp);
    test(err, 9);
    err = snd_pcm_hw_params_get_buffer_size(hwp, &bufsiz);
    test(err, 10);
    err = snd_pcm_hw_params_get_period_size(hwp, &persiz, NULL);
    test(err, 11);

    bsz = bufsiz;
    psz = persiz;
    
    // set software params
 
    err = snd_pcm_sw_params_current(hw, swp);
    test(err, 12);
    //err = snd_pcm_sw_params_set_xfer_align(hw, swp, 4);	// deprecated
    //test(err, 13);
    err = snd_pcm_sw_params(hw, swp);
    test(err, 14);

error_return:
    free_null(hwp);
    free_null(swp);

    return (err);
} 

static int
set_io_params(long di)
{
    int erro = 0, erri = 0;
    ARDEV *a;

    a = _ardev[di];
    if (a->ncda) {
	erro = set_params(di, hwo[di], a->ncda);
    }
    if (a->ncad) {
	erri = set_params(di, hwi[di], a->ncad);
    }

    return (erro || erri);
}

static long 
list_rates(long di)
{
    int i;
    long err = 0, gdsr = 0;
    unsigned int r;
    snd_pcm_hw_params_t *hwp;
    ARDEV *a;

    a = _ardev[di];

    hwp = calloc(1, snd_pcm_hw_params_sizeof());
    
    err = snd_pcm_hw_params_any(hwo[di], hwp);
    test(err, 21);

    for (i = 0; i < SRLSTSZ; i++) {
	if (_ar_SRlist[i] <= 0)
	    continue;
	r = (unsigned int) _ar_SRlist[i];
	err = snd_pcm_hw_params_set_rate_near(hwo[di], hwp, &r, 0);
	test(err, 22);
	err =  snd_pcm_hw_params_get_rate(hwp, &r, NULL);
	test(err, 23);
	if (r == _ar_SRlist[i])
	    gdsr |= 1 << i;
    }

    r = (unsigned int) a->a_rate;
    err = snd_pcm_hw_params_set_rate_near(hwo[di], hwp, &r, 0);
    test(err, 24);
    err =  snd_pcm_hw_params_get_rate(hwp, &r, NULL);
    test(err, 25);
    a->rate = r;

    free_null(hwp);

    return (gdsr);
}

/* trim - remove whitespace from end of string */

static void 
trim(char *s)
{
    char *e;

    for (e = s; *e; e++)
	continue;
    while (e > s && e[-1] <= ' ')
	e--;
    *e = '\0';
}

/* atoh - ASCII to hexidecimal */

static int 
atoh(char *s)
{
    int i = 0;

    while (*s && *s <= ' ')	/* skip whitespace */
	s++;
    while (*s) {
	if (*s > '0' && *s < '9') {
	    i = 16 * i + *s - '0';
	} else if (*s >= 'A' && *s <= 'F') {
	    i = 16 * i + 10 + *s - 'A';
	} else if (*s >= 'a' && *s <= 'f') {
	    i = 16 * i + 10 + *s - 'a';
	}
	s++;
    }

    return (i);
}

/* getmv - get list of mvfs */

static void 
getmv(char *s, double *v)
{
    int ch, mv = 0;

    for (ch = 0; ch < MAXNCH; ch++) {
	if (*s) {
	    while (*s && *s <= ' ')	/* skip whitespace */
		s++;
	    mv = atoi(s);
	    while (*s > ' ')
		s++;
	}
	v[ch] = mv * 0.001;
    }
}

/* read_rc - read cardinfo file */

static void 
read_rc()
{
    char s[80];
    int i = -1;
    CARDINFO ci;
    FILE *fp;
    static char *fn = "/usr/local/etc/arscrc";
    void _arsc_set_cardinfo(CARDINFO, int);
    static CARDINFO zci = {{0},0,0,0,0,0,0,{0},{0}};

    fp = fopen(fn, "r");
    if (fp == NULL) {
	return;
    }
    while (fgets(s, 80, fp) != NULL) {
	trim(s);
	if (strncasecmp(s, "[CardType", 9) == 0) {
	    i = atoi(s + 9);
	    ci = zci;
	} else if (strncasecmp(s, "Name", 4) == 0) {
	    strncpy(ci.name, s + 5, MAX_CT_NAME);
	} else if (strncasecmp(s, "bits", 4) == 0) {
	    ci.bits = atoi(s + 5);
	} else if (strncasecmp(s, "left", 4) == 0) {
	    ci.left = atoi(s + 5);
	} else if (strncasecmp(s, "nbps", 4) == 0) {
	    ci.nbps = atoi(s + 5);
	} else if (strncasecmp(s, "ncad", 4) == 0) {
	    ci.ncad = atoi(s + 5);
	} else if (strncasecmp(s, "ncda", 4) == 0) {
	    ci.ncda = atoi(s + 5);
	} else if (strncasecmp(s, "gdsr", 4) == 0) {
	    ci.gdsr = atoh(s + 5);
	} else if (strncasecmp(s, "ad_mv_fs", 8) == 0) {
	    getmv(s + 9, ci.ad_vfs);
	} else if (strncasecmp(s, "da_mv_fs", 8) == 0) {
	    getmv(s + 9, ci.da_vfs);
	} else if (i >= 0 && i < MAXNCT) {
	    _arsc_set_cardinfo(ci, i);
	    i = -1;
	}
    }
    fclose(fp);
} 

/***************************************************************************/

static int
subdevice_count(snd_ctl_t *h, int dev, int io, char *s)
{
    int err, count;
    snd_pcm_info_t *pcm_info;
    snd_pcm_stream_t stream;

    stream = io ? SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE;
    snd_pcm_info_alloca (&pcm_info);
    snd_pcm_info_set_stream (pcm_info, stream);
    snd_pcm_info_set_device (pcm_info, dev);
    snd_pcm_info_set_subdevice (pcm_info, 0);
    if ((err = snd_ctl_pcm_info (h, pcm_info)) < 0) {
	count = 0;
    } else {
	count = snd_pcm_info_get_subdevices_count (pcm_info);
    }
    sprintf(s, "%s", snd_pcm_info_get_id(pcm_info));

    return (count);
}

static long
init_dev()
{
    char ctl_name[64], crd_name[64], dev_name[64], name[64];
    int c0, c1, crd, dev, err, nd = 0;
    snd_ctl_t *h;

    if (!card_info) {
	snd_ctl_card_info_alloca(&card_info); 
    }
    crd = -1;
    while (1) {
        if (snd_card_next (&crd) < 0 || crd < 0) {
	    break;
        }
	sprintf (ctl_name, "hw:%d", crd);
	if ((err = snd_ctl_open (&h, ctl_name, 0)) < 0) {
	    continue;
	}
	if ((err = snd_ctl_card_info (h, card_info)) < 0) {
	    snd_ctl_close (h);
	    continue;
	}
        strcpy(crd_name, snd_ctl_card_info_get_name(card_info));
	dev = -1;
	while (1) {
	    if (snd_ctl_pcm_next_device (h, &dev) < 0)
		;
	    if (dev < 0)
		break;
	    c0 = subdevice_count(h, dev, 0, dev_name);
	    c1 = subdevice_count(h, dev, 1, dev_name);
	    sprintf(name, "%s %s", crd_name, dev_name);
	    strcpy(dcs[nd].dnm, name);
	    dcs[nd].crd = crd;
	    dcs[nd].dev = dev;
	    dcs[nd].ich = c0;
	    dcs[nd].och = c1;
	    nd++;
	}
	snd_ctl_close(h);
    }

    return (nd);
}

/* _ar_alsa_num_dev - return number of ALSA devices */

static int32_t
_ar_alsa_num_dev()
{
    if (ndv < 0) {
	ndv = init_dev();
    }

    return (ndv);
}

/* _ar_alsa_close - close I/O device */

static void
_ar_alsa_close(int32_t di)
{
    ARDEV *a;

    if (debug) {
	fprintf(stderr, "alsa_close\n");
    }
    a = _ardev[di];

    if (!opened[di]) {
	return;
    }

    if (linked[di]) {
	snd_pcm_unlink(hwi[di]);
	linked[di] = 0;
    }

    if (a->ncad) {
	reset_params(di, hwo[di]);
	snd_pcm_drop(hwi[di]);
	snd_pcm_hw_free(hwi[di]);
	snd_pcm_close(hwi[di]);
	hwi[di] = NULL;
	free_null(qi[di]);
    }

    if (a->ncda) {
	reset_params(di, hwo[di]);
	snd_pcm_drop(hwo[di]);
	snd_pcm_hw_free(hwo[di]);
	snd_pcm_close(hwo[di]);
	hwo[di] = NULL;
	free_null(qo[di]);
	free_null(td[di]);
    }

    free_null(bridge);
    opened[di] = 0;
    if (debug) {
	fprintf(stderr, "<-alsa_close\n");
    }
}

/* _ar_alsa_open - open I/O device */

static int32_t
_ar_alsa_open(int32_t di)
{
    char pcm_name[20];
    int mode, sbd, block = 0;
    long err = 0;
    ARDEV *a;

    if (debug) {
	fprintf(stderr, "alsa_open: di=%ld\n", (long) di);
    }
    a = _ardev[di];
    
    if (opened[di]) {
	_ar_alsa_close(di);
    }

    a->ntlv = 1;	    // always interleave channel samples

    if (a->ncda) {
	sbd = a->a_chnoff_o;
	sprintf(pcm_name, "hw:%d,%d,%d", dcs[di].crd, dcs[di].dev, sbd);
	mode = block ? 0 : SND_PCM_NONBLOCK;
        err = snd_pcm_open(&hwo[di], 
	    pcm_name, SND_PCM_STREAM_PLAYBACK, mode);
        test(err, 1);
	if (err)
	    return (100);
    }

    if (a->ncad) {
	sbd = a->a_chnoff_i;
	sprintf(pcm_name, "hw:%d,%d,%d", dcs[di].crd, dcs[di].dev, sbd);
	mode = block ? 0 : SND_PCM_NONBLOCK;
        err = snd_pcm_open(&hwi[di], 
	    pcm_name, SND_PCM_STREAM_CAPTURE, mode);
        test(err, 2);
	if (err)
	    return (100);
    }

    if (set_io_params(di)) {
	return (100);
    }

    linked[di] = 0;
    if (a->ncad && a->ncda && a->nbps > 2) {
	err = snd_pcm_link(hwi[di], hwo[di]);
        test(err, 20);
	if (err)
	    return (100);
	linked[di] = 1;
    }
    opened[di] = 1;
    if (debug) {
	fprintf(stderr, "alsa_open: ncad=%ld ncda=%ld ncda=%ld\n",
	    (long) a->ncad, (long) a->ncda, (long) a->nbps);
	fprintf(stderr, "<-alsa_open: opened=%d linked=%d \n",
	    opened[di], linked[di]);
    }

    return (0);
}

/* _ar_alsa_io_prepare - prepare device and buffers for I/O */

static int32_t
_ar_alsa_io_prepare(int32_t di)
{
    ARDEV *a;
    ARFMT  *f;

    if (debug) {
	fprintf(stderr, "alsa_io_prepare\n");
    }
    a = _ardev[di];
    f = &_arfmt[di]; 
    set_io_params(di); 
    if (debug) {
	fprintf(stderr, "a_nbps=%ld a_ntlv=%ld\n", (long) f->a_nbps, (long) f->a_ntlv);
	fprintf(stderr, "a_ncad=%ld a_ncda=%ld\n", (long) a->a_ncad, (long) a->a_ncda);
	fprintf(stderr, "nbps=%ld ntlv=%ld\n", (long) a->nbps, (long) a->ntlv);
	fprintf(stderr, "ncad=%ld ncda=%ld\n", (long) a->ncad, (long) a->ncda);
	fprintf(stderr, "nibp=%ld nobp=%ld\n", (long) a->nibp, (long) a->nobp);
	fprintf(stderr, "rate=%ld\n", (long) a->rate);
	fprintf(stderr, "card,dev=%d,%d", dcs[di].crd, dcs[di].dev);
	fprintf(stderr, "smptot=%ld\n", (long) a->smptot);
	fprintf(stderr, "buffer_size=%d\n", bsz);
	fprintf(stderr, "period_size=%d\n", psz);
	fprintf(stderr, "linked=%d\n", linked[di]);
    }
    free_null(qi[di]);
    free_null(qo[di]);
    free_null(td[di]);
    free_null(bridge);
    if (a->ncad) {
	qi[di] = calloc(a->segswp, sizeof(long));
	if (!qi[di])
	    return (1);
    }
    if (a->ncda) {
	qo[di] = calloc(a->segswp, sizeof(long));
	td[di] = calloc(a->segswp, sizeof(double));
	if (!qi[di])
	    return (2);
    }
    bridge = calloc(psz, 2 * a->nbps);
    if (!bridge)
       return (3);

    if (debug) {
	fprintf(stderr, "<-alsa_io_prepare\n");
    }
    return (0);
}

/* _ar_alsa_xfer_seg - this segment is ready to go */

static int32_t
_ar_alsa_xfer_seg(int32_t di, int32_t b)
{
    ARDEV *a;

    a = _ardev[di];
    if (a->ncad) {
	qi[di][b] = a->sizptr[b];
    }
    if (a->ncda) {
	qo[di][b] = a->sizptr[b];
    }
    return (0);
}

/* _ar_alsa_chk_seg - check for segment completion */

static int32_t
_ar_alsa_chk_seg(int32_t di, int32_t b)
{
    int    done;
    ARDEV *a;

    a = _ardev[di];
    if(a->ntlv) {
	done = chk_done_i(di, b);
    } else {
        done = chk_done_n(di, b);
    }
    if (done && a->ncda && !a->ncad) {
	done = chk_time_out(di, b);
    }

    return (done);
}

/* _ar_alsa_io_start - start I/O */

static void
_ar_alsa_io_start(int32_t di)
{
    int err;
    ARDEV *a;

    if (debug) {
	fprintf(stderr, "alsa_io_start\n");
    }
    a = _ardev[di];
    if (a->ncad) {
	err = snd_pcm_prepare(hwi[di]);
	test(err, 101);
	err = snd_pcm_nonblock(hwi[di], 1);
	test(err, 102);
    }
    if (a->ncda) {
	err = snd_pcm_prepare(hwo[di]);
	test(err, 103);
	err = snd_pcm_nonblock(hwo[di], 1);
	test(err, 104);
    }
    gettimeofday(&tv0, NULL);
    if (debug) {
	fprintf(stderr, "<-alsa_io_start\n");
    }
}

/* _ar_alsa_io_stop - stop I/O */

static void
_ar_alsa_io_stop(int32_t di)
{
    ARDEV *a;

    if (debug) {
	fprintf(stderr, "alsa_io_stop\n");
    }
    a = _ardev[di];
    if (a->ncda) {
	snd_pcm_drop(hwo[di]);
    }
    if (debug) {
	fprintf(stderr, "<-alsa_io_stop\n");
    }
}

/* _ar_alsa_dev_name - return name of I/O device */

static char   *
_ar_alsa_dev_name(int32_t di)
{
    return (dcs[di].dnm);
}

/* _ar_alsa_list_rates - create a list of available sample rates */

static int32_t
_ar_alsa_list_rates(int32_t di)
{
    if (debug) {
	fprintf(stderr, "alsa_list_rates\n");
    }
    return (int32_t) list_rates(di);
}

/* _ar_os_bind - bind ALSA functions, return number of devices */

int32_t
_ar_os_bind(int32_t ndt, int32_t tnd)
{
    char *e;
    long nd;

    e = getenv("ARSC_DEBUG");
    if (e){
	debug = atoi(e);
    }
    nd = _ar_alsa_num_dev();
    if (debug) {
	fprintf(stderr, "ARSC: debug=%d, nd=%ld\n", debug, nd);
    }
    if (nd > 0) {
        _ardvt[ndt].num_dev = _ar_alsa_num_dev;
	_ardvt[ndt].dev_name = _ar_alsa_dev_name;
	_ardvt[ndt].io_stop = _ar_alsa_io_stop;
	_ardvt[ndt].close = _ar_alsa_close;
	_ardvt[ndt].open = _ar_alsa_open;
	_ardvt[ndt].io_prepare = _ar_alsa_io_prepare;
	_ardvt[ndt].io_start = _ar_alsa_io_start;
	_ardvt[ndt].xfer_seg = _ar_alsa_xfer_seg;
	_ardvt[ndt].chk_seg = _ar_alsa_chk_seg;
	_ardvt[ndt].list_rates = _ar_alsa_list_rates;
	dio = tnd;
    }
    read_rc();

    return (nd);
}

/**************************************************************************/

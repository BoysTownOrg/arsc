/* arch_chk.c - check soundcard operation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "arsclib.h"
#ifdef WIN32
#include <io.h>
#include <Windows.h>
#else
#include <unistd.h>
#define _access access
#endif

#define round(x)        ((int)((x)+0.5))
#define limit(mn,aa,mx) (((aa)<(mn))?(mn):((aa)>(mx))?(mx):(aa))
#define MAXSTR   256
#define MAXLEN   80
#ifdef KB
#define ESC      27
int  _getch(void);
int  _kbhit(void);
void kbd_init(void);
void kbd_restore(void);
#endif

static char *ret_ptr = NULL;    // unused return value
static char str[MAXSTR+1];	// message buffer
static double rate = 32000;     // sampling rate
static double tdur = 8;         // observed output duration
static int ad_ch = 0;		// selected A/D channel
static int adcpol[MAXNCH];      // A/D polarity
static int da_ch = 0;		// selected D/A channel
static int dacpol[MAXNCH];      // D/A polarity
static int ict = 1;		// current card type
static int io_dev = 0;		// device identifier
static int nct = 2;		// number of card types
static int ct; 		// card type
static int setoff = 0;		// I/O termination flag
static CARDINFO ci[MAXNCT];

static void
msleep(long d)
{
#ifdef WIN32
    Sleep(d);
#else
    struct timespec delay = {0};
    delay.tv_nsec = d * 1000;
    nanosleep(&delay, &delay);
#endif
}

static int
input()
{
    char *s = str;
    int n = MAXSTR;

    fflush(stdin);
    ret_ptr = fgets(s, n, stdin);
    while (*s >= ' ')
	s++;
    *s = '\0';	// remove trailing control chars

    return (strlen(str));
}

static void
print_error(char *msg)
{
    fprintf(stderr, "ERROR: %s\n", msg);
}

static int
check_accept(int *x, int *y, int n, int cnt)
{
    // implement "artifact rejection" here
    // return 0 to reject sweep
    return (1);
}

static int
check_rule(int *x, int n, int cnt)
{
    // implement "measurement-based stopping rules" here
    // return 1 to stop averaging
    return (0);
}

static int
check_user()
{
    // allow user to terminate averaging immediately
    // return 1 to stop averaging
#ifdef KB
    if (_kbhit()) {
	if (_getch() == ESC) {
	    setoff++;
        }
    }
#endif
    return (setoff);
}

static double
vdot(float *x, float *y, int n)
{
    double d = 0;
    int i;

    for (i = 0; i < n; i++) {
	d += x[i] * y[i];
    }

    return (d);
}

static void
vmul(float *x, float *y, float *z, int n)
{
    int i;

    for (i = 0; i < n; i++) {
	z[i] = x[i] * y[i];
    }

}

static int
vmax(float *x, int n)
{
    int i, j = 0;

    for (i = 1; i < n; i++) {
	if (x[j] < x[i]) {
	   j = i;
	}
    }

    return (j);
}

static void
square(float *s, int n, double a, double p, double dc, double rate)
{
    int i, j, k, m;

    k = round(p * dc * rate);
    m = round(p * rate);
    for (i = 0; i < n; i += m) {
	for (j = 0; (j < k) && ((i + j) < n); j++) {
	    s[i + j] = (float) a;
	}
	for (j = k; (j < m) && ((i + j) < n); j++) {
	    s[i + j] = (float) -a;
	}
    }
}

static void
tone(float *s, int n, double a, double f, double rate)
{
    double p;
    int i;

    p = 2 * M_PI * f / rate;
    for (i = 0; i < n; i++) {
	s[i] = (float) (a * sin(p * i));
    }
}

static void
ramp(float *b, int n, double nr)
{
    double  s, a, p;
    int     i, j, m;

    p = M_PI / (2 * nr) ;
    m = (int) ceil(nr);
    for (i = 0; i < m; i++) {
	s = sin(p * i);
	a = s * s;
	j = n - 1 - i;
	b[i] *= (float) a;
	b[j] *= (float) a;
    }
}

static int
sync_avg(int id, float *x, float *y, double at,
    int npnt, int nswp, int nskp, int co, int ci)
{
    char msg[MAXLEN];
    double sc;
    int nchn = 2;
    int i, j, n, cur, cnt, prv, err, nseg, *ix[2], *iy[2], *jx, *jy;
    SINT4 *siz; 
    void **inn, **out; 
    static int nbuf = 2; 			/* double buffer */
    static SINT4 fmt[2] = {ARSC_DATA_I4, 0};	/* SINT4,non-interleaved */

    n = npnt;
    memset(y, 0, n * sizeof(float));	/* zero response */
    if (nswp < 1 || nchn < 1) {
	return (0);
    }
    /* configure stimulus & response buffers */
    for (i = 0; i < nbuf; i++) {
	ix[i] = (int *) calloc(n, sizeof(int));
	iy[i] = (int *) calloc(n, sizeof(int));
    }
    jx = ix[0];
    sc = pow(2, 31) - 1;
    for (j = 0; j < npnt; j++) {
        jx[j] = round(x[j] * sc);
	y[j]  = 0;
    }
    /* configure i/o buffers */
    nseg = nswp + nskp + 1;
    inn = (void **) calloc(nseg * nchn, sizeof(void *));
    out = (void **) calloc(nseg * nchn, sizeof(void *));
    siz = (SINT4 *) calloc(nseg, sizeof(SINT4));
    for (i = 0; i < nseg; i++) {
	out[i * nchn + co] = (i < (nswp + nskp)) ? ix[0] : NULL;
	inn[i * nchn + ci] = iy[i % nbuf];
	siz[i] = npnt;
    }
    /* initiate i/o */
    err = ar_set_fmt(id, fmt);
    if (err) {
	ar_err_msg(err, msg, MAXLEN);
	print_error(msg);
	return (0);
    }
    err = ar_io_prep(id, inn, out, siz, nseg, nseg);
    if (err) {
	ar_err_msg(err, msg, MAXLEN);
	print_error(msg);
	return (0);
    }
    err = ar_io_start(id);
    if (err) {
	ar_err_msg(err, msg, MAXLEN);
	print_error(msg);
	return (0);
    }
    jx = ix[1];
    cur = cnt = prv = 0;
    while (cur < nseg && cnt < nswp) {
	cur = ar_io_cur_seg(id);
	if (cur != prv && cur > nskp && cnt < nswp) {
	    jy = iy[(cur - 1) % nbuf];
	    if (check_accept(jx, jy, n, cnt)) {
                for (j = 0; j < n; j++) {
                    y[j] += jy[j];
                }
	        cnt++;
	    }
	    prv = cur;
            if (check_rule(jx, n, cnt)) {
                nswp = cnt;
	    }
	}
        if (check_user()) {
              break;
	}
    }
    for (j = 0; j < npnt; j++) {
        y[j] /= (float) sc;
    }
    /* de-allocate buffers */
    for (i = 0; i < nbuf; i++) {
	free(ix[i]);
	free(iy[i]);
    }
    free(inn);
    free(out);
    free(siz);

    return (cnt);
} 

// reduce channel number by pairwise offset
static void
chan_pair(int *c, int *o, int nc)
{
    *o = 0;
    while (*c > nc) {
	*c = *c - nc;
	*o = *o + nc;
    }
}

static int
get_response(int np, int nc, int oc, int ic, float *st, float *rs)
{
    int cnt, oi = 0, oo = 0;
    static double at = 0;
    static int na = 1;
    static int sk = 0;

    chan_pair(&oc, &oo, nc);
    chan_pair(&ic, &oi, nc);
    ar_io_open_off(io_dev, rate, nc, nc, oi, oo);
#ifdef KB
    setoff = 0;
    kbd_init();
#endif
    cnt = sync_avg(io_dev, st, rs, at, np, na, sk, oc, ic);
#ifdef KB
    kbd_restore();
#endif
    ar_io_close(io_dev);

    return (cnt);
}

void
device_select()
{
    char name[ARSC_NAMLEN];
    int n, i;

    printf("\n");
    n = ar_num_devs();
    if (n < 1) {
        printf("No devices!\n\n");
    } else if (n == 1) {
        printf("Only one device!\n\n");
    } else {
        for (i = 0; i < n; i++) {
            if (ar_dev_name(i, name, ARSC_NAMLEN)) {
 	        break;
	    }
            printf("%s %d = %s\n", (i == io_dev) ? ">" : " ", i + 1, name);
        }
        printf("\nWhich device (1-%d) [%d]? ", n, io_dev + 1);
	input();
	if (str[0] >= '0' && str[0] <= '9') {
            io_dev = atoi(str) - 1;
	}
    }
    ct = (int) ar_get_cardinfo(io_dev, &ci[ict]);	// selected device
    printf("\nWhich card type (1-%d) [%d]? ", nct, ict);
    input();
    if (str[0] >= '0' && str[0] <= '9') {
        ict = atoi(str);
	ict = limit(0, ict, nct);
	nct = limit(ict + 1, nct, MAXNCT);
    }
    printf("\nSampling rate [%.0f]? ", rate);
    input();
    if (str[0] >= '0' && str[0] <= '9') {
        rate = atof(str);
    }
}

void
device_name()
{
    char name[ARSC_NAMLEN];

    printf("device #%d: cardtype=%d\n", io_dev + 1, ict);
    if (ar_dev_name(io_dev, name, ARSC_NAMLEN)) {
        printf("device identifier (%d) is invalid\n", io_dev + 1);
    } else {
	printf("%s\n", name);
	strncpy(ci[ict].name, name, ARSC_NAMLEN);
    }
}

void
set_duration()
{
    double d;

    printf("\nConfirm I/O duration (s) [%.0f]: ", tdur);
    if (input()) {
	d = atof(str);
	if (d > 0 && d < 99) {
	    tdur = d;
	}
    }
}

double
polarity(int oc, int ic, double vt, double td, double dc)
// oc - output channel
// ic - input channel
// vt - volt target
// td - stimulus duration
// dc - duty cycle
{
    double a, vfs, r, rs_st, rs_rs, st_st;
    float *st, *rs;
    int np;
    static double p = 0.1;
    static int nc = 2;

    vfs = ci[ict].da_vfs[oc];
    a = sqrt(2) * fabs(vt / vfs);
    np = round(td * rate);
    st = (float *) calloc(np, sizeof(float));
    rs = (float *) calloc(np, sizeof(float));
    square(st, np, a, p, dc, rate);
    get_response(np, nc, oc, ic, st, rs);
    rs_st = vdot(rs, st, np);
    rs_rs = vdot(rs, rs, np);
    st_st = vdot(st, st, np);
    r = rs_st / sqrt(rs_rs * st_st);
    free(st);
    free(rs);

    return (r);
}

void
init_polarity()
{
    int c;

    for (c = 0; c < ci[ict].ncda; c++) {
	dacpol[c] = (ci[ict].da_vfs[c] < 0) ? -1 : 1;
    }
    for (c = 0; c < ci[ict].ncad; c++) {
	adcpol[c] = (ci[ict].ad_vfs[c] < 0) ? -1 : 1;
    }
}

void
dac_polarity()
{
    double vfs;
    int n, p;
    static double dc = 0.1;
    static double vt = 1.0;

    printf("\nConfirm D/A channel [%d]: ", da_ch + 1);
    if (input()) {
	n = atoi(str);
	if (n >= 1 && n <= ci[ict].ncda) {
	    da_ch = n - 1;
	}
	printf(" --> D/A channel = %d\n", da_ch + 1);
    }
    vfs = ci[ict].da_vfs[da_ch];
    printf("\nObserve D/A polarity on scope...");
    fflush(stdout);
    (void) polarity(da_ch, ad_ch, vt, tdur, dc);
    if (setoff) {
	return;
    }
    p = (vfs < 0) ? -1 : 1;
    printf("\nConfirm D/A polarity (1 or -1) [%d]: ", p);
    if (input()) {
	p = atoi(str);
    }
    if (p != 0) {
	dacpol[da_ch] = (p < 0) ? -1 : 1;
    }
}

void
adc_polatity()
{
    double r;
    int n, p;
    static double dc = 0.5;
    static double td = 1.0;
    static double vt = 1.0;

    printf("\nConfirm D/A channel [%d]: ", da_ch + 1);
    if (input()) {
	n = atoi(str);
	if (n >= 1 && n <= ci[ict].ncda) {
	    da_ch = n - 1;
	}
        printf(" --> D/A channel = %d\n", da_ch + 1);
    }
    printf("\nConfirm A/D channel [%d]: ", ad_ch + 1);
    if (input()) {
	n = atoi(str);
	if (n >= 1 && n <= ci[ict].ncad) {
	    ad_ch = n - 1;
	}
        printf(" --> A/D channel = %d\n", ad_ch + 1);
    }
    printf("\nMeasuring A/D polarity...");
    fflush(stdout);
    r = polarity(da_ch, ad_ch, vt, td, dc) * dacpol[da_ch];
    p = (r < -0.1) ? -1 : (r > 0.1) ? 1 : 0; 
    printf("\nConfirm A/D polarity (1 or -1) [%d]: ", p);
    if (input()) {
	p = atoi(str);
    }
    if (p != 0) {
	adcpol[ad_ch] = (p < 0) ? -1 : 1;
    }
}

double
play_tone(int oc, int ic, double vt, double td)
// oc - output channel
// ic - input channel
// vt - volt target
// td - stimulus duration
{
    double a, vfs, r, rs_rs, st_st;
    float *st, *rs;
    int np;
    static double f = 1000; // tone frequency
    static double d = 0.01; // ramp duration
    static int nc = 2;

    vfs = ci[ict].da_vfs[oc];
    a = sqrt(2) * fabs(vt / vfs);
    np = round(td * rate);
    st = (float *) calloc(np, sizeof(float));
    rs = (float *) calloc(np, sizeof(float));
    tone(st, np, a, f, rate);
    ramp(st, np, d * rate);
    get_response(np, nc, oc, ic, st, rs);
    rs_rs = vdot(rs, rs, np);
    st_st = vdot(st, st, np);
    r = sqrt(rs_rs) / sqrt(st_st);
    free(st);
    free(rs);

    return (r);
}

void
dac_vfs()
{
    double vfs, vrms;
    int n;
    static double vt = 1.0;

    printf("\nConfirm D/A channel [%d]: ", da_ch + 1);
    if (input()) {
	n = atoi(str);
	if (n >= 1 && n <= ci[ict].ncda) {
	    da_ch = n - 1;
	}
        printf(" --> D/A channel = %d\n", da_ch + 1);
    }
    printf("\nObserve D/A volts-rms on DVM...");
    fflush(stdout);
    (void) play_tone(da_ch, ad_ch, vt, tdur);
    if (setoff) {
	return;
    }
    vrms = vt;
    printf("\nConfirm D/A volts-rms [%.3f]: ", vrms);
    if (input()) {
	vrms = atof(str);
    }
    vfs = dacpol[da_ch] * fabs(ci[ict].da_vfs[da_ch] * vrms / vt);
    n = da_ch + 1;
    if (fabs(vfs) > 0.1) {
        ci[ict].da_vfs[da_ch] = vfs;
        printf("D/A channel %d volts-full-scale = %.3f\n", n, vfs);
    } else {
        printf("D/A channel %d volts-full-scale too low!\n", n);
    }

}

void
adc_vfs()
{
    double r, vfs, vrms;
    int n;
    static double vt = 1.0;

    printf("\nConfirm D/A channel [%d]: ", da_ch + 1);
    if (input()) {
	n = atoi(str);
	if (n >= 1 && n <= ci[ict].ncda) {
	    da_ch = n - 1;
	}
        printf(" --> D/A channel = %d\n", da_ch + 1);
    }
    printf("\nConfirm A/D channel [%d]: ", ad_ch + 1);
    if (input()) {
	n = atoi(str);
	if (n >= 1 && n <= ci[ict].ncad) {
	    ad_ch = n - 1;
	}
        printf(" --> A/D channel = %d\n", ad_ch + 1);
    }
    printf("\nObserve A/D volts-rms on DVM...");
    fflush(stdout);
    r = play_tone(da_ch, ad_ch, vt, tdur);
    if (setoff) {
	return;
    }
    vrms = vt;
    printf("\nConfirm A/D volts-rms [%.3f]: ", vrms);
    if (input()) {
	vrms = atof(str);
    }
    vfs = adcpol[ad_ch] * fabs(ci[ict].da_vfs[da_ch] * vrms / vt) / r;
    n = ad_ch + 1;
    if (fabs(vfs) < 0.01) {
        printf("A/D channel %d volts-full-scale too low! (%.3f)\n", n, vfs);
    } else if (fabs(vfs) > 100) {
        printf("A/D channel %d volts-full-scale too high! (%.0f)\n", n, vfs);
    } else {
        ci[ict].ad_vfs[ad_ch] = vfs;
        printf("A/D channel %d volts-full-scale = %.3f\n", n, vfs);
    }
}

int
device_vfs()
{
    init_polarity();
    while (1) {
	printf("\n");
	device_name();
	printf("\n");
        printf("1 - Set I/O duration (%.0f s)\n", tdur);
        printf("2 - Check D/A polarity\n");
        printf("3 - Check A/D polarity\n");
        printf("4 - Check D/A volts-full-scale\n");
        printf("5 - Check A/D volts-full-scale\n");
        printf("6 - Return\n");
        printf("\nDo what? ");
	input();
        switch (str[0]) {
        case '1':
            set_duration();
            break;
        case '2':
            dac_polarity();
            break;
        case '3':
            adc_polatity();
            break;
        case '4':
            dac_vfs();
            break;
        case '5':
            adc_vfs();
            break;
        case '6':
        case 'q':
            return (0);
        case 'x':
            exit(0);
	}
    }
}

void
device_output()
{
    char msg[MAXLEN];
    double vfs;
    float *s, *z;
    int i, j, np, cseg, err;
    SINT4 siz[4];
    void *out[8];
    static double ft = 1000;
    static double rd = 0.010;
    static double td = 0.500;
    static double vo = 1.00;	// volts out
    static int nchn = 2;
    static int nseg = 4;
    static int nswp = 1;
    static int lseg = 1;
    static SINT4 fmt[2] = {ARSC_DATA_F4, 0};

    vfs = ci[ict].da_vfs[0];
    np = round((td + rd) * rate);
    s = (float *) calloc(np, sizeof(float));
    z = (float *) calloc(np, sizeof(float));
    tone(s, np, vo / vfs, ft, rate);
    ramp(s, np, rd * rate);
    err = ar_out_open(io_dev, rate, 2);
    if (err) {
	ar_err_msg(err, msg, MAXLEN);
	print_error(msg);
	free(s);
	free(z);
	return;
    }
    ar_set_fmt(io_dev, fmt);
    printf("output: ");
    fflush(stdout);
    for (i = 0; i < nseg; i++) {
        siz[i] = np;
	for (j = 0; j < nchn; j++) {
            out[i * nchn + j] = ((j * 2) == i) ? s : z;
	}
    }
    ar_out_prepare(io_dev, out, siz, nseg, nswp);
    ar_io_start(io_dev);
    cseg = lseg = -1;
    while(cseg < nseg) {
	if (cseg == lseg) {
	    msleep(10);
	} else {
	    if ((cseg % 2) == 0) {
		printf("%d...", cseg / 2 + 1);
		fflush(stdout);
	    }
	    lseg = cseg;
	}
	cseg = ar_io_cur_seg(io_dev);
    }
    printf("\n");
    ar_io_close(io_dev);
    free(s);
    free(z);
}

double
latency(int oc, int ic, double vt, double td)
// oc - output channel
// ic - input channel
// vt - volt target
// td - stimulus duration
{
    double a, vfs, d, t;
    float *st, *rs;
    int np, m;
    static int nc = 2;

    vfs = ci[ict].da_vfs[oc];
    a = sqrt(2) * fabs(vt / vfs);
    np = round(td * rate);
    st = (float *) calloc(np, sizeof(float));
    rs = (float *) calloc(np, sizeof(float));
    st[0] = (float) a;
    get_response(np, nc, oc, ic, st, rs);
    vmul(rs, rs, rs, np);
    m = vmax(rs, np);
    d = (rs[m-1]-rs[m+1])/(rs[m-1]-2*rs[m]+rs[m+1])/2;
    t = (m + d) / rate;
    free(st);
    free(rs);

    return (t);
}

void
device_latency()
{
    double t, tsm, tss, d;
    int i, n;
    static double td = 0.05;
    static double vt = 1.0;
    static int na = 80;

    printf("\nConfirm D/A channel [%d]: ", da_ch + 1);
    if (input()) {
	n = atoi(str);
	if (n >= 1 && n <= ci[ict].ncda) {
	    da_ch = n - 1;
	}
        printf(" --> D/A channel = %d\n", da_ch + 1);
    }
    printf("\nConfirm A/D channel [%d]: ", ad_ch + 1);
    if (input()) {
	n = atoi(str);
	if (n >= 1 && n <= ci[ict].ncad) {
	    ad_ch = n - 1;
	}
        printf(" --> A/D channel = %d\n", ad_ch + 1);
    }
    printf("\nMeasuring I/O latency...");
    fflush(stdout);
    tsm = tss = 0;
    for (i = 0; i < na; i++) {
	t = latency(da_ch, ad_ch, vt, td);
	tsm += t;
	tss += t * t;
    }
    printf("\n\n");
    t = tsm / na;
    d = sqrt(tss / na - t * t);
    printf("I/O latency = %.3f, sd = %.3f (ms)\n", 1000 * t, 1000 * d);
}

void
edit_info()
{
    double mv;
    int j, n;

    printf("Name=%s: ", ci[ict].name);
    if (input()) {
	strncpy(ci[ict].name, str, MAX_CT_NAME);
    }
    printf("bits=%d:", ci[ict].bits);
    if (input()) {
	n = atoi(str);
	if (n > 0) {
	   ci[ict].bits = n;
	}
    }
    printf("left=%d:", ci[ict].left);
    if (input()) {
	n = atoi(str);
	if (n >= 0) {
	   ci[ict].left = n;
	}
    }
    printf("nbps=%d:", ci[ict].nbps);
    if (input()) {
	n = atoi(str);
	if (n > 0) {
	   ci[ict].nbps = n;
	}
    }
    printf("ncad=%d:", ci[ict].ncad);
    if (input()) {
	n = atoi(str);
	if (n > 0) {
	   ci[ict].ncad = n;
	}
    }
    printf("ncda=%d:", ci[ict].ncda);
    if (input()) {
	n = atoi(str);
	if (n > 0) {
	   ci[ict].ncda = n;
	}
    }
    for (j = 0; j < ci[ict].ncad; j++) {
	mv = ci[ict].ad_vfs[j] * 1000;
	printf("ad%d_mv_fs=%.0f:", j + 1, mv);
	if (input()) {
	    mv = atof(str);
	    if (mv != 0) {
	        ci[ict].ad_vfs[j] = mv / 1000;
	    }
	}
    }
    for (j = 0; j < ci[ict].ncda; j++) {
	mv = ci[ict].da_vfs[j] * 1000;
	printf("da%d_mv_fs=%.0f:", j + 1, mv);
	if (input()) {
	    mv = atof(str);
	    if (mv != 0) {
	        ci[ict].da_vfs[j] = mv / 1000;
	    }
	}
    }
}

void
write_registry_file(char *fn)
{
    int i, j, mvfs;
    FILE *fp;

    fp = fopen(fn, "wt");
    if (fp == NULL)
	return;
    fputs("REGEDIT4\n\n", fp);
    fputs("[HKEY_LOCAL_MACHINE\\SOFTWARE\\BTNRH]\n\n", fp);
    fputs("[HKEY_LOCAL_MACHINE\\SOFTWARE\\BTNRH\\ARSC]\n\n", fp);
    fputs("[HKEY_LOCAL_MACHINE\\SOFTWARE\\BTNRH\\ARSC\\CardInfo]\n\n", fp);
    strcpy(ci[0].name, "Default");
    for (i = 0; i < nct; i++) {
        if (ci[i].name[0] == 0) continue;
        fputs("[HKEY_LOCAL_MACHINE\\SOFTWARE\\BTNRH\\ARSC\\CardInfo", fp);
	fprintf(fp, "\\CardType%d]\n", i);
	fprintf(fp, "\"Name\"=\"%s\"\n", ci[i].name);
	fprintf(fp, "\"bits\"=dword:%08X      ; %2d\n", ci[i].bits, ci[i].bits);
	fprintf(fp, "\"left\"=dword:%08X\n", ci[i].left);
	fprintf(fp, "\"nbps\"=dword:%08X\n", ci[i].nbps);
	fprintf(fp, "\"ncad\"=dword:%08X\n", ci[i].ncad);
	fprintf(fp, "\"ncda\"=dword:%08X\n", ci[i].ncda);
	fprintf(fp, "\"gdsr\"=dword:%08X\n", ci[i].gdsr);
	for (j = 0; j < ci[i].ncad; j++) {
            mvfs = round(ci[i].ad_vfs[j] * 1000);
	    fprintf(fp, "\"ad%d_mv_fs\"=dword:%08X ; %5d\n", j + 1, mvfs, mvfs);
	}
	for (j = 0; j < ci[i].ncda; j++) {
            mvfs = round(ci[i].da_vfs[j] * 1000);
	    fprintf(fp, "\"da%d_mv_fs\"=dword:%08X ; %5d\n", j + 1, mvfs, mvfs);
	}
        fputs("\n", fp);
    }
    fclose(fp);
}

void
write_rc_file(char *fn)
{
    int i, j, mvfs;
    FILE *fp;

    fp = fopen(fn, "wt");
    if (fp == NULL)
	return;
    fprintf(fp, "# %s - ARSC card info\n\n", fn);
    strcpy(ci[0].name, "Default");
    for (i = 0; i < nct; i++) {
        if (ci[i].name[0] == 0) continue;
	fprintf(fp, "[CardType%d]\n", i);
	fprintf(fp, "Name=%s\n", ci[i].name);
	fprintf(fp, "bits=%d\n", ci[i].bits);
	fprintf(fp, "left=%d\n", ci[i].left);
	fprintf(fp, "nbps=%d\n", ci[i].nbps);
	fprintf(fp, "ncad=%d\n", ci[i].ncad);
	fprintf(fp, "ncda=%d\n", ci[i].ncda);
	fprintf(fp, "gdsr=%08X\n", ci[i].gdsr);
	fprintf(fp, "ad_mv_fs=");
	for (j = 0; j < ci[i].ncad; j++) {
            mvfs = round(ci[i].ad_vfs[j] * 1000);
	    fprintf(fp, " %5d", mvfs);
	}
        fputs("\n", fp);
	fprintf(fp, "da_mv_fs=");
	for (j = 0; j < ci[i].ncda; j++) {
            mvfs = round(ci[i].da_vfs[j] * 1000);
	    fprintf(fp, " %5d", mvfs);
	}
        fputs("\n", fp);
        fputs("\n", fp);
    }
    fclose(fp);
}

void
write_info(int type)
{
    char *fn;

    if (type == 0) {
        fn = "arsc.reg";
        write_registry_file(fn);
    } else {
#ifdef WIN32
        fn = "arsc.cfg";
#else
        fn = "arscrc";
#endif
        write_rc_file(fn);
    }
    printf("\nCard info written to %s.\n", fn);
}

void
display_info()
{
    char name[ARSC_NAMLEN];
    int c;

    printf("\n");
    if (ar_dev_name(io_dev, name, ARSC_NAMLEN)) {
        printf("device identifier (%d) is invalid\n", io_dev + 1);
    } else {
        printf("old card_type = %d\n", ct);
        printf("new card_type = %d\n\n", ict);
        printf("name = %s\n", ci[ict].name);
	printf("bits = %d\n", ci[ict].bits); 
	printf("left = %d\n", ci[ict].left);
	printf("nbps = %d\n", ci[ict].nbps);
	printf("ncad = %d\n", ci[ict].ncad);
	printf("ncda = %d\n", ci[ict].ncda);
	printf("ad_vfs =");
	for (c = 0; c < ci[ict].ncad; c++) {
	    printf(" %6.3f", ci[ict].ad_vfs[c]);
	}
        printf("\n");
	printf("da_vfs =");
	for (c = 0; c < ci[ict].ncda; c++) {
	    printf(" %6.3f", ci[ict].da_vfs[c]);
	}
        printf("\n");
    }
}

void
card_info()
{
    display_info();
    while (1) {
	printf("\n");
        printf("1 - Edit cardinfo\n");
        printf("2 - Write registry file\n");
        printf("3 - Write config file\n");
        printf("4 - Return\n");
        printf("\nDo what? ");
	input();
        switch (str[0]) {
        case '1':
            edit_info();
            break;
        case '2':
            write_info(0);
            break;
        case '3':
            write_info(1);
            break;
        case '4':
        case 'q':
            return;
        case 'x':
            exit(0);
        default:
            display_info();
	}
    }
}

int
do_loop()
{
    while (1) {
	printf("\n");
	device_name();
	printf("\n");
        printf("1 - Select device & cardtype\n");
        printf("2 - Check device volts-full-scale\n");
        printf("3 - Check device output\n");
        printf("4 - Check device latency\n");
        printf("5 - Check cardinfo\n");
        printf("6 - Exit\n");
        printf("\nDo what? ");
	input();
        switch (str[0]) {
        case '1':
            device_select();
            break;
        case '2':
            device_vfs();
            break;
        case '3':
            device_output();
            break;
        case '4':
            device_latency();
            break;
        case '5':
            card_info();
            break;
        case '6':
        case 'q':
            return (0);
        case 'x':
            exit(0);
	}
    }
}

int
main(int ac, char **av)
{
    printf("\n");
    printf("------ Soundcard Check ------\n");
    printf("%s\n", ar_version());
    io_dev = ar_find_dev(ARSC_PREF_SYNC);
    printf("-----------------------------\n");
    ct = ar_get_cardinfo(0,      &ci[0]);	// default device
    ct = ar_get_cardinfo(io_dev, &ci[1]);	// selected device
    ict = 1;
    do_loop();
    return (0);
}


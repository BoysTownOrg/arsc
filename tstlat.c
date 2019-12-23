/* tstlat.c - test soundcard latency */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "arsclib.h"

static double sr = 44100;	// sampling rate
static double *rd = NULL;
static int io_dev = 0;
static int cqper = 0;
static int nclk = 64;
static int lflag = 0;	
static int qflag = 0;	
static int enter = 0;
static int zflag = 0;
static int nsmp = 0;
static int16_t *cc;
static int16_t *ml;

static int
check_resp(int32_t *b, int n)
{
    double sm0, sm1, sm2, a, dc;
    int     i, i1, i2, c, m = 0;
    int32_t   bv, mxbv = 0;

    for (i = 0; i < n; i++) {
	bv = b[i];
	if (mxbv < bv) {
	    mxbv = bv;
	    m = i;
	}
	if (mxbv < -bv) {
	    mxbv = -bv;
	    m = i;
	}
    }
    if (qflag) { // check click response quality
	i1 = m - n / 200;
	if (i1 < 0)
	    i1 = 0;
	i2 = i1 + n / 100;
	sm0 = sm1 = sm2 = 0;
	c = 0;
        for (i = 0; i < n; i++) {
	    if (i < i1 || i >= i2) {
		sm0 += b[i];
		c++;
	    }
	}
	dc = sm0 / c;
        for (i = 0; i < n; i++) {
	    a = b[i] - dc;
	    if (i >= i1 && i < i2)
		sm1 += a * a;
	    sm2 += a * a;
	}
	cqper = (int) floor(100 * sqrt(sm1 / sm2) + 0.5);
    } else {
        cc[m]++;
    }

    return (m);
}

// sum_rsp - sum response

static void
sum_rsp(int32_t *rl, int nsmp)
{
    int i;

    if (rd == NULL) {
	rd = (double *) calloc(nsmp, sizeof(double));
    }
    if (rl && rd) {
	for (i = 0; i < nsmp; i++) {
	    rd[i] += (double) rl[i];
	}
    }
}

// avg_rsp - average response

static void
avg_rsp(int nsmp, int nclk, char * name)
{
    int i;
    FILE *fp;
    static char *fn = "tstlat.txt";

    if (rd && nclk > 0) {
        fp = fopen(fn, "wt");
	if (fp) {
	    fprintf(fp, "; %s\n", fn);
	    fprintf(fp, "; i/o device: %s\n", name);
	    fprintf(fp, ";\n");
	    fprintf(fp, "tlabel=%s\n", name);
	    fprintf(fp, "size=%d\n", nsmp);
	    fprintf(fp, "count=%d\n", nclk);
	    fprintf(fp, "rate=%.0f\n", sr);
	    fprintf(fp, ";\n");
	    for (i = 0; i < nsmp; i++) {
		fprintf(fp, "%9.3f\n", rd[i] / nclk);
	    }
	    fclose(fp);
	}
	free(rd);
	rd = NULL;
    }
}

// tst - check error code

static void
tst(int e)
{
    char msg[ARSC_MSGLEN];
    FILE *fp;

    if (e) { // write error message to file, if any
	fp = fopen("arsc_tst.log", "wt");
	ar_err_msg(e, msg, ARSC_MSGLEN);
	fprintf(fp, "error code %d: %s\n", e, msg);
	fclose(fp);
#ifndef WINGUI
	fprintf(stderr, "error code %d: %s\n", e, msg);
#endif
	exit(1);
    }
}

// test_io - test input/output

static int
test_io(int c)
{
    int d, m, nc = 2;
    int32_t *sl, *rl, fmt[2];
    void *od[2], *id[2];
    static int32_t nseg = 1;	// number of segments
    static int32_t nswp = 1;	// number of sweeps

    // set up i/o

    d = io_dev;
    fmt[0] = 4;			    // sample format = int32_t
    fmt[1] = 0;			    // interleave = no
    tst(ar_set_fmt(d, fmt));	    // set format
    tst(ar_io_open(d, sr, nc, nc)); // open i/o device
    sr = ar_get_rate(d);	    // get device sampling rate

    // set up stimulus & response buffers

    id[0] = calloc(nsmp * nc, sizeof(int32_t));
    od[0] = calloc(nsmp * nc, sizeof(int32_t));
    id[1] = (void *) ((int32_t *)id[0] + nsmp);
    od[1] = (void *) ((int32_t *)od[0] + nsmp);
    rl = id[c];
    sl = od[c];
    sl[0] = 0x7FFFFFFF;

    // perform i/o

    tst(ar_io_prepare(d, id, od, (int32_t *) &nsmp, nseg, nswp));
    tst(ar_io_start(d));
    while (ar_io_cur_seg(d) < nseg) {
	continue;
    }
    tst(ar_io_stop(d));

    // examine response buffer

    m = check_resp(rl, nsmp);

    // sum response

    sum_rsp(rl, nsmp);

    // clean up stimulus & response buffers

    free(id[0]);
    free(od[0]);

    // clean up i/o

    tst(ar_io_close(d));

    return (m);
}

static int
compute_std_dev(int16_t *c, int n, double *mn, double *sd)
{
    int i, cnt = 0;
    double v, smv = 0, ssv = 0;

    for (i = 0; i < n; i++) {
	v = i;
	smv += v * c[i];
	ssv += v * v * c[i];
	cnt += c[i];
    }
    if (mn)
        *mn = cnt ? smv / cnt : 0;
    if (sd)
        *sd = (cnt > 1) ? sqrt((ssv - smv * smv / cnt) / (cnt - 1)) : 0;

    return (cnt);
}

void
devlst(int io_dev)
{
    char name[ARSC_NAMLEN];
    int i;

    printf("-- i/o device list --\n");
    for (i = 0; i < 20; i++) {
	if (ar_dev_name(i, name, ARSC_NAMLEN))
	    break;
	printf("%s %d = %s\n", (i == io_dev) ? ">" : " ", i, name);
    }
}

void
done(int status)
{
    if (enter) {
        printf("[Press Enter]\n");
	getchar();
    }
    exit(status);
}

void
usage()
{
    printf("usage: tstlat [-option] [device_number]\n");
    printf("options:\n");
    printf("-bN  set buffer size to N\n");
    printf("-cN  channel number (0 or 1)\n");
    printf("-h   print usage\n");
    printf("-l   list devices\n");
    printf("-nN  set number of clicks to N\n");
    printf("-p   pause before exiting\n");
    printf("-q   check click response quality\n");
    printf("-sN  set sampling rate to N\n");
    printf("-z   zero click\n");
    printf("\n");
    printf("Soundcard should be in loopback configuration.\n");
    done(0);
}

int
main(int ac, char **av)
{
    char name[ARSC_NAMLEN], *q;
    double sd, mn, sc;
    int i, n, c = 0;
    FILE *fp;
    static char *fn = "tstlat.log";

    io_dev = ar_find_dev(ARSC_PREF_SYNC);

    // check command line option
    while (ac > 1) {
	if (av[1][0] == '-') {
	    if (av[1][1] == 'b') {
		if (av[1][2])
		    nsmp = atoi(&av[1][2]);
	    } else if (av[1][1] == 'c') {
		c = 1;
	    } else if (av[1][1] == 'h') {
		usage();
	    } else if (av[1][1] == 'l') {
		lflag++;
	    } else if (av[1][1] == 'n') {
		nclk = atoi(&av[1][2]);
	    } else if (av[1][1] == 'p') {
		enter++;
	    } else if (av[1][1] == 'q') {
		qflag++;
	    } else if (av[1][1] == 's') {
		sr = atof(&av[1][2]);
	    } else if (av[1][1] == 'z') {
		zflag++;
	    }
	} else {
	    n = atoi(av[1]);
	    if (n > 0) {
		io_dev = n - 1;
	    } else {
		io_dev = ar_find_dev_name(av[1]);
	    }
	}
	ac--;
	av++;
    }
    ar_set_latency(io_dev, 0);

    // say hello

    printf("%s\n", ar_version());
    if (lflag) {
	devlst(io_dev);
	done(0);
    }
    printf("-- i/o loopback latency test --\n");
    if (ar_dev_name(io_dev, name, ARSC_NAMLEN)) {
	printf("device identifier = %d is invalid\n", io_dev);
	done(0);
    }
    printf("device (%d): %s\n", io_dev + 1, name);

    // set sampling rate

    if (nsmp == 0)
	nsmp = 2048;
    cc = (int16_t *) calloc(nsmp, sizeof(int16_t));
    printf(" sampling rate = %.0f Hz\n", sr);
    printf(" buffer length = %d samples\n", nsmp);

    // check click quality

    if (qflag) {
	test_io(c);
	printf(" click response quality = %d%%\n", cqper);
	qflag = 0;
    }

    // test latency 

    ml = (int16_t *) calloc(nclk, sizeof(int16_t));
    for (i = 0; i < nclk; i++) {
	ml[i] = test_io(c);
        fputs(".", stdout);
	fflush(stdout);
    }
    fputs("\n", stdout);

    // report

    printf(" smpl | msec | count\n");
    avg_rsp(nsmp, nclk, name);
    sc = 1000 / sr;
    for (i = 0; i < nsmp; i++) {
	if (cc[i] > 0)
	    printf("%6d %6.3f: %d\n", i, i * sc, cc[i]);
    }
    n = compute_std_dev(cc, nsmp, &mn, &sd);
    if (mn < 1)
	q = "no latency!";
    else if (sd < 0.01)
	q = "outstanding";
    else if(sd < 0.1)
	q = "excellent";
    else if(sd < 1)
	q = "good";
    else if(sd < 10)
	q = "fair";
    else 
	q = "poor";
    printf("total response count = %d\n", n);
    if (n == 1) {
        printf("latency = %.3f msec = %.3f samples\n", mn * sc, mn);
    } else if (n > 1) {
        printf("mean_latency = %5.3f msec = %6.3f samples\n", mn * sc, mn);
	printf("     std_dev = %5.3f msec = %6.3f samples\n", sd * sc, sd);
	printf("latency consistency is %s\n", q);
    }
    fp = fopen(fn, "wt");
    fprintf(fp,"%s - device = %s\n", fn, name);
    fprintf(fp,"clk#  samp#  delay (ms)\n");
    for (i = 0; i < nclk; i++) {
	fprintf(fp, "%4d %6d %9.3f\n", i, ml[i], ml[i] * (1000 / sr));
    }
    if (n == 1) {
        fprintf(fp, "latency = %.3f msec = %.3f samples\n", mn * sc, mn);
    } else if (n > 1) {
        fprintf(fp, "mean_latency = %.3f msec = %.3f samples\n", mn * sc, mn);
	fprintf(fp, "std_dev = %.3f msec = %.3f samples\n", sd * sc, sd);
	fprintf(fp, "latency consistency is %s\n", q);

    }
    fclose(fp);
    free(cc);
    free(ml);
    done(0);

    return (0);
}

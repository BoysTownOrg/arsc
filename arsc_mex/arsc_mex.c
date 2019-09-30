/* arsc_mex.c - MATLAB interface to ARSC functions */

#include <math.h>
#include <mex.h>
#include <stdio.h>
#include <string.h>
#include "../arsclib.h"
#include "../arscdev.h"

#define MAXLEN 		80
#define MAXSEG 		80
#define limit(min,val,max)    ((val<min)?min:((val>max)?max:val))
#define round(x)              ((int)((x)+0.5))

static char msg[MAXLEN];
static char xfer_inn_name[MAXLEN];
static char xfer_out_name[MAXLEN];
static double rate[MAXDEV];
static void *inn[MAXDEV][MAXSEG] = {{NULL}};
static void *out[MAXDEV][MAXSEG] = {{NULL}};
static long siz[MAXDEV][MAXSEG] = {{0}}; 
static long nc_i[MAXDEV];
static long nc_o[MAXDEV];
static long *sl;

static long
dev_id(int nrhs, const mxArray *prhs[])
{
    int dvid;
    const mxArray *a;
    void *v;

    if(nrhs < 2) {
        dvid = ar_find_dev(0);
    } else {
	a = prhs[1];
        v = (void *) mxGetPr(a);
        if (mxIsDouble(a)) {
	    dvid = (long) (((double *)v)[0]);
	} else if (mxIsInt16(a)) {
	    dvid = (long) (((short *)v)[0]);
	} else if (mxIsUint16(a)) {
	    dvid = (long) (((unsigned short *)v)[0]);
	} else if (mxIsInt32(a)) {
	    dvid = ((long *)v)[0];
	} else if (mxIsUint32(a)) {
	    dvid = ((long *)v)[0];
	} else {
	    mexErrMsgTxt("device-id data-type isn't supported");
	}
    }
    return (dvid);
} 

static void
xfer(long seg, char *xfer_name)
{
    double *a;
    int     err, dims[2], nlhs = 0, nrhs = 1;
    mxArray *plhs[1], *prhs[1];

    dims[0] = dims[1] = 1;
    prhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL); 
    a = (double *) mxGetPr(prhs[0]);
    a[0] = seg;
    err = mexCallMATLAB(nlhs, plhs, nrhs, prhs, xfer_name);
} 

static void
xfer_inn(long seg)
{
    xfer(seg, xfer_inn_name);
} 

static void
xfer_out(long seg)
{
    xfer(seg, xfer_out_name);
} 

static void
sync_avg(int id, double *x, double *y, int npnt, int nswp, int nskp, int nchn)
{
    double xx, mi, sc;
    int dbg = 0;
    int c, i, j, k, n, cur, cnt, prv, err, nseg, *ix[2], *iy[2], *jx, *jy;
    long *siz; 
    void **inn, **out; 
    static int nbuf = 2; 			/* double buffer */
    static long fmt[2] = {ARSC_DATA_I4, 1};	/* long,interleaved */

    n = npnt * nchn;
    memset(y, 0, n * sizeof(double));	/* zero response */
    if (nswp < 1 || nchn < 1) {
        return;
    }
    /* configure stimulus & response buffers */
    for (i = 0; i < nbuf; i++) {
        ix[i] = (int *) mxMalloc(n * sizeof(int));
        iy[i] = (int *) mxMalloc(n * sizeof(int));
    }
    mi = pow(2, 31) - 1; 
    jx = ix[0];
    for (j = 0; j < npnt; j++) {
        for (c = 0; c < nchn; c++) {
            xx = floor(mi * limit(-1, x[c * npnt + j], 1) + 0.5);
            jx[c + j * nchn] = (int) xx;
        }
    }
    /* configure i/o buffers */
    nskp = limit(0, nskp, npnt);
    k = (nskp > 0) ? 1 : 0;
    nseg = nswp + k + 1;
    inn = (void **) mxMalloc(nseg * sizeof(void *));
    out = (void **) mxMalloc(nseg * sizeof(void *));
    siz = (long *) mxMalloc(nseg * sizeof(long));
    if (k) {
        out[0] = ix[0] + npnt - nskp;
        inn[0] = NULL;
        siz[0] = nskp;
    }
    for (i = k; i < nseg; i++) {
        out[i] = ((i - k) < nswp) ? ix[0] : NULL;
        inn[i] = iy[i % nbuf];
        siz[i] = npnt;
    }
    /* initiate i/o */
    err = ar_set_fmt(id, fmt);
    if (err) {
        ar_err_msg(err, msg, MAXLEN);
        mexErrMsgTxt(msg);
        goto dealloc_return;
    }
    err = ar_io_prep(id, inn, out, siz, nseg, nseg);
    if (err) {
        ar_err_msg(err, msg, MAXLEN);
        mexErrMsgTxt(msg);
        goto dealloc_return;
    }
    err = ar_io_start(id);
    if (err) {
        ar_err_msg(err, msg, MAXLEN);
        mexErrMsgTxt(msg);
        goto dealloc_return;
    }
    jx = ix[1];
    cnt = cur = prv = 0;
    while (cur < nseg) {
        cur = ar_io_cur_seg(id);
        if (cur > prv && cnt < nswp && cur == (cnt + k + 1)) {
            jy = iy[(cur - 1) % nbuf];
            for (j = 0; j < n; j++) {
                jx[j] += jy[j] >> 8;
            }
            cnt++;
            prv = cur;
        }
    }
    /* fetch response */
    sc = 256 / (mi * cnt);	/* response scale factor */
    for (j = 0; j < npnt; j++) {
        for (c = 0; c < nchn; c++) {
          y[c * npnt + j] = jx[c + j * nchn] * sc;
        }
    }
    /* de-allocate buffers */
dealloc_return:
    for (i = 0; i < nbuf; i++) {
        mxFree(ix[i]);
        mxFree(iy[i]);
    }
    mxFree(inn);
    mxFree(out);
    mxFree(siz);
} 

static void
get_cardinfo(int dvid, double *c)
{
    CARDINFO ci;

    ar_get_cardinfo(dvid, &ci);
    c[0] = ci.bits;
    c[1] = ci.left;
    c[2] = ci.nbps;
    c[3] = ci.ncad;
    c[4] = ci.ncda;
    c[5] = ci.gdsr;
} 

void
mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    char   *s, name[MAXLEN];
    double *x, *y, *a, *a11b, *dsig;
    double ad_vfs[MAXNCH], da_vfs[MAXNCH], c[8];
    int     dvid, err, flg, i, j, m, n, n_i, n_o, dims[2];
    long  **cell, nswp, coi, coo, s_i, s_o, *lsig, fmt[2];
    void  (*inn_xfer)(long);
    void  (*out_xfer)(long);
    mxArray *p_i, *p_o, *a11a;
    static long dat_fmt[2] = {0, 0};

    dims[0] = dims[1] = 1;
    a11a = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL); 
    a11b = (double *) mxGetPr(a11a);
    if(nrhs < 1) {
        a11b[0] = ar_io_wait_seg(0);
	if(nlhs >= 1) {
	    plhs[0] = a11a;
	}
	return;
    }
    mxGetString(prhs[0], name, MAXLEN);
    if (*name == 0) {
	mexErrMsgTxt("No function name");
    } else if (strcmp(name, "close_all") == 0) {
	ar_close_all();
    } else if (strcmp(name, "dev_name") == 0) {
	dvid = dev_id(nrhs, prhs);
	err = ar_dev_name(dvid, name, MAXLEN);
	if (err) {
	    ar_err_msg(err, msg, MAXLEN);
	    mexErrMsgTxt(msg);
	}
	if(nlhs < 1) {
        mexPrintf("%s\n", name);
    } else {
	    plhs[0] = mxCreateString(name); 
    }
    } else if (strcmp(name, "find_dev") == 0) {
	if(nrhs < 2) {
	    flg = ARSC_PREF_NONE;
	} else {
	    mxGetString(prhs[1], name, MAXLEN);
	    if (strcmp(name, "sync") == 0) {
		flg = ARSC_PREF_SYNC;
	    } else if (strcmp(name, "asio") == 0) {
		flg = ARSC_PREF_ASIO;
	    } else if (strcmp(name, "alsa") == 0) {
		flg = ARSC_PREF_ALSA;
	    } else {
		flg = ARSC_PREF_NONE;
	    }
	} 
	if(nlhs >= 1) {
	    dims[0] = dims[1] = 1;
	    plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL); 
            a = (double *) mxGetPr(plhs[0]);
	    a[0] = ar_find_dev(flg); 
	}
    } else if (strcmp(name, "find_dev_name") == 0) {
	if(nrhs < 2) {
	    flg = ARSC_PREF_NONE;
	} else {
	    mxGetString(prhs[1], name, MAXLEN);
	} 
	if(nlhs >= 1) {
	    dims[0] = dims[1] = 1;
	    plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL); 
            a = (double *) mxGetPr(plhs[0]);
	    x[0] = ar_find_dev_name(name); 
	}
    } else if (strcmp(name, "get_cardinfo") == 0) {
	dvid = dev_id(nrhs, prhs);
	if(nlhs >= 1) {
	    dims[0] = 1;
	    dims[1] = 6;
	    plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL); 
            a = (double *) mxGetPr(plhs[0]);
            get_cardinfo(dvid, a);
	}
    } else if (strcmp(name, "get_fmt") == 0) {
	dvid = dev_id(nrhs, prhs);
	err = ar_get_fmt(dvid, fmt);
	if (err) {
	    ar_err_msg(err, msg, MAXLEN);
	    mexErrMsgTxt(msg);
	}
	if(nlhs >= 1) {
	    dims[0] = 1;
	    dims[1] = 2;
	    plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL); 
            a = (double *) mxGetPr(plhs[0]);
	    a[0] = fmt[0];
	    a[1] = fmt[1];
	}
    } else if (strcmp(name, "get_rate") == 0) {
	dvid = dev_id(nrhs, prhs);
	if(nlhs >= 1) {
	    plhs[0] = mxCreateDoubleMatrix(1, 1,  mxREAL); 
            x = mxGetPr(plhs[0]);
	    x[0] = rate[dvid];
	}
    } else if (strcmp(name, "get_vfs") == 0) {
	dvid = dev_id(nrhs, prhs);
	ar_get_vfs(dvid, da_vfs, ad_vfs);
	get_cardinfo(dvid, c);
        m = round(c[3]);
	if(nlhs >= 1) {
            n = round(c[4]);
	    dims[0] = 1;
	    dims[1] = n;
	    plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL); 
            a = (double *) mxGetPr(plhs[0]);
            for (i = 0; i < n; i++) {
	        a[i] = da_vfs[i];
            }
	}
	if(nlhs >= 2) {
            n = round(c[3]);
	    dims[0] = 1;
	    dims[1] = n;
	    plhs[1] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL); 
            a = (double *) mxGetPr(plhs[1]);
            for (i = 0; i < n; i++) {
	        a[i] = ad_vfs[i];
            }
	}
    } else if (strcmp(name, "io_close") == 0) {
	dvid = dev_id(nrhs, prhs);
	err = ar_io_close(dvid);
	if (err) {
	    ar_err_msg(err, msg, MAXLEN);
	    mexErrMsgTxt(msg);
	}
    } else if (strcmp(name, "io_cur_seg") == 0) {
	dvid = dev_id(nrhs, prhs);
        n = ar_io_cur_seg(dvid);
	if(nlhs >= 1) {
	    dims[0] = dims[1] = 1;
	    plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL); 
            a = (double *) mxGetPr(plhs[0]);
	    a[0] = n;
	}
    } else if (strcmp(name, "io_open") == 0) {
	dvid = dev_id(nrhs, prhs);
	if(nrhs > 2) {
	    x = mxGetPr(prhs[2]);
	    rate[dvid] = x[0];
	} else {
	    rate[dvid] = 44100;
	} 
	if(nrhs > 3) {
	    x = mxGetPr(prhs[3]);
	    m = (int) x[0];
	} else {
	    m = 1;
	} 
	if(nrhs > 4) {
	    x = mxGetPr(prhs[4]);
	    n = (int) x[0];
	} else {
	    n = m;
	} 
	if(nrhs > 5) {
	    x = mxGetPr(prhs[5]);
	    coi = (long) x[0];
	} else {
	    coi = 0;
	} 
	if(nrhs > 6) {
	    x = mxGetPr(prhs[6]);
	    coo = (long) x[0];
	} else {
	    coo = 0;
	} 
	nc_i[dvid] = m;
	nc_o[dvid] = n;
	err = ar_io_open_off(dvid, rate[dvid], nc_i[dvid], nc_o[dvid],
            coi, coo);
	if (err) {
	    ar_err_msg(err, msg, MAXLEN);
	    mexErrMsgTxt(msg);
	}
	rate[dvid] = ar_get_rate(dvid); 
    } else if (strcmp(name, "io_prepare") == 0) {
	if(nrhs < 5) {
	    nswp = 1;
	} else {
	    x = mxGetPr(prhs[4]);
	    nswp = (int) x[0];
	} 
	if(nrhs < 4)
	    mexErrMsgTxt("Too few input arguments");
	if (!mxIsCell(prhs[3]))
	    mexErrMsgTxt("Input must be cell array");
	if (!mxIsCell(prhs[2]))
	    mexErrMsgTxt("Output must be cell array");
	dvid = dev_id(nrhs, prhs);
	n = mxGetN(prhs[3]);
	if (n < 1)
    	    mexErrMsgTxt("No output segments");
	m = mxGetN(prhs[2]);
	if (m < 1)
    	    mexErrMsgTxt("No input segments");
	if (m != n)
    	    mexErrMsgTxt("Output segments must equal input");
	for (i = 0; i < n; i++) {
	    p_o = mxGetCell(prhs[3], i);
	    if (mxGetM(p_o) > 1) {
	        s_o = mxGetM(p_o);
	        n_o = mxGetN(p_o);
	    } else {
	        s_o = mxGetN(p_o);
		n_o = 1;
	    }
	    if (n_o != nc_o[dvid])
	        mexErrMsgTxt("Output has wrong number of channels");
	    p_i = mxGetCell(prhs[2], i);
	    if (mxGetM(p_i) > 1) {
	        s_i = mxGetM(p_i);
	        n_i = mxGetN(p_i);
	    } else {
	        s_i = mxGetN(p_i);
		n_i = 1;
	    }
	    if (n_i != nc_i[dvid])
	        mexErrMsgTxt("Output has wrong number of channels");
	    if (s_o != s_i)
	        mexErrMsgTxt("Output size must equal input");
	    if (s_o == 0) {
	        siz[dvid][i] = 0;
	        for (j = 0; j < n_o; j++) {
	            out[dvid][i * n_o + j] = NULL;
		}
	        for (j = 0; j < n_i; j++) {
	            inn[dvid][i * n_i + j] = NULL;
		}
	    } else if (mxIsInt32(p_o) && mxIsInt32(p_i)) {
	        siz[dvid][i] = s_o;
	        lsig = (long *) mxGetPr(p_o);
	        for (j = 0; j < n_o; j++) {
		    out[dvid][i * n_o + j] = lsig + j * s_o;
		}
	        lsig = (long *) mxGetPr(p_i);
	        for (j = 0; j < n_i; j++) {
		    inn[dvid][i * n_i + j] = lsig + j * s_i;
		}
		dat_fmt[0] = ARSC_DATA_I4;
	    } else if (mxIsDouble(p_o) && mxIsDouble(p_i)) {
	        siz[dvid][i] = s_o;
	        dsig = (double *) mxGetPr(p_o);
	        for (j = 0; j < n_o; j++) {
		    out[dvid][i * n_o + j] = dsig + j * s_o;
		}
	        dsig = (double *) mxGetPr(p_i);
	        for (j = 0; j < n_i; j++) {
		    inn[dvid][i * n_i + j] = dsig + j * s_i;
		}
		dat_fmt[0] = ARSC_DATA_F8;
	    } else {
	       mexErrMsgTxt("I/O arrays must be int32 or double");
	    }
	}
	dat_fmt[1] = 0;	/* non-interleaved data */
        err = ar_set_fmt(dvid, dat_fmt);
	if (err) {
	    ar_err_msg(err, msg, MAXLEN);
	    mexErrMsgTxt(msg);
	}
        err = ar_io_prepare(dvid, inn[dvid], out[dvid], siz[dvid], n, nswp);
	if (err) {
	    ar_err_msg(err, msg, MAXLEN);
	    mexErrMsgTxt(msg);
	}
    } else if (strcmp(name, "io_start") == 0) {
	dvid = dev_id(nrhs, prhs);
	err = ar_io_start(dvid);
	if (err) {
	    ar_err_msg(err, msg, MAXLEN);
	    mexErrMsgTxt(msg);
	}
    } else if (strcmp(name, "io_stop") == 0) {
	dvid = dev_id(nrhs, prhs);
	err = ar_io_stop(dvid);
	if (err) {
	    ar_err_msg(err, msg, MAXLEN);
	    mexErrMsgTxt(msg);
	}
    } else if (strcmp(name, "io_wait_seg") == 0) {
	dvid = dev_id(nrhs, prhs);
        n = ar_io_wait_seg(dvid);
	if(nlhs >= 1) {
	    dims[0] = dims[1] = 1;
	    plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL); 
            a = (double *) mxGetPr(plhs[0]);
	    a[0] = n;
	}
    } else if (strcmp(name, "num_devs") == 0) {
	if(nlhs >= 1) {
	    dims[0] = dims[1] = 1;
	    plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL); 
            a = (double *) mxGetPr(plhs[0]);
	    a[0] = ar_num_devs();
	}
    } else if (strcmp(name, "out_fill") == 0) {
	dvid = dev_id(nrhs, prhs);
	if(nrhs >= 4) {
	    x = mxGetPr(prhs[2]);
	    n = (int) x[0];
	    x = mxGetPr(prhs[3]);
	    m = mxGetM(prhs[3]) * mxGetN(prhs[3]);
	    if (m != siz[dvid][n]) {
	        mexPrintf("out_fill: siz[%d][%d]=%d\n", dvid, n, siz[dvid][n]);
            sprintf(msg, "doesn't match %d", m);
	        mexErrMsgTxt(msg);
	    }
	    if (dat_fmt[0] = ARSC_DATA_I4) {
            lsig = (long *) out[dvid][n];
            for (i = 0; i < m; i++) {
                lsig[i] = (long) x[i];
            }
	    } else if (dat_fmt[0] = ARSC_DATA_F8) {
            dsig = (double *) out[dvid][n];
            for (i = 0; i < m; i++) {
                dsig[i] = x[i];
            }
	    }
	} 
    } else if (strcmp(name, "out_open") == 0) {
	dvid = dev_id(nrhs, prhs);
	if(nrhs > 2) {
	    x = mxGetPr(prhs[2]);
	    rate[dvid] = x[0];
	} else {
	    rate[dvid] = 44100;
	} 
	if(nrhs > 3) {
	    x = mxGetPr(prhs[3]);
	    n = (int) x[0];
	} else {
	    n = 1;
	} 
	nc_o[dvid] = n;
	err = ar_out_open(dvid, rate[dvid], nc_o[dvid]);
	if (err) {
	    ar_err_msg(err, msg, MAXLEN);
	    mexErrMsgTxt(msg);
	}
	rate[dvid] = ar_get_rate(dvid); 
    } else if (strcmp(name, "out_prepare") == 0) {
	if(nrhs < 4) {
	    nswp = 1;
	} else {
	    x = mxGetPr(prhs[3]);
	    nswp = (int) x[0];
	} 
	if(nrhs < 3)
	    mexErrMsgTxt("Too few inputs");
	if (!mxIsCell(prhs[2]))
	    mexErrMsgTxt("Output must be cell array");
	dvid = dev_id(nrhs, prhs);
	n = mxGetN(prhs[2]);
	if (n < 1)
    	    mexErrMsgTxt("No output segments");
	for (i = 0; i < n; i++) {
	    p_o = mxGetCell(prhs[2], i);
	    if (mxGetM(p_o) > 1) {
	        s_o = mxGetM(p_o);
	        n_o = mxGetN(p_o);
	    } else {
	        s_o = mxGetN(p_o);
		n_o = 1;
	    }
	    if (n_o != nc_o[dvid])
	        mexErrMsgTxt("Output has wrong number of channels");
	    if (s_o == 0) {
	        siz[dvid][i] = 0;
	        for (j = 0; j < n_o; j++) {
	            out[dvid][i * n_o + j] = NULL;
		}
	    } else if (mxIsInt32(p_o)) {
	        siz[dvid][i] = s_o;
	        lsig = (long *) mxGetPr(p_o);
	        for (j = 0; j < n_o; j++) {
		    out[dvid][i * n_o + j] = lsig + j * s_o;
		}
		fmt[0] = ARSC_DATA_I4;
	    } else if (mxIsDouble(p_o)) {
	        siz[dvid][i] = s_o;
	        dsig = (double *) mxGetPr(p_o);
	        for (j = 0; j < n_o; j++) {
		    out[dvid][i * n_o + j] = dsig + j * s_o;
		}
		fmt[0] = ARSC_DATA_F8;
	    } else {
	       mexErrMsgTxt("Output arrays must be int32 or double");
	    }
	}
	fmt[1] = 0;	/* non-interleaved data */
	err = ar_set_fmt(dvid, fmt);
	if (err) {
	    ar_err_msg(err, msg, MAXLEN);
	    mexErrMsgTxt(msg);
	}
	err = ar_out_prepare(dvid, out[dvid], siz[dvid], n, nswp);
        if (err) {
	    ar_err_msg(err, msg, MAXLEN);
	    mexErrMsgTxt(msg);
	}
    } else if (strcmp(name, "out_seg_fill") == 0) {
	dvid = dev_id(nrhs, prhs);
	err = ar_out_seg_fill(dvid);
        if (err) {
	    ar_err_msg(err, msg, MAXLEN);
	    mexErrMsgTxt(msg);
	}
    } else if (strcmp(name, "set_xfer") == 0) {
	dvid = dev_id(nrhs, prhs);
	if(nrhs > 2) {
	    mxGetString(prhs[2], xfer_inn_name, MAXLEN);
	    inn_xfer = xfer_inn;
	} else {
       	    strcpy(xfer_inn_name, "");
	    inn_xfer = NULL;
	}
	if(nrhs > 3) {
	    mxGetString(prhs[3], xfer_out_name, MAXLEN);
	    out_xfer = xfer_out;
	} else {
       	    strcpy(xfer_out_name, "");
	    out_xfer = NULL;
	}
	err = ar_set_xfer(dvid, inn_xfer, out_xfer);
        if (err) {
	    ar_err_msg(err, msg, MAXLEN);
	    mexErrMsgTxt(msg);
	}
    } else if (strcmp(name, "set_vfs") == 0) {
	dvid = dev_id(nrhs, prhs);
	if(nrhs > 1) {
            x = mxGetPr(prhs[1]);
	    da_vfs[0] = x[0];
	    da_vfs[1] = x[1];
	}
	if(nrhs > 2) {
            x = mxGetPr(prhs[1]);
	    ad_vfs[0] = x[0];
	    ad_vfs[1] = x[1];
	}
	ar_set_vfs(dvid, da_vfs, ad_vfs);
    } else if (strcmp(name, "sync_avg") == 0) {
	dvid = dev_id(nrhs, prhs);
	if(nlhs < 1) {
	    mexErrMsgTxt("Output argument required");
	} else if(nrhs < 3) {
	    mexErrMsgTxt("Three input arguments required");
	} else if(nc_o[dvid] != nc_i[dvid]) {
	    mexErrMsgTxt("Ouput must equal input");
	} else {
	    if (nrhs > 4) {
	        x = mxGetPr(prhs[4]);
	        j = (int) x[0];		/* nskp */
	    } else {
	        j = 0;
	    }
	    if (nrhs > 3) {
	        x = mxGetPr(prhs[3]);
	        n = (int) x[0];		/* nswp */
	    } else {
	        n = 1;
	    }
	    dims[0] = mxGetM(prhs[2]);	/* rows */
	    dims[1] = mxGetN(prhs[2]);	/* cols */
	    plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL); 
            x = mxGetPr(prhs[2]);
            y = mxGetPr(plhs[0]);
	    sync_avg(dvid, x, y, dims[0], n, j, dims[1]);
	}
    } else if (strcmp(name, "version") == 0) {
        if(nlhs < 1) {
            mexPrintf("%s\n", ar_version());
        } else {
            plhs[0] = mxCreateString(ar_version()); 
        }
    } else if (strcmp(name, "xruns") == 0) {
        dvid = dev_id(nrhs, prhs);
        if(nlhs >= 1) {
            dims[0] = dims[1] = 1;
            plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL); 
            a = (double *) mxGetPr(plhs[0]);
            a[0] = ar_xruns(dvid);
        }
    } else {
        mexErrMsgTxt("Unknown function name");
    }
}

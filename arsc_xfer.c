/* arsc_xfer.c - ARSC data transfer functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "arsclib.h"
#include "arscdev.h"

#include "arsc_common.h"

#undef MAXINT16
#undef MAXINT32
#define MAXINT16 32767.         // 2^15-1
#define MAXINT32 2147483647.    // 2^31-1
#define NDT      4              // number of data types
#define round(x,y)  floor((x)*(y)+0.5)

static double mxfli = 1;        // maximum float in
static double mxflo = 1;        // maximum float out

/*****************************************************************************/
// input: app & dev have equal number of input channels, dev interleaved
/*****************************************************************************/

static void				// dev short, app short
xfer_issiie(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    c = a->ncad;
    spd = a->i_data[b];
    spa = a->ibptr[b];
    if (spa) {
        for (i = 0; i < n * c; i++) {
            spa[i] = spd[i];
	}
    }
}

static void				// dev short, app long
xfer_isliie(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    c = a->ncad;
    spd = a->i_data[b];
    lpa = a->ibptr[b];
    if (lpa) {
        for (i = 0; i < n * c; i++) {
            lpa[i] = (SINT4) spd[i] << 16;
        }
    }
}

static void				// dev short, app float
xfer_isfiie(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (mxfli / MAXINT16);
    n = a->sizptr[b];
    c = a->ncad;
    spd = a->i_data[b];
    fpa = a->ibptr[b];
    if (fpa) {
        for (i = 0; i < n * c; i++) {
            fpa[i] = spd[i] * sf;
	}
    }
}

static void				// dev short, app double
xfer_isdiie(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    double *dpa, sf;
    SINT2 *spd;

    sf = mxfli / MAXINT16;
    n = a->sizptr[b];
    c = a->ncad;
    spd = a->i_data[b];
    dpa = a->ibptr[b];
    if (dpa) {
        for (i = 0; i < n * c; i++) {
            dpa[i] = spd[i] * sf;
        }
    }
}

static void				// dev long, app short
xfer_ilsiie(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    c = a->ncad;
    lpd = a->i_data[b];
    spa = a->ibptr[b];
    if (spa) {
        for (i = 0; i < n * c; i++) {
	    spa[i] = (short) (lpd[i] >> 16);
	}
    }
}

static void				// dev long, app long
xfer_illiie(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    c = a->ncad;
    lpd = a->i_data[b];
    lpa = a->ibptr[b];
    if (lpa) {
        for (i = 0; i < n * c; i++) {
            lpa[i] = lpd[i];
	}
    }
}

static void				// dev long, app float
xfer_ilfiie(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (mxfli / MAXINT32);
    n = a->sizptr[b];
    c = a->ncad;
    lpd = a->i_data[b];
    fpa = a->ibptr[b];
    if (fpa) {
        for (i = 0; i < n * c; i++) {
            fpa[i] = lpd[i] * sf;
	}
    }
}

static void				// dev long, app double
xfer_ildiie(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    double *dpa, sf;
    SINT4 *lpd;

    sf = mxfli / MAXINT32;
    n = a->sizptr[b];
    c = a->ncad;
    lpd = a->i_data[b];
    dpa = a->ibptr[b];
    if (dpa) {
        for (i = 0; i < n * c; i++) {
            dpa[i] = lpd[i] * sf;
	}
    }
}

static void		    		// dev short, app short
xfer_issine(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, c;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    c = a->ncad;
    spd = a->i_data[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
            spa = a->ibptr[k];
            if (spa) {
	        spa[i] = spd[m];
	    }
	}
    }
}

static void				// dev short, app long
xfer_isline(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, c;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    c = a->ncad;
    spd = a->i_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    lpa = a->ibptr[k];
            if (lpa) {
	        lpa[i] = (SINT4) spd[m] << 16;
	    }
	}
    }
}

static void		    		// dev short, app float
xfer_isfine(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, c;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (mxfli / MAXINT16);
    n = a->sizptr[b];
    c = a->ncad;
    spd = a->i_data[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    fpa = a->ibptr[k];
            if (fpa) {
	        fpa[i] = spd[m] * sf;
	    }
	}
    }
}

static void				// dev short, app double
xfer_isdine(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, c;
    double *dpa, sf;
    SINT2 *spd;

    sf = mxfli / MAXINT16;
    n = a->sizptr[b];
    c = a->ncad;
    spd = a->i_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    dpa = a->ibptr[k];
            if (dpa) {
	        dpa[i] = spd[m] * sf;
	    }
	}
    }
}

static void				// dev long, app short
xfer_ilsine(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, c;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    c = a->ncad;
    lpd = a->i_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    spa = a->ibptr[k];
            if (spa) {
	        spa[i] = (short) (lpd[m] >> 16);
	    }
	}
    }
}

static void				// dev long, app long
xfer_illine(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, c;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    c = a->ncad;
    lpd = a->i_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    lpa = a->ibptr[k];
            if (lpa) {
	        lpa[i] = lpd[m];
	    }
	}
    }
}

static void				// dev long, app float
xfer_ilfine(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, c;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (mxfli / MAXINT32);
    n = a->sizptr[b];
    c = a->ncad;
    lpd = a->i_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    fpa = a->ibptr[k];
            if (fpa) {
	        fpa[i] = lpd[m] * sf;
	    }
	}
    }
}

static void				// dev long, app double
xfer_ildine(ARDEV *a, int b)            // input, equal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, c;
    double *dpa, sf;
    SINT4 *lpd;

    sf = mxfli / MAXINT32;
    n = a->sizptr[b];
    c = a->ncad;
    lpd = a->i_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    dpa = a->ibptr[k];
            if (dpa) {
	        dpa[i] = lpd[m] * sf;
	    }
	}
    }
}

/*****************************************************************************/
// input: app & dev have unequal number of input channels, dev interleaved
/*****************************************************************************/

static void				// dev short, app short
xfer_issiiu(ARDEV *a, int b)            // input, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    spd = a->i_data[b];
    spa = a->ibptr[b];
    if (spa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    spa[k] = spd[m];
		} else {
		    spa[k] = 0;
		}
	    }
	}
    }
}

static void				// dev short, app long
xfer_isliiu(ARDEV *a, int b)            // input, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    spd = a->i_data[b];
    lpa = a->ibptr[b];
    if (lpa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    lpa[k] = (SINT4) spd[m] << 16;
		} else {
		    lpa[k] = 0;
		}
	    }
        }
    }
}

static void				// dev short, app float
xfer_isfiiu(ARDEV *a, int b)            // input, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (mxfli / MAXINT16);
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    spd = a->i_data[b];
    fpa = a->ibptr[b];
    if (fpa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    fpa[k] = spd[m] * sf;
		} else {
		    fpa[k] = 0;
		}
	    }
	}
    }
}

static void				// dev short, app double
xfer_isdiiu(ARDEV *a, int b)            // input, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT2 *spd;

    sf = mxfli / MAXINT16;
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    spd = a->i_data[b];
    dpa = a->ibptr[b];
    if (dpa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    dpa[k] = (SINT4) spd[m] * sf;
		} else {
		    dpa[k] = 0;
		}
	    }
        }
    }
}

static void				// dev long, app short
xfer_ilsiiu(ARDEV *a, int b)            // input, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    lpd = a->i_data[b];
    spa = a->ibptr[b];
    if (spa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    spa[k] = (short) (lpd[m] >> 16);
		} else {
		    spa[k] = 0;
		}
	    }
	}
    }
}

static void				// dev long, app long
xfer_illiiu(ARDEV *a, int b)            // input, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    lpd = a->i_data[b];
    lpa = a->ibptr[b];
    if (lpa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    lpa[k] = lpd[m];
		} else {
		    lpa[k] = 0;
		}
	    }
	}
    }
}

static void				// dev long, app float
xfer_ilfiiu(ARDEV *a, int b)            // input, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (mxfli / MAXINT32);
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    lpd = a->i_data[b];
    fpa = a->ibptr[b];
    if (fpa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    fpa[k] = lpd[m] * sf;
		} else {
		    fpa[k] = 0;
		}
	    }
	}
    }
}

static void				// dev long, app double
xfer_ildiiu(ARDEV *a, int b)            // input, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT4 *lpd;

    sf = mxfli / MAXINT32;
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    lpd = a->i_data[b];
    dpa = a->ibptr[b];
    if (dpa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    dpa[k] = lpd[m] * sf;
		} else {
		    dpa[k] = 0;
		}
	    }
	}
    }
}

static void		    		// dev short, app short
xfer_issinu(ARDEV *a, int b)            // input, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT2 *spd, *spa;


    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    spd = a->i_data[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    spa = a->ibptr[k];
            if (spa) {
		if (j < cd) {
		    spa[i] = spd[m];
		} else {
		    spa[i] = 0;
		}
	    }
	}
    }
}

static void				// dev short, app long
xfer_islinu(ARDEV *a, int b)		// input, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    spd = a->i_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
            lpa = a->ibptr[k];
            if (lpa) {
		if (j < cd) {
		    lpa[i] = (SINT4) spd[m] << 16;
		} else {
		    lpa[i] = 0;
		}
	    }
	}
    }
}

static void		    		// dev short, app float
xfer_isfinu(ARDEV *a, int b)            // input, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (mxfli / MAXINT16);
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    spd = a->i_data[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    fpa = a->ibptr[k];
            if (fpa) {
		if (j < cd) {
		    fpa[i] = spd[m] * sf;
		} else {
		    fpa[i] = 0;
		}
	    }
	}
    }
}

static void				// dev short, app double
xfer_isdinu(ARDEV *a, int b)		// input, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT2 *spd;

    sf = mxfli / MAXINT16;
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    spd = a->i_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    dpa = a->ibptr[k];
            if (dpa) {
		if (j < cd) {
		    dpa[i] = spd[m] * sf;
		} else {
		    dpa[i] = 0;
		}
	    }
	}
    }
}

static void				// dev long, app short
xfer_ilsinu(ARDEV *a, int b)            // input, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    lpd = a->i_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    spa = a->ibptr[k];
            if (spa) {
		if (j < cd) {
		    spa[i] = (short) (lpd[m] >> 16);
		} else {
		    spa[i] = 0;
		}
	    }
	}
    }
}

static void				// dev long, app long
xfer_illinu(ARDEV *a, int b)            // input, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    lpd = a->i_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    lpa = a->ibptr[k];
            if (lpa) {
		if (j < cd) {
		    lpa[i] = lpd[m];
		} else {
		    lpa[i] = 0;
		}
	    }
	}
    }
}

static void				// dev long, app float
xfer_ilfinu(ARDEV *a, int b)            // input, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (mxfli / MAXINT32);
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    lpd = a->i_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    fpa = a->ibptr[k];
            if (fpa) {
		if (j < cd) {
		    fpa[i] = lpd[m] * sf;
		} else {
		    fpa[i] = 0;
		}
	    }
	}
    }
}

static void				// dev long, app double
xfer_ildinu(ARDEV *a, int b)            // input, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT4 *lpd;

    sf = mxfli / MAXINT32;
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    lpd = a->i_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
            dpa = a->ibptr[k];
            if (dpa) {
		if (j < cd) {
		    dpa[i] = lpd[m] * sf;
		} else {
		    dpa[i] = 0;
		}
	    }
	}
    }
}

/*****************************************************************************/
// input: app & dev have equal number of input channels, dev non-interleaved
/*****************************************************************************/

static void				// dev short, app short
xfer_issnie(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    c = a->ncad;
    spa = a->ibptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    spd = a->i_data[k];
            if (spd) {
	        spa[m] = spd[i];
	    }
	}
    }
}

static void				// dev short, app long
xfer_islnie(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    c = a->ncad;
    lpa = a->ibptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    spd = a->i_data[k];
            if (spd) {
		lpa[m] = (SINT4) spd[i] << 16;
	    }
	}
    }
}

static void				// dev short, app float
xfer_isfnie(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (mxfli / MAXINT16);
    n = a->sizptr[b];
    c = a->ncad;
    fpa = a->ibptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
            spd = a->i_data[k];
            if (spd) {
	        fpa[m] = spd[i] * sf;
	    }
	}
    }
}

static void				// dev short, app double
xfer_isdnie(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    double *dpa, sf;
    SINT2 *spd;

    sf = mxfli / MAXINT16;
    n = a->sizptr[b];
    c = a->ncad;
    dpa = a->ibptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    spd = a->i_data[k];
            if (spd) {
		dpa[m] = spd[i] * sf;
	    }
	}
    }
}

static void				// dev long, app short
xfer_ilsnie(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    c = a->ncad;
    spa = a->ibptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    lpd = a->i_data[k];
            if (lpd) {
		spa[m] = (short) (lpd[i] >> 16);
	    }
	}
    }
}

static void				// dev long, app long
xfer_illnie(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    c = a->ncad;
    lpa = a->ibptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    lpd = a->i_data[k];
            if (lpd) {
	        lpa[m] = lpd[i];
	    }
	}
    }
}

static void				// dev long, app float
xfer_ilfnie(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (mxfli / MAXINT32);
    n = a->sizptr[b];
    c = a->ncad;
    fpa = a->ibptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    lpd = a->i_data[k];
            if (lpd) {
		fpa[m] = lpd[i] * sf;
	    }
	}
    }
}

static void				// dev long, app double
xfer_ildnie(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    double *dpa, sf;
    SINT4 *lpd;

    sf = mxfli / MAXINT32;
    n = a->sizptr[b];
    c = a->ncad;
    dpa = a->ibptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    lpd = a->i_data[k];
            if (lpd) {
	        dpa[m] = lpd[i] * sf;
	    }
	}
    }
}

static void		    		// dev short, app short
xfer_issnne(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, n, c;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        spd = a->i_data[k];
        spa = a->ibptr[k];
        if (spa) {
            for (i = 0; i < n; i++) {
                spa[i] = spd[i];
            }
        }
    }
}

static void				// dev short, app long
xfer_islnne(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, n, c;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        spd = a->i_data[k];
        lpa = a->ibptr[k];
        if (lpa) {
            for (i = 0; i < n; i++) {
                lpa[i] = (SINT4) spd[i] << 16;
            }
        }
    }
}

static void		    		// dev short, app float
xfer_isfnne(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, n, c;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (mxfli / MAXINT16);
    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        spd = a->i_data[k];
        fpa = a->ibptr[k];
        if (fpa) {
            for (i = 0; i < n; i++) {
                fpa[i] = spd[i] * sf;
            }
        }
    }
}

static void				// dev short, app double
xfer_isdnne(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, n, c;
    double *dpa, sf;
    SINT2 *spd;

    sf = mxfli / MAXINT16;
    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        spd = a->i_data[k];
        dpa = a->ibptr[k];
        if (dpa) {
            for (i = 0; i < n; i++) {
                dpa[i] = spd[i] * sf;
            }
        }
    }
}

static void				// dev long, app short
xfer_ilsnne(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, n, c;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        lpd = a->i_data[k];
        spa = a->ibptr[k];
        if (spa) {
            for (i = 0; i < n; i++) {
                spa[i] = (short) (lpd[i] >> 16);
            }
        }
    }
}

static void				// dev long, app long
xfer_illnne(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, n, c;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        lpd = a->i_data[k];
        lpa = a->ibptr[k];
        if (lpa) {
            for (i = 0; i < n; i++) {
                lpa[i] = lpd[i];
            }
        }
    }
}

static void				// dev long, app float
xfer_ilfnne(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, n, c;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (mxfli / MAXINT32);
    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        lpd = a->i_data[k];
        fpa = a->ibptr[k];
        if (fpa) {
            for (i = 0; i < n; i++) {
                fpa[i] = lpd[i] * sf;
            }
        }
    }
}

static void				// dev long, app double
xfer_ildnne(ARDEV *a, int b)            // input, equal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, n, c;
    double *dpa, sf;
    SINT4 *lpd;

    sf = mxfli / MAXINT32;
    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        lpd = a->i_data[k];
        dpa = a->ibptr[k];
        if (dpa) {
            for (i = 0; i < n; i++) {
                dpa[i] = lpd[i] * sf;
            }
        }
    }
}

/*****************************************************************************/
// input: app & dev have unequal number of input channels, dev non-interleaved
/*****************************************************************************/

static void				// dev short, app short
xfer_issniu(ARDEV *a, int b)            // input, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    spa = a->ibptr[b];
    if (spa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    spd = a->i_data[k];
		    spa[k] = spd[m];
		} else {
		    spa[k] = 0;
		}
	    }
	}
    }
}

static void				// dev short, app long
xfer_islniu(ARDEV *a, int b)            // input, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    lpa = a->ibptr[b];
    if (lpa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    spd = a->i_data[k];
		    lpa[k] = (SINT4) spd[m] << 16;
		} else {
		    lpa[k] = 0;
		}
	    }
        }
    }
}

static void				// dev short, app float
xfer_isfniu(ARDEV *a, int b)            // input, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (mxfli / MAXINT16);
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    fpa = a->ibptr[b];
    if (fpa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    spd = a->i_data[k];
		    fpa[k] = spd[m] * sf;
		} else {
		    fpa[k] = 0;
		}
	    }
	}
    }
}

static void				// dev short, app double
xfer_isdniu(ARDEV *a, int b)            // input, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT2 *spd;

    sf = mxfli / MAXINT16;
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    dpa = a->ibptr[b];
    if (dpa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    spd = a->i_data[k];
		    dpa[k] = spd[m] * sf;
		} else {
		    dpa[k] = 0;
		}
	    }
        }
    }
}

static void				// dev long, app short
xfer_ilsniu(ARDEV *a, int b)            // input, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    spa = a->ibptr[b];
    if (spa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    lpd = a->i_data[k];
		    spa[k] = (short) (lpd[m] >> 16);
		} else {
		    spa[k] = 0;
		}
	    }
	}
    }
}

static void				// dev long, app long
xfer_illniu(ARDEV *a, int b)            // input, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    lpa = a->ibptr[b];
    if (lpa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    lpd = a->i_data[k];
		    lpa[k] = lpd[m];
		} else {
		    lpa[k] = 0;
		}
	    }
	}
    }
}

static void				// dev long, app float
xfer_ilfniu(ARDEV *a, int b)            // input, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (mxfli / MAXINT32);
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    fpa = a->ibptr[b];
    if (fpa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    lpd = a->i_data[k];
		    fpa[k] = lpd[m] * sf;
		} else {
		    fpa[k] = 0;
		}
	    }
	}
    }
}

static void				// dev long, app double
xfer_ildniu(ARDEV *a, int b)            // input, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT4 *lpd;

    sf = mxfli / MAXINT32;
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    dpa = a->ibptr[b];
    if (dpa) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < ca; j++) {
	        k = j + i * ca;
	        m = j + i * cd;
		if (j < cd) {
		    lpd = a->i_data[k];
		    dpa[k] = lpd[m] * sf;
		} else {
		    dpa[k] = 0;
		}
	    }
	}
    }
}

static void		    		// dev short, app short
xfer_issnnu(ARDEV *a, int b)            // input, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    for (i = 0; i < n; i++) {
	for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    spa = a->ibptr[k];
            if (spa) {
		if (j < cd) {
		    spd = a->i_data[k];
		    spa[i] = spd[m];
		} else {
		    spa[i] = 0;
		}
	    }
	}
    }
}

static void				// dev short, app long
xfer_islnnu(ARDEV *a, int b)		// input, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    for (i = 0; i < n; i++) {
        for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    lpa = a->ibptr[k];
            if (lpa) {
		if (j < cd) {
		    spd = a->i_data[k];
		    lpa[i] = (SINT4) spd[m] << 16;
		} else {
		    lpa[i] = 0;
		}
	    }
	}
    }
}

static void		    		// dev short, app float
xfer_isfnnu(ARDEV *a, int b)            // input, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (mxfli / MAXINT16);
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    for (i = 0; i < n; i++) {
	for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    fpa = a->ibptr[k];
            if (fpa) {
		if (j < cd) {
		    spd = a->i_data[k];
		    fpa[i] = spd[m] * sf;
		} else {
		    fpa[i] = 0;
		}
	    }
	}
    }
}

static void				// dev short, app double
xfer_isdnnu(ARDEV *a, int b)		// input, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT2 *spd;

    sf = mxfli / MAXINT16;
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    for (i = 0; i < n; i++) {
        for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    dpa = a->ibptr[k];
            if (dpa) {
		if (j < cd) {
		    spd = a->i_data[k];
		    dpa[i] = spd[m] * sf;
		} else {
		    dpa[i] = 0;
		}
	    }
	}
    }
}

static void				// dev long, app short
xfer_ilsnnu(ARDEV *a, int b)            // input, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    for (i = 0; i < n; i++) {
        for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    spa = a->ibptr[k];
            if (spa) {
		if (j < cd) {
		    lpd = a->i_data[k];
		    spa[i] = (short) (lpd[m] >> 16);
		} else {
		    spa[i] = 0;
		}
	    }
	}
    }
}

static void				// dev long, app long
xfer_illnnu(ARDEV *a, int b)            // input, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    for (i = 0; i < n; i++) {
        for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    lpa = a->ibptr[k];
            if (lpa) {
		if (j < cd) {
		    lpd = a->i_data[k];
		    lpa[i] = lpd[m];
		} else {
		    lpa[i] = 0;
		}
	    }
	}
    }
}

static void				// dev long, app float
xfer_ilfnnu(ARDEV *a, int b)            // input, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (mxfli / MAXINT32);
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    for (i = 0; i < n; i++) {
        for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    fpa = a->ibptr[k];
            if (fpa) {
		if (j < cd) {
		    lpd = a->i_data[k];
		    fpa[i] = lpd[m] * sf;
		} else {
		    fpa[i] = 0;
		}
	    }
	}
    }
}

static void				// dev long, app double
xfer_ildnnu(ARDEV *a, int b)            // input, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT4 *lpd;

    sf = mxfli / MAXINT32;
    n = a->sizptr[b];
    cd = a->ncad;
    ca = a->a_ncad;
    for (i = 0; i < n; i++) {
        for (j = 0; j < ca; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    dpa = a->ibptr[k];
            if (dpa) {
		if (j < cd) {
		    lpd = a->i_data[k];
		    dpa[i] = lpd[m] * sf;
		} else {
		    dpa[i] = 0;
		}
	    }
	}
    }
}

/*****************************************************************************/
// output: app & dev have equal number of output channels, dev interleaved
/*****************************************************************************/

static void				// dev short, app short
xfer_ossiie(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    c = a->ncda;
    spd = a->o_data[b];
    spa = a->obptr[b];
    for (i = 0; i < n * c; i++) {
        if (!d && spa) {
            spd[i] = spa[i];
        } else {
            spd[i] = 0;
	}
    }
}

static void				// dev short, app long
xfer_osliie(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    c = a->ncda;
    spd = a->o_data[b];
    lpa = a->obptr[b];
    for (i = 0; i < n * c; i++) {
        if (!d && lpa) {
            spd[i] = (short) (lpa[i] >> 16);
        } else {
            spd[i] = 0;
        }
    }
}

static void				// dev short, app float
xfer_osfiie(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (MAXINT16 / mxflo);
    n = a->sizptr[b];
    c = a->ncda;
    spd = a->o_data[b];
    fpa = a->obptr[b];
    for (i = 0; i < n * c; i++) {
        if (!d && fpa) {
            spd[i] = (short) round(fpa[i], sf);
        } else {
            spd[i] = 0;
	}
    }
}

static void				// dev short, app double
xfer_osdiie(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    double *dpa, sf;
    SINT2 *spd;

    sf = MAXINT16 / mxflo;
    n = a->sizptr[b];
    c = a->ncda;
    spd = a->o_data[b];
    dpa = a->obptr[b];
    for (i = 0; i < n * c; i++) {
        if (!d && dpa) {
            spd[i] = (short) round(dpa[i], sf);
        } else {
            spd[i] = 0;
        }
    }
}

static void				// dev long, app short
xfer_olsiie(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    c = a->ncda;
    lpd = a->o_data[b];
    spa = a->obptr[b];
    for (i = 0; i < n * c; i++) {
        if (!d && spa) {
	    lpd[i] = (SINT4) spa[i] << 16;
	} else {
	    lpd[i] = 0;
	}
    }
}

static void				// dev long, app long
xfer_olliie(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    c = a->ncda;
    lpd = a->o_data[b];
    lpa = a->obptr[b];
    for (i = 0; i < n * c; i++) {
        if (!d && lpa) {
            lpd[i] = lpa[i];
        } else {
            lpd[i] = 0;
	}
    }
}

static void				// dev long, app float
xfer_olfiie(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (MAXINT32 / mxflo);
    n = a->sizptr[b];
    c = a->ncda;
    lpd = a->o_data[b];
    fpa = a->obptr[b];
    for (i = 0; i < n * c; i++) {
        if (!d && fpa) {
	    lpd[i] = (SINT4) round(fpa[i], sf);
	} else {
	    lpd[i] = 0;
	}
    }
}

static void				// dev long, app double
xfer_oldiie(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved, app interleaved
    int i, n, c;
    double *dpa, sf;
    SINT4 *lpd;

    sf = MAXINT32 / mxflo;
    n = a->sizptr[b];
    c = a->ncda;
    lpd = a->o_data[b];
    dpa = a->obptr[b];
    for (i = 0; i < n * c; i++) {
        if (!d && dpa) {
	    lpd[i] = (SINT4) round(dpa[i], sf);
        } else {
            lpd[i] = 0;
	}
    }
}

static void				// dev short, app short
xfer_ossine(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, c;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    c = a->ncda;
    spd = a->o_data[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
            spa = a->obptr[k];
            if (!d && spa) {
	        spd[m] = spa[i];
	    } else {
	        spd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app long
xfer_osline(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved to app non-interleaved
    int i, j, k, m, n, c;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    c = a->ncda;
    spd = a->o_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    lpa = a->obptr[k];
            if (!d && lpa) {
	        spd[m] = (short) (lpa[i] >> 16);
	    } else {
	        spd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app float
xfer_osfine(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, c;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (MAXINT16 / mxflo);
    n = a->sizptr[b];
    c = a->ncda;
    spd = a->o_data[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    fpa = a->obptr[k];
            if (!d && fpa) {
	        spd[m] = (short) round(fpa[i], sf);
	    } else {
	        spd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app double
xfer_osdine(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved to app non-interleaved
    int i, j, k, m, n, c;
    double *dpa, sf;
    SINT2 *spd;

    sf = MAXINT16 / mxflo;
    n = a->sizptr[b];
    c = a->ncda;
    spd = a->o_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    dpa = a->obptr[k];
            if (!d && dpa) {
	        spd[m] = (short) round(dpa[i], sf);
	    } else {
	        spd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app short
xfer_olsine(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, c;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    c = a->ncda;
    lpd = a->o_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    spa = a->obptr[k];
            if (!d && spa) {
	        lpd[m] = (SINT4) spa[i] << 16;
	    } else {
	        lpd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app long
xfer_olline(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, c;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    c = a->ncda;
    lpd = a->o_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    lpa = a->obptr[k];
            if (!d && lpa) {
	        lpd[m] = lpa[i];
	    } else {
	        lpd[m] = 0;
	    }
	}
    }

}

static void				// dev long, app float
xfer_olfine(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, c;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (MAXINT32 / mxflo);
    n = a->sizptr[b];
    c = a->ncda;
    lpd = a->o_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    fpa = a->obptr[k];
            if (!d && fpa) {
	        lpd[m] = (SINT4) round(fpa[i], sf);
	    } else {
	        lpd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app double
xfer_oldine(ARDEV *a, int b, int d)     // output, equal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, c;
    double *dpa, sf;
    SINT4 *lpd;

    sf = MAXINT32 / mxflo;
    n = a->sizptr[b];
    c = a->ncda;
    lpd = a->o_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    dpa = a->obptr[k];
            if (!d && dpa) {
	        lpd[m] = (SINT4) round(dpa[i], sf);
	    } else {
	        lpd[m] = 0;
	    }
	}
    }

}

/*****************************************************************************/
// output: app & dev have unequal number of output channels, dev interleaved
/*****************************************************************************/

static void				// dev short, app short
xfer_ossiiu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    spd = a->o_data[b];
    spa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
            if (j < ca && !d && spa) {
	        spd[m] = spa[k];
	    } else {
		spd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app long
xfer_osliiu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    spd = a->o_data[b];
    lpa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
            if (j < ca && !d && lpa) {
	        spd[m] = (short) (lpa[k] >> 16);
	    } else {
		spd[m] = 0;
	    }
        }
    }
}

static void				// dev short, app float
xfer_osfiiu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (MAXINT16 / mxflo);
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    spd = a->o_data[b];
    fpa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
            if (j < ca && !d && fpa) {
	        spd[m] = (short) round(fpa[k], sf);
	    } else {
		spd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app double
xfer_osdiiu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT2 *spd;

    sf = MAXINT16 / mxflo;
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    spd = a->o_data[b];
    dpa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
            if (j < ca && !d && dpa) {
	        spd[m] = (short) round(dpa[k], sf);
	    } else {
		spd[m] = 0;
	    }
        }
    }
}

static void				// dev long, app short
xfer_olsiiu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    lpd = a->o_data[b];
    spa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
	    if (j < ca && !d && spa) {
		lpd[m] = (SINT4) spa[k] << 16;
	    } else {
		lpd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app long
xfer_olliiu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    lpd = a->o_data[b];
    lpa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
	    if (j < ca && !d && lpa) {
		lpd[m] = lpa[k];
	    } else {
		lpd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app float
xfer_olfiiu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (MAXINT32 / mxflo);
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    lpd = a->o_data[b];
    fpa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
	    if (j < ca && !d && fpa) {
		lpd[m] = (SINT4) round(fpa[k], sf);
	    } else {
		lpd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app double
xfer_oldiiu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT4 *lpd;

    sf = MAXINT32 / mxflo;
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    lpd = a->o_data[b];
    dpa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
	    if (j < ca && !d && dpa) {
		lpd[m] = (SINT4) round(dpa[k], sf);
	    } else {
		lpd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app short
xfer_ossinu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    spd = a->o_data[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    spa = a->obptr[k];
            if (j < ca && !d && spa) {
		spd[m] = spa[k];
	    } else {
	        spd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app long
xfer_oslinu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    spd = a->o_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    lpa = a->obptr[k];
            if (j < ca && !d && lpa) {
	        spd[m] = (SINT2) (lpa[i] >> 16);
	    } else {
	        spd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app float
xfer_osfinu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (MAXINT16 / mxflo);
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    spd = a->o_data[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    fpa = a->obptr[k];
            if (j < ca && !d && fpa) {
		spd[m] = (SINT2) round(fpa[k], sf);
	    } else {
	        spd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app double
xfer_osdinu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa;
    SINT2 *spd;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    spd = a->o_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    dpa = a->obptr[k];
            if (j < ca && !d && dpa) {
	        spd[m] = (SINT2) round(dpa[i], 16);
	    } else {
	        spd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app short
xfer_olsinu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    lpd = a->o_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    spa = a->obptr[k];
            if (j < ca && !d && spa) {
	        lpd[m] = (SINT4) spa[i] << 16;
	    } else {
	        lpd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app long
xfer_ollinu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    lpd = a->o_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    lpa = a->obptr[k];
            if (j < ca && !d && lpa) {
	        lpd[m] = lpa[i];
	    } else {
	        lpd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app float
xfer_olfinu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (MAXINT32 / mxflo);
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    lpd = a->o_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    fpa = a->obptr[k];
            if (j < ca && !d && fpa) {
	        lpd[m] = (SINT4) round(fpa[i], sf);
	    } else {
	        lpd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app double
xfer_oldinu(ARDEV *a, int b, int d)     // output, unequal
{					// dev interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT4 *lpd;

    sf = MAXINT32 / mxflo;
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    lpd = a->o_data[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    dpa = a->obptr[k];
            if (j < ca && !d && dpa) {
	        lpd[m] = (SINT4) round(dpa[i], sf);
	    } else {
	        lpd[m] = 0;
	    }
	}
    }
}

/*****************************************************************************/
// output: app & dev have equal number of output channels, dev non-interleaved
/*****************************************************************************/

static void				// dev short, app short
xfer_ossnie(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    c = a->ncda;
    spa = a->obptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    spd = a->o_data[k];
            if (!d && spa) {
	        spd[i] = spa[m];
            } else {
	        spd[i] = 0;
	    }
	}
    }
}

static void				// dev short, app long
xfer_oslnie(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    c = a->ncda;
    lpa = a->obptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    spd = a->o_data[k];
            if (!d && lpa) {
	        spd[i] = (SINT2) (lpa[m] >> 16);
            } else {
	        spd[i] = 0;
	    }
	}
    }
}

static void				// dev short, app float
xfer_osfnie(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (MAXINT16 / mxflo);
    n = a->sizptr[b];
    c = a->ncda;
    fpa = a->obptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    spd = a->o_data[k];
            if (!d && fpa) {
	        spd[i] = (SINT2) round(fpa[m], sf);
            } else {
	        spd[i] = 0;
	    }
	}
    }
}

static void				// dev short, app double
xfer_osdnie(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    double *dpa, sf;
    SINT2 *spd;

    sf = MAXINT16 / mxflo;
    n = a->sizptr[b];
    c = a->ncda;
    dpa = a->obptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    spd = a->o_data[k];
            if (!d && dpa) {
	        spd[i] = (SINT2) round(dpa[m], sf);
            } else {
	        spd[i] = 0;
	    }
	}
    }
}

static void				// dev long, app short
xfer_olsnie(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    c = a->ncda;
    spa = a->obptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    lpd = a->o_data[k];
            if (!d && spa) {
	        lpd[i] = (SINT4) spa[m] << 16;
            } else {
	        lpd[i] = 0;
	    }
	}
    }
}

static void				// dev long, app long
xfer_ollnie(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    c = a->ncda;
    lpa = a->obptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    lpd = a->o_data[k];
            if (!d && lpa) {
	        lpd[i] = lpa[m];
            } else {
	        lpd[i] = 0;
	    }
	}
    }
}

static void				// dev long, app float
xfer_olfnie(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (MAXINT32 / mxflo);
    n = a->sizptr[b];
    c = a->ncda;
    fpa = a->obptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    lpd = a->o_data[k];
            if (!d && fpa) {
	        lpd[i] = (SINT4) round(fpa[m], sf);
            } else {
	        lpd[i] = 0;
	    }
	}
    }
}

static void				// dev long, app double
xfer_oldnie(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, c;
    double *dpa, sf;
    SINT4 *lpd;

    sf = MAXINT32 / mxflo;
    n = a->sizptr[b];
    c = a->ncda;
    dpa = a->obptr[b];
    for (i = 0; i < n; i++) {
	for (j = 0; j < c; j++) {
	    k = j + b * c;
	    m = j + i * c;
	    lpd = a->o_data[k];
            if (!d && dpa) {
	        lpd[i] = (SINT4) round(dpa[m], sf);
            } else {
	        lpd[i] = 0;
	    }
	}
    }
}

static void				// dev short, app short
xfer_ossnne(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, n, c;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        spd = a->o_data[k];
        spa = a->obptr[k];
        for (i = 0; i < n; i++) {
            if (!d && spa) {
                spd[i] = spa[i];
            } else {
                spd[i] = 0;
	    }
        }
    }
}

static void				// dev short, app long
xfer_oslnne(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved to app non-interleaved
    int i, j, k, n, c;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        spd = a->o_data[k];
        lpa = a->obptr[k];
        for (i = 0; i < n; i++) {
            if (!d && lpa) {
                spd[i] = (SINT2) (lpa[i] >> 16);
            } else {
                spd[i] = 0;
	    }
        }
    }
}

static void				// dev short, app float
xfer_osfnne(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, n, c;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (MAXINT16 / mxflo);
    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        spd = a->o_data[k];
        fpa = a->obptr[k];
        for (i = 0; i < n; i++) {
            if (!d && fpa) {
                spd[i] = (SINT2) round(fpa[i], sf);
            } else {
                spd[i] = 0;
	    }
        }
    }
}

static void				// dev short, app double
xfer_osdnne(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved to app non-interleaved
    int i, j, k, n, c;
    double *dpa, sf;
    SINT2 *spd;

    sf = MAXINT16 / mxflo;
    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        spd = a->o_data[k];
        dpa = a->obptr[k];
        for (i = 0; i < n; i++) {
            if (!d && dpa) {
                spd[i] = (SINT2) round(dpa[i], sf);
            } else {
                spd[i] = 0;
	    }
        }
    }
}

static void				// dev long, app short
xfer_olsnne(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, n, c;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        lpd = a->o_data[k];
        spa = a->obptr[k];
        for (i = 0; i < n; i++) {
            if (!d && spa) {
                lpd[i] = (SINT4) spa[i] << 16;
            } else {
                lpd[i] = 0;
	    }
        }
    }
}

static void				// dev long, app long
xfer_ollnne(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, n, c;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        lpd = a->o_data[k];
        lpa = a->obptr[k];
        for (i = 0; i < n; i++) {
            if (!d && lpa) {
                lpd[i] = lpa[i];
            } else {
                lpd[i] = 0;
	    }
        }
    }
}

static void				// dev long, app float
xfer_olfnne(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, n, c;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (MAXINT32 / mxflo);
    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        lpd = a->o_data[k];
        fpa = a->obptr[k];
        for (i = 0; i < n; i++) {
            if (!d && fpa) {
                lpd[i] = (SINT4) round(fpa[i], sf);
            } else {
                lpd[i] = 0;
	    }
        }
    }
}

static void				// dev long, app double
xfer_oldnne(ARDEV *a, int b, int d)     // output, equal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, n, c;
    double *dpa, sf;
    SINT4 *lpd;

    sf = MAXINT32 / mxflo;
    n = a->sizptr[b];
    c = a->ncda;
    for (j = 0; j < c; j++) {
	k = j + b * c;
        lpd = a->o_data[k];
        dpa = a->obptr[k];
        for (i = 0; i < n; i++) {
            if (!d && dpa) {
                lpd[i] = (SINT4) round(dpa[i], sf);
            } else {
                lpd[i] = 0;
	    }
        }
    }
}

/*****************************************************************************/
// output: app & dev have unequal number of output channels, dev non-interleaved
/*****************************************************************************/

static void				// dev short, app short
xfer_ossniu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    spa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
	    spd = a->o_data[k];
            if (j < ca && !d && spa) {
	        spd[m] = spa[k];
	    } else {
		spd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app long
xfer_oslniu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    lpa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
	    spd = a->o_data[k];
            if (j < ca && !d && lpa) {
	        spd[m] = (SINT2) (lpa[k] >> 16);
	    } else {
		spd[m] = 0;
	    }
        }
    }
}

static void				// dev short, app float
xfer_osfniu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (MAXINT16 / mxflo);
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    fpa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
	    spd = a->o_data[k];
            if (j < ca && !d && fpa) {
	        spd[m] = (SINT2) round(fpa[k], sf);
	    } else {
		spd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app double
xfer_osdniu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT2 *spd;

    sf = MAXINT16 / mxflo;
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    dpa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
	    spd = a->o_data[k];
            if (j < ca && !d && dpa) {
	        spd[m] = (short) round(dpa[k], sf);
	    } else {
		spd[m] = 0;
	    }
        }
    }
}

static void				// dev long, app SINT2
xfer_olsniu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    spa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
	    lpd = a->o_data[k];
	    if (j < ca && !d && spa) {
		lpd[m] = (SINT4) spa[k] << 16;
	    } else {
		lpd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app long
xfer_ollniu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    lpa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
	    lpd = a->o_data[k];
	    if (j < ca && !d && lpa) {
		lpd[m] = lpa[k];
	    } else {
		lpd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app float
xfer_olfniu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (MAXINT32 / mxflo);
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    fpa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
	    lpd = a->o_data[k];
	    if (j < ca && !d && fpa) {
		lpd[m] = (SINT4) round(fpa[k], sf);
	    } else {
		lpd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app double
xfer_oldniu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT4 *lpd;

    sf = MAXINT32 / mxflo;
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    dpa = a->obptr[b];
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + i * ca;
	    m = j + i * cd;
	    lpd = a->o_data[k];
	    if (j < ca && !d && dpa) {
		lpd[m] = (SINT4) round(dpa[k], sf);
	    } else {
		lpd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app short
xfer_ossnnu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT2 *spd, *spa;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    for (i = 0; i < n; i++) {
	for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    spd = a->o_data[k];
	    spa = a->obptr[k];
            if (j < ca && !d && spa) {
	        spd[m] = spa[i];
	    } else {
	        spd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app long
xfer_oslnnu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpa;
    SINT2 *spd;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    spd = a->o_data[k];
	    lpa = a->obptr[k];
            if (j < ca && !d && lpa) {
	        spd[m] = (SINT2) (lpa[i] >> 16);
	    } else {
	        spd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app float
xfer_osfnnu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT2 *spd;

    sf = (float) (MAXINT16 / mxflo);
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    for (i = 0; i < n; i++) {
	for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    spd = a->o_data[k];
	    fpa = a->obptr[k];
            if (j < ca && !d && fpa) {
	        spd[m] = (SINT2) round(fpa[i], sf);
	    } else {
	        spd[m] = 0;
	    }
	}
    }
}

static void				// dev short, app double
xfer_osdnnu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT2 *spd;

    sf = MAXINT16 / mxflo;
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    spd = a->o_data[k];
            dpa = a->obptr[k];
            if (j < ca && !d && dpa) {
	        spd[m] = (SINT2) round(dpa[i], sf);
	    } else {
	        spd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app short
xfer_olsnnu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd;
    SINT2 *spa;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    lpd = a->o_data[k];
	    spa = a->obptr[k];
            if (j < ca && !d && spa) {
	        lpd[m] = (SINT4) spa[i] << 16;
	    } else {
	        lpd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app long
xfer_ollnnu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    SINT4 *lpd, *lpa;

    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    lpd = a->o_data[k];
            lpa = a->obptr[k];
            if (j < ca && !d && lpa) {
	        lpd[m] = lpa[i];
	    } else {
	        lpd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app float
xfer_olfnnu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    float *fpa, sf;
    SINT4 *lpd;

    sf = (float) (MAXINT32 / mxflo);
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    lpd = a->o_data[k];
	    fpa = a->obptr[k];
            if (j < ca && !d && fpa) {
	        lpd[m] = (SINT4) round(fpa[i], sf);
	    } else {
	        lpd[m] = 0;
	    }
	}
    }
}

static void				// dev long, app double
xfer_oldnnu(ARDEV *a, int b, int d)     // output, unequal
{					// dev non-interleaved, app non-interleaved
    int i, j, k, m, n, cd, ca;
    double *dpa, sf;
    SINT4 *lpd;

    sf = MAXINT32 / mxflo;
    n = a->sizptr[b];
    cd = a->ncda;
    ca = a->a_ncda;
    for (i = 0; i < n; i++) {
        for (j = 0; j < cd; j++) {
	    k = j + b * ca;
	    m = j + i * cd;
	    lpd = a->o_data[k];
            dpa = a->obptr[k];
            if (j < ca && !d && dpa) {
	        lpd[m] = (SINT4) round(dpa[i], sf);
	    } else {
	        lpd[m] = 0;
	    }
	}
    }
}

/*****************************************************************************/

static void (*xfer_in[NDT*16])(ARDEV*,int) = {
    xfer_issine,    // dev shrt, int; app shrt, non; input, equal
    xfer_isline,    // dev shrt, int; app long, non; input, equal
    xfer_isfine,    // dev shrt, int; app flt,  non; input, equal
    xfer_isdine,    // dev shrt, int; app dbl,  non; input, equal
    xfer_ilsine,    // dev long, int; app shrt, non; input, equal
    xfer_illine,    // dev long, int; app long, non; input, equal
    xfer_ilfine,    // dev long, int; app flt,  non; input, equal
    xfer_ildine,    // dev long, int; app dbl,  non; input, equal
    xfer_issiie,    // dev shrt, int; app shrt, int; input, equal
    xfer_isliie,    // dev shrt, int; app long, int; input, equal
    xfer_isfiie,    // dev shrt, int; app flt,  int; input, equal
    xfer_isdiie,    // dev shrt, int; app dbl,  int; input, equal
    xfer_ilsiie,    // dev long, int; app shrt, int; input, equal
    xfer_illiie,    // dev long, int; app long, int; input, equal
    xfer_ilfiie,    // dev long, int; app flt,  int; input, equal
    xfer_ildiie,    // dev long, int; app dbl,  int; input, equal
    xfer_issnne,    // dev shrt, non; app shrt, non; input, equal
    xfer_islnne,    // dev shrt, non; app long, non; input, equal
    xfer_isfnne,    // dev shrt, non; app flt,  non; input, equal
    xfer_isdnne,    // dev shrt, non; app dbl,  non; input, equal
    xfer_ilsnne,    // dev long, non; app shrt, non; input, equal
    xfer_illnne,    // dev long, non; app long, non; input, equal
    xfer_ilfnne,    // dev long, non; app flt,  non; input, equal
    xfer_ildnne,    // dev long, non; app dbl,  non; input, equal
    xfer_issnie,    // dev shrt, non; app shrt, int; input, equal
    xfer_islnie,    // dev shrt, non; app long, int; input, equal
    xfer_isfnie,    // dev shrt, non; app flt,  int; input, equal
    xfer_isdnie,    // dev shrt, non; app dbl,  int; input, equal
    xfer_ilsnie,    // dev long, non; app shrt, int; input, equal
    xfer_illnie,    // dev long, non; app long, int; input, equal
    xfer_ilfnie,    // dev long, non; app flt,  int; input, equal
    xfer_ildnie,    // dev long, non; app dbl,  int; input, equal
    xfer_issinu,    // dev shrt, int; app shrt, non; input, unequal
    xfer_islinu,    // dev shrt, int; app long, non; input, unequal
    xfer_isfinu,    // dev shrt, int; app flt,  non; input, unequal
    xfer_isdinu,    // dev shrt, int; app dbl,  non; input, unequal
    xfer_ilsinu,    // dev long, int; app shrt, non; input, unequal
    xfer_illinu,    // dev long, int; app long, non; input, unequal
    xfer_ilfinu,    // dev long, int; app flt,  non; input, unequal
    xfer_ildinu,    // dev long, int; app dbl,  non; input, unequal
    xfer_issiiu,    // dev shrt, int; app shrt, int; input, unequal
    xfer_isliiu,    // dev shrt, int; app long, int; input, unequal
    xfer_isfiiu,    // dev shrt, int; app flt,  int; input, unequal
    xfer_isdiiu,    // dev shrt, int; app dbl,  int; input, unequal
    xfer_ilsiiu,    // dev long, int; app shrt, int; input, unequal
    xfer_illiiu,    // dev long, int; app long, int; input, unequal
    xfer_ilfiiu,    // dev long, int; app flt,  int; input, unequal
    xfer_ildiiu,    // dev long, int; app dbl,  int; input, unequal
    xfer_issnnu,    // dev shrt, non; app shrt, non; input, unequal
    xfer_islnnu,    // dev shrt, non; app long, non; input, unequal
    xfer_isfnnu,    // dev shrt, non; app flt,  non; input, unequal
    xfer_isdnnu,    // dev shrt, non; app dbl,  non; input, unequal
    xfer_ilsnnu,    // dev long, non; app shrt, non; input, unequal
    xfer_illnnu,    // dev long, non; app long, non; input, unequal
    xfer_ilfnnu,    // dev long, non; app flt,  non; input, unequal
    xfer_ildnnu,    // dev long, non; app dbl,  non; input, unequal
    xfer_issniu,    // dev shrt, non; app shrt, int; input, unequal
    xfer_islniu,    // dev shrt, non; app long, int; input, unequal
    xfer_isfniu,    // dev shrt, non; app flt,  int; input, unequal
    xfer_isdniu,    // dev shrt, non; app dbl,  int; input, unequal
    xfer_ilsniu,    // dev long, non; app shrt, int; input, unequal
    xfer_illniu,    // dev long, non; app long, int; input, unequal
    xfer_ilfniu,    // dev long, non; app flt,  int; input, unequal
    xfer_ildniu,    // dev long, non; app dbl,  int; input, unequal
};
static void (*xfer_out[NDT*16])(ARDEV*,int,int) = {
    xfer_ossine,    // dev shrt, int; app shrt, non; output, equal
    xfer_osline,    // dev shrt, int; app long, non; output, equal
    xfer_osfine,    // dev shrt, int; app flt,  non; output, equal
    xfer_osdine,    // dev shrt, int; app dbl,  non; output, equal
    xfer_olsine,    // dev long, int; app shrt, non; output, equal
    xfer_olline,    // dev long, int; app long, non; output, equal
    xfer_olfine,    // dev long, int; app flt,  non; output, equal
    xfer_oldine,    // dev long, int; app dbl,  non; output, equal
    xfer_ossiie,    // dev shrt, int; app shrt, int; output, equal
    xfer_osliie,    // dev shrt, int; app long, int; output, equal
    xfer_osfiie,    // dev shrt, int; app flt,  int; output, equal
    xfer_osdiie,    // dev shrt, int; app dbl,  int; output, equal
    xfer_olsiie,    // dev long, int; app shrt, int; output, equal
    xfer_olliie,    // dev long, int; app long, int; output, equal
    xfer_olfiie,    // dev long, int; app flt,  int; output, equal
    xfer_oldiie,    // dev long, int; app dbl,  int; output, equal
    xfer_ossnne,    // dev shrt, non; app shrt, non; output, equal
    xfer_oslnne,    // dev shrt, non; app long, non; output, equal
    xfer_osfnne,    // dev shrt, non; app flt,  non; output, equal
    xfer_osdnne,    // dev shrt, non; app dbl,  non; output, equal
    xfer_olsnne,    // dev long, non; app shrt, non; output, equal
    xfer_ollnne,    // dev long, non; app long, non; output, equal
    xfer_olfnne,    // dev long, non; app flt,  non; output, equal
    xfer_oldnne,    // dev long, non; app dbl,  non; output, equal
    xfer_ossnie,    // dev shrt, non; app shrt, int; output, equal
    xfer_oslnie,    // dev shrt, non; app long, int; output, equal
    xfer_osfnie,    // dev shrt, non; app flt,  int; output, equal
    xfer_osdnie,    // dev shrt, non; app dbl,  int; output, equal
    xfer_olsnie,    // dev long, non; app shrt, int; output, equal
    xfer_ollnie,    // dev long, non; app long, int; output, equal
    xfer_olfnie,    // dev long, non; app flt,  int; output, equal
    xfer_oldnie,    // dev long, non; app dbl,  int; output, equal
    xfer_ossinu,    // dev shrt, int; app shrt, non; output, unequal
    xfer_oslinu,    // dev shrt, int; app long, non; output, unequal
    xfer_osfinu,    // dev shrt, int; app flt,  non; output, unequal
    xfer_osdinu,    // dev shrt, int; app dbl,  non; output, unequal
    xfer_olsinu,    // dev long, int; app shrt, non; output, unequal
    xfer_ollinu,    // dev long, int; app long, non; output, unequal
    xfer_olfinu,    // dev long, int; app flt,  non; output, unequal
    xfer_oldinu,    // dev long, int; app dbl,  non; output, unequal
    xfer_ossiiu,    // dev shrt, int; app shrt, int; output, unequal
    xfer_osliiu,    // dev shrt, int; app long, int; output, unequal
    xfer_osfiiu,    // dev shrt, int; app flt,  int; output, unequal
    xfer_osdiiu,    // dev shrt, int; app dbl,  int; output, unequal
    xfer_olsiiu,    // dev long, int; app shrt, int; output, unequal
    xfer_olliiu,    // dev long, int; app long, int; output, unequal
    xfer_olfiiu,    // dev long, int; app flt,  int; output, unequal
    xfer_oldiiu,    // dev long, int; app dbl,  int; output, unequal
    xfer_ossnnu,    // dev shrt, non; app shrt, non; output, unequal
    xfer_oslnnu,    // dev shrt, non; app long, non; output, unequal
    xfer_osfnnu,    // dev shrt, non; app flt,  non; output, unequal
    xfer_osdnnu,    // dev shrt, non; app dbl,  non; output, unequal
    xfer_olsnnu,    // dev long, non; app shrt, non; output, unequal
    xfer_ollnnu,    // dev long, non; app long, non; output, unequal
    xfer_olfnnu,    // dev long, non; app flt,  non; output, unequal
    xfer_oldnnu,    // dev long, non; app dbl,  non; output, unequal
    xfer_ossniu,    // dev shrt, non; app shrt, int; output, unequal
    xfer_oslniu,    // dev shrt, non; app long, int; output, unequal
    xfer_osfniu,    // dev shrt, non; app flt,  int; output, unequal
    xfer_osdniu,    // dev shrt, non; app dbl,  int; output, unequal
    xfer_olsniu,    // dev long, non; app shrt, int; output, unequal
    xfer_ollniu,    // dev long, non; app long, int; output, unequal
    xfer_olfniu,    // dev long, non; app flt,  int; output, unequal
    xfer_oldniu,    // dev long, non; app dbl,  int; output, unequal
};

// set_xfer - bind functions to handle i/o data segment transfers

SINT4
_ar_xfer_bind(SINT4 di)
{
    int d_nbps, a_xfer, a_ncad, a_ncda, t, i, u;
    ARDEV *a;
    ARFMT *f;
    ARXFR *x;

    a = _ardev[di];
    f = &_arfmt[di];
    x = &_arxfr[di];

    d_nbps = a->nbps;
    a_xfer = f->a_xfer;
    a_ncad = a->ibptr ? a->a_ncad : 0;
    a_ncda = a->obptr ? a->a_ncda : 0;

    mxfli = (f->mxfli > 0) ? f->mxfli : 1;	// maximum float in
    mxflo = (f->mxflo > 0) ? f->mxflo : 1;	// maximum float out

    if (d_nbps == 2) {		// dev short
        t = a_xfer - 1;
    } else if (d_nbps == 4) {	// dev long
        t = a_xfer + 3;
    } else {	
	return (1);				// bad sample format
    }
    i = t + (f->a_ntlv ? (NDT*2) : 0) + (a->ntlv ? 0 : (NDT*4));
    if (a_ncad) {
	u = (a_ncad == a->ncad) ? 0 : (NDT*8);
	x->xfer_in = xfer_in[i + u];		// input xfer
    } else {
	x->xfer_in = NULL;
    }
    if (a_ncda) {
	u = (a_ncda == a->ncda) ? 0 : (NDT*8);

	x->xfer_out = xfer_out[i + u];		// output xfer

    } else {
	x->xfer_out = NULL;
    }
    return (0);
}

/* arsc_win.c - soundcard functions for Windows/wave-audio devices */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "arsclib.h"
#include "arscdev.h"
#include "arsc_common.h"

#define strncasecmp	_strnicmp
#define free_null(p)    if (p){free(p);p=NULL;}
#define ANY_RATE	0xFFFFFFFF

/***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

#ifdef mgw
#undef WAVE_FORMAT_EXTENSIBLE
#endif
#ifdef WAVE_FORMAT_EXTENSIBLE
static int wav_fmt_ext[MAXDEV];		// use extensible format ?
#endif

static int dio = 0;			// device identifier offset

static CRITICAL_SECTION cs;
static HWAVEIN hwi[MAXDEV];
static HWAVEOUT hwo[MAXDEV];
static WAVEHDR **whi[MAXDEV] = {0};
static WAVEHDR **who[MAXDEV] = {0};

/***************************************************************************/

/* chkreg - check registry for card information */

static int
check_registry()
{
    char key_name[80], card_name[MAX_CT_NAME];
    int i, j;
    SINT4 n, r, t, temp;
    CARDINFO ci;
    HKEY hKey;
    void _arsc_set_cardinfo(CARDINFO, int);

    sprintf(key_name,"SOFTWARE\\BTNRH\\ARSC\\CardInfo");
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, KEY_QUERY_VALUE, &hKey)
	!= ERROR_SUCCESS) {
	return (1);
    }
    for (i = 0; i < MAXNCT; i++) {
	memset(&ci, 0, sizeof(ci));
        sprintf(key_name,"SOFTWARE\\BTNRH\\ARSC\\CardInfo\\CardType%d", i);
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, KEY_QUERY_VALUE, &hKey)
	    == ERROR_SUCCESS) {
	    n = MAX_CT_NAME;
	    t = REG_SZ;
	    r = RegQueryValueEx(hKey, "Name", NULL, (LPDWORD) &t, (LPBYTE) card_name, (LPDWORD) &n);
	    if (r == 0) {
		strncpy(ci.name, card_name, MAX_CT_NAME);
	    } else {
		continue;
	    }
	    n = sizeof(SINT4);
	    t = REG_DWORD;
	    r = RegQueryValueEx(hKey, "bits", NULL, (LPDWORD) &t, (LPBYTE) &temp, (LPDWORD) &n);
	    if (r == 0) {
		ci.bits = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "left", NULL, (LPDWORD) &t, (LPBYTE) &temp, (LPDWORD) &n);
	    if (r == 0) {
		ci.left = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "nbps", NULL, (LPDWORD) &t, (LPBYTE) &temp, (LPDWORD) &n);
	    if (r == 0) {
		ci.nbps = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "ncio", NULL, (LPDWORD) &t, (LPBYTE) &temp, (LPDWORD) &n);
	    if (r == 0) {
		ci.ncad = ci.ncda = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "ncad", NULL, (LPDWORD) &t, (LPBYTE) &temp, (LPDWORD) &n);
	    if (r == 0) {
		ci.ncad = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "ncda", NULL, (LPDWORD) &t, (LPBYTE) &temp, (LPDWORD) &n);
	    if (r == 0) {
		ci.ncda = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "gdsr", NULL, (LPDWORD) &t, (LPBYTE) &temp, (LPDWORD) &n);
	    if (r == 0) {
		ci.gdsr = (int) temp;
	    }
	    r = RegQueryValueEx(hKey, "ad_mv_fs", NULL, (LPDWORD) &t, (LPBYTE) &temp, (LPDWORD) &n);
	    if (r == 0) {
		if (temp != 0)
		    for (j = 0; j < MAXNCH; j++)
			ci.ad_vfs[j] = temp * 0.001;
	    }
	    r = RegQueryValueEx(hKey, "da_mv_fs", NULL, (LPDWORD) &t, (LPBYTE) &temp, (LPDWORD) &n);
	    if (r == 0) {
		if (temp != 0)
		    for (j = 0; j < MAXNCH; j++)
			ci.da_vfs[j] = temp * 0.001;
	    }
	    for (j = 0; j < MAXNCH; j++) {
		sprintf(key_name, "ad%d_mv_fs", j + 1);
		r = RegQueryValueEx(hKey, key_name, NULL, (LPDWORD) &t, (LPBYTE) &temp, (LPDWORD) &n);
		if (r == 0) {
		    ci.ad_vfs[j] = temp * 0.001;
		}
		sprintf(key_name, "da%d_mv_fs", j + 1);
		r = RegQueryValueEx(hKey, key_name, NULL, (LPDWORD) &t, (LPBYTE) &temp, (LPDWORD) &n);
		if (r == 0) {
		    ci.da_vfs[j] = temp * 0.001;
		}
	    }
            RegCloseKey(hKey);
	}
	_arsc_set_cardinfo(ci, i);
    }
    return (0);
}

/***************************************************************************/

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
    char s[80], *env;
    int i = -1;
    CARDINFO ci;
    FILE *fp;
    static char fn[256] = "C:/Program Files/BTNRH/ARSC/arsc.cfg";
    void _arsc_set_cardinfo(CARDINFO, int);
    static CARDINFO zci = {{0},0,0,0,0,0,0,{0},{0}};

    // Look for alternate configuration file
    env = getenv("ARSC.CFG");
    if (env) {
	strncpy(fn, env, 256);
    }

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

void CALLBACK waveInProc(
  HWAVEIN hwi,
  UINT uMsg,
  DWORD di,
  DWORD dwParam1,
  DWORD dwParam2
)
{
    if (uMsg == WIM_DATA) {
	if (_arsc_wind)
	    PostMessage((HWND) _arsc_wind, WM_ARSC, 1, di);
    }
}

void CALLBACK waveOutProc(
  HWAVEOUT hwo,
  UINT uMsg,
  DWORD di,
  DWORD dwParam1,
  DWORD dwParam2
)
{
    if (uMsg == WOM_DONE) {
	if (_arsc_wind)
	    PostMessage((HWND) _arsc_wind, WM_ARSC, 2, di);
    }
}

/***************************************************************************/

/* open_io - open input and ouput devices */

static int
open_io(SINT4 di)
{
    SINT4   d, sync_call = WAVE_ALLOWSYNC | CALLBACK_FUNCTION;
    ARDEV *a;
    struct {
	WAVEFORMATEX  f;
	WORD    ValidBits;	/* bits of precision */
	DWORD   ChannelMask;	/* which channels are present in stream */
	GUID    SubFormat;	/* globally unique identifier */
    } w;
    MMRESULT Result;

    a = _ardev[di];
    d = di - dio;

    // modify i/o channel masks

    if (a->ncad == 1) {
        a->chnmsk_i = 3;
        a->ncad = 2;	
    }
    if (a->ncda == 1) {
        a->chnmsk_o = 3;
        a->ncda = 2;	
    }
    a->nbps = 4;

    memset(&w, 0, sizeof(w));
    w.f.cbSize = sizeof(w) - sizeof(WAVEFORMATEX);
    w.f.nSamplesPerSec = a->rate;
    w.f.wBitsPerSample = (WORD) a->nbps * 8;
    w.f.wFormatTag = WAVE_FORMAT_PCM;
#ifdef WAVE_FORMAT_EXTENSIBLE
    if (wav_fmt_ext[d]) {
        w.f.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    }
    w.ValidBits = (unsigned short) a->bits;
    w.ChannelMask = a->chnmsk_o;
    w.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
#endif /* WAVE_FORMAT_EXTENSIBLE */

    // open "In" before "Out" to avoid "allocated" error
    if (a->ncad) {
        w.ChannelMask = a->chnmsk_i;
        w.f.nChannels = (unsigned short) a->ncad;
        w.f.nBlockAlign = (unsigned short) (a->ncad * a->nbps);
        w.f.nAvgBytesPerSec = (w.f.nBlockAlign * w.f.nSamplesPerSec);
        Result = waveInOpen(&hwi[d], d,
	    (PWAVEFORMATEX) &w, 
	    (DWORD) waveInProc, (DWORD) di,
	    sync_call);
#ifdef WAVE_FORMAT_EXTENSIBLE
	if ((Result != 0) && (!wav_fmt_ext[d])) {
	    w.f.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	    Result = waveInOpen(&hwi[d], d, 
		(PWAVEFORMATEX) &w, 
		(DWORD) waveInProc, (DWORD) di,
		sync_call);
	    wav_fmt_ext[d] = (Result == 0);
        }
#endif /* WAVE_FORMAT_EXTENSIBLE */
        if (Result != 0) {
            return (Result);
        }
    }

    if (a->ncda) {
        w.ChannelMask = a->chnmsk_o;
        w.f.nChannels = (unsigned short) a->ncda;
        w.f.nBlockAlign = (unsigned short) (a->ncda * a->nbps);
        w.f.nAvgBytesPerSec = (w.f.nBlockAlign * w.f.nSamplesPerSec);
	Result = waveOutOpen(&hwo[d], d,
	    (PWAVEFORMATEX)&w,
	    (DWORD) waveOutProc, (DWORD) di,
	    sync_call);
#ifdef WAVE_FORMAT_EXTENSIBLE
	if ((Result != 0) && (!wav_fmt_ext[d])) {
            w.f.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
            Result = waveOutOpen(&hwo[d], d, 
		(PWAVEFORMATEX)&w,
		(DWORD) waveOutProc, (DWORD) di,
		sync_call);
	    wav_fmt_ext[d] = (Result == 0);
        }
#endif /* WAVE_FORMAT_EXTENSIBLE */
        if (Result != 0) {
            if (a->ncad)
                waveInClose(hwi[d]);
            return (Result);
        }
    }

    /*
     * MMSYSERR codes:
     *  1 = unspecified error
     *  2 = device ID out of range 
     *  3 = driver enable failed 
     *  4 = device already allocated 
     *  6 = no device driver present
     *  7 = memory allocation error
     * 10 = invalied flag passed
     * WAVERR codes:
     * 32 = unsupported wave format 
     */

    // Set volume to max
    waveOutSetVolume(hwo[d], 0xFFFFFFFF);

    if (a->ncad || a->ncda)
	InitializeCriticalSection(&cs);

    return (0);
}

/* close_io - close input and ouput devices */

static void
close_io(SINT4 di)
{
    SINT4 d;
    ARDEV *a;

    a = _ardev[di];
    d = di - dio;

    if (a->ncad) {			    // close waveIn
        (void) waveInReset(hwi[d]);
	(void) waveInClose(hwi[d]);
	hwi[d] = 0;
    }
    if (a->ncda) {			    // close waveOut
        (void) waveOutReset(hwo[d]);
	(void) waveOutClose(hwo[d]);
	hwo[d] = 0;
    }
    if (a->ncad || a->ncda)
        DeleteCriticalSection(&cs);
}

/***************************************************************************/

/* _ar_win_num_dev - return number of devices */

static SINT4
_ar_win_num_dev()
{
    SINT4 nd;

    if (_arsc_find & ARSC_PREF_IN) {
        nd = (int) waveInGetNumDevs();
    } else {
        nd = (int) waveOutGetNumDevs();
    }

    return (nd);
}

/* _ar_win_dev_name - return name of I/O device */

static char   *
_ar_win_dev_name(SINT4 d)
{
    WAVEINCAPS WavInCaps;
    WAVEOUTCAPS WavOutCaps;
    MMRESULT Result;
    static char name[MAXNAM];

    if (_arsc_find & ARSC_PREF_IN) {
	Result = waveInGetDevCaps(d, &WavInCaps, sizeof(WAVEINCAPS));
	strncpy(name, WavInCaps.szPname,MAXNAM);
    } else {
	Result = waveOutGetDevCaps(d, &WavOutCaps, sizeof(WAVEOUTCAPS));
	strncpy(name, WavOutCaps.szPname, MAXNAM);
    }
    if (Result != 0) {
	*name = '\0';
    }

    return (name);
}

/* _ar_win_list_rates - create a list of available sample rates */

static SINT4
_ar_win_list_rates(SINT4 di)
{
    SINT4    i, d, gdsr;
    ARDEV  *a;
    struct {
	WAVEFORMATEX  f;
	WORD    ValidBits;	/* bits of precision */
	DWORD   ChannelMask;	/* which channels are present in stream */
	GUID    SubFormat;	/* globally unique identifier */
    } w;
    MMRESULT Result;

    a = _ardev[di];
    d = di - dio;

/* set WAVEFORMATEX structure with proposed value and see if they're
 * accepted 
 */

    // Initialize the waveformat structure
    memset(&w, 0, sizeof(w));
    w.f.cbSize = sizeof(w) - sizeof(WAVEFORMATEX);
    w.f.wBitsPerSample = (WORD) (a->nbps * 8);
    w.f.nChannels = (unsigned short) a->ncda;
    w.f.nBlockAlign = (unsigned short) (a->ncda * a->nbps);
    w.f.nAvgBytesPerSec = (w.f.nBlockAlign * w.f.nSamplesPerSec);
    w.f.wFormatTag = WAVE_FORMAT_PCM;
#ifdef WAVE_FORMAT_EXTENSIBLE
    if (wav_fmt_ext[d]) {
        w.f.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    }
    w.ValidBits = (unsigned short) a->bits;
    w.ChannelMask = a->chnmsk_o;
    w.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
#endif /* WAVE_FORMAT_EXTENSIBLE */
    gdsr = 0;
    for (i = 0; i < SRLSTSZ; i++) {
	if (_ar_SRlist[i] <= 0)
	    continue;
        w.f.nSamplesPerSec = _ar_SRlist[i];
	//wfmt.nAvgBytesPerSec = (wfmt.nBlockAlign * wfmt.nSamplesPerSec);
	// Although one would think that the preceeding line might be necessary,
        // in fact, when included, Result returns zero even if an absurd
	// sampling rate is requested
        Result = waveInOpen(0, d, (PWAVEFORMATEX)&w, 0L, 0L, WAVE_FORMAT_QUERY);
	if (Result == 0)
	    gdsr |= 1 << i;
    }

    return (gdsr);
}

/* _ar_win_close - close wave-audio device */

static void
_ar_win_close(SINT4 di)
{
    close_io(di);
}

/* _ar_win_open - open wave-audio device */

static SINT4
_ar_win_open(SINT4 di)
{
    SINT4 err;
    ARDEV *a;

    a = _ardev[di];

    // Initialize sampling rates
    if (!a->gdsr)
        a->gdsr = _ar_win_list_rates(di);
    a->rate = _ar_adjust_rate(di, a->a_rate);

    // open i/o device
    a->ntlv = 1;			// always interleave channel samples
    err = open_io(di);

    return (err);
}

/* _ar_win_io_prepare - prepare device and buffers for I/O */

static SINT4
_ar_win_io_prepare(SINT4 di)
{
    int     b, no, ni, ss;
    SINT4   *sz, d;
    ARDEV  *a;
    MMRESULT Result;

    FDBUG ( (_arS, "win_io_prepare:\n") );

    a = _ardev[di];
    d = di - dio;

    ss = a->segswp;
    sz = a->sizptr;

    if (a->ncad) {			    // Prepare wave_in header.

        close_io(di);			    // reopen to improve Indigo synch
	open_io(di);

        whi[d] = (WAVEHDR **) calloc(ss, sizeof(WAVEHDR *));

	for (b = 0; b < ss; b++) {
	    whi[d][b] = (WAVEHDR *) calloc(1, sizeof(WAVEHDR));
	    ni = a->ncad * a->nbps * sz[b];
	    whi[d][b]->lpData = (HPSTR) a->i_data[b];
	    whi[d][b]->dwBufferLength = ni;
	    whi[d][b]->dwFlags = 0L;
	    whi[d][b]->dwLoops = 0L;
	    Result = waveInPrepareHeader(hwi[d], whi[d][b], sizeof(WAVEHDR));
	    if ( Result != MMSYSERR_NOERROR ) {
		return ( -1 );
	    } // fi
	}

        Result = waveInStop(hwi[d]);
        if ( Result != MMSYSERR_NOERROR ) {
            return ( -1 );
        } // fi
    }

    if (a->ncda) {			    // Prepare wave_out header.

        who[d] = (WAVEHDR **) calloc(ss, sizeof(WAVEHDR *));

        for (b = 0; b < ss; b++) {
            who[d][b] = (WAVEHDR *) calloc(1, sizeof(WAVEHDR));
            no = a->ncda * a->nbps * sz[b];
            who[d][b]->lpData = (HPSTR) a->o_data[b];
            who[d][b]->dwBufferLength = no;
            who[d][b]->dwFlags = 0L;			// Must be 0
            who[d][b]->dwLoops = 0L;
	    Result = waveOutPrepareHeader( hwo[d], who[d][b], sizeof(WAVEHDR) );
            if ( Result != MMSYSERR_NOERROR ) {
                return ( -1 );
            } // fi
        } // rof

        Result = waveOutPause(hwo[d]);
        if ( Result != MMSYSERR_NOERROR ) {
            return ( -1 );
        } // fi
    } // fi

    return (0);
}

/* _ar_win_xfer_seg - this segment is ready to go */

static SINT4
_ar_win_xfer_seg(SINT4 di, SINT4 b)
{
    SINT4 d;
    ARDEV  *a;
    MMRESULT Result;

    a = _ardev[di];
    d = di - dio;

    if (a->ncad) {	// queue input data buffer to the input device
        Result = waveInAddBuffer(hwi[d], whi[d][b], sizeof(WAVEHDR));
        if ( Result != MMSYSERR_NOERROR ) {
            return ( -1 );
        } // fi
    }

    if (a->ncda) {	// queue ouput data buffer to the output device.

        Result = waveOutWrite(hwo[d], who[d][b], sizeof(WAVEHDR));
        if ( Result != MMSYSERR_NOERROR ) {
            return ( -1 );
        } // fi
    }

    return ( 0 );
}

/* _ar_win_chk_seg - check for segment completion */

static SINT4
_ar_win_chk_seg(SINT4 di, SINT4 b)
{
    int id, od;
    SINT4 d;
    ARDEV  *a;

    a = _ardev[di];
    d = di - dio;

    id = !a->ncad || (whi[d][b]->dwFlags & WHDR_DONE);	// input done ?
    od = !a->ncda || (who[d][b]->dwFlags & WHDR_DONE);	// output done ?

    return (id && od);
}

/* _ar_win_io_start - start I/O */

static void
_ar_win_io_start(SINT4 di)
{
    SINT4 d;
    ARDEV  *a;

    a = _ardev[di];
    d = di - dio;

    // start output and input at the same time
    if (a->ncad || a->ncda) {
	EnterCriticalSection(&cs);
    }
    if (a->ncad) {
        (void) waveInStart(hwi[d]);
    }
    if (a->ncda) {
        (void) waveOutRestart(hwo[d]);
    }
    if (a->ncad || a->ncda) {
	LeaveCriticalSection(&cs);
    }
}

/* _ar_win_io_stop - stop I/O */

static void
_ar_win_io_stop(SINT4 di)
{
    SINT4    b, d;
    ARDEV  *a;

    a = _ardev[di];
    d = di - dio;

    if (a->ncad) {		    // stop input
        waveInReset(hwi[d]);
	for (b = 0; b < a->segswp; b++) {
	    waveInUnprepareHeader(hwi[d], whi[d][b], sizeof(WAVEHDR));
	    free_null(whi[d][b]);
	}
	free_null(whi[d]);
    }
    if (a->ncda) {		    // stop output
	waveOutReset(hwo[d]);
	for (b = 0; b < a->segswp; b++) {
	    waveOutUnprepareHeader(hwo[d], who[d][b], sizeof(WAVEHDR));
	    free_null(who[d][b]);
	}
	free_null(who[d]);
    }
}

/* _ar_os_bind - bind WIN functions, return number of devices */

SINT4
_ar_os_bind(SINT4 ndt, SINT4 tnd)
{
    SINT4 nd;

    nd = _ar_win_num_dev();
    if (nd > 0) {
        _ardvt[ndt].num_dev = _ar_win_num_dev;
	_ardvt[ndt].dev_name = _ar_win_dev_name;
	_ardvt[ndt].io_stop = _ar_win_io_stop;
	_ardvt[ndt].close = _ar_win_close;
	_ardvt[ndt].open = _ar_win_open;
	_ardvt[ndt].io_prepare = _ar_win_io_prepare;
	_ardvt[ndt].io_start = _ar_win_io_start;
	_ardvt[ndt].xfer_seg = _ar_win_xfer_seg;
	_ardvt[ndt].chk_seg = _ar_win_chk_seg;
	_ardvt[ndt].list_rates = _ar_win_list_rates;
	dio = tnd;
    }
    if (check_registry()) {
        read_rc();
    }

    return (nd);
}

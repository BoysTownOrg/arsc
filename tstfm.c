/* tstfm.c - test soundcard streaming output */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "arsclib.h"

#define ESC  27

int chk_msg();
void cue(int);

static double rate = 44100;	// sampling rate
static double oct = 0;
static int dvid = 0;
static int last_seg = 0;
static int setoff = 0;
static int stop = 0;
static int ncue = 31;
static int nsmp = 2500;
static int nseg = 2;
static SINT4 *stm[2];

static void
tone_seg(SINT4 s)
{
    double a, c, d, fs, ss, dp, r, o, t, fr, mr = 0.05;
    int i, b, n;
    static double tpi = 2 * M_PI;
    static double ph = 0;
    static double rd = 0.01; // ramp duration (s)

    fs = pow(2, 31) - 1;    // full-scale amplitude
    ss = fs / 10;           // steady-state amplitude
    if (!last_seg && setoff) {
	last_seg = s;
	setoff = 0;
    }
    d = tpi / rate;
    c = d * nsmp;
    t = c * s;
    b = s % nseg;
    n = (int) floor(rd * rate + 0.5);
    r = M_PI / n;
    for (i = 0; i < nsmp; i++) {
	// calculate instantaneous frequency
	oct = sin(mr * t);
	fr = 1000 * pow(2, oct);
	// increment phase
	dp = fr * d;
	ph += dp;
	// apply on/off ramps
	if (s == 0 || s == last_seg) {
	    if (i < n) {
		o = s ? 1 : -1;
		a = ss * (1 + o * cos(i * r)) / 2;
	    } else {
		a = s ? 0 : ss;
	    }
	} else if (last_seg && s > last_seg) {
	    a = 0;
	} else {
	    a = ss;
	}
	// fill output buffer
	stm[b][i] = (SINT4) floor(a * sin(ph) + 0.5);
	// increment time-step
	t += d;
    }
    ph = fmod(ph, tpi);
}

// tst - check error code

static void
tst(int e)
{
    char msg[ARSC_MSGLEN];
    FILE *fp;

    if (e) { // if error, write message to file & console
	fp = fopen("arsc_tst.log", "wt");
	ar_err_msg(e, msg, ARSC_MSGLEN);
	fprintf(fp, "error code %d: %s\n", e, msg);
	fclose(fp);
	fprintf(stderr, "error code %d: %s\n", e, msg);
	exit(1);
    }
}

// test_tone - output a test tone

static void
test_tone(double f, double t)	// tone frequency and duration
{
    int c;
    SINT4 siz[2], fmt[2];
    void *out[4];
    static int nchn = 2;
    static int nswp = 0;	// 0 for streaming

    // set up i/o

    tst(ar_out_open(dvid, rate, nchn));

    // set up stimulus

    rate = ar_get_rate(dvid);
    stm[0] = (SINT4 *) calloc(nsmp, sizeof(SINT4));
    stm[1] = (SINT4 *) calloc(nsmp, sizeof(SINT4));
    out[0] = stm[0]; out[1] = stm[0];  // stm0 on left & right
    out[2] = stm[1]; out[3] = stm[1];  // stm1 on left & right
    siz[0] = siz[1] = nsmp;

    // perform i/o

    fmt[0] = 4;	                        // number of bytes per sample
    fmt[1] = 0;	                        // samples non-interleaved
    tst(ar_set_fmt(dvid, fmt));         // specify data format
    tst(ar_set_xfer(dvid, NULL, tone_seg)); // specify output callback
    tst(ar_out_prepare(dvid, out, siz, nseg, nswp)); // prepare
    tst(ar_io_start(dvid));
    while (!stop) {
	c = (int) floor((1 + oct) * ncue / 2);
	cue(c);
	chk_msg();
    };
    tst(ar_io_close(dvid));

    // clean up stimulus

    free(stm[0]);
    free(stm[1]);
}

/***************************************************************************/

#ifdef WINGUI

#include <windows.h>
#include <mmsystem.h>

static int xwin = 490;
static HANDLE  hInst = 0;
static HWND    hWnd = 0;

void
rectangle(int x1, int y1, int x2, int y2, int r, int g, int b)
{
    HBRUSH  brsh;
    HDC     dc;
    RECT    rect;

    rect.left = x1;
    rect.top = y1;
    rect.right = x2;
    rect.bottom = y2;

    brsh = CreateSolidBrush(RGB(r, g, b));
    dc = GetDC(hWnd);
    FillRect(dc, &rect, brsh);
    ReleaseDC(hWnd, dc);
    DeleteObject(brsh);
    InvalidateRect(hWnd, &rect, FALSE);
    UpdateWindow(hWnd);
}

void
cue(int s)
{
    int i, x, y, w, r, g, b;
    static int ss = -1;

    if (s == ss) {
	return;
    }
    ss = s;
    // visual cue
    for (i = (s - 1); i <= (s + 1); i++) {
	if (i < 0 || i >= ncue)
	    continue;
	w = xwin / (ncue + 1);
	x = w * i + w / 2;
	y = (w * 3) / 4;
	w = xwin / (ncue + 1);
	r = 250;
	g = r;
	b = (s == i) ? 100 : r;
        rectangle(x, y, x + w, y + w, r, g, b);
    }
}
 
/*
 * WndProc() - Processes Windows messages
 */
LRESULT CALLBACK
wind_proc(HWND hWindow, UINT message, WPARAM wParam, SINT4 lParam)
{
    int seg;
    PAINTSTRUCT   ps;

    switch (message) {

    case WM_ACTIVATE:
        if (!GetSystemMetrics(SM_MOUSEPRESENT)) {
            ShowCursor(wParam);
        }
        if (wParam && (!HIWORD(lParam))) {
            SetFocus(hWindow);
	}
        break;

    case WM_PAINT:
        BeginPaint(hWindow, (LPPAINTSTRUCT)&ps);
        EndPaint(hWindow, (LPPAINTSTRUCT)&ps);
        break;

    case WM_ARSC:
	seg = ar_io_cur_seg(lParam);
	if (last_seg && seg > last_seg) {
	    stop++;
	}
        break ;

    case WM_LBUTTONDOWN:
        break ;

    case WM_RBUTTONDOWN:
        break ;

    case WM_KEYDOWN:
        if (wParam == 'M')
            ar_out_seg_fill(dvid);
        if (wParam == ESC)
            setoff++;
    break ;

    case WM_CLOSE:
	setoff++;
        PostQuitMessage(0);
        break;

    default:
        return(DefWindowProc(hWindow, message, wParam, lParam));
        break;
    }
    return(0);
}

int
win_init(HINSTANCE inst)
{
    WCHAR       *cn = TEXT("arsc");
    WCHAR       title[80];
    WNDCLASS    OutClass;
    static char ver[80]= {
	"                    "
	"                    "
	"                    "
	"                    "
    };

    if (hInst == 0) {
        hInst = inst;
        OutClass.lpszClassName = cn;
        OutClass.lpfnWndProc   = wind_proc;
        OutClass.style         = 0;
        OutClass.hInstance     = hInst;
        OutClass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
        OutClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        OutClass.hbrBackground = (HBRUSH) GetStockObject(GRAY_BRUSH);
        OutClass.lpszMenuName  = NULL;
        OutClass.cbClsExtra    = 0;
        OutClass.cbWndExtra    = 0;
        if(!RegisterClass(&OutClass))
            return(FALSE); 
    }
    if (hWnd == 0) {
	strncpy(ver, ar_version(), 80);
	MultiByteToWideChar( 0,0, ver, 80, title, 80);
        hWnd = CreateWindow(cn, title, WS_OVERLAPPEDWINDOW, 
	    CW_USEDEFAULT, CW_USEDEFAULT, xwin, (xwin * 4) / (ncue + 1), 
	    NULL, NULL, hInst, NULL);
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    ar_wind((SINT4) hWnd);

    return(TRUE);
}

static int
chk_msg()
{
    int   pm;
    MSG   msg;

    pm = PeekMessage (&msg, NULL, 0, 0, PM_REMOVE);
    if (pm) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (pm);
}

int WINAPI 
WinMain (HINSTANCE hIn, HINSTANCE hPr, char *C, int S)
{
    win_init(hPr);
    test_tone(1000, 1);

    return (0);
}

#else	// WINGUI

#include <unistd.h>

#ifdef KB
int _getch(void);
int _kbhit(void);
#endif

int
chk_msg()
{
    int seg;

    seg = ar_io_cur_seg(dvid);
    if (last_seg && seg > last_seg) {
	stop++;
    }
#ifdef KB
    if (_kbhit()) {
	if (_getch() == ESC) {
	    setoff++;
        }
    }
#endif

    return (1);
}

void
cue(int s)
{
    int i;
    struct timespec delay = {0, 1234 * 1000};

    if (!stop) {
        fputc('\r', stdout);
        for (i = 0; i < s; i++)
            fputc(' ', stdout);
        fputs("||", stdout);
        for (i = 0; i < ncue - s; i++)
            fputc(' ', stdout);
	fflush(stdout);
        nanosleep(&delay, &delay);
    }
}

int
main(int ac, char **av)
{
    char name[ARSC_NAMLEN];

    if (ac > 1) {
        dvid = atoi(av[1]);
    } else {
        dvid = ar_find_dev(ARSC_PREF_NONE);
    }
    if (ar_dev_name(dvid, name, ARSC_NAMLEN)) {
        fprintf(stderr, "tstfm: no device\n");
	return (1);
    }
    fprintf(stderr, "tstfm: %s\n", name);
    test_tone(1000, 1);
    fprintf(stderr, "\ndone!\n");

    return (0);
}

#endif // WINGUI

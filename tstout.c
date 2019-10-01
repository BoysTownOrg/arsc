/* tstout.c - test soundcard output */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "arsclib.h"

void cue(int);

static char *ret_ptr = NULL;    // unused return value
static int io_dev = -1;

static void
compute_tone(double f, double a, int32_t *b, int n)
{
    double  p;	    // amplitude and phase of stimulus tone;
    double  c;	    // number of cycles per buf
    int     i;

    c = floor(f * n + 0.5);
    p = 2 * M_PI * c / n;
    for (i = 0; i < n; i++)
	b[i] = (int32_t) (a * sin(p * i) + 0.5);
}

static void
ramp_ends(int32_t *b, int n, double nr)
{
    double  s, a, p;
    int     i, j, m;

    p = M_PI / (2 * nr) ;
    m = (int) ceil(nr);
    for (i = 0; i < m; i++) {
	s = sin(p * i);
	a = s * s;
	j = n - 1 - i;
	b[i] = (int32_t) (a * b[i] + 0.5);
	b[j] = (int32_t) (a * b[j] + 0.5);
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

// test_tone - output a test tone

static void
test_tone(double f, double t)	// tone frequency and duration
{
    double a, nr;
    int d, s, p, nsmp, noff;
    int32_t sz[5], fmt[2], *sl;
    void *out[10];
    static double r = 44100;	// sampling rate
    static int nseg = 5;	// number of segments
    static int nswp = 1;	// number of sweeps
    static int nbps = 4;	// number of bytes per sample
    static int ntlv = 0;	// samples non-interleaved

    // set up i/o

    d = (io_dev < 0) ? ar_find_dev(0) : io_dev;
    tst(ar_out_open(d, r, 2));
    r = ar_get_rate(d);

    // set up stimulus

    nsmp = (int) floor(r * t + 0.5);
    noff = (nsmp * 3) / 2;
    sl = (int32_t *) calloc(nsmp * 2, sizeof(int32_t));
    a = pow(2, nbps * 8 - 1) - 1;
    compute_tone(f / r, a / 2, sl, nsmp);
    nr = r / 100;
    ramp_ends(sl, nsmp, nr);
    out[0] = NULL; out[1] = NULL; sz[0] = noff;	    // seg 1
    out[2] = sl;   out[3] = NULL; sz[1] = nsmp;	    // seg 2
    out[4] = NULL; out[5] = NULL; sz[2] = noff;	    // seg 3
    out[6] = NULL; out[7] = sl;   sz[3] = nsmp;	    // seg 4
    out[8] = NULL; out[9] = NULL; sz[4] = noff;	    // seg 5
    fmt[0] = nbps;
    fmt[1] = ntlv;

    // perform i/o

    tst(ar_set_fmt(d, fmt));
    tst(ar_out_prepare(d, out, sz, nseg, nswp));
    tst(ar_io_start(d));
    p = -1;
    do {
	s = ar_io_cur_seg(d);
	if (s > p) {
	    cue(s);
            p = s;
	}
    } while (s < nseg);

    // clean up i/o

    tst(ar_io_stop(d));
    tst(ar_io_close(d));

    // clean up stimulus

    free(sl);
}

/***************************************************************************/

#ifdef WINGUI

#include <windows.h>
#include <mmsystem.h>

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

    // visual cue
    for (i = 0; i < 5; i++) {
	x = 110 * i + 25;
	y = 40;
	w = 100;
	r = (i % 2)  ? 250 : 200;
	g = r;
	b = (s == i) ? 100 : r;
        rectangle(x, y, x + w, y + w, r, g, b);
    }
}
 
/*
 * WndProc() - Processes Windows messages
 */
LRESULT CALLBACK
wind_proc(HWND hWindow, UINT message, WPARAM wParam, int32_t lParam)
{
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

    case WM_LBUTTONDOWN:
    test_tone(1000, 0.5);
    break ;

    case WM_RBUTTONDOWN:
    test_tone(1000, 0.1);
    break ;

    case WM_KEYDOWN:
    break ;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return(DefWindowProc(hWindow, message, wParam, lParam));
        break;
    }
    return(0);
}

int
win_init(HINSTANCE inst, char *title)
{
    LPSTR       cn = "arsc";
    WNDCLASS    OutClass;

    if (hInst == 0) {
        hInst = inst;
        OutClass.lpszClassName = cn;
        OutClass.lpfnWndProc   = (WNDPROC)wind_proc;
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
        hWnd = CreateWindow(cn, (LPSTR) title, WS_OVERLAPPEDWINDOW, 
	    CW_USEDEFAULT, CW_USEDEFAULT, 600, 200, 
	    NULL, NULL, hInst, NULL);
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    return(TRUE);
}

static void
msg_loop()
{
    MSG   msg;

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

int WINAPI 
WinMain (HINSTANCE hIn, HINSTANCE hPr, char *C, int S)
{
    win_init(hPr, ar_version());
    cue(-1);
    msg_loop();

    return (0);
}

#else	// WINGUI

void
cue(int s)
{
    if (s == 1 || s == 3)
	fputs("#####", stderr);
    else
	fputs("     ", stderr);
}

int
main(int ac, char **av)
{
    char r[2] = {0};

    io_dev = (ac > 1) ? (atoi(av[1]) - 1) : -1;
    fprintf(stderr, "tstout: %d\n", io_dev + 1);
    while (r[0] < ' ') {
    	test_tone(1000, 0.5);
    	fprintf(stderr, "[Press Enter] ");
    	ret_ptr = fgets(r, 2, stdin);
    }

    return (0);
}

#endif // WINGUI

/* kb.c */
 
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#ifdef GRX
#include <vga.h>
#include <vgamouse.h>
#endif

#define TRUE    1
#define FALSE   0
/*
 * keyboard stuff 
 */
static struct termios kbd_setup;
static struct termios kbd_reset;
static int    kbd_initted = FALSE;
static int    kbd_isatty;
static int    kbd_lastchr;
static int    kbd_filedsc;
static int    opt_act = 0;
static enum { normal, test_key, wait_key, unknown } kbd_mode = unknown;

void
kbd_restore(void)
{
    if(kbd_initted && kbd_isatty && (kbd_mode != normal)) {
        tcsetattr(kbd_filedsc, opt_act, &kbd_reset);
        kbd_mode = normal;
    }
}

void
kbd_init(void)
{
    if(!kbd_initted) {
        kbd_initted = TRUE;
        kbd_lastchr = EOF;
        kbd_filedsc = fileno(stdin);
        kbd_isatty  = isatty(kbd_filedsc);	
        if(kbd_isatty) {
            tcgetattr(kbd_filedsc, &kbd_setup);
            tcgetattr(kbd_filedsc, &kbd_reset);
            kbd_setup.c_lflag &= ~(ICANON | ECHO );
            kbd_setup.c_iflag &= ~(INLCR  | ICRNL);
            atexit(kbd_restore);
            kbd_mode = normal;
        }
    }
}

int
_kbhit(void)
{
    unsigned char buf[4];
    if(!kbd_initted) {
        kbd_init();
    }
    if(!kbd_isatty) {
        return(TRUE);
    }
    if(kbd_lastchr != EOF) {
        return(TRUE);
    }
    if(kbd_mode != test_key) {
        kbd_setup.c_cc[VMIN]  = 0;
        kbd_setup.c_cc[VTIME] = 0;
        if(tcsetattr(kbd_filedsc, opt_act, &kbd_setup) == EOF) return(FALSE);
            kbd_mode = test_key;
        }
    if(read(kbd_filedsc,buf,1) > 0) {
        kbd_lastchr = buf[0];
        return(TRUE);
    }
   return(FALSE);
}

int 
_getch(void)
{
    unsigned char buf[4];
    if(!kbd_initted) {
        kbd_init();
     }
    if(!kbd_isatty) {
    return(getc(stdin));
}
    if(kbd_lastchr != EOF) {
        buf[0] = kbd_lastchr;
        kbd_lastchr = EOF;
        return(buf[0]);
    }
    if(kbd_mode != wait_key) {
        kbd_setup.c_cc[VMIN]  = 1;
        kbd_setup.c_cc[VTIME] = 0;
        if(tcsetattr(kbd_filedsc, opt_act, &kbd_setup) == EOF)
	    return(EOF);
        kbd_mode = wait_key;
    }
    if(read(kbd_filedsc,buf,1) > 0) {
        return(buf[0]);
    }
    return(EOF);
}

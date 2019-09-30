/* arsc_dll.c


#include <windows.h>

/**************************************************************************/

#ifdef WIN32

/* LibMain - entry point for the DLL */

int _stdcall
LibMain(void *hinstDll, unsigned long dwReason, void *reserved)
{
	return(1);
}

#endif

/**************************************************************************/
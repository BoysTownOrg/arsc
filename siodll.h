/* siodll.h */

#ifdef WIN32

#define FUNC(type) __declspec(dllexport) type _stdcall

#else // WIN32

#define FUNC(type) type

#endif // WIN32

FUNC(double) ssio_set_att_out(double);
FUNC(double) ssio_set_rate(double);
FUNC(int) ssio_open();
FUNC(int) ssio_set_device(int);
FUNC(void) ssio_close();
FUNC(void) ssio_get_device(char *);
FUNC(void) ssio_get_info(char *);
FUNC(void) ssio_get_vfs(double *,double *);
FUNC(void) ssio_io(int,int);
FUNC(void) ssio_set_io(float *,float *,float *,float *,int);
FUNC(void) ssio_set_vfs(double *,double *);

/* sio.h */

# define SIO_GET_LATENCY    9999
# define SIO_PREF_SYNC	    -1
# define SIO_PREF_ASIO	    -2

char   *sio_version(void);
double  sio_set_att_in(double);
double  sio_set_att_out(double);
double  sio_set_rate(double);
int 	sio_get_device(char *);
int     sio_get_info(char *);
int     sio_get_nioch(int *, int *);
int     sio_get_vfs(double *, double *);
int     sio_open(void);
int     sio_set_device(int);
int     sio_set_latency(int);
void    sio_close(void);
void    sio_io_chk(void (*)(int *));
void    sio_io(int, int, int, int);
void    sio_set_average(float **, int *);
void    sio_set_channel_offset(int, int);
void    sio_set_escape(int (*)());
void    sio_set_input(int, int, int *, float **);
void    sio_set_output(int, int, int *, float **);
void    sio_set_reject(float *, int *, int *, int *);
void    sio_set_size(int, int, double);
void    sio_set_vfs(double *, double *);
void    sio_stop(int);

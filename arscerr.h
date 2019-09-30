// arscerr.h - ARSC error messages

typedef struct {
    int code;
    char *mesg;
} ARERR;

static ARERR _arerr[] ={
    { -1, "unknown error code"},
    {  0, "no errors"},
    {  1, "device identifier out of range"},
    {  2, "device not open"},
    {101, "io_open - MMSYSERR = unspecified error"},
    {102, "io_open - MMSYSERR = device ID out of range"},
    {103, "io_open - MMSYSERR = driver enable failed"},
    {104, "io_open - MMSYSERR = device already allocated"},
    {105, "io_open - MMSYSERR = unknown error"},
    {106, "io_open - MMSYSERR = no device driver present"},
    {107, "io_open - MMSYSERR = memory allocation error"},
    {108, "io_open - WAVERR = unsupported wave format"},
    {109, "io_open - WIND unknown device identifier"},
    {110, "io_open - WIND open error"},
    {120, "io_open - ASIO open error"},
    {140, "io_open - ALSA open error"},
    {160, "io_open - OS X open error"},
    {201, "io_prepare - device not opened"},
    {202, "io_prepare - unsupported data format conversion"},
    {203, "io_prepare - size matters"},
    {204, "io_prepare - too many segments per sweep"},
    {205, "io_prepare - too many sweeps"},
    {206, "io_prepare - failed low level prepare"},
    {301, "io_start - device not prepared"},
    {401, "io_set_fmt - NULL format"},
    {402, "io_set_fmt - unsupported bytes per sample"},
    {403, "io_set_fmt - unsupported interleave"},
    {404, "io_set_fmt - unsupported format conversion"},
};


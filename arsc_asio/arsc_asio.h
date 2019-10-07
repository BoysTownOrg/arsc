#ifndef ARSC_ARSC_ASIO_ARSC_ASIO
#define ARSC_ARSC_ASIO_ARSC_ASIO

#include "../arscdev.h"
extern int32_t (*ar_asio_devices)();
extern char* (*ar_asio_device_name)(int32_t);
extern void (*ar_asio_io_stop)(int32_t);

#endif

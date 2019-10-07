#ifndef ARSC_ARSC_ASIO_ARSC_ASIO
#define ARSC_ARSC_ASIO_ARSC_ASIO

#include "../arscdev.h"
extern int32_t (*ar_asio_devices)();
extern char* (*ar_asio_device_name)(int32_t);
extern void (*ar_asio_io_stop)(int32_t);
extern void (*ar_asio_close)(int32_t);
extern int32_t(*ar_asio_open)(int32_t);
extern int32_t(*ar_asio_io_prepare)(int32_t);
extern void (*ar_asio_io_start)(int32_t);
extern int32_t(*ar_asio_transfer_segment)(int32_t, int32_t);
extern int32_t(*ar_asio_check_segment)(int32_t, int32_t);
extern int32_t(*ar_asio_latency)(int32_t, int32_t);
extern int32_t(*ar_asio_list_rates)(int32_t);
int32_t _ar_asio_open(int32_t);

#endif

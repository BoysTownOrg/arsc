#ifndef ARSC_CHECK_TESTS_ARSC_ASIO_TESTS_COMMON_H_
#define ARSC_CHECK_TESTS_ARSC_ASIO_TESTS_COMMON_H_

#define ASIO
#include <arsc_asio.h>
#include <check.h>

ARDEV** devices_(int32_t device);
ARDEV* devices(int32_t device);

#endif
#ifndef ARSC_CHECK_TESTS_ARSC_ASIO_TESTS_COMMON_H_
#define ARSC_CHECK_TESTS_ARSC_ASIO_TESTS_COMMON_H_

#define ASIO
#include <arsc_asio.h>
#include <check.h>

#define ASSERT_EQUAL_ANY(a, b) ck_assert(a == b)

ARDEV** devices_(int32_t device);
ARDEV* devices(int32_t device);
void free_device(int device);
void allocate_device(int device);
int32_t* integer_array_at(int32_t* a, int i);
int32_t read_integer_array_at(int32_t* a, int i);
void assign_integer_array(int32_t* a, int i, int32_t what);
void set_device_desired_output_channels(int i, int32_t c);

#endif
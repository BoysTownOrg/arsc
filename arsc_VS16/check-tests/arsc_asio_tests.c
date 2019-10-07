#include "arsc_asio_tests.h"
#define ASIO
#include <arsc_asio.h>

static int32_t(*devices_restore)();
static char* (*device_name_restore)(int32_t);
static void (*io_stop_restore)(int32_t);

static int32_t devices;
static char* device_name;

static int32_t devices_stub() {
	return devices;
}

static char* device_name_stub(int32_t n) {
	return device_name;
}

static void io_stop_stub(int32_t n) {
	n;
}

static void setup(void) {
	devices_restore = ar_asio_devices;
	device_name_restore = ar_asio_device_name;
	io_stop_restore = ar_asio_io_stop;
	ar_asio_devices = devices_stub;
	ar_asio_device_name = device_name_stub;
	ar_asio_io_stop = io_stop_stub;
}

static void teardown(void) {
	ar_asio_devices = devices_restore;
	ar_asio_device_name = device_name_restore;
	ar_asio_io_stop = io_stop_restore;
}

static int32_t bind_with_device_type(int32_t device_type) {
	return _ar_asio_bind(device_type, 0);
}

static int32_t bind(void) {
	return bind_with_device_type(0);
}

static void set_devices(int32_t n) {
	devices = n;
}

static void set_nonzero_devices(void) {
	set_devices(1);
}

static int32_t(*bound_devices_impl(int32_t device_type))() {
	return _ardvt[device_type].num_dev;
}

static char*(*bound_device_name_impl(int32_t device_type))(int32_t) {
	return _ardvt[device_type].dev_name;
}

static void (*bound_io_stop_impl(int32_t device_type))(int32_t) {
	return _ardvt[device_type].io_stop;
}

static void bind_nonzero_devices_with_device_type(int32_t device_type) {
	set_nonzero_devices();
	bind_with_device_type(device_type);
}

#define ASSERT_EQUAL_INT(a, b) ck_assert_int_eq(a, b)
#define ASSERT_EQUAL_ANY(a, b) ck_assert(a == b)

#define ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, stub, impl)\
	bind_nonzero_devices_with_device_type(device_type);\
	ASSERT_EQUAL_ANY(stub, impl(device_type))

#define ASSERT_BIND_ASSIGNS_DEVICES_IMPL_WHEN_NONZERO_DEVICES(device_type)\
	ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, devices_stub, bound_devices_impl)

#define ASSERT_BIND_ASSIGNS_DEVICE_NAME_IMPL_WHEN_NONZERO_DEVICES(device_type)\
	ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, device_name_stub, bound_device_name_impl)

#define ASSERT_BIND_ASSIGNS_IO_STOP_IMPL_WHEN_NONZERO_DEVICES(device_type)\
	ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, io_stop_stub, bound_io_stop_impl)

START_TEST(bind_returns_number_of_devices) {
	set_devices(3);
	ASSERT_EQUAL_INT(3, bind());
}

START_TEST(bind_assigns_devices_impl_to_device_type_zero_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_DEVICES_IMPL_WHEN_NONZERO_DEVICES(0);
}

START_TEST(bind_assigns_devices_impl_to_device_type_one_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_DEVICES_IMPL_WHEN_NONZERO_DEVICES(1);
}

START_TEST(bind_assigns_device_name_impl_to_device_type_zero_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_DEVICE_NAME_IMPL_WHEN_NONZERO_DEVICES(0);
}

START_TEST(bind_assigns_device_name_impl_to_device_type_one_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_DEVICE_NAME_IMPL_WHEN_NONZERO_DEVICES(1);
}

START_TEST(bind_assigns_io_stop_impl_to_device_type_zero_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_IO_STOP_IMPL_WHEN_NONZERO_DEVICES(0);
}

static void add_test(TCase* test_case, const TTest* test) {
	tcase_add_test(test_case, test);
}

Suite* arsc_asio_test_suite() {
	Suite* suite = suite_create("arsc_asio");
	TCase* test_case = tcase_create("idk");
	tcase_add_checked_fixture(test_case, setup, teardown);
	add_test(test_case, bind_returns_number_of_devices);
	add_test(test_case, bind_assigns_devices_impl_to_device_type_zero_when_nonzero_devices);
	add_test(test_case, bind_assigns_devices_impl_to_device_type_one_when_nonzero_devices);
	add_test(test_case, bind_assigns_device_name_impl_to_device_type_zero_when_nonzero_devices);
	add_test(test_case, bind_assigns_device_name_impl_to_device_type_one_when_nonzero_devices);
	add_test(test_case, bind_assigns_io_stop_impl_to_device_type_zero_when_nonzero_devices);
	suite_add_tcase(suite, test_case);
	return suite;
}
#include "arsc_asio_tests.h"
#define ASIO
#include <arsc_asio.h>

static int32_t(*ar_asio_devices_restore)();

static int32_t devices;

static int32_t ar_asio_devices_stub() {
	return devices;
}

static void setup(void) {
	ar_asio_devices_restore = ar_asio_devices;
	ar_asio_devices = ar_asio_devices_stub;
}

static void teardown(void) {
	ar_asio_devices = ar_asio_devices_restore;
}

static int32_t ar_asio_bind_with_device_type(int32_t device_type) {
	return _ar_asio_bind(device_type, 0);
}

static int32_t ar_asio_bind_(void) {
	return ar_asio_bind_with_device_type(0);
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

#define ASSERT_EQUAL_INT(a, b) ck_assert_int_eq(a, b)
#define ASSERT_EQUAL_ANY(a, b) ck_assert(a == b)

static void assert_asio_bind_assigns_devices_impl_when_nonzero_devices(int32_t device_type) {
	set_nonzero_devices();
	ar_asio_bind_with_device_type(device_type);
	ASSERT_EQUAL_ANY(ar_asio_devices_stub, bound_devices_impl(device_type));
}

START_TEST(ar_asio_bind_returns_number_of_devices) {
	set_devices(3);
	ASSERT_EQUAL_INT(3, ar_asio_bind_());
}

START_TEST(ar_asio_bind_assigns_devices_impl_to_device_type_zero_when_nonzero_devices) {
	assert_asio_bind_assigns_devices_impl_when_nonzero_devices(0);
}

START_TEST(ar_asio_bind_assigns_devices_impl_to_device_type_one_when_nonzero_devices) {
	assert_asio_bind_assigns_devices_impl_when_nonzero_devices(1);
}

static void add_test(TCase* test_case, const TTest* test) {
	tcase_add_test(test_case, test);
}

Suite* arsc_asio_test_suite() {
	Suite* suite = suite_create("arsc_asio");
	TCase* test_case = tcase_create("idk");
	tcase_add_checked_fixture(test_case, setup, teardown);
	add_test(test_case, ar_asio_bind_returns_number_of_devices);
	add_test(test_case, ar_asio_bind_assigns_devices_impl_to_device_type_zero_when_nonzero_devices);
	add_test(test_case, ar_asio_bind_assigns_devices_impl_to_device_type_one_when_nonzero_devices);
	suite_add_tcase(suite, test_case);
	return suite;
}
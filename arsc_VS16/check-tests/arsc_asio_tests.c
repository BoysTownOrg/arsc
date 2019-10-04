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

static int32_t ar_asio_bind_() {
	return _ar_asio_bind(0, 0);
}

static void set_devices(int32_t n) {
	devices = n;
}

static void set_nonzero_devices() {
	set_devices(1);
}

#define ASSERT_EQUAL_INT(a, b) ck_assert_int_eq(a, b)
#define ASSERT_EQUAL_ANY(a, b) ck_assert(a == b)

START_TEST(ar_asio_bind_returns_number_of_devices) {
	set_devices(3);
	ASSERT_EQUAL_INT(3, ar_asio_bind_());
}

START_TEST(ar_asio_bind_assigns_devices_impl_when_nonzero_devices) {
	set_nonzero_devices();
	ar_asio_bind_();
	ASSERT_EQUAL_ANY(ar_asio_devices_stub, _ardvt[0].num_dev);
}

static void add_test(TCase* test_case, const TTest* test) {
	tcase_add_test(test_case, test);
}

Suite* arsc_asio_test_suite() {
	Suite* suite = suite_create("arsc_asio");
	TCase* test_case = tcase_create("idk");
	tcase_add_checked_fixture(test_case, setup, teardown);
	add_test(test_case, ar_asio_bind_returns_number_of_devices);
	add_test(test_case, ar_asio_bind_assigns_devices_impl_when_nonzero_devices);
	suite_add_tcase(suite, test_case);
	return suite;
}
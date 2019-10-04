#include "arsc_asio_tests.h"
#define ASIO
#include <arsc_asio.h>

static int32_t _ar_asio_num_dev_stub() {
	return 3;
}

START_TEST(ar_asio_bind_returns_number_of_devices) {
	int32_t (*restore)() = _ar_asio_num_dev;
	_ar_asio_num_dev = _ar_asio_num_dev_stub;
	ck_assert_int_eq(3, _ar_asio_bind(0, 0));
	_ar_asio_num_dev = restore;
}
END_TEST

Suite* arsc_asio_test_suite() {
	Suite* suite = suite_create("arsc_asio");
	TCase* test_case = tcase_create("idk");
	tcase_add_test(test_case, ar_asio_bind_returns_number_of_devices);
	suite_add_tcase(suite, test_case);
	return suite;
}
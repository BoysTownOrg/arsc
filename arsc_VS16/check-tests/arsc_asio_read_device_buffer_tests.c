#include "arsc_asio_read_device_buffer_tests.h"
#include "arsc_asio_tests_common.h"

static void setup(void) {

}

static void teardown(void) {

}

START_TEST(tbd) {
	ASSERT_EQUAL_ANY(1, 2);
}

Suite* arsc_asio_read_device_buffer_suite() {
	Suite* suite = suite_create("arsc_asio_read_device_buffer");
	TCase* test_case = tcase_create("read_device_buffer");
	tcase_add_checked_fixture(test_case, setup, teardown);
	add_test(test_case, tbd);
	suite_add_tcase(suite, test_case);
	return suite;
}

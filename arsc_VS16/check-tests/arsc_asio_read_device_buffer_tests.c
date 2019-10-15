#include "arsc_asio_read_device_buffer_tests.h"
#include "arsc_asio_tests_common.h"

static void setup(void) {

}

static void teardown(void) {

}

START_TEST(tbd) {
	int32_t buffer[3];
	buffer[0] = 1;
	buffer[1] = 2;
	buffer[2] = 3;
	int32_t data[3];
	TResponseData response;
	response.Magic = 0xBEEF;
	response.channel = 0;
	response.Index = 0;
	response.data = data;
	ar_asio_read_device_buffer(buffer, 3, &response);
	ASSERT_EQUAL_ANY(1, data[0]);
	ASSERT_EQUAL_ANY(2, data[1]);
	ASSERT_EQUAL_ANY(3, data[2]);
}

Suite* arsc_asio_read_device_buffer_suite() {
	Suite* suite = suite_create("arsc_asio_read_device_buffer");
	TCase* test_case = tcase_create("read_device_buffer");
	tcase_add_checked_fixture(test_case, setup, teardown);
	add_test(test_case, tbd);
	suite_add_tcase(suite, test_case);
	return suite;
}

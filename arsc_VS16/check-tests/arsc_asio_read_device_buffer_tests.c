#include "arsc_asio_read_device_buffer_tests.h"
#include "arsc_asio_tests_common.h"

static void setup(void) {
	allocate_device(0);
	ar_current_device = devices(0);
}

static void teardown(void) {
	free_device(0);
}

START_TEST(tbd) {
	set_device_desired_input_channels(0, 1);
	assign_device_segments(0, 1);

	int32_t buffer[3];
	buffer[0] = 1;
	buffer[1] = 2;
	buffer[2] = 3;
	int32_t data[3];
	TResponseData response;
	response.Magic = 0xBEEF;
	response.channel = 0;
	response.Index = 0;
	response.size = sizeof data / sizeof data[0];
	response.data = data;
	response.LatencyReached = 1;
	response.segment = 0;
	ar_asio_read_device_buffer(buffer, 3, &response);
	ASSERT_EQUAL_ANY(1, data[0]);
	ASSERT_EQUAL_ANY(2, data[1]);
	ASSERT_EQUAL_ANY(3, data[2]);
}

START_TEST(tbd2) {
	set_device_desired_input_channels(0, 1);
	assign_device_segments(0, 2);

	int32_t buffer[7];
	buffer[0] = 1;
	buffer[1] = 2;
	buffer[2] = 3;
	buffer[3] = 4;
	buffer[4] = 5;
	buffer[5] = 6;
	buffer[6] = 7;
	int32_t data1[3];
	int32_t data2[4];
	TResponseData response[2];
	
	response[0].Magic = 0xBEEF;
	response[0].channel = 0;
	response[0].Index = 0;
	response[0].size = sizeof data1 / sizeof data1[0];
	response[0].data = data1;
	response[0].LatencyReached = 1;
	response[0].segment = 0;

	response[1].Magic = 0xBEEF;
	response[1].channel = 0;
	response[1].Index = 0;
	response[1].size = sizeof data2 / sizeof data2[0];
	response[1].data = data2;
	response[1].LatencyReached = 1;
	response[1].segment = 1;
	
	ar_asio_read_device_buffer(buffer, 7, response);

	ASSERT_EQUAL_ANY(1, data1[0]);
	ASSERT_EQUAL_ANY(2, data1[1]);
	ASSERT_EQUAL_ANY(3, data1[2]);
	ASSERT_EQUAL_ANY(4, data2[0]);
	ASSERT_EQUAL_ANY(5, data2[1]);
	ASSERT_EQUAL_ANY(6, data2[2]);
	ASSERT_EQUAL_ANY(7, data2[3]);
}

Suite* arsc_asio_read_device_buffer_suite() {
	Suite* suite = suite_create("arsc_asio_read_device_buffer");
	TCase* test_case = tcase_create("read_device_buffer");
	tcase_add_checked_fixture(test_case, setup, teardown);
	add_test(test_case, tbd);
	add_test(test_case, tbd2);
	suite_add_tcase(suite, test_case);
	return suite;
}

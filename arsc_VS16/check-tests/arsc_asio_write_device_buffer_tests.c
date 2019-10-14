#include "arsc_asio_tests_common.h"
#include "arsc_asio_write_device_buffer_tests.h"
#include <stdlib.h>

enum {
	sufficiently_large = 100
};

static int32_t device_buffer[sufficiently_large];
static ArAsioSegment segments[2];
static int32_t stimuli[2][sufficiently_large];

static ArAsioSegment initialized_segment() {
	ArAsioSegment s;
	s.Magic = 0xBEEF;
	s.Index = 0;
	s.channel = 0;
	s.segment = 0;
	s.OutputDone = 0;
	return s;
}

static ArAsioSegment* segment_at(ArAsioSegment* s, int i) {
	return s + i;
}

static void initialize_segment(ArAsioSegment* s, int i) {
	*segment_at(s, i) = initialized_segment();
}

static void assign_segment_data(ArAsioSegment* s, int i, int32_t* data) {
	segment_at(s, i)->data = data;
}

static void assign_segment_size(ArAsioSegment* s, int i, int32_t size) {
	segment_at(s, i)->size = size;
}

static void assign_segment_segment(ArAsioSegment* s, int i, int32_t segment) {
	segment_at(s, i)->segment = segment;
}

static void assign_segment_index(ArAsioSegment* s, int i, int32_t index) {
	segment_at(s, i)->Index = index;
}

static void setup_write_device_buffer(void) {
	for (int i = 0; i < 2; ++i) {
		initialize_segment(segments, i);
		assign_segment_data(segments, i, stimuli[i]);
		assign_segment_size(segments, i, sizeof stimuli[i] / sizeof stimuli[i][0]);
		assign_segment_segment(segments, i, i);
	}
	allocate_device(0);
	assign_device_segments(0, 1);
	set_device_desired_output_channels(0, 1);
	ar_current_device = devices(0);
	global_asio_segment = segments;
}

static void teardown_write_device_buffer(void) {
	memset(device_buffer, 0, sizeof device_buffer);
	memset(segments, 0, sizeof segments);
	memset(stimuli, 0, sizeof stimuli);
	free_device(0);
}

static void assign_first_stimulus(int i, int32_t what) {
	assign_integer_array(stimuli[0], i, what);
}

static void assign_second_stimulus(int i, int32_t what) {
	assign_integer_array(stimuli[1], i, what);
}

static void write_device_buffer(int32_t n, ArAsioSegment* s) {
	ar_asio_write_device_buffer(device_buffer, n, s);
}

static void write_device_buffer_(int32_t n) {
	ar_asio_write_device_buffer(device_buffer, n, segments);
}

#define ASSERT_INTEGER_ARRAY_AT_EQUALS(a, b, c)\
	ASSERT_EQUAL_ANY(a, read_integer_array_at(b, c))

#define ASSERT_DEVICE_BUFFER_AT_EQUALS(a, b)\
	ASSERT_INTEGER_ARRAY_AT_EQUALS(b, device_buffer, a)

START_TEST(write_device_buffer_one_segment) {
	assign_first_stimulus(0, 5);
	assign_first_stimulus(1, 6);
	assign_first_stimulus(2, 7);
	write_device_buffer_(3);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 5);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 6);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 7);
}

START_TEST(write_device_buffer_one_segment_offset) {
	assign_segment_index(segments, 0, 1);

	assign_first_stimulus(1, 5);
	assign_first_stimulus(2, 6);
	assign_first_stimulus(3, 7);
	write_device_buffer_(3);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 5);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 6);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 7);
}

START_TEST(write_device_buffer_two_segments) {
	assign_device_segments(0, 2);

	assign_segment_size(segments, 0, 3);
	assign_first_stimulus(0, 11);
	assign_first_stimulus(1, 12);
	assign_first_stimulus(2, 13);

	assign_segment_size(segments, 1, 4);
	assign_second_stimulus(0, 14);
	assign_second_stimulus(1, 15);
	assign_second_stimulus(2, 16);
	assign_second_stimulus(3, 17);

	write_device_buffer_(7);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 12);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 13);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(3, 14);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(4, 15);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(5, 16);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(6, 17);
}

START_TEST(write_device_buffer_wrap_segments) {
	assign_device_segments(0, 2);

	assign_segment_size(segments, 0, 3);
	assign_first_stimulus(0, 11);
	assign_first_stimulus(1, 12);
	assign_first_stimulus(2, 13);

	assign_segment_size(segments, 1, 4);
	assign_second_stimulus(0, 14);
	assign_second_stimulus(1, 15);
	assign_second_stimulus(2, 16);
	assign_second_stimulus(3, 17);

	write_device_buffer(7, segments + 1);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 14);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 15);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 16);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(3, 17);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(4, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(5, 12);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(6, 13);
}

Suite* arsc_asio_write_device_buffer_suite() {
	Suite* suite = suite_create("arsc_asio_write_device_buffer");
	TCase* test_case = tcase_create("write_device_buffer");
	tcase_add_checked_fixture(test_case, setup_write_device_buffer, teardown_write_device_buffer);
	add_test(test_case, write_device_buffer_one_segment);
	add_test(test_case, write_device_buffer_one_segment_offset);
	add_test(test_case, write_device_buffer_two_segments);
	add_test(test_case, write_device_buffer_wrap_segments);
	suite_add_tcase(suite, test_case);
	return suite;
}

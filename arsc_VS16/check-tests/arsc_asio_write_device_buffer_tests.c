#include "arsc_asio_tests_common.h"
#include "arsc_asio_write_device_buffer_tests.h"
#include <stdlib.h>

enum {
	sufficiently_large = 100,
	buffer_count = sufficiently_large
};

static int32_t device_buffer[sufficiently_large];
static ArAsioChannelBuffer channel_buffers[buffer_count];
static int32_t audio_buffers[buffer_count][sufficiently_large];

static ArAsioChannelBuffer initialized_channel_buffer() {
	ArAsioChannelBuffer s;
	s.Index = 0;
	s.channel = 0;
	s.segment = 0;
	return s;
}

static ArAsioChannelBuffer* channel_buffer_at(ArAsioChannelBuffer* s, int i) {
	return s + i;
}

static void initialize_channel_buffer(ArAsioChannelBuffer* s, int i) {
	*channel_buffer_at(s, i) = initialized_channel_buffer();
}

static void assign_channel_buffer_data(ArAsioChannelBuffer* s, int i, int32_t* data) {
	channel_buffer_at(s, i)->data = data;
}

static void assign_channel_buffer_size(ArAsioChannelBuffer* s, int i, int32_t size) {
	channel_buffer_at(s, i)->size = size;
}

static void assign_channel_buffer_segment(ArAsioChannelBuffer* s, int i, int32_t segment) {
	channel_buffer_at(s, i)->segment = segment;
}

static void assign_channel_buffer_index(ArAsioChannelBuffer* s, int i, int32_t index) {
	channel_buffer_at(s, i)->Index = index;
}

static void setup_write_device_buffer(void) {
	for (int i = 0; i < buffer_count; ++i) {
		initialize_channel_buffer(channel_buffers, i);
		assign_channel_buffer_data(channel_buffers, i, audio_buffers[i]);
		assign_channel_buffer_size(channel_buffers, i, sizeof audio_buffers[i] / sizeof audio_buffers[i][0]);
		assign_channel_buffer_segment(channel_buffers, i, i);
	}
	allocate_device(0);
	assign_device_segments(0, 1);
	set_device_desired_output_channels(0, 1);
	set_device_desired_input_channels(0, 0);
	ar_current_device = devices(0);
	global_asio_channel_buffers = channel_buffers;
}

static void teardown_write_device_buffer(void) {
	memset(device_buffer, 0, sizeof device_buffer);
	memset(channel_buffers, 0, sizeof channel_buffers);
	memset(audio_buffers, 0, sizeof audio_buffers);
	free_device(0);
}

static void assign_audio_buffer(int i, int j, int32_t what) {
	assign_integer_array(audio_buffers[i], j, what);
}

static void assign_first_audio_buffer(int i, int32_t what) {
	assign_audio_buffer(0, i, what);
}

static void assign_second_audio_buffer(int i, int32_t what) {
	assign_audio_buffer(1, i, what);
}

static void write_device_buffer(int32_t n, ArAsioChannelBuffer* s) {
	ar_asio_write_device_buffer(device_buffer, n, s);
}

static void write_device_buffer_(int32_t n) {
	ar_asio_write_device_buffer(device_buffer, n, channel_buffers);
}

#define ASSERT_INTEGER_ARRAY_AT_EQUALS(a, b, c)\
	ASSERT_EQUAL_ANY(a, read_integer_array_at(b, c))

#define ASSERT_DEVICE_BUFFER_AT_EQUALS(a, b)\
	ASSERT_INTEGER_ARRAY_AT_EQUALS(b, device_buffer, a)

START_TEST(write_device_buffer_one_segment) {
	assign_first_audio_buffer(0, 5);
	assign_first_audio_buffer(1, 6);
	assign_first_audio_buffer(2, 7);
	write_device_buffer_(3);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 5);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 6);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 7);
}

START_TEST(write_device_buffer_one_segment_wrap) {
	assign_channel_buffer_size(channel_buffers, 0, 3);
	assign_first_audio_buffer(0, 11);
	assign_first_audio_buffer(1, 12);
	assign_first_audio_buffer(2, 13);

	write_device_buffer_(5);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 12);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 13);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(3, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(4, 12);
}

START_TEST(write_device_buffer_one_segment_wrap_two_channels) {
	set_device_desired_output_channels(0, 2);

	channel_buffers[0].channel = 0;
	channel_buffers[1].channel = 1;
	assign_channel_buffer_segment(channel_buffers, 0, 0);
	assign_channel_buffer_segment(channel_buffers, 1, 0);

	assign_channel_buffer_size(channel_buffers, 0, 3);
	assign_first_audio_buffer(0, 11);
	assign_first_audio_buffer(1, 12);
	assign_first_audio_buffer(2, 13);

	write_device_buffer_(5);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 12);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 13);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(3, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(4, 12);
}

START_TEST(write_device_buffer_one_segment_wrap_second_channel) {
	set_device_desired_output_channels(0, 2);

	channel_buffers[0].channel = 0;
	channel_buffers[1].channel = 1;
	assign_channel_buffer_segment(channel_buffers, 0, 0);
	assign_channel_buffer_segment(channel_buffers, 1, 0);

	assign_channel_buffer_size(channel_buffers, 1, 3);
	assign_second_audio_buffer(0, 11);
	assign_second_audio_buffer(1, 12);
	assign_second_audio_buffer(2, 13);

	write_device_buffer(5, channel_buffers + 1);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 12);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 13);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(3, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(4, 12);
}

START_TEST(write_device_buffer_one_segment_offset) {
	assign_channel_buffer_index(channel_buffers, 0, 1);

	assign_first_audio_buffer(1, 5);
	assign_first_audio_buffer(2, 6);
	assign_first_audio_buffer(3, 7);
	write_device_buffer_(3);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 5);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 6);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 7);
}

START_TEST(write_device_buffer_two_segments_one_channel) {
	assign_device_segments(0, 2);

	assign_channel_buffer_size(channel_buffers, 0, 3);
	assign_first_audio_buffer(0, 11);
	assign_first_audio_buffer(1, 12);
	assign_first_audio_buffer(2, 13);

	assign_second_audio_buffer(0, 14);
	assign_second_audio_buffer(1, 15);
	assign_second_audio_buffer(2, 16);
	assign_second_audio_buffer(3, 17);

	write_device_buffer_(7);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 12);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 13);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(3, 14);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(4, 15);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(5, 16);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(6, 17);
}

START_TEST(write_device_buffer_two_segments_wrap_one_channel) {
	assign_device_segments(0, 2);

	assign_channel_buffer_size(channel_buffers, 1, 4);
	assign_second_audio_buffer(0, 14);
	assign_second_audio_buffer(1, 15);
	assign_second_audio_buffer(2, 16);
	assign_second_audio_buffer(3, 17);

	assign_first_audio_buffer(0, 11);
	assign_first_audio_buffer(1, 12);
	assign_first_audio_buffer(2, 13);

	write_device_buffer(7, channel_buffers + 1);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 14);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 15);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 16);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(3, 17);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(4, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(5, 12);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(6, 13);
}

START_TEST(tbd) {
	set_device_desired_output_channels(0, 2);
	assign_device_segments(0, 3);
	ar_current_device->seg_ic = 0;
	ar_current_device->seg_oc = 2;

	channel_buffers[5].channel = 1;
	assign_channel_buffer_segment(channel_buffers, 5, 2);
	assign_channel_buffer_size(channel_buffers, 5, 4);
	assign_audio_buffer(5, 0, 11);
	assign_audio_buffer(5, 1, 12);
	assign_audio_buffer(5, 2, 13);
	assign_audio_buffer(5, 3, 14);

	channel_buffers[1].channel = 1;
	assign_channel_buffer_size(channel_buffers, 1, 3);
	assign_audio_buffer(1, 0, 15);
	assign_audio_buffer(1, 1, 16);
	assign_audio_buffer(1, 2, 17);

	write_device_buffer(7, channel_buffers + 5);

	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 12);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 13);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(3, 14);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(4, 15);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(5, 16);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(6, 17);
}

START_TEST(tbd2) {
	assign_device_segments(0, 2);
	set_device_desired_output_channels(0, 3);
	ar_current_device->seg_ic = 0;
	ar_current_device->seg_oc = 1;

	channel_buffers[2].channel = 2;
	assign_channel_buffer_segment(channel_buffers, 2, 0);
	assign_channel_buffer_size(channel_buffers, 2, 4);
	assign_audio_buffer(2, 0, 11);
	assign_audio_buffer(2, 1, 12);
	assign_audio_buffer(2, 2, 13);
	assign_audio_buffer(2, 3, 14);

	channel_buffers[5].channel = 2;
	assign_channel_buffer_segment(channel_buffers, 5, 1);
	assign_channel_buffer_size(channel_buffers, 5, 3);
	assign_audio_buffer(5, 0, 15);
	assign_audio_buffer(5, 1, 16);
	assign_audio_buffer(5, 2, 17);

	write_device_buffer(7, channel_buffers + 2);

	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 12);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 13);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(3, 14);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(4, 15);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(5, 16);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(6, 17);
}

Suite* arsc_asio_write_device_buffer_suite() {
	Suite* suite = suite_create("arsc_asio_write_device_buffer");
	TCase* test_case = tcase_create("write_device_buffer");
	tcase_add_checked_fixture(test_case, setup_write_device_buffer, teardown_write_device_buffer);
	add_test(test_case, write_device_buffer_one_segment);
	add_test(test_case, write_device_buffer_one_segment_wrap);
	add_test(test_case, write_device_buffer_one_segment_offset);
	add_test(test_case, write_device_buffer_one_segment_wrap_two_channels);
	add_test(test_case, write_device_buffer_one_segment_wrap_second_channel);
	add_test(test_case, write_device_buffer_two_segments_one_channel);
	add_test(test_case, write_device_buffer_two_segments_wrap_one_channel);
	add_test(test_case, tbd);
	add_test(test_case, tbd2);
	suite_add_tcase(suite, test_case);
	return suite;
}

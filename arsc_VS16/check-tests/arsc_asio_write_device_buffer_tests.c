#include "arsc_asio_tests_common.h"
#include "arsc_asio_write_device_buffer_tests.h"
#include <stdlib.h>

enum {
	sufficiently_large = 100,
	buffer_count = sufficiently_large,
	device_index = 0
};

static int32_t device_buffer[sufficiently_large];
static ArAsioOutputAudio audio[buffer_count];
static int32_t audio_buffers[buffer_count][sufficiently_large];

static ArAsioOutputAudio initialized_audio() {
	ArAsioOutputAudio s;
	s.Index = 0;
	s.channel = 0;
	s.segment = 0;
	return s;
}

static ArAsioOutputAudio* audio_at(ArAsioOutputAudio* s, int i) {
	return s + i;
}

static void initialize_audio(ArAsioOutputAudio* s, int i) {
	*audio_at(s, i) = initialized_audio();
}

static void assign_audio_data(ArAsioOutputAudio* s, int i, int32_t* data) {
	audio_at(s, i)->data = data;
}

static void assign_audio_size(ArAsioOutputAudio* s, int i, int32_t size) {
	audio_at(s, i)->size = size;
}

static void assign_audio_size_(int i, int32_t size) {
	audio_at(audio, i)->size = size;
}

static void assign_first_audio_size(int32_t size) {
	assign_audio_size_(0, size);
}

static void assign_audio_segment(ArAsioOutputAudio* s, int i, int32_t segment) {
	audio_at(s, i)->segment = segment;
}

static void assign_audio_index(ArAsioOutputAudio* s, int i, int32_t index) {
	audio_at(s, i)->Index = index;
}

static void set_output_channels(int32_t n) {
	set_device_desired_output_channels(device_index, n);;
}

static void set_segments(int32_t n) {
	assign_device_segments(device_index, n);
}

static void setup_write_device_buffer(void) {
	for (int i = 0; i < buffer_count; ++i) {
		initialize_audio(audio, i);
		assign_audio_data(audio, i, audio_buffers[i]);
		assign_audio_size_(i, sizeof audio_buffers[i] / sizeof audio_buffers[i][0]);
	}
	allocate_device(device_index);
	set_segments(1);
	set_output_channels(1);
	ar_current_device = devices(0);
	global_output_audio = audio;
}

static void teardown_write_device_buffer(void) {
	memset(device_buffer, 0, sizeof device_buffer);
	memset(audio, 0, sizeof audio);
	memset(audio_buffers, 0, sizeof audio_buffers);
	free_device(device_index);
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

static void write_device_buffer(int32_t n, ArAsioOutputAudio* s) {
	ar_asio_write_device_buffer(device_buffer, n, s);
}

static void write_device_buffer_(int32_t n) {
	ar_asio_write_device_buffer(device_buffer, n, audio);
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
	assign_first_audio_size(3);
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
	set_output_channels(2);

	audio[0].channel = 0;
	audio[1].channel = 1;

	assign_first_audio_size(3);
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
	set_output_channels(2);

	audio[0].channel = 0;
	audio[1].channel = 1;

	assign_audio_size_(1, 3);
	assign_second_audio_buffer(0, 11);
	assign_second_audio_buffer(1, 12);
	assign_second_audio_buffer(2, 13);

	write_device_buffer(5, audio + 1);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 12);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 13);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(3, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(4, 12);
}

START_TEST(write_device_buffer_one_segment_offset) {
	assign_audio_index(audio, 0, 1);

	assign_first_audio_buffer(1, 5);
	assign_first_audio_buffer(2, 6);
	assign_first_audio_buffer(3, 7);
	write_device_buffer_(3);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 5);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 6);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 7);
}

START_TEST(write_device_buffer_two_segments_one_channel) {
	set_segments(2);
	assign_audio_segment(audio, 0, 0);
	assign_audio_segment(audio, 1, 1);

	assign_first_audio_size(3);
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
	set_segments(2);
	assign_audio_segment(audio, 0, 0);
	assign_audio_segment(audio, 1, 1);

	assign_audio_size(audio, 1, 4);
	assign_second_audio_buffer(0, 14);
	assign_second_audio_buffer(1, 15);
	assign_second_audio_buffer(2, 16);
	assign_second_audio_buffer(3, 17);

	assign_first_audio_buffer(0, 11);
	assign_first_audio_buffer(1, 12);
	assign_first_audio_buffer(2, 13);

	write_device_buffer(7, audio + 1);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 14);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 15);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 16);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(3, 17);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(4, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(5, 12);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(6, 13);
}

START_TEST(write_device_buffer_three_segments_two_channels) {
	set_output_channels(2);
	set_segments(3);

	audio[5].channel = 1;
	assign_audio_segment(audio, 5, 2);
	assign_audio_size_(5, 4);
	assign_audio_buffer(5, 0, 11);
	assign_audio_buffer(5, 1, 12);
	assign_audio_buffer(5, 2, 13);
	assign_audio_buffer(5, 3, 14);

	audio[1].channel = 1;
	assign_audio_size_(1, 3);
	assign_audio_buffer(1, 0, 15);
	assign_audio_buffer(1, 1, 16);
	assign_audio_buffer(1, 2, 17);

	write_device_buffer(7, audio + 5);

	ASSERT_DEVICE_BUFFER_AT_EQUALS(0, 11);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(1, 12);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(2, 13);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(3, 14);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(4, 15);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(5, 16);
	ASSERT_DEVICE_BUFFER_AT_EQUALS(6, 17);
}

START_TEST(write_device_buffer_two_segments_three_channels) {
	set_segments(2);
	set_output_channels(3);

	audio[2].channel = 2;
	assign_audio_segment(audio, 2, 0);
	assign_audio_size_(2, 4);
	assign_audio_buffer(2, 0, 11);
	assign_audio_buffer(2, 1, 12);
	assign_audio_buffer(2, 2, 13);
	assign_audio_buffer(2, 3, 14);

	audio[5].channel = 2;
	assign_audio_segment(audio, 5, 1);
	assign_audio_size_(5, 3);
	assign_audio_buffer(5, 0, 15);
	assign_audio_buffer(5, 1, 16);
	assign_audio_buffer(5, 2, 17);

	write_device_buffer(7, audio + 2);

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
	add_test(test_case, write_device_buffer_three_segments_two_channels);
	add_test(test_case, write_device_buffer_two_segments_three_channels);
	suite_add_tcase(suite, test_case);
	return suite;
}

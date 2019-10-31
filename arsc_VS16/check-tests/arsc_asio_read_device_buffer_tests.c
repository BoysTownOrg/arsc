#include "arsc_asio_read_device_buffer_tests.h"
#include "arsc_asio_tests_common.h"

enum {
	sufficiently_large = 100,
	buffer_count = sufficiently_large,
	device_index = 0
};

static int32_t device_buffer[sufficiently_large];
static ArAsioInputAudio audio[buffer_count];
static int32_t audio_buffers[buffer_count][sufficiently_large];

static ArAsioInputAudio initialized_audio() {
	ArAsioInputAudio s;
	s.Index = 0;
	s.channel = 0;
	s.segment = 0;
	s.LatencyReached = 1;
	return s;
}

static ArAsioInputAudio* audio_at(ArAsioInputAudio* s, int i) {
	return s + i;
}

static ArAsioInputAudio* response_at(int i) {
	return audio_at(audio, i);
}

static void initialize_audio_(int i) {
	*response_at(i) = initialized_audio();
}

static void assign_audio_data_(int i, int32_t* data) {
	response_at(i)->data = data;
}

static void assign_audio_size(ArAsioInputAudio* s, int i, int32_t size) {
	audio_at(s, i)->size = size;
}

static void assign_audio_size_(int i, int32_t size) {
	audio_at(audio, i)->size = size;
}

static void assign_first_audio_size(int32_t size) {
	assign_audio_size_(0, size);
}

static void assign_audio_segment(ArAsioInputAudio* s, int i, int32_t segment) {
	audio_at(s, i)->segment = segment;
}

static void assign_audio_index(ArAsioInputAudio* s, int i, int32_t index) {
	audio_at(s, i)->Index = index;
}

static void set_input_channels(int32_t n) {
	set_device_desired_input_channels(device_index, n);;
}

static void set_segments(int32_t n) {
	assign_device_segments(device_index, n);
}

static void setup(void) {
	for (int i = 0; i < buffer_count; ++i) {
		initialize_audio_(i);
		assign_audio_data_(i, audio_buffers[i]);
		assign_audio_size_(i, sizeof audio_buffers[i] / sizeof audio_buffers[i][0]);
	}
	allocate_device(device_index);
	set_segments(1);
	set_input_channels(1);
	global_ar_asio_current_device = devices(0);
}

static void teardown(void) {
	memset(device_buffer, 0, sizeof device_buffer);
	memset(audio, 0, sizeof audio);
	memset(audio_buffers, 0, sizeof audio_buffers);
	free_device(device_index);
}

static void assign_device_buffer(int i, int32_t n) {
	assign_integer_array(device_buffer, i, n);
}

static void read_device_buffer_(int32_t n, ArAsioInputAudio *response) {
	ar_asio_read_device_buffer(device_buffer, n, response);
}

static void read_device_buffer_into_second_response(int32_t n) {
	read_device_buffer_(n, audio + 1);
}

static void read_device_buffer(int32_t n) {
	read_device_buffer_(n, audio);
}

static void set_first_response_size(int32_t n) {
	assign_audio_size_(0, n);
}

static void set_second_response_size(int32_t n) {
	assign_audio_size_(1, n);
}

static void set_response_channel(int i, int32_t c) {
	response_at(i)->channel = c;
}

static void set_response_segment(int i, int32_t c) {
	response_at(i)->segment = c;
}

#define ASSERT_NTH_AUDIO_BUFFER_AT_EQUALS(n, a, b)\
	ASSERT_EQUAL_ANY(b, audio_buffers[n][a]);

#define ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(a, b)\
	ASSERT_NTH_AUDIO_BUFFER_AT_EQUALS(0, a, b)

#define ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(a, b)\
	ASSERT_NTH_AUDIO_BUFFER_AT_EQUALS(1, a, b)

START_TEST(read_device_buffer_one_segment) {
	assign_device_buffer(0, 1);
	assign_device_buffer(1, 2);
	assign_device_buffer(2, 3);
	read_device_buffer(3);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(0, 1);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(1, 2);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(2, 3);
}

START_TEST(read_device_buffer_one_segment_offset) {
	assign_audio_index(audio, 0, 1);

	assign_device_buffer(0, 5);
	assign_device_buffer(1, 6);
	assign_device_buffer(2, 7);
	read_device_buffer(3);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(1, 5);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(2, 6);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(3, 7);
}

START_TEST(read_device_buffer_one_segment_wrap) {
	set_first_response_size(3);
	assign_device_buffer(0, 1);
	assign_device_buffer(1, 2);
	assign_device_buffer(2, 3);
	assign_device_buffer(3, 4);
	assign_device_buffer(4, 5);

	read_device_buffer(5);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(0, 4);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(1, 5);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(2, 3);
}

START_TEST(read_device_buffer_one_segment_wrap_two_channels) {
	set_input_channels(2);
	set_response_channel(0, 0);
	set_response_channel(1, 1);

	set_first_response_size(3);
	assign_device_buffer(0, 1);
	assign_device_buffer(1, 2);
	assign_device_buffer(2, 3);
	assign_device_buffer(3, 4);
	assign_device_buffer(4, 5);

	read_device_buffer(5);

	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(0, 4);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(1, 5);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(2, 3);
}

START_TEST(read_device_buffer_one_segment_wrap_second_channel) {
	set_input_channels(2);
	set_response_channel(0, 0);
	set_response_channel(1, 1);

	set_second_response_size(3);
	assign_device_buffer(0, 1);
	assign_device_buffer(1, 2);
	assign_device_buffer(2, 3);
	assign_device_buffer(3, 4);
	assign_device_buffer(4, 5);

	read_device_buffer_into_second_response(5);

	ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(0, 4);
	ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(1, 5);
	ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(2, 3);
}

START_TEST(read_device_buffer_two_segments) {
	set_segments(2);
	set_response_segment(0, 0);
	set_response_segment(1, 1);

	assign_device_buffer(0, 1);
	assign_device_buffer(1, 2);
	assign_device_buffer(2, 3);
	assign_device_buffer(3, 4);
	assign_device_buffer(4, 5);
	assign_device_buffer(5, 6);
	assign_device_buffer(6, 7);

	set_first_response_size(3);
	read_device_buffer(7);

	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(0, 1);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(1, 2);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(2, 3);

	ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(0, 4);
	ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(1, 5);
	ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(2, 6);
	ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(3, 7);
}

START_TEST(read_device_buffer_two_segments_wrap_one_channel) {
	set_segments(2);
	set_response_segment(0, 0);
	set_response_segment(1, 1);

	assign_device_buffer(0, 1);
	assign_device_buffer(1, 2);
	assign_device_buffer(2, 3);
	assign_device_buffer(3, 4);
	assign_device_buffer(4, 5);
	assign_device_buffer(5, 6);
	assign_device_buffer(6, 7);

	set_second_response_size(3);
	read_device_buffer_into_second_response(7);

	ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(0, 1);
	ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(1, 2);
	ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(2, 3);

	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(0, 4);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(1, 5);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(2, 6);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(3, 7);
}

START_TEST(read_device_buffer_two_segments_three_channels) {
	set_segments(2);
	set_input_channels(3);
	set_response_segment(2, 0);
	set_response_segment(5, 1);
	set_response_channel(2, 2);
	set_response_channel(5, 2);

	assign_device_buffer(0, 1);
	assign_device_buffer(1, 2);
	assign_device_buffer(2, 3);
	assign_device_buffer(3, 4);
	assign_device_buffer(4, 5);
	assign_device_buffer(5, 6);
	assign_device_buffer(6, 7);

	assign_audio_size_(2, 3);
	read_device_buffer_(7, audio + 2);

	ASSERT_NTH_AUDIO_BUFFER_AT_EQUALS(2, 0, 1);
	ASSERT_NTH_AUDIO_BUFFER_AT_EQUALS(2, 1, 2);
	ASSERT_NTH_AUDIO_BUFFER_AT_EQUALS(2, 2, 3);

	ASSERT_NTH_AUDIO_BUFFER_AT_EQUALS(5, 0, 4);
	ASSERT_NTH_AUDIO_BUFFER_AT_EQUALS(5, 1, 5);
	ASSERT_NTH_AUDIO_BUFFER_AT_EQUALS(5, 2, 6);
	ASSERT_NTH_AUDIO_BUFFER_AT_EQUALS(5, 3, 7);
}

Suite* arsc_asio_read_device_buffer_suite() {
	Suite* suite = suite_create("arsc_asio_read_device_buffer");
	TCase* test_case = tcase_create("read_device_buffer");
	tcase_add_checked_fixture(test_case, setup, teardown);
	add_test(test_case, read_device_buffer_one_segment);
	add_test(test_case, read_device_buffer_one_segment_offset);
	add_test(test_case, read_device_buffer_one_segment_wrap);
	add_test(test_case, read_device_buffer_one_segment_wrap_two_channels);
	add_test(test_case, read_device_buffer_one_segment_wrap_second_channel);
	add_test(test_case, read_device_buffer_two_segments);
	add_test(test_case, read_device_buffer_two_segments_wrap_one_channel);
	add_test(test_case, read_device_buffer_two_segments_three_channels);
	suite_add_tcase(suite, test_case);
	return suite;
}

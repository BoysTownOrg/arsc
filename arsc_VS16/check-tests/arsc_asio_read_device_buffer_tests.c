#include "arsc_asio_read_device_buffer_tests.h"
#include "arsc_asio_tests_common.h"

enum {
	sufficiently_large = 100,
	buffer_count = sufficiently_large,
	device_index = 0
};

static int32_t device_buffer[sufficiently_large];
static TResponseData responses[buffer_count];
static int32_t audio_buffers[buffer_count][sufficiently_large];

static TResponseData initialized_channel_response() {
	TResponseData s;
	s.Magic = 0xBEEF;
	s.Index = 0;
	s.channel = 0;
	s.segment = 0;
	s.LatencyReached = 1;
	return s;
}

static TResponseData* channel_response_at(TResponseData* s, int i) {
	return s + i;
}

static void initialize_channel_response(TResponseData* s, int i) {
	*channel_response_at(s, i) = initialized_channel_response();
}

static void assign_channel_response_data(TResponseData* s, int i, int32_t* data) {
	channel_response_at(s, i)->data = data;
}

static void assign_channel_response_size(TResponseData* s, int i, int32_t size) {
	channel_response_at(s, i)->size = size;
}

static void assign_channel_response_size_(int i, int32_t size) {
	channel_response_at(responses, i)->size = size;
}

static void assign_first_channel_response_size(int32_t size) {
	assign_channel_response_size_(0, size);
}

static void assign_channel_response_segment(TResponseData* s, int i, int32_t segment) {
	channel_response_at(s, i)->segment = segment;
}

static void assign_channel_response_index(TResponseData* s, int i, int32_t index) {
	channel_response_at(s, i)->Index = index;
}

static void set_input_channels(int32_t n) {
	set_device_desired_input_channels(device_index, n);;
}

static void set_segments(int32_t n) {
	assign_device_segments(device_index, n);
}

static void setup(void) {
	for (int i = 0; i < buffer_count; ++i) {
		initialize_channel_response(responses, i);
		assign_channel_response_data(responses, i, audio_buffers[i]);
		assign_channel_response_size_(i, sizeof audio_buffers[i] / sizeof audio_buffers[i][0]);
	}
	allocate_device(device_index);
	set_segments(1);
	set_input_channels(1);
	ar_current_device = devices(0);
}

static void teardown(void) {
	memset(device_buffer, 0, sizeof device_buffer);
	memset(responses, 0, sizeof responses);
	memset(audio_buffers, 0, sizeof audio_buffers);
	free_device(device_index);
}

static void assign_device_buffer(int i, int32_t n) {
	assign_integer_array(device_buffer, i, n);
}

static void read_device_buffer(int32_t n) {
	ar_asio_read_device_buffer(device_buffer, n, responses);
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

START_TEST(read_device_buffer_two_segments) {
	set_segments(2);
	responses[0].segment = 0;
	responses[1].segment = 1;

	assign_device_buffer(0, 1);
	assign_device_buffer(1, 2);
	assign_device_buffer(2, 3);
	assign_device_buffer(3, 4);
	assign_device_buffer(4, 5);
	assign_device_buffer(5, 6);
	assign_device_buffer(6, 7);

	responses[0].size = 3;
	read_device_buffer(7);

	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(0, 1);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(1, 2);
	ASSERT_FIRST_AUDIO_BUFFER_AT_EQUALS(2, 3);

	ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(0, 4);
	ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(1, 5);
	ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(2, 6);
	ASSERT_SECOND_AUDIO_BUFFER_AT_EQUALS(3, 7);
}

Suite* arsc_asio_read_device_buffer_suite() {
	Suite* suite = suite_create("arsc_asio_read_device_buffer");
	TCase* test_case = tcase_create("read_device_buffer");
	tcase_add_checked_fixture(test_case, setup, teardown);
	add_test(test_case, read_device_buffer_one_segment);
	add_test(test_case, read_device_buffer_two_segments);
	suite_add_tcase(suite, test_case);
	return suite;
}

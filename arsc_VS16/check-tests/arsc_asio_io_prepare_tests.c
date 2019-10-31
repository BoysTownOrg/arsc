#include "arsc_asio_io_prepare_tests.h"
#include "arsc_asio_tests_common.h"

enum {
	sufficiently_large = 100,
	device_index = 0
};

static void* output_buffers[sufficiently_large];
static void* input_buffers[sufficiently_large];
static int32_t buffer_size[sufficiently_large];

static void assign_device_output_buffers(int device, void** buffers) {
	devices(device)->o_data = buffers;
}

static void assign_device_input_buffers(int device, void** buffers) {
	devices(device)->i_data = buffers;
}

static void assign_device_sizes(int device, int32_t* sizes) {
	devices(device)->sizptr = sizes;
}

static void set_segments(int32_t n) {
	assign_device_segments(device_index, n);
}

static void setup(void) {
	allocate_device(device_index);
	set_segments(1);
	assign_device_sizes(device_index, buffer_size);
	assign_device_output_buffers(device_index, output_buffers);
	assign_device_input_buffers(device_index, input_buffers);
}

static void teardown(void) {
	free_device(device_index);
	free(global_ar_asio_input_audio);
	free(global_ar_asio_output_audio);
}

static void io_prepare(void) {
	_ar_asio_io_prepare(0);
}

static void assign_buffer_size(int i, int32_t size) {
	assign_integer_array(buffer_size, i, size);
}

static void assign_pointer_array(void** a, int i, void* what) {
	a[i] = what;
}

static void assign_output_buffer(int i, void* buffer) {
	assign_pointer_array(output_buffers, i, buffer);
}

static void assign_input_buffer(int i, void* buffer) {
	assign_pointer_array(input_buffers, i, buffer);
}

static ArAsioOutputAudio* global_output_audio_(int i) {
	return global_ar_asio_output_audio + i;
}

static ArAsioInputAudio* global_input_audio_(int i) {
	return global_ar_asio_input_audio + i;
}

static int32_t* global_output_audio_data(int i) {
	return global_output_audio_(i)->data;
}

static int32_t* global_input_audio_data(int i) {
	return global_input_audio_(i)->data;
}

static void set_output_channels(int32_t n) {
	assign_device_output_channels(device_index, n);
}

static void set_input_channels(int32_t n) {
	assign_device_input_channels(device_index, n);
}

#define ASSERT_OUTPUT_AUDIO_AT_BUFFER(a, b)\
	ASSERT_EQUAL_ANY(b, global_output_audio_data(a));

#define ASSERT_INPUT_AUDIO_AT_BUFFER(a, b)\
	ASSERT_EQUAL_ANY(b, global_input_audio_data(a));

#define ASSERT_OUTPUT_AUDIO_AT_SIZE(a, b)\
	ASSERT_EQUAL_ANY(b, global_output_audio_(a)->size)

#define ASSERT_INPUT_AUDIO_AT_SIZE(a, b)\
	ASSERT_EQUAL_ANY(b, global_input_audio_(a)->size)

#define ASSERT_OUTPUT_AUDIO_AT_SEGMENT(a, b)\
	ASSERT_EQUAL_ANY(b, global_output_audio_(a)->segment)

#define ASSERT_INPUT_AUDIO_AT_SEGMENT(a, b)\
	ASSERT_EQUAL_ANY(b, global_input_audio_(a)->segment)

#define ASSERT_OUTPUT_AUDIO_AT_CHANNEL(a, b)\
	ASSERT_EQUAL_ANY(b, global_output_audio_(a)->channel)

#define ASSERT_INPUT_AUDIO_AT_CHANNEL(a, b)\
	ASSERT_EQUAL_ANY(b, global_input_audio_(a)->channel)

#define ASSERT_OUTPUT_AUDIO_AT_INDEX(a, b)\
	ASSERT_EQUAL_ANY(b, global_output_audio_(a)->Index)

#define ASSERT_INPUT_AUDIO_AT_INDEX(a, b)\
	ASSERT_EQUAL_ANY(b, global_input_audio_(a)->Index)

START_TEST(io_prepare_initializes_output_audio) {
	set_output_channels(2);
	set_segments(3);
	assign_buffer_size(0, 4);
	assign_buffer_size(1, 5);
	assign_buffer_size(2, 6);

	io_prepare();

	ASSERT_OUTPUT_AUDIO_AT_SIZE(0, 4);
	ASSERT_OUTPUT_AUDIO_AT_SIZE(1, 4);
	ASSERT_OUTPUT_AUDIO_AT_SIZE(2, 5);
	ASSERT_OUTPUT_AUDIO_AT_SIZE(3, 5);
	ASSERT_OUTPUT_AUDIO_AT_SIZE(4, 6);
	ASSERT_OUTPUT_AUDIO_AT_SIZE(5, 6);

	ASSERT_OUTPUT_AUDIO_AT_SEGMENT(0, 0);
	ASSERT_OUTPUT_AUDIO_AT_SEGMENT(1, 0);
	ASSERT_OUTPUT_AUDIO_AT_SEGMENT(2, 1);
	ASSERT_OUTPUT_AUDIO_AT_SEGMENT(3, 1);
	ASSERT_OUTPUT_AUDIO_AT_SEGMENT(4, 2);
	ASSERT_OUTPUT_AUDIO_AT_SEGMENT(5, 2);

	ASSERT_OUTPUT_AUDIO_AT_CHANNEL(0, 0);
	ASSERT_OUTPUT_AUDIO_AT_CHANNEL(1, 1);
	ASSERT_OUTPUT_AUDIO_AT_CHANNEL(2, 0);
	ASSERT_OUTPUT_AUDIO_AT_CHANNEL(3, 1);
	ASSERT_OUTPUT_AUDIO_AT_CHANNEL(4, 0);
	ASSERT_OUTPUT_AUDIO_AT_CHANNEL(5, 1);

	ASSERT_OUTPUT_AUDIO_AT_INDEX(0, 0);
	ASSERT_OUTPUT_AUDIO_AT_INDEX(1, 0);
	ASSERT_OUTPUT_AUDIO_AT_INDEX(2, 0);
	ASSERT_OUTPUT_AUDIO_AT_INDEX(3, 0);
	ASSERT_OUTPUT_AUDIO_AT_INDEX(4, 0);
	ASSERT_OUTPUT_AUDIO_AT_INDEX(5, 0);
}

START_TEST(io_prepare_initializes_input_audio) {
	set_input_channels(2);
	set_segments(3);
	assign_buffer_size(0, 4);
	assign_buffer_size(1, 5);
	assign_buffer_size(2, 6);

	io_prepare();

	ASSERT_INPUT_AUDIO_AT_SIZE(0, 4);
	ASSERT_INPUT_AUDIO_AT_SIZE(1, 4);
	ASSERT_INPUT_AUDIO_AT_SIZE(2, 5);
	ASSERT_INPUT_AUDIO_AT_SIZE(3, 5);
	ASSERT_INPUT_AUDIO_AT_SIZE(4, 6);
	ASSERT_INPUT_AUDIO_AT_SIZE(5, 6);

	ASSERT_INPUT_AUDIO_AT_SEGMENT(0, 0);
	ASSERT_INPUT_AUDIO_AT_SEGMENT(1, 0);
	ASSERT_INPUT_AUDIO_AT_SEGMENT(2, 1);
	ASSERT_INPUT_AUDIO_AT_SEGMENT(3, 1);
	ASSERT_INPUT_AUDIO_AT_SEGMENT(4, 2);
	ASSERT_INPUT_AUDIO_AT_SEGMENT(5, 2);

	ASSERT_INPUT_AUDIO_AT_CHANNEL(0, 0);
	ASSERT_INPUT_AUDIO_AT_CHANNEL(1, 1);
	ASSERT_INPUT_AUDIO_AT_CHANNEL(2, 0);
	ASSERT_INPUT_AUDIO_AT_CHANNEL(3, 1);
	ASSERT_INPUT_AUDIO_AT_CHANNEL(4, 0);
	ASSERT_INPUT_AUDIO_AT_CHANNEL(5, 1);

	ASSERT_INPUT_AUDIO_AT_INDEX(0, 0);
	ASSERT_INPUT_AUDIO_AT_INDEX(1, 0);
	ASSERT_INPUT_AUDIO_AT_INDEX(2, 0);
	ASSERT_INPUT_AUDIO_AT_INDEX(3, 0);
	ASSERT_INPUT_AUDIO_AT_INDEX(4, 0);
	ASSERT_INPUT_AUDIO_AT_INDEX(5, 0);
}

START_TEST(io_prepare_initializes_output_audio_data) {
	set_output_channels(3);
	int32_t first;
	assign_output_buffer(0, &first);
	int32_t second;
	assign_output_buffer(1, &second);
	int32_t third;
	assign_output_buffer(2, &third);

	io_prepare();

	ASSERT_OUTPUT_AUDIO_AT_BUFFER(0, &first);
	ASSERT_OUTPUT_AUDIO_AT_BUFFER(1, &second);
	ASSERT_OUTPUT_AUDIO_AT_BUFFER(2, &third);
}

START_TEST(io_prepare_initializes_input_audio_data) {
	set_input_channels(3);
	int32_t first;
	assign_input_buffer(0, &first);
	int32_t second;
	assign_input_buffer(1, &second);
	int32_t third;
	assign_input_buffer(2, &third);

	io_prepare();

	ASSERT_INPUT_AUDIO_AT_BUFFER(0, &first);
	ASSERT_INPUT_AUDIO_AT_BUFFER(1, &second);
	ASSERT_INPUT_AUDIO_AT_BUFFER(2, &third);
}

Suite* arsc_asio_io_prepare_suite() {
	Suite* suite = suite_create("arsc_asio_io_prepare");
	TCase* test_case = tcase_create("io_prepare");
	tcase_add_checked_fixture(test_case, setup, teardown);
	add_test(test_case, io_prepare_initializes_output_audio);
	add_test(test_case, io_prepare_initializes_input_audio);
	add_test(test_case, io_prepare_initializes_output_audio_data);
	add_test(test_case, io_prepare_initializes_input_audio_data);
	suite_add_tcase(suite, test_case);
	return suite;
}

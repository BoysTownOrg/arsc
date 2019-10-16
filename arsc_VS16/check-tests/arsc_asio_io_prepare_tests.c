#include "arsc_asio_io_prepare_tests.h"
#include "arsc_asio_tests_common.h"

enum {
	sufficiently_large = 100
};

static void* output_buffers[sufficiently_large];
static void* input_buffers[sufficiently_large];
static int32_t buffer_size[sufficiently_large];

static void assign_device_input_channels(int device, int32_t channels) {
	devices(device)->ncad = channels;
}

static void assign_device_output_buffers(int device, void** buffers) {
	devices(device)->o_data = buffers;
}

static void assign_device_input_buffers(int device, void** buffers) {
	devices(device)->i_data = buffers;
}

static void assign_device_output_channels(int device, int32_t channels) {
	devices(device)->ncda = channels;
}

static void assign_device_sizes(int device, int32_t* sizes) {
	devices(device)->sizptr = sizes;
}

static void setup_io_prepare(void) {
	allocate_device(0);
	assign_device_input_channels(0, 0);
	assign_device_segments(0, 1);
	assign_device_output_buffers(0, output_buffers);
	assign_device_sizes(0, buffer_size);
	assign_device_input_buffers(0, input_buffers);
}

static void teardown_io_prepare(void) {
	free_device(0);
}

static void io_prepare(void) {
	_ar_asio_io_prepare(0);
}

static void assign_output_buffer_size(int i, int32_t size) {
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
	return global_output_audio + i;
}

static ArAsioInputAudio* global_input_audio_(int i) {
	return global_input_audio + i;
}

static int32_t* global_output_audio_data(int i) {
	return global_output_audio_(i)->data;
}

static int32_t* global_input_audio_data(int i) {
	return global_input_audio_(i)->data;
}

#define ASSERT_OUTPUT_AUDIO_BUFFER(a, b)\
	ASSERT_EQUAL_ANY(a, global_output_audio_data(b));

#define ASSERT_INPUT_AUDIO_BUFFER(a, b)\
	ASSERT_EQUAL_ANY(a, global_input_audio_data(b));

#define ASSERT_OUTPUT_AUDIO_AT_SIZE(a, b)\
	ASSERT_EQUAL_ANY(b, global_output_audio_(a)->size)

#define ASSERT_OUTPUT_AUDIO_AT_SEGMENT(a, b)\
	ASSERT_EQUAL_ANY(b, global_output_audio_(a)->segment)

#define ASSERT_OUTPUT_AUDIO_AT_CHANNEL(a, b)\
	ASSERT_EQUAL_ANY(b, global_output_audio_(a)->channel)

#define ASSERT_OUTPUT_AUDIO_AT_INDEX(a, b)\
	ASSERT_EQUAL_ANY(b, global_output_audio_(a)->Index)

START_TEST(io_prepare_initializes_output_audio) {
	assign_device_output_channels(0, 2);
	assign_device_segments(0, 3);
	assign_output_buffer_size(0, 4);
	assign_output_buffer_size(1, 5);
	assign_output_buffer_size(2, 6);

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

START_TEST(io_prepare_initializes_output_audio_data) {
	assign_device_output_channels(0, 3);
	int32_t first;
	assign_output_buffer(0, &first);
	int32_t second;
	assign_output_buffer(1, &second);
	int32_t third;
	assign_output_buffer(2, &third);

	io_prepare();

	ASSERT_OUTPUT_AUDIO_BUFFER(&first, 0);
	ASSERT_OUTPUT_AUDIO_BUFFER(&second, 1);
	ASSERT_OUTPUT_AUDIO_BUFFER(&third, 2);
}

START_TEST(io_prepare_initializes_input_audio_data) {
	assign_device_input_channels(0, 3);
	int32_t first;
	assign_input_buffer(0, &first);
	int32_t second;
	assign_input_buffer(1, &second);
	int32_t third;
	assign_input_buffer(2, &third);

	io_prepare();

	ASSERT_INPUT_AUDIO_BUFFER(&first, 0);
	ASSERT_INPUT_AUDIO_BUFFER(&second, 1);
	ASSERT_INPUT_AUDIO_BUFFER(&third, 2);
}

Suite* arsc_asio_io_prepare_suite() {
	Suite* suite = suite_create("arsc_asio_io_prepare");
	TCase* io_prepare_test_case = tcase_create("io_prepare");
	tcase_add_checked_fixture(io_prepare_test_case, setup_io_prepare, teardown_io_prepare);
	add_test(io_prepare_test_case, io_prepare_initializes_output_audio);
	add_test(io_prepare_test_case, io_prepare_initializes_output_audio_data);
	add_test(io_prepare_test_case, io_prepare_initializes_input_audio_data);
	suite_add_tcase(suite, io_prepare_test_case);
	return suite;
}

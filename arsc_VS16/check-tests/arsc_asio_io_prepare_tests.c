#include "arsc_asio_io_prepare_tests.h"
#include "arsc_asio_tests_common.h"

enum {
	sufficiently_large = 100
};

static void* output_buffers[sufficiently_large];
static int32_t output_buffer_size[sufficiently_large];

static void assign_device_input_channels(int device, int32_t channels) {
	devices(device)->ncad = channels;
}

static void assign_device_output_buffers(int device, void** buffers) {
	devices(device)->o_data = buffers;
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
	assign_device_sizes(0, output_buffer_size);
}

static void teardown_io_prepare(void) {
	free_device(0);
}

static void io_prepare(void) {
	_ar_asio_io_prepare(0);
}

static void assign_output_buffer_size(int i, int32_t size) {
	assign_integer_array(output_buffer_size, i, size);
}

static void assign_pointer_array(void** a, int i, void* what) {
	a[i] = what;
}

static void assign_output_buffer(int i, void* buffer) {
	assign_pointer_array(output_buffers, i, buffer);
}

static ArAsioOutputAudio* global_asio_segment_(int i) {
	return global_asio_channel_buffers + i;
}

static int32_t* global_asio_segment_data(int i) {
	return global_asio_segment_(i)->data;
}

#define ASSERT_SEGMENT_BUFFER(a, b)\
	ASSERT_EQUAL_ANY(a, global_asio_segment_data(b));

#define ASSERT_SEGMENT_SAMPLES(a, b)\
	ASSERT_EQUAL_ANY(a, global_asio_segment_(b)->size)

#define ASSERT_SEGMENT_SEGMENT(a, b)\
	ASSERT_EQUAL_ANY(a, global_asio_segment_(b)->segment)

#define ASSERT_SEGMENT_CHANNEL(a, b)\
	ASSERT_EQUAL_ANY(a, global_asio_segment_(b)->channel)

#define ASSERT_SEGMENT_INDEX(a, b)\
	ASSERT_EQUAL_ANY(a, global_asio_segment_(b)->Index)

START_TEST(io_prepare_initializes_segments) {
	assign_device_output_channels(0, 2);
	assign_device_segments(0, 3);
	assign_output_buffer_size(0, 4);
	assign_output_buffer_size(1, 5);
	assign_output_buffer_size(2, 6);

	io_prepare();

	ASSERT_SEGMENT_SAMPLES(4, 0);
	ASSERT_SEGMENT_SAMPLES(4, 1);
	ASSERT_SEGMENT_SAMPLES(5, 2);
	ASSERT_SEGMENT_SAMPLES(5, 3);
	ASSERT_SEGMENT_SAMPLES(6, 4);
	ASSERT_SEGMENT_SAMPLES(6, 5);

	ASSERT_SEGMENT_SEGMENT(0, 0);
	ASSERT_SEGMENT_SEGMENT(0, 1);
	ASSERT_SEGMENT_SEGMENT(1, 2);
	ASSERT_SEGMENT_SEGMENT(1, 3);
	ASSERT_SEGMENT_SEGMENT(2, 4);
	ASSERT_SEGMENT_SEGMENT(2, 5);

	ASSERT_SEGMENT_CHANNEL(0, 0);
	ASSERT_SEGMENT_CHANNEL(1, 1);
	ASSERT_SEGMENT_CHANNEL(0, 2);
	ASSERT_SEGMENT_CHANNEL(1, 3);
	ASSERT_SEGMENT_CHANNEL(0, 4);
	ASSERT_SEGMENT_CHANNEL(1, 5);

	ASSERT_SEGMENT_INDEX(0, 0);
	ASSERT_SEGMENT_INDEX(0, 1);
	ASSERT_SEGMENT_INDEX(0, 2);
	ASSERT_SEGMENT_INDEX(0, 3);
	ASSERT_SEGMENT_INDEX(0, 4);
	ASSERT_SEGMENT_INDEX(0, 5);
}

START_TEST(io_prepare_initializes_segment_data) {
	assign_device_output_channels(0, 3);
	int32_t first;
	assign_output_buffer(0, &first);
	int32_t second;
	assign_output_buffer(1, &second);
	int32_t third;
	assign_output_buffer(2, &third);

	io_prepare();

	ASSERT_SEGMENT_BUFFER(&first, 0);
	ASSERT_SEGMENT_BUFFER(&second, 1);
	ASSERT_SEGMENT_BUFFER(&third, 2);
}

Suite* arsc_asio_io_prepare_suite() {
	Suite* suite = suite_create("arsc_asio_io_prepare");
	TCase* io_prepare_test_case = tcase_create("io_prepare");
	tcase_add_checked_fixture(io_prepare_test_case, setup_io_prepare, teardown_io_prepare);
	add_test(io_prepare_test_case, io_prepare_initializes_segments);
	add_test(io_prepare_test_case, io_prepare_initializes_segment_data);
	suite_add_tcase(suite, io_prepare_test_case);
	return suite;
}

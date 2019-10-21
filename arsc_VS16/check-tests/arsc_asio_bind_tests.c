#include "arsc_asio_tests_common.h"
#include "arsc_asio_bind_tests.h"
#include <arsc_asio_wrappers.h>
#include <stdlib.h>

static int32_t(*devices_restore)();
static char* (*device_name_restore)(int32_t);
static void (*io_stop_restore)(int32_t);
static void (*close_restore)(int32_t);
static int32_t(*open_restore)(int32_t);
static int32_t(*io_prepare_restore)(int32_t);
static int32_t(*list_rates_restore)(int32_t);
static void(*io_start_restore)(int32_t);
static int32_t(*transfer_segment_restore)(int32_t, int32_t);
static int32_t(*check_segment_restore)(int32_t, int32_t);
static int32_t(*latency_restore)(int32_t, int32_t);

static int32_t device_count;
static char* device_name;
static int32_t opened_device;
static int32_t rates;
static int32_t list_rates_device;

static int32_t devices_stub() {
	return device_count;
}

static char* device_name_stub(int32_t n) {
	return device_name;
}

static void io_stop_stub(int32_t n) {
	n;
}

static void close_stub(int32_t n) {
	n;
}

static int32_t open_stub(int32_t n) {
	n;
	return opened_device;
}

static int32_t io_prepare_stub(int32_t n) {
	n;
	return 0;
}

static int32_t list_rates_stub(int32_t n) {
	list_rates_device = n;
	return rates;
}

static void io_start_stub(int32_t n) {
	n;
}

static int32_t transfer_segment_stub(int32_t m, int32_t n) {
	m;
	n;
	return 1;
}

static int32_t check_segment_stub(int32_t m, int32_t n) {
	m;
	n;
	return 1;
}

static int32_t latency_stub(int32_t m, int32_t n) {
	m;
	n;
	return 1;
}

static void assign_device_input_channels(int device, int32_t channels) {
	devices(device)->ncad = channels;
}

static void assign_device_output_channels(int device, int32_t channels) {
	devices(device)->ncda = channels;
}

static void setup(void) {
	allocate_device(0);
	allocate_device(1);
	devices_restore = ar_asio_devices;
	device_name_restore = ar_asio_device_name;
	io_stop_restore = ar_asio_io_stop;
	close_restore = ar_asio_close;
	open_restore = ar_asio_open;
	io_prepare_restore = ar_asio_io_prepare;
	list_rates_restore = ar_asio_list_rates;
	io_start_restore = ar_asio_io_start;
	transfer_segment_restore = ar_asio_transfer_segment;
	check_segment_restore = ar_asio_check_segment;
	latency_restore = ar_asio_latency;
	ar_asio_devices = devices_stub;
	ar_asio_device_name = device_name_stub;
	ar_asio_io_stop = io_stop_stub;
	ar_asio_close = close_stub;
	ar_asio_open = open_stub;
	ar_asio_io_prepare = io_prepare_stub;
	ar_asio_list_rates = list_rates_stub;
	ar_asio_io_start = io_start_stub;
	ar_asio_transfer_segment = transfer_segment_stub;
	ar_asio_check_segment = check_segment_stub;
	ar_asio_latency = latency_stub;
}

static void teardown(void) {
	ar_asio_devices = devices_restore;
	ar_asio_device_name = device_name_restore;
	ar_asio_io_stop = io_stop_restore;
	ar_asio_close = close_restore;
	ar_asio_open = open_restore;
	ar_asio_io_prepare = io_prepare_restore;
	ar_asio_list_rates = list_rates_restore;
	ar_asio_io_start = io_start_restore;
	ar_asio_transfer_segment = transfer_segment_restore;
	ar_asio_check_segment = check_segment_restore;
	ar_asio_latency = latency_restore;
	free_device(0);
	free_device(1);
}

static int32_t open_device(int32_t n) {
	return _ar_asio_open(n);
}

static int32_t open() {
	return open_device(0);
}

static int32_t bind_with_device_type(int32_t device_type) {
	return _ar_asio_bind(device_type, 0);
}

static int32_t bind_(void) {
	return bind_with_device_type(0);
}

static void set_devices(int32_t n) {
	device_count = n;
}

static void set_nonzero_devices(void) {
	set_devices(1);
}

static ARDVT device_types(int32_t device_type) {
	return _ardvt[device_type];
}

static int32_t(*bound_devices_impl(int32_t device_type))() {
	return device_types(device_type).num_dev;
}

static char*(*bound_device_name_impl(int32_t device_type))(int32_t) {
	return device_types(device_type).dev_name;
}

static void (*bound_io_stop_impl(int32_t device_type))(int32_t) {
	return device_types(device_type).io_stop;
}

static void (*bound_close_impl(int32_t device_type))(int32_t) {
	return device_types(device_type).close;
}

static int32_t (*bound_open_impl(int32_t device_type))(int32_t) {
	return device_types(device_type).open;
}

static int32_t(*bound_io_prepare_impl(int32_t device_type))(int32_t) {
	return device_types(device_type).io_prepare;
}

static int32_t(*bound_list_rates_impl(int32_t device_type))(int32_t) {
	return device_types(device_type).list_rates;
}

static void(*bound_io_start_impl(int32_t device_type))(int32_t) {
	return device_types(device_type).io_start;
}

static int32_t(*bound_transfer_segment_impl(int32_t device_type))(int32_t, int32_t) {
	return device_types(device_type).xfer_seg;
}

static int32_t(*bound_check_segment_impl(int32_t device_type))(int32_t, int32_t) {
	return device_types(device_type).chk_seg;
}

static int32_t(*bound_latency_impl(int32_t device_type))(int32_t, int32_t) {
	return device_types(device_type).latency;
}

static void bind_nonzero_devices_with_device_type(int32_t device_type) {
	set_nonzero_devices();
	bind_with_device_type(device_type);
}

#define ASSERT_EQUAL_INT(a, b) ck_assert_int_eq(a, b)

#define ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, stub, impl)\
	bind_nonzero_devices_with_device_type(device_type);\
	ASSERT_EQUAL_ANY(stub, impl(device_type))

#define ASSERT_BIND_ASSIGNS_DEVICES_IMPL_WHEN_NONZERO_DEVICES(device_type)\
	ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, devices_stub, bound_devices_impl)

#define ASSERT_BIND_ASSIGNS_DEVICE_NAME_IMPL_WHEN_NONZERO_DEVICES(device_type)\
	ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, device_name_stub, bound_device_name_impl)

#define ASSERT_BIND_ASSIGNS_IO_STOP_IMPL_WHEN_NONZERO_DEVICES(device_type)\
	ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, io_stop_stub, bound_io_stop_impl)

#define ASSERT_BIND_ASSIGNS_CLOSE_IMPL_WHEN_NONZERO_DEVICES(device_type)\
	ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, close_stub, bound_close_impl)

#define ASSERT_BIND_ASSIGNS_OPEN_IMPL_WHEN_NONZERO_DEVICES(device_type)\
	ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, open_stub, bound_open_impl)

#define ASSERT_BIND_ASSIGNS_IO_PREPARE_IMPL_WHEN_NONZERO_DEVICES(device_type)\
	ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, io_prepare_stub, bound_io_prepare_impl)

#define ASSERT_BIND_ASSIGNS_LIST_RATES_IMPL_WHEN_NONZERO_DEVICES(device_type)\
	ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, list_rates_stub, bound_list_rates_impl)

#define ASSERT_BIND_ASSIGNS_IO_START_IMPL_WHEN_NONZERO_DEVICES(device_type)\
	ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, io_start_stub, bound_io_start_impl)

#define ASSERT_BIND_ASSIGNS_TRANSFER_SEGMENT_IMPL_WHEN_NONZERO_DEVICES(device_type)\
	ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, transfer_segment_stub, bound_transfer_segment_impl)

#define ASSERT_BIND_ASSIGNS_CHECK_SEGMENT_IMPL_WHEN_NONZERO_DEVICES(device_type)\
	ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, check_segment_stub, bound_check_segment_impl)

#define ASSERT_BIND_ASSIGNS_LATENCY_IMPL_WHEN_NONZERO_DEVICES(device_type)\
	ASSERT_BIND_ASSIGNS_IMPL_WHEN_NONZERO_DEVICES(device_type, latency_stub, bound_latency_impl)

START_TEST(bind_returns_number_of_devices) {
	set_devices(3);
	ASSERT_EQUAL_INT(3, bind_());
}

START_TEST(bind_assigns_devices_impl_to_device_type_zero_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_DEVICES_IMPL_WHEN_NONZERO_DEVICES(0);
}

START_TEST(bind_assigns_devices_impl_to_device_type_one_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_DEVICES_IMPL_WHEN_NONZERO_DEVICES(1);
}

START_TEST(bind_assigns_device_name_impl_to_device_type_zero_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_DEVICE_NAME_IMPL_WHEN_NONZERO_DEVICES(0);
}

START_TEST(bind_assigns_device_name_impl_to_device_type_one_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_DEVICE_NAME_IMPL_WHEN_NONZERO_DEVICES(1);
}

START_TEST(bind_assigns_io_stop_impl_to_device_type_zero_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_IO_STOP_IMPL_WHEN_NONZERO_DEVICES(0);
}

START_TEST(bind_assigns_io_stop_impl_to_device_type_one_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_IO_STOP_IMPL_WHEN_NONZERO_DEVICES(1);
}

START_TEST(bind_assigns_close_to_device_type_zero_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_CLOSE_IMPL_WHEN_NONZERO_DEVICES(0);
}

START_TEST(bind_assigns_close_to_device_type_one_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_CLOSE_IMPL_WHEN_NONZERO_DEVICES(1);
}

START_TEST(bind_assigns_open_to_device_type_zero_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_OPEN_IMPL_WHEN_NONZERO_DEVICES(0);
}

START_TEST(bind_assigns_open_to_device_type_one_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_OPEN_IMPL_WHEN_NONZERO_DEVICES(1);
}

START_TEST(bind_assigns_io_prepare_to_device_type_zero_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_IO_PREPARE_IMPL_WHEN_NONZERO_DEVICES(0);
}

START_TEST(bind_assigns_io_prepare_to_device_type_one_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_IO_PREPARE_IMPL_WHEN_NONZERO_DEVICES(1);
}

START_TEST(bind_assigns_list_rates_to_device_type_zero_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_LIST_RATES_IMPL_WHEN_NONZERO_DEVICES(0);
}

START_TEST(bind_assigns_list_rates_to_device_type_one_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_LIST_RATES_IMPL_WHEN_NONZERO_DEVICES(1);
}

START_TEST(bind_assigns_io_start_to_device_type_zero_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_IO_START_IMPL_WHEN_NONZERO_DEVICES(0);
}

START_TEST(bind_assigns_io_start_to_device_type_one_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_IO_START_IMPL_WHEN_NONZERO_DEVICES(1);
}

START_TEST(bind_assigns_transfer_segment_to_device_type_zero_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_TRANSFER_SEGMENT_IMPL_WHEN_NONZERO_DEVICES(0);
}

START_TEST(bind_assigns_transfer_segment_to_device_type_one_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_TRANSFER_SEGMENT_IMPL_WHEN_NONZERO_DEVICES(1);
}

START_TEST(bind_assigns_check_segment_to_device_type_zero_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_CHECK_SEGMENT_IMPL_WHEN_NONZERO_DEVICES(0);
}

START_TEST(bind_assigns_check_segment_to_device_type_one_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_CHECK_SEGMENT_IMPL_WHEN_NONZERO_DEVICES(1);
}

START_TEST(bind_assigns_latency_to_device_type_zero_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_LATENCY_IMPL_WHEN_NONZERO_DEVICES(0);
}

START_TEST(bind_assigns_latency_to_device_type_one_when_nonzero_devices) {
	ASSERT_BIND_ASSIGNS_LATENCY_IMPL_WHEN_NONZERO_DEVICES(1);
}

Suite* arsc_asio_test_suite() {
	Suite* suite = suite_create("arsc_asio_bind_and_open");
	TCase* test_case = tcase_create("bind_and_open");
	tcase_add_checked_fixture(test_case, setup, teardown);
	add_test(test_case, bind_returns_number_of_devices);
	add_test(test_case, bind_assigns_devices_impl_to_device_type_zero_when_nonzero_devices);
	add_test(test_case, bind_assigns_devices_impl_to_device_type_one_when_nonzero_devices);
	add_test(test_case, bind_assigns_device_name_impl_to_device_type_zero_when_nonzero_devices);
	add_test(test_case, bind_assigns_device_name_impl_to_device_type_one_when_nonzero_devices);
	add_test(test_case, bind_assigns_io_stop_impl_to_device_type_zero_when_nonzero_devices);
	add_test(test_case, bind_assigns_io_stop_impl_to_device_type_one_when_nonzero_devices);
	add_test(test_case, bind_assigns_close_to_device_type_zero_when_nonzero_devices);
	add_test(test_case, bind_assigns_close_to_device_type_one_when_nonzero_devices);
	add_test(test_case, bind_assigns_open_to_device_type_zero_when_nonzero_devices);
	add_test(test_case, bind_assigns_open_to_device_type_one_when_nonzero_devices);
	add_test(test_case, bind_assigns_io_prepare_to_device_type_zero_when_nonzero_devices);
	add_test(test_case, bind_assigns_io_prepare_to_device_type_one_when_nonzero_devices);
	add_test(test_case, bind_assigns_list_rates_to_device_type_zero_when_nonzero_devices);
	add_test(test_case, bind_assigns_list_rates_to_device_type_one_when_nonzero_devices);
	add_test(test_case, bind_assigns_io_start_to_device_type_zero_when_nonzero_devices);
	add_test(test_case, bind_assigns_io_start_to_device_type_one_when_nonzero_devices);
	add_test(test_case, bind_assigns_transfer_segment_to_device_type_zero_when_nonzero_devices);
	add_test(test_case, bind_assigns_transfer_segment_to_device_type_one_when_nonzero_devices);
	add_test(test_case, bind_assigns_check_segment_to_device_type_zero_when_nonzero_devices);
	add_test(test_case, bind_assigns_check_segment_to_device_type_one_when_nonzero_devices);
	add_test(test_case, bind_assigns_latency_to_device_type_zero_when_nonzero_devices);
	add_test(test_case, bind_assigns_latency_to_device_type_one_when_nonzero_devices);
	suite_add_tcase(suite, test_case);
	return suite;
}
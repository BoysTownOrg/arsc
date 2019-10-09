#include "arsc_asio_tests.h"
#define ASIO
#include <arsc_asio.h>
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
static int32_t(*pLockAndLoadRestore)(int32_t);
static bool(*SDKAsioSetSampleRateRestore)(ASIOSampleRate);
static bool (*SDKAsioGetBufferSizeRestore)(
	long* alngMinBufferSize,
	long* alngMaxBufferSize,
	long* aslngPreferredBufferSize,
	long* alngGranularity
);

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

static bool SDKAsioSetSampleRateStub(ASIOSampleRate r) {
	r;
	return 1;
}

static bool SDKAsioGetBufferSizeStub(
	long* alngMinBufferSize,
	long* alngMaxBufferSize,
	long* aslngPreferredBufferSize,
	long* alngGranularity
) {
	alngMinBufferSize;
	alngMaxBufferSize;
	aslngPreferredBufferSize;
	alngGranularity;
	return 1;
}

static ARDEV** devices_(int32_t device) {
	return &_ardev[device];
}

static ARDEV* devices(int32_t device) {
	return *devices_(device);
}

static int32_t pLockAndLoadStub(int32_t device) {
	device;
	return 1;
}

static void allocate_device(int device) {
	*devices_(device) = calloc(1, sizeof(ARDEV));
}

static void assign_device_input_channels(int device, int32_t channels) {
	devices(device)->ncad = channels;
}

static void assign_device_output_channels(int device, int32_t channels) {
	devices(device)->ncda = channels;
}

static void assign_device_segments(int device, int32_t segments) {
	devices(device)->segswp = segments;
}

static void assign_device_output_buffers(int device, void **buffers) {
	devices(device)->o_data = buffers;
}

static void assign_device_sizes(int device, int32_t *sizes) {
	devices(device)->sizptr = sizes;
}

static void free_device(int device) {
	free(devices(device));
}

static void setup(void) {
	allocate_device(0);
	allocate_device(1);
	assign_device_input_channels(0, -2);
	assign_device_input_channels(1, -2);
	assign_device_output_channels(0, -2);
	assign_device_output_channels(1, -2);
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
	pLockAndLoadRestore = pLockAndLoad;
	SDKAsioSetSampleRateRestore = SDKAsioSetSampleRate;
	SDKAsioGetBufferSizeRestore = SDKAsioGetBufferSize;
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
	pLockAndLoad = pLockAndLoadStub;
	SDKAsioSetSampleRate = SDKAsioSetSampleRateStub;
	SDKAsioGetBufferSize = SDKAsioGetBufferSizeStub;
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
	pLockAndLoad = pLockAndLoadRestore;
	SDKAsioSetSampleRate = SDKAsioSetSampleRateRestore;
	SDKAsioGetBufferSize = SDKAsioGetBufferSizeRestore;
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

static ASIOBufferInfo* bufferInfo_(int i) {
	return bufferInfos + i;
}

static ASIOBool bufferInfoIsInput(int i) {
	return bufferInfo_(i)->isInput;
}

static long bufferInfoChannelNumber(int i) {
	return bufferInfo_(i)->channelNum;
}

static TStimulusData* stimulusData_(int i) {
	return stimulusData + i;
}

static int32_t *stimulusDataBlock(int i) {
	return stimulusData_(i)->StimulusBlock;
}

static void set_device_desired_output_channels(int i, int32_t c) {
	devices(i)->a_ncda = c;
}

#define ASSERT_EQUAL_INT(a, b) ck_assert_int_eq(a, b)
#define ASSERT_EQUAL_ANY(a, b) ck_assert(a == b)

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

#define ASSERT_BUFFER_INFO_IS_INPUT_EQUALS(a, b) ASSERT_EQUAL_ANY(a, bufferInfoIsInput(b))

#define ASSERT_BUFFER_INFO_IS_INPUT_FOR_DEVICE_RANGE(a, b)\
for (int i = a; i < b; ++i)\
	ASSERT_BUFFER_INFO_IS_INPUT_EQUALS(ASIOFalse, i)

#define ASSERT_BUFFER_INFO_IS_OUTPUT_FOR_DEVICE_RANGE(a, b)\
for (int i = a; i < b; ++i)\
	ASSERT_BUFFER_INFO_IS_INPUT_EQUALS(ASIOTrue, i)

#define ASSERT_BUFFER_INFO_CHANNEL_NUMBER(a, b) ASSERT_EQUAL_ANY(a, bufferInfoChannelNumber(b))

#define ASSERT_STIMULUS_DATA_BUFFER(a, b)\
	ASSERT_EQUAL_ANY(a, stimulusDataBlock(b));

#define ASSERT_STIMULUS_DATA_SAMPLES(a, b)\
	ASSERT_EQUAL_ANY(a, stimulusData_(b)->Samples)

#define ASSERT_STIMULUS_DATA_SEGMENT(a, b)\
	ASSERT_EQUAL_ANY(a, stimulusData_(b)->SegmentNumber)

#define ASSERT_STIMULUS_DATA_CHANNEL(a, b)\
	ASSERT_EQUAL_ANY(a, stimulusData_(b)->ChannelNumber)

#define ASSERT_STIMULUS_DATA_INDEX(a, b)\
	ASSERT_EQUAL_ANY(a, stimulusData_(b)->Index)

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

START_TEST(open_assigns_good_sample_rates) {
	rates = 1;
	open();
	ASSERT_EQUAL_ANY(1, devices(0)->gdsr);
}

START_TEST(open_passes_device_to_list_rates) {
	open_device(1);
	ASSERT_EQUAL_ANY(1, list_rates_device);
}

START_TEST(open_initializes_buffer_infos) {
	set_device_desired_output_channels(1, 2);
	devices(1)->a_ncad = 3;
	open_device(1);
	ASSERT_BUFFER_INFO_IS_INPUT_FOR_DEVICE_RANGE(0, 2);
	ASSERT_BUFFER_INFO_IS_OUTPUT_FOR_DEVICE_RANGE(2, 2 + 3);
	ASSERT_BUFFER_INFO_CHANNEL_NUMBER(0, 0);
	ASSERT_BUFFER_INFO_CHANNEL_NUMBER(1, 1);
	ASSERT_BUFFER_INFO_CHANNEL_NUMBER(0, 2);
	ASSERT_BUFFER_INFO_CHANNEL_NUMBER(1, 3);
	ASSERT_BUFFER_INFO_CHANNEL_NUMBER(2, 4);
}

static void assign_integer_array(int32_t* a, int i, int32_t what) {
	a[i] = what;
}

enum {
	sufficiently_large = 100
};

static void* output_buffers[sufficiently_large];
static int32_t output_buffer_size[sufficiently_large];

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

START_TEST(io_prepare_initializes_stimulus_data) {
	assign_device_output_channels(0, 2);
	assign_device_segments(0, 3);
	assign_output_buffer_size(0, 4);
	assign_output_buffer_size(1, 5);
	assign_output_buffer_size(2, 6);

	io_prepare();

	ASSERT_STIMULUS_DATA_SAMPLES(4, 0);
	ASSERT_STIMULUS_DATA_SAMPLES(4, 1);
	ASSERT_STIMULUS_DATA_SAMPLES(5, 2);
	ASSERT_STIMULUS_DATA_SAMPLES(5, 3);
	ASSERT_STIMULUS_DATA_SAMPLES(6, 4);
	ASSERT_STIMULUS_DATA_SAMPLES(6, 5);

	ASSERT_STIMULUS_DATA_SEGMENT(0, 0);
	ASSERT_STIMULUS_DATA_SEGMENT(0, 1);
	ASSERT_STIMULUS_DATA_SEGMENT(1, 2);
	ASSERT_STIMULUS_DATA_SEGMENT(1, 3);
	ASSERT_STIMULUS_DATA_SEGMENT(2, 4);
	ASSERT_STIMULUS_DATA_SEGMENT(2, 5);

	ASSERT_STIMULUS_DATA_CHANNEL(0, 0);
	ASSERT_STIMULUS_DATA_CHANNEL(1, 1);
	ASSERT_STIMULUS_DATA_CHANNEL(0, 2);
	ASSERT_STIMULUS_DATA_CHANNEL(1, 3);
	ASSERT_STIMULUS_DATA_CHANNEL(0, 4);
	ASSERT_STIMULUS_DATA_CHANNEL(1, 5);

	ASSERT_STIMULUS_DATA_INDEX(0, 0);
	ASSERT_STIMULUS_DATA_INDEX(0, 1);
	ASSERT_STIMULUS_DATA_INDEX(0, 2);
	ASSERT_STIMULUS_DATA_INDEX(0, 3);
	ASSERT_STIMULUS_DATA_INDEX(0, 4);
	ASSERT_STIMULUS_DATA_INDEX(0, 5);
}

START_TEST(io_prepare_initializes_stimulus_data_blocks) {
	assign_device_output_channels(0, 3);
	int32 local_first;
	assign_output_buffer(0, &local_first);
	int32 local_second;
	assign_output_buffer(1, &local_second);
	int32 local_third;
	assign_output_buffer(2, &local_third);

	io_prepare();
	
	ASSERT_STIMULUS_DATA_BUFFER(&local_first, 0);
	ASSERT_STIMULUS_DATA_BUFFER(&local_second, 1);
	ASSERT_STIMULUS_DATA_BUFFER(&local_third, 2);
}

static void setupSendStimulusData(void) {
	allocate_device(0);
	assign_device_segments(0, 1);
}

static void teardownSendStimulusData(void) {
	free_device(0);
}

START_TEST(pSendStimulusDataTbd) {
	int32 stimulus[3];
	TStimulusData localStimulusData;
	localStimulusData.StimulusBlock = stimulus;
	localStimulusData.Samples = 3;
	localStimulusData.Magic = 0xBEEF;
	localStimulusData.Index = 0;
	localStimulusData.ChannelNumber = 0;
	localStimulusData.SegmentNumber = 0;
	localStimulusData.OutputDone = 0;
	set_device_desired_output_channels(0, 1);
	ar_current_device = devices(0);

	assign_integer_array(stimulus, 0, 5);
	assign_integer_array(stimulus, 1, 6);
	assign_integer_array(stimulus, 2, 7);
	int32_t buffer[3];
	pSendStimulusData(buffer, 3, &localStimulusData);
	ASSERT_EQUAL_ANY(5, buffer[0]);
	ASSERT_EQUAL_ANY(6, buffer[1]);
	ASSERT_EQUAL_ANY(7, buffer[2]);
}

START_TEST(pSendStimulusDataTbd2) {
	int32 stimulus1[3];
	int32 stimulus2[4];
	TStimulusData localStimulusData[2];
	localStimulusData[0].StimulusBlock = stimulus1;
	localStimulusData[0].Samples = 3;
	localStimulusData[0].Magic = 0xBEEF;
	localStimulusData[0].Index = 0;
	localStimulusData[0].ChannelNumber = 0;
	localStimulusData[0].SegmentNumber = 0;
	localStimulusData[0].OutputDone = 0;
	localStimulusData[1].StimulusBlock = stimulus2;
	localStimulusData[1].Samples = 4;
	localStimulusData[1].Magic = 0xBEEF;
	localStimulusData[1].Index = 0;
	localStimulusData[1].ChannelNumber = 0;
	localStimulusData[1].SegmentNumber = 1;
	localStimulusData[1].OutputDone = 0;
	set_device_desired_output_channels(0, 1);
	assign_device_segments(0, 2);
	ar_current_device = devices(0);

	assign_integer_array(stimulus1, 0, 11);
	assign_integer_array(stimulus1, 1, 12);
	assign_integer_array(stimulus1, 2, 13);

	assign_integer_array(stimulus2, 0, 14);
	assign_integer_array(stimulus2, 1, 15);
	assign_integer_array(stimulus2, 2, 16);
	assign_integer_array(stimulus2, 3, 17);

	int32_t buffer[7];
	pSendStimulusData(buffer, 7, localStimulusData);
	ASSERT_EQUAL_ANY(11, buffer[0]);
	ASSERT_EQUAL_ANY(12, buffer[1]);
	ASSERT_EQUAL_ANY(13, buffer[2]);
	ASSERT_EQUAL_ANY(14, buffer[3]);
	ASSERT_EQUAL_ANY(15, buffer[4]);
	ASSERT_EQUAL_ANY(16, buffer[5]);
	ASSERT_EQUAL_ANY(17, buffer[6]);
}

static void add_test(TCase* test_case, const TTest* test) {
	tcase_add_test(test_case, test);
}

Suite* arsc_asio_test_suite() {
	Suite* suite = suite_create("arsc_asio");
	TCase* test_case = tcase_create("idk");
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
	add_test(test_case, open_assigns_good_sample_rates);
	add_test(test_case, open_passes_device_to_list_rates);
	add_test(test_case, open_initializes_buffer_infos);
	suite_add_tcase(suite, test_case);
	TCase* io_prepare_test_case = tcase_create("io_prepare");
	tcase_add_checked_fixture(io_prepare_test_case, setup_io_prepare, teardown_io_prepare);
	add_test(io_prepare_test_case, io_prepare_initializes_stimulus_data);
	add_test(io_prepare_test_case, io_prepare_initializes_stimulus_data_blocks);
	suite_add_tcase(suite, io_prepare_test_case);
	TCase* sendStimulusDataTestCase = tcase_create("sendStimulusData");
	tcase_add_checked_fixture(sendStimulusDataTestCase, setupSendStimulusData, teardownSendStimulusData);
	add_test(sendStimulusDataTestCase, pSendStimulusDataTbd);
	add_test(sendStimulusDataTestCase, pSendStimulusDataTbd2);
	suite_add_tcase(suite, sendStimulusDataTestCase);
	return suite;
}
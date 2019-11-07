#include "arsc_asio_combined_tests.h"
#include "arsc_asio_tests_common.h"
#include <arsc_asio_wrappers.h>
#include <Windows.h>
#include <process.h>
#include <synchapi.h>
#include <stdlib.h>

static int32_t(*list_rates_restore)(int32_t);
static int32_t(*pLockAndLoadRestore)(int32_t);
static int(*SDKAsioSetSampleRateRestore)(ASIOSampleRate);
static int (*SDKAsioGetBufferSizeRestore)(
	long* alngMinBufferSize,
	long* alngMaxBufferSize,
	long* aslngPreferredBufferSize,
	long* alngGranularity
	);
static int (*SDKAsioCreateBuffersRestore)(
	ASIOBufferInfo* bufferInfos,
	long numChannels,
	long bufferSize,
	ASIOCallbacks* callbacks
	);
static int (*SDKAsioGetLatenciesRestore)(long* inputLatency, long* outputLatency);
static int (*SDKAsioStartRestore)(void);

static int32_t list_rates_stub(int32_t n) {
	n;
	return 0;
}

static int SDKAsioSetSampleRateStub(ASIOSampleRate r) {
	r;
	return 1;
}

static int SDKAsioGetBufferSizeStub(
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

static int32_t pLockAndLoadStub(int32_t device) {
	device;
	return 1;
}

static int SDKAsioCreateBuffersStub(
	ASIOBufferInfo* bufferInfo,
	long numChannels,
	long bufferSize,
	ASIOCallbacks* callbacks
) {
	bufferInfo;
	numChannels;
	bufferSize;
	callbacks;
	return 1;
}

static int SDKAsioGetLatenciesStub(long* inputLatency, long* outputLatency) {
	inputLatency;
	outputLatency;
	return 1;
}

static int SDKAsioStartStub(void) {
	return 1;
}

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

static void setup(void) {
	for (int i = 0; i < buffer_count; ++i) {
		initialize_audio(audio, i);
		assign_audio_data(audio, i, audio_buffers[i]);
		assign_audio_size_(i, sizeof audio_buffers[i] / sizeof audio_buffers[i][0]);
	}
	allocate_device(device_index);
	set_segments(1);
	set_output_channels(1);
	global_ar_asio_current_device = devices(0);
	global_ar_asio_output_audio = audio;
	list_rates_restore = ar_asio_list_rates;
	pLockAndLoadRestore = pLockAndLoad;
	SDKAsioSetSampleRateRestore = SDKAsioSetSampleRate;
	SDKAsioGetBufferSizeRestore = SDKAsioGetBufferSize;
	SDKAsioCreateBuffersRestore = SDKAsioCreateBuffers;
	SDKAsioGetLatenciesRestore = SDKAsioGetLatencies;
	SDKAsioStartRestore = SDKAsioStart;
	ar_asio_list_rates = list_rates_stub;
	pLockAndLoad = pLockAndLoadStub;
	SDKAsioSetSampleRate = SDKAsioSetSampleRateStub;
	SDKAsioGetBufferSize = SDKAsioGetBufferSizeStub;
	SDKAsioCreateBuffers = SDKAsioCreateBuffersStub;
	SDKAsioGetLatencies = SDKAsioGetLatenciesStub;
	SDKAsioStart = SDKAsioStartStub;
}

static void teardown(void) {
	ar_asio_list_rates = list_rates_restore;
	pLockAndLoad = pLockAndLoadRestore;
	SDKAsioSetSampleRate = SDKAsioSetSampleRateRestore;
	SDKAsioGetBufferSize = SDKAsioGetBufferSizeRestore;
	SDKAsioCreateBuffers = SDKAsioCreateBuffersRestore;
	SDKAsioGetLatencies = SDKAsioGetLatenciesRestore;
	SDKAsioStart = SDKAsioStartRestore;
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

static LONG exit_client = 0;
static int32_t check_sum = 0;

static void client(void* ignored) {
	while (1) {
		check_sum += _ar_asio_chk_seg(device_index, 0);
		if (InterlockedCompareExchange(&exit_client, 0, 1))
			return;
	}
}

static void audio_thread(void* ignored) {
	write_device_buffer_(buffer_count - 1);
	InterlockedOr(&exit_client, 1);
}

static void simulate_audio_and_client_threads(void) {
	HANDLE client_handle = (HANDLE)_beginthread(client, 0, NULL);
	HANDLE audio_handle = (HANDLE)_beginthread(audio_thread, 0, NULL);
	WaitForSingleObject(audio_handle, INFINITE);
	WaitForSingleObject(client_handle, INFINITE);
}

START_TEST(tbd) {
	set_device_desired_input_channels(device_index, 0);
	assign_device_input_channels(device_index, -2);
	assign_device_output_channels(device_index, -2);
	_ar_asio_open(device_index);
	_ar_asio_io_start(device_index);

	simulate_audio_and_client_threads();

	check_sum += _ar_asio_chk_seg(device_index, 0);
	ASSERT_EQUAL_ANY(0, check_sum);
	ASSERT_EQUAL_ANY(0, _ar_asio_chk_seg(device_index, 0));

	simulate_audio_and_client_threads();

	check_sum += _ar_asio_chk_seg(device_index, 0);
	ASSERT_EQUAL_ANY(1, check_sum);
	ASSERT_EQUAL_ANY(0, _ar_asio_chk_seg(device_index, 0));
}

START_TEST(tbd2) {
	set_segments(2);
	assign_audio_segment(audio, 0, 0);
	assign_audio_segment(audio, 1, 1);
	set_device_desired_input_channels(device_index, 0);
	assign_device_input_channels(device_index, -2);
	assign_device_output_channels(device_index, -2);
	_ar_asio_open(device_index);
	_ar_asio_io_start(device_index);

	ASSERT_EQUAL_ANY(0, _ar_asio_chk_seg(device_index, 0));
	ASSERT_EQUAL_ANY(0, _ar_asio_chk_seg(device_index, 1));
	write_device_buffer_(buffer_count);
	ASSERT_EQUAL_ANY(1, _ar_asio_chk_seg(device_index, 0));
	ASSERT_EQUAL_ANY(0, _ar_asio_chk_seg(device_index, 1));
	write_device_buffer_(buffer_count);
	ASSERT_EQUAL_ANY(1, _ar_asio_chk_seg(device_index, 0));
	ASSERT_EQUAL_ANY(1, _ar_asio_chk_seg(device_index, 0));
	ASSERT_EQUAL_ANY(1, devices(device_index)->xrun);
}

Suite* arsc_asio_combined_suite() {
	Suite* suite = suite_create("arsc_asio_combined");
	TCase* test_case = tcase_create("combined");
	tcase_add_checked_fixture(test_case, setup, teardown);
	add_test(test_case, tbd);
	//add_test(test_case, tbd2);
	suite_add_tcase(suite, test_case);
	return suite;
}

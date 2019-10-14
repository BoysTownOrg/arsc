#include "arsc_asio_tests_common.h"

ARDEV** devices_(int32_t device) {
	return &_ardev[device];
}

ARDEV* devices(int32_t device) {
	return *devices_(device);
}

void free_device(int device) {
	free(devices(device));
}

void allocate_device(int device) {
	*devices_(device) = calloc(1, sizeof(ARDEV));
}

void assign_integer_array(int32_t* a, int i, int32_t what) {
	*integer_array_at(a, i) = what;
}

void set_device_desired_output_channels(int i, int32_t c) {
	devices(i)->a_ncda = c;
}

void set_device_desired_input_channels(int i, int32_t c) {
	devices(i)->a_ncad = c;
}

void assign_device_segments(int device, int32_t segments) {
	devices(device)->segswp = segments;
}

void add_test(TCase* test_case, const TTest* test) {
	tcase_add_test(test_case, test);
}

int32_t read_integer_array_at(int32_t* a, int i) {
	return *integer_array_at(a, i);
}

int32_t* integer_array_at(int32_t* a, int i) {
	return a + i;
}

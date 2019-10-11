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

#include "arsc_asio_tests_common.h"

ARDEV** devices_(int32_t device) {
	return &_ardev[device];
}

ARDEV* devices(int32_t device) {
	return *devices_(device);
}

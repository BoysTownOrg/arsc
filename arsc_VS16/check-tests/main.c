#include "arsc_asio_tests.h"
#include "arsc_asio_write_device_buffer_tests.h"
#include <stddef.h>

int main() {
	SRunner* runner = srunner_create(arsc_asio_test_suite());
	srunner_add_suite(runner, arsc_asio_write_device_buffer_suite());
	srunner_run_all(runner, CK_NORMAL);
	srunner_free(runner);
}

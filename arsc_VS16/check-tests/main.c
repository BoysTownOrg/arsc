#include "arsc_asio_bind_tests.h"
#include "arsc_asio_read_device_buffer_tests.h"
#include "arsc_asio_write_device_buffer_tests.h"
#include "arsc_asio_io_prepare_tests.h"
#include "arsc_asio_open_device_tests.h"
#include "arsc_asio_combined_tests.h"

static void add_suite(SRunner *runner, Suite *suite) {
    srunner_add_suite(runner, suite);
}

int main() {
    SRunner *runner = srunner_create(arsc_asio_bind_suite());
    add_suite(runner, arsc_asio_read_device_buffer_suite());
    add_suite(runner, arsc_asio_write_device_buffer_suite());
    add_suite(runner, arsc_asio_io_prepare_suite());
    add_suite(runner, arsc_asio_open_device_suite());
	add_suite(runner, arsc_asio_combined_suite());
    srunner_run_all(runner, CK_NORMAL);
    srunner_free(runner);
}

#include "arsc_asio_tests.h"
#include <stddef.h>

int main() {
	SRunner* runner = srunner_create(arsc_asio_test_suite());
	srunner_run_all(runner, CK_NORMAL);
	srunner_free(runner);
}

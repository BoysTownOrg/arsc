#include <check.h>
#include <stddef.h>

int main() {
	SRunner* runner = srunner_create(NULL);
	srunner_run_all(runner, CK_NORMAL);
	srunner_free(runner);
}

#include "arsc_asio_tests.h"
#define ASIO
#include <arscdev.h>

START_TEST(tbd) {
	ck_assert_int_eq(0, _ar_asio_bind(0, 0));
	ck_assert(2 + 2 == 5);
}
END_TEST

Suite* arsc_asio_test_suite() {
	Suite* suite = suite_create("arsc_asio");
	TCase* test_case = tcase_create("idk");
	tcase_add_test(test_case, tbd);
	suite_add_tcase(suite, test_case);
	return suite;
}
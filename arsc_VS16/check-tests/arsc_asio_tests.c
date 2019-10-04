#include "arsc_asio_tests.h"

START_TEST(tbd) {
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
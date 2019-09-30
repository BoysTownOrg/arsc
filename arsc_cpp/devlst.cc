// devlst.cpp - calls ARSC from C++

#include <iostream>

extern "C" {
#include <arsclib.h>
}

using namespace std;

void
devlst(int io_dev)
{
    char name[ARSC_NAMLEN];
    int i;

    printf("-- i/o device list --\n");
    for (i = 0; i < 20; i++) {
	if (ar_dev_name(i, name, ARSC_NAMLEN))
	    break;
	printf("%s %d = %s\n", (i == io_dev) ? ">" : " ", i, name);
    }
}

int
main()
{
    int io_dev, i;

    printf("%s\n", ar_version());
    devlst(ar_find_dev(ARSC_PREF_SYNC));

    return 0;
}

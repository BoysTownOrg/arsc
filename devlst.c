/* devlst.c - print device list */

#include <stdio.h>
#include <stdlib.h>
#include "arsclib.h"

void
devlst(int io_dev)
{
    char name[ARSC_NAMLEN];
    int i;

    printf("-- i/o device list --\n");
    for (i = 0; i < 20; i++) {
	if (ar_dev_name(i, name, ARSC_NAMLEN))
	    break;
	printf("%s %d = %s\n", (i == io_dev) ? ">" : " ", i + 1, name);
    }
}

int
main(int ac, char **av)
{
    devlst(ar_find_dev(ARSC_PREF_SYNC));

    return (0);
}

/* wrtnfo.c - write ARSC card info to file */

#include <stdio.h>
#include <string.h>
#include "cardinfo.h"

#define round(x) ((int)((x)+0.5))

int
write_registry_file(char *fn)
{
    int i, admvfs, damvfs;
    CARDINFO ci;
    FILE *fp;
    CARDINFO _arsc_get_cardinfo(int);

    fp = fopen(fn, "wt");
    if (fp == NULL)
	return (1);
    fputs("REGEDIT4\n\n", fp);
    fputs("[HKEY_LOCAL_MACHINE\\SOFTWARE\\BTNRH]\n\n", fp);
    fputs("[HKEY_LOCAL_MACHINE\\SOFTWARE\\BTNRH\\ARSC]\n\n", fp);
    fputs("[HKEY_LOCAL_MACHINE\\SOFTWARE\\BTNRH\\ARSC\\CardInfo]\n\n", fp);
    for (i = 0; i < NCT; i++) {
	ci = _arsc_get_cardinfo(i);
	if (ci.name[0] == 0)
	    continue;
        admvfs = round(ci.ad_vfs[0] * 1000);
	damvfs = round(ci.da_vfs[0] * 1000);
        fputs("[HKEY_LOCAL_MACHINE\\SOFTWARE\\BTNRH\\ARSC\\CardInfo", fp);
	fprintf(fp, "\\CardType%d]\n", i);
	fprintf(fp, "\"Name\"=\"%s\"\n", ci.name);
	fprintf(fp, "\"bits\"=dword:%08X\n", ci.bits);
	fprintf(fp, "\"left\"=dword:%08X\n", ci.left);
	fprintf(fp, "\"nbps\"=dword:%08X\n", ci.nbps);
	fprintf(fp, "\"ncad\"=dword:%08X\n", ci.ncad);
	fprintf(fp, "\"ncda\"=dword:%08X\n", ci.ncda);
	fprintf(fp, "\"gdsr\"=dword:%08X\n", ci.gdsr);
	fprintf(fp, "\"ad1_mv_fs\"=dword:%08X ; %5d\n", admvfs, admvfs);
	fprintf(fp, "\"da1_mv_fs\"=dword:%08X ; %5d\n", damvfs, damvfs);
        fputs("\n", fp);
    }
    fclose(fp);
    return (0);
}

int
write_rc_file(char *fn)
{
    int i, admvfs, damvfs;
    CARDINFO ci;
    FILE *fp;
    CARDINFO _arsc_get_cardinfo(int);

    fp = fopen(fn, "wt");
    if (fp == NULL)
	return (1);
    fputs("# arscrc - ARSC card info\n\n", fp);
    for (i = 0; i < NCT; i++) {
	ci = _arsc_get_cardinfo(i);
	if (ci.name[0] == 0)
	    continue;
        admvfs = round(ci.ad_vfs[0] * 1000);
	damvfs = round(ci.da_vfs[0] * 1000);
	fprintf(fp, "[CardType%d]\n", i);
	fprintf(fp, "Name=\"%s\"\n", ci.name);
	fprintf(fp, "bits=%d\n", ci.bits);
	fprintf(fp, "left=%d\n", ci.left);
	fprintf(fp, "nbps=%d\n", ci.nbps);
	fprintf(fp, "ncad=%d\n", ci.ncad);
	fprintf(fp, "ncda=%d\n", ci.ncda);
	fprintf(fp, "gdsr=%d\n", ci.gdsr);
	fprintf(fp, "ad_mv_fs=%d\n", admvfs);
	fprintf(fp, "da_mv_fs=%d\n", damvfs);
    }
    fclose(fp);
    return (0);
}

int
main(int ac, char **av)
{
#ifdef WIN32
    write_registry_file("arsc.reg");
#else
    write_rc_file("arscrc");
#endif

    return (0);
}


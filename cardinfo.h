/* cardinfo.h */

#define MAX_CT_NAME 40		/* max length of cardType name */
#define NCT	    20		/* max number of sourdcard types */
#define MAXNCH	    8		/* max number of I/O channels */

typedef struct {
    char    name[MAX_CT_NAME];
    int     bits;
    int     left;
    int     nbps;
    int     ncad;
    int     ncda;
    int     gdsr;
    double  ad_vfs[MAXNCH];
    double  da_vfs[MAXNCH];
} CARDINFO;

/* Copyright (c) 2013, Robert Ferguson (rob@bitscience.ca) */
/* All Rights Reserved. See LICENSE for details */

/* The diskimage library (http://www.paradroid.net/diskimage/) is copyright Per Olofsson. */

typedef struct _DIDirent {
	int type;
	int locked;
	int closed;
	int size;
	int rl; /* only valid for T_REL files */
	unsigned char rawname[16];	
	char name[17];	
	struct _DIDirent *next;
} DIDirent;

typedef struct _DIRDir {
	ImageFile *dh;
	unsigned char buffer[254];
	int bufsiz;
	int count;
	DIDirent *chain;
} DIDir;

extern DIDir *di_opendir(DiskImage *di);
extern DIDirent *di_readdir(DIDir *dirp);
extern int di_closedir(DIDir *dirp);
extern void ptoa(char *s);

static char *di_ftype[] = {
  "del",
  "seq",
  "prg",
  "usr",
  "rel",
  "cbm",
  "dir",
  "???"
};

/* Copyright (c) 2013, Robert Ferguson (rob@bitscience.ca) */
/* All Rights Reserved. See LICENSE for details */

/* The diskimage library (http://www.paradroid.net/diskimage/) is copyright Per Olofsson. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "diskimage.h"
#include "di_dirent.h"

extern void dump(char *s, int n);
extern void error(char *s, ...);
extern void debug(char *s, ...);

void
usage(char *pn)
{
	fprintf(stderr, "usage: %s [-v][-e fname,fmt] image [image ...]\n", pn);
}


int
main(int argc, char *argv[])
{
	int c, i;
	char *imagefile = NULL, *outdir = NULL;
	char fname[32];
	char *exfn = NULL;
	int extype = 0;
	int verbose = 0;
	char *p;
	char imagename[17];
	char imageid[6];
	unsigned char buffer[BUFSIZ];
	int len, size;
	FILE *fout;
	DiskImage *pdi;
	DIDir *pdir;
	DIDirent *pdirent;
	ImageFile *pinfile;

	while ((c = getopt(argc, argv, "e:v")) != -1) {
		switch(c) {
		case 'v':
			verbose = 1;
			break;
		case 'e':
			exfn = strdup(optarg);
			if ((p = strchr(exfn, ',')) != NULL) {
				*p++ = '\0';
				for (i = 0; i < sizeof(di_ftype)/sizeof(char *); i++) {
					if (strcasecmp(di_ftype[i], p) == 0) {
						extype = i;
						break;
					}
				}

				if (extype == 0) {
					error("extract: format extension '%s' not found", p);
					usage(argv[0]);
					exit(-1);
				}
			}
			break;
		default:
			usage(argv[0]);
			exit(-1);
		}
	}

	if (optind == argc) {
		usage(argv[0]);
		exit(-1);
	}

	for (i = optind; i < argc; i++) {

		imagefile = argv[i];

		if ((pdi = di_load_image(imagefile)) == NULL) {
			error("Can't open diskimage %s", imagefile);
			exit(-1);
		}

		/* Get the image name and id, convert them to ascii */

  		di_name_from_rawname(imagename, di_title(pdi));
  		ptoa(imagename);

  		memcpy(imageid, di_title(pdi) + 18, 5);
  		imageid[5] = 0;
  		ptoa(imageid);


		if (verbose) { 
	 		printf("0 \"%-16s\" %s\n", imagename, imageid);
		} else {
			printf("%s\n", imagename);
		}

		/* open the image directory, and iterate */
	
		if ((pdir = di_opendir(pdi)) == NULL) {
			error("Open of diskimage directory failed");
			exit(-1);
		}

		while ((pdirent = di_readdir(pdir)) != NULL) {

			/* if these are not the droids we're looking for, continue the search */

			if ((exfn != NULL) && ((strcmp(pdirent->name, exfn) != 0) || (pdirent->type != extype))) {
				continue;
			}

			if (verbose) {
				printf("%-4d  %-18s%c%s%c\n", pdirent->size, pdirent->name, 
					pdirent->closed ? ' ' : '*',
					di_ftype[pdirent->type], pdirent->locked ? '<' : ' ');
			} else {
				printf("      %-18s %s\n", pdirent->name, di_ftype[pdirent->type]);
			}
		}

		di_closedir(pdir);
		di_free_image(pdi);
	}

	return(0);
}

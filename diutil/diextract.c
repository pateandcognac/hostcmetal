/* Copyright (c) 2013, Robert Ferguson (rob@bitscience.ca) */
/* All Rights Reserved. See LICENSE for details */

/* The diskimage library (http://www.paradroid.net/diskimage/) is copyright Per Olofsson. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "diskimage.h"
#include "di_dirent.h"

#define PIPCMD "pipcmd.txt"

extern void dump(char *s, int n);
extern void error(char *s, ...);
extern void debug(char *s, ...);

void
usage(char *pn)
{
	fprintf(stderr, "usage: %s [-v][-f outdir][-e fname,fmt][-p basepath] image [image ...]\n", pn);
}


int
main(int argc, char *argv[])
{
	int c, i;
	char *imagefile = NULL, *outdir = NULL;
	char *pipbase = NULL;
	char fname[32];
	char *exfn = NULL;
	int extype = 0;
	int verbose = 0;
	char *p;
	unsigned char buffer[BUFSIZ];
	int len, size;
	FILE *fout;
	FILE *fpip = NULL;
	DiskImage *pdi;
	DIDir *pdir;
	DIDirent *pdirent;
	ImageFile *pinfile;

	while ((c = getopt(argc, argv, "f:e:vp:")) != -1) {
		switch(c) {
		case 'f':
			outdir = optarg;
			break;
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
					error("%s: format extension '%s' not found", argv[0], p);
					usage(argv[0]);
					exit(-1);
				}
			}
			break;
		case 'p':
			pipbase = optarg;
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

	/* put the output in the current directory unless we are told differently */
	/* FUTURE: if flag is set, extract disk title & use that as directory name */

	if (outdir != NULL) {
		if (mkdir(outdir, 0775) == -1) {
			error("Can't create output directory %s", outdir);
			exit(-1);
		}

		if (chdir(outdir) == -1) {
			error("Can't chdir() to output directory %s", outdir);
			exit(-1);
		}
	}

	if (pipbase) {
		if ((fpip = fopen(PIPCMD, "wx")) == NULL) {
			error("Can't open PIPCMD file %s", PIPCMD);
			exit(-1);
		}
	}

	for (i = optind; i < argc; i++) {

		imagefile = argv[i];

		if ((pdi = di_load_image(imagefile)) == NULL) {
			error("Can't open diskimage %s", imagefile);
			exit(-1);
		}


		/* open the image directory, and iterate */
	
		if ((pdir = di_opendir(pdi)) == NULL) {
			error("Open of diskimage directory failed");
			exit(-1);
		}

		while ((pdirent = di_readdir(pdir)) != NULL) {
			if (verbose) {
				printf("%-4d  %-18s%c%s%c\n", pdirent->size, pdirent->name, pdirent->closed ? ' ' : '*',
					di_ftype[pdirent->type], pdirent->locked ? '<' : ' ');
			}

			/* if these are not the droids we're looking for, continue the search */

			if ((exfn != NULL) && ((strcmp(pdirent->name, exfn) != 0) || (pdirent->type != extype))) {
				continue;
			}

			if ((pinfile = di_open(pdi, pdirent->rawname, pdirent->type, "rb")) == NULL) {
				error("Couldn't open diskimage file %s for reading; status %d (%d, %d)", 
					pdirent->name, pdi->status, pdi->statusts.track, pdi->statusts.sector);
				exit(-1);
			}

			/* create output filename, adding file-type; for fixed files, encode the rl */

			if (pdirent->rl != 0) {
				snprintf(fname, sizeof(fname), "(f:%d)%s,%s", pdirent->rl, 
					pdirent->name, di_ftype[pdirent->type]);
			} else {
				snprintf(fname, sizeof(fname), "%s,%s", pdirent->name, di_ftype[pdirent->type]);
			}
			
			/* 
			 * CBM allows '/' in filenames; replace occurances by '~'.
			 * There's a potential that this could cause a collision, but 
			 * if one happens it'll be caught by during the extraction.
			 */

			while ((p = strchr(fname, '/')) != NULL) {
				*p = '~';
			}

			if ((fout = fopen(fname, "wx")) == NULL) {
				error("Can't open output file %s for writing", fname);
				exit(-1);	
			}

			size = 0;
			while ((len = di_read(pinfile, buffer, BUFSIZ)) > 0) {
				if (fwrite(buffer, 1, len, fout) != len) {
					error("fwrite to output file failed");
					exit(-1);
				}
				size += len;
			}

			/* sanity check the extraction */

			if ((size > (pdirent->size * 254)) || ((size + 254) < (pdirent->size * 254)))  {
				error("File %s: number of bytes read %d doesn't match directory entry (%d blocks)",
					pdirent->name, size, pdirent->size);
				/* soft error, continue extraction */
			}

			/* create an appropriate entry in the pip command file */

			if (fpip) {
				
				fprintf(fpip, "copy disk/0.\"%s\",%s=", 
						pdirent->name, di_ftype[pdirent->type]);

				switch(pdirent->type) {
				case(T_PRG):
					fprintf(fpip, "(v)host.%s/%s", pipbase, fname);
					break;

 				case(T_REL):
					fprintf(fpip, "(f:%d)host.\"%s/%s\"", 
						pdirent->rl, pipbase, fname);
					break;
				
				case(T_SEQ):
				default:
					fprintf(fpip, "host.\"%s/%s\"", pipbase, fname);
					break;
				}
				fprintf(fpip, "%c", 0xd);
			}

			fclose(fout);
			di_close(pinfile);
		}

		/* clean up */

		di_closedir(pdir);
		di_free_image(pdi);
	}

	return(0);
}

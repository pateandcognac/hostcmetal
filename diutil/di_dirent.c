/* Copyright (c) 2013, Robert Ferguson (rob@bitscience.ca) */
/* All Rights Reserved. See LICENSE for details */

/* diskimage library (http://www.paradroid.net/diskimage/) is copyright Per Olofsson. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "diskimage.h"
#include "di_dirent.h"

void ptoa(char *s) {
  unsigned char c;

  while ((c = (unsigned char) *s)) {
    c &= 0x7f;
    if (c >= 'A' && c <= 'Z') {
      c += 32;
    } else if (c >= 'a' && c <= 'z') {
      c -= 32;
    } else if (c == 0x7f) {
      c = 0x3f;
    }
    *s++ = c;
  }
}

/*
 * di_opendir(), di_readdir(), di_closedir() parallel the stdlibc functions 
 */

DIDir *
di_opendir(DiskImage *di)
{
	DIDir *dirp;
	ImageFile *dh;

	if ((dh = di_open(di, (unsigned char *) "$", T_PRG, "rb")) == NULL)
		return(NULL);

	if ((dirp = malloc(sizeof(DIDir))) == NULL) {
		di_close(dh);
		return(NULL);
	}

	/* and read the first directory block */ 

	if ((dirp->bufsiz = di_read(dh, dirp->buffer, 254)) != 254) {
		di_close(dh);
		free(dirp);
		return(NULL);
	}

	dirp->dh = dh;
	dirp->count = 0;
	dirp->chain = NULL;
	
	return(dirp);
}

DIDirent *
di_readdir(DIDir *dirp)
{
	DIDirent *direntp = NULL;
	DIDirent *p = dirp->chain;
	int offset;

	while ((direntp == NULL) && (dirp->bufsiz == 254)) {
		offset = dirp->count * 32;

		/* filetype == 0 implies empty or scratched directory entry */

		if (dirp->buffer[offset]) {
			if ((direntp = malloc(sizeof(DIDirent))) == NULL) {
				return(NULL);
			}

			/* add new dirent to chain */
			if (p == NULL) {
				dirp->chain = p;
			} else {
				while (p->next != NULL) {
					p = p->next;
				}
				p->next = direntp;
			}
		
			direntp->next = NULL;	
			direntp->type = dirp->buffer[offset] & 7;
			direntp->closed = dirp->buffer[offset] & 0x80;
			direntp->locked = dirp->buffer[offset] & 0x40;
			direntp->size = dirp->buffer[offset + 29] << 8 | dirp->buffer[offset + 28];	

			if (direntp->type == T_REL) {
				direntp->rl = dirp->buffer[offset + 21];
			} else {
				direntp->rl = 0;
			}

			memcpy(direntp->rawname, &(dirp->buffer[offset + 3]), 16);
			di_name_from_rawname(direntp->name, direntp->rawname);
			ptoa(direntp->name);
		}

		dirp->count = (dirp->count + 1) % 8;
		
		if (dirp->count == 0) {
			dirp->bufsiz = di_read(dirp->dh, dirp->buffer, 254);

		}
	}	
	return(direntp);
}

int 
di_closedir(DIDir *dirp)
{
	DIDirent *p, *q;

	di_close(dirp->dh);

	/* free the chain of allocated dirents */

	p = dirp->chain;

	while (p) {
		q = p->next;
		free(p);
		p = q;
	}

	return(0);
}

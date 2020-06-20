/* HOSTCM. Copyright (c) 2013, Robert Ferguson (rob@bitscience.ca) */
/* All Rights Reserved. See LICENCE for details */

#include "hostcm.h"

/* generate a HOSTCM buffer checksum */

static char sumchar[] = "ABCDEFGHIJKLMNOP";

char
checksum(char *b, int n)
{
	int i, s = 0;

	for (i = 0; i < n; i++) {
		s += b[i] & 0xf;
	}

	return (sumchar[s&0xf]);
}

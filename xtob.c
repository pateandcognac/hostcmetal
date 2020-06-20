/* HOSTCM. Copyright (c) 2013, Robert Ferguson (rob@bitscience.ca) */
/* All Rights Reserved. See LICENCE for details */

#include <ctype.h>
#include <stdio.h>
#include "hostcm.h"

/* convert ascii encoded hex to binary */

void
xtob(char *p, int n)
{
	int i;
	unsigned char low, high;

	for (i = 0; i < n; i+=2) {
		if (isxdigit(p[i]) && isxdigit(p[i+1])) {
			high = (p[i] <= '9')?(p[i] - '0'):((toupper(p[i]) - 'A')+10);
			low = (p[i+1] <= '9')?(p[i+1] - '0'):((toupper(p[i+1]) - 'A')+10);
			p[i/2] = ((high << 4) + low);
		} else {
			error("invalid hex digits %c%c", p[i], p[i+1]);
		}
	}
}

static char *hex = "0123456789ABCDEF";

/* convert binary to ascii encoded hex; presumes that there's enough space in buffer to hold 2n characters */

void
btox(char *p, int n)
{
	int i;

	for (i = n - 1; i >= 0; i--) {
		p[i*2+1] = hex[p[i] & 0xf];
		p[i*2] = hex[(p[i] & 0xf0) >> 4];
	}
}

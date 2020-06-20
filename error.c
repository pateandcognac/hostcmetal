/* HOSTCM. Copyright (c) 2013, Robert Ferguson (rob@bitscience.ca) */
/* All Rights Reserved. See LICENCE for details */

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "hostcm.h"

FILE *errorfp;

#ifdef DEBUG
FILE *debugfp;
#endif

void
dump(char *s, int n)
{
#ifdef DEBUG
	int i;

	for (i = 0; i < n; i++) {
		if (isprint(s[i]))
			fprintf(debugfp, "%c", s[i]);
		else
			fprintf(debugfp, "\\%hhx", s[i]);
	}
	printf("\n");
#endif
}

void
debug(char *s, ...)
{
#ifdef DEBUG
	va_list ap;

	va_start(ap, s);
	vfprintf(debugfp, s, ap);
	va_end(ap);
	fprintf(debugfp, "\n");
#endif
}

void
error(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	vfprintf(errorfp, s, ap);
	va_end(ap);

	if (errno) {
		fprintf(errorfp, ": %s", strerror(errno));
	}

	fprintf(errorfp, "\n");
}

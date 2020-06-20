/* HOSTCM. Copyright (c) 2013, Robert Ferguson (rob@bitscience.ca) */
/* All Rights Reserved. See LICENCE for details */

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

void
dump(char *s, int n)
{
#ifdef DEBUG
	int i;

	for (i = 0; i < n; i++) {
		if (isprint(s[i]))
			printf("%c", s[i]);
		else
			printf("\\%hhx", s[i]);
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
	vfprintf(stdout, s, ap);
	va_end(ap);
	fprintf(stdout, "\n");
#endif
}

void
error(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);

	if (errno) {
		fprintf(stderr, ": %s", strerror(errno));
	}

	printf("\n");
}

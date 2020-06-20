/* HOSTCM. Copyright (c) 2013, Robert Ferguson (rob@bitscience.ca) */
/* All Rights Reserved. See LICENCE for details */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>
#include "hostcm.h"

HCOpts opt = { 	B9600,
		PARODD,
		0,
		0xd,
		0x11,
		0x13,
		0};

/* baud rate conversion table. Sigh. */

static int xbaud[13][2] = {
	{50, 	B50},
	{75, 	B75},
	{110, 	B110},
	{134,	B134},
	{150,	B150},
	{200, 	B200},
	{300,	B300},
	{600,	B600},
	{1200,	B1200},
	{2400,	B2400},
	{4800,	B4800},
	{9600,	B9600},
	{19200, B19200}
};

void
usage(char *pn)
{
	fprintf(stderr, "usage: %s [-e][-b][-p (even|odd)][-s baudrate][ttyname]\n", pn);
}

int
main(int argc, char *argv[])
{
	int c, n, i;
	int fdin, fdout;
	char *pname = argv[0];
	FILE *fp;
	struct termios tsave;

	errorfp = stderr;
#ifdef DEBUG
	debugfp = stdout;
#endif

	if (argc == 1) {
		/* 
		 * legacy mode: server uses stdin, stdout; errors, debug to logfile.
		 * presumes line discipline already set 
		 */

		fdin = STDIN_FILENO;
		fdout = STDOUT_FILENO;

		if ((fp = fopen(LOGFILE, "a")) == NULL) {
			error("Can't open log file");
			exit(-1);
		}

		if (setvbuf(fp, NULL, _IONBF, 0) == EOF) {
			error("Can't set LOGFILE buffering");
			exit(-1);
		}

		errorfp = fp;
#ifdef DEBUG
		debugfp = fp;
#endif
		opt.speed = B0; /* don't change line discipline */
		
	} else {

		while ((c = getopt(argc, argv, "bp:s:e")) != -1) {
			switch(c) {
			case 'b':
				opt.stopb = CSTOPB;
				break;

			case 's':
				n = atoi(optarg);

				opt.speed = B0;

				/* validate baud rate; find approp. constant */
				for (i = 0 ; i < sizeof(xbaud)/(sizeof(int) * 2); i++) {
					if ( n == xbaud[i][0]) {
						opt.speed = xbaud[i][1];
						break;
					}
				}

				if (opt.speed == B0) {
					error("invalid argument for speed: %d", n);
					usage(pname);
					exit(-1);
				}

				break;

			case 'p':
				if (strcasecmp("even", optarg) == 0) {
					opt.parity = 0;
					break;
				} 
				if (strcasecmp("odd", optarg) == 0) {
					opt.parity = PARODD;
					break;
				}
				error("invalid argument to -p; must be 'even' or 'odd'");
				usage(pname);
				exit(-1);

			case 'e':
				opt.ext |= DIREXT;
				break;

			default:
				usage(pname);
				exit(-1);
			}
		}

		argc -= optind;
		argv += optind;

		if (argc != 1) {
			error("missing ttyname");
			usage(pname);
			exit(-1);
		}

		if ((fdin = open(argv[0], O_RDWR )) == -1) {
			error("Can't open tty %s", argv[0]);
			exit(-1);
		}

		if (!isatty(fdin)) {
			error("%s not a tty", argv[0]);
			exit(-1);
		}

		fdout = fdin;
	} 

	if (tcgetattr(fdin, &tsave) == -1) {
		error("Can't save input termios");
		exit(-1);
	}

	if (setterm(fdin, &opt) == -1) {
		exit(-1);
	}

	server(fdin, fdout);

	if (tcsetattr(fdin, TCSANOW, &tsave) == -1) {
		error("Can't restore input termios");
		exit(-1);
	}

	return(0);
}

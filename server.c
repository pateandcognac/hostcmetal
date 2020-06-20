/* HOSTCM. Copyright (c) 2013, Robert Ferguson (rob@bitscience.ca) */
/* All Rights Reserved. See LICENCE for details */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include "hostcm.h"

/* presumes that fds are appropriately initialized */

void
server(int fdin, int fdout)
{
	char outbuf[BUFSIZ];
	char inbuf[BUFSIZ];
	int count;
	int n;

	while (1) {
		if ((n = receive(fdin, inbuf, BUFSIZ)) == -1) {
			error("receive failed");
			exit(-1);
		}

		debug("received (n=%d):", n);
		dump(inbuf, n);

		/* only validate checksum if we received a substantive command */

		if ((n > 1) && (checksum(inbuf, n-1) != inbuf[n-1])) {

			/* checksum failed -- send a NAK and retry */
			debug("checksum failed");

			count = 0;
			outbuf[count++] = opt.response;  
			outbuf[count++] = 'N';

			/* NAK does not to include checksum */	
/*			outbuf[count++] = checksum(&(outbuf[1]), 1);  */ 

			outbuf[count++] = opt.lineend;
			outbuf[count++] = opt.prompt;  

			if ((n = send(fdout, outbuf, count)) == -1) {
				exit(-1);
			}

			continue;
		} 

		/* command names are from the perspective of the client */

		switch(inbuf[0]) {
		case ('v'): /* starting a new session */
			debug("initialize");
			count = hostinit(inbuf, n, outbuf);
			break;

		case ('o'): /* create a file or open an existing one */
			debug("open");
			count = hostopen(inbuf, n, outbuf);
			break;

		case ('c'): /* close a currently open file */
			debug("close");
			count = hostclose(inbuf, n, outbuf);
			break;

		case ('p'): /* being sent a file */
			debug("put");
			count = hostput(inbuf, n, outbuf);
			break;

		case ('g'): /* 'sending a file */
			debug("get");
			count = hostget(inbuf, n, outbuf);
			break;

		case ('d'): /* directory open */
			debug("diropen");
			count = hostdiropen(inbuf, n, outbuf);
			break;

		case ('f'): /* directory read */
			debug("dirread");
			count = hostdirread(inbuf, n, outbuf);
			break;

		case ('k'): /* directory close */
			debug("dirclose");
			count = hostdirclose(inbuf, n, outbuf);
			break;

		case ('w'): /* file to rename from */
			debug("rename from");
			count = hostrenamefm(inbuf, n, outbuf);
			break;

		case ('b'): /* file to rename to */
			debug("rename to");
			count = hostrenameto(inbuf, n, outbuf);
			break;

		case ('y'): /* "scratch" (delete/unlink) file */
			debug("scratch");
			count = hostscratch(inbuf, n, outbuf);
			break;

		case ('r'): /* seek (record) */
			debug("seek");
			count = hostseek(inbuf, n, outbuf);
			break;

		case ('N'): /* NAK of previous response; resend */
			debug("NAK");
			/* do nothing; if count > 0, the current buffer will be resent */
			break;

		case ('q'): /* quit, when in legacy mode */
			debug("quit");
			hostquit();
			return;

		default:
			debug("?");
			count = hosterror(outbuf, "protocol error: unknown command");
			break;
		}

		if (count > 0) {

			debug("send:");
			dump(outbuf, count - 1);

			if ((n = send(fdout, outbuf, count)) == -1) {
				error("send failed");
				exit(-1);
			}
		}
	}
}

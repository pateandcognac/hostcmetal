/* HOSTCM. Copyright (c) 2013, Robert Ferguson (rob@bitscience.ca) */
/* All Rights Reserved. See LICENCE for details */

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "hostcm.h"


/* manage commuications channel with client machine using previously opened tty fd */
/* Serial lines are ideosyncratic. Setup is something of a mess. */

int
setterm(int fd, HCOpts *po)
{
	struct termios	t;
	int mbits;

	if (tcgetattr(fd, &t) == -1) {
		error("Can't get tty attributes for fd %d", fd);
		return(-1);
	}

	debug("starting termios settings:\n");
	debug("\tc_iflag = 0x%lx", t.c_iflag);
	debug("\tc_oflag = 0x%lx", t.c_oflag);
	debug("\tc_cflag = 0x%lx", t.c_cflag);
	debug("\tc_lflag = 0x%lx", t.c_lflag);
	debug("\tc_ispeed = %ld", t.c_ispeed);
	debug("\tc_ospeed = %ld", t.c_ospeed);

#ifdef UNIX_LIKE
	if (ioctl(fd, TIOCMGET, &mbits) == -1) {
		error("Can't TIOCMGET on tty fd=%d", fd);
		return(-1);
	} 
	debug("mbits = 0x%x", mbits);
#endif

	t.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	t.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	t.c_oflag &= ~(OPOST);

	if (po->speed != B0) {

		debug("speed = %d", po->speed);
		if (cfsetspeed(&t, po->speed) == -1) {
			error("Can't set line speed %d", po->speed);
			return(-1);
		}

		t.c_iflag |= IGNBRK | PARMRK | INPCK; 
		t.c_cflag &= ~(CSIZE);	/* clear mask before setting CS */
		t.c_cflag |= CS7 | PARENB | po->parity | po->stopb 
#ifdef UNIX_LIKE
			| CRTSCTS
#endif
			| CLOCAL | CREAD;
		t.c_lflag |= EXTPROC;
		t.c_ispeed = po->speed;
		t.c_ospeed = po->speed;
	} 

	if (tcsetattr(fd, TCSANOW, &t) == -1) {
		error("Can't set tty attributes for fd %d", fd);
		return(-1);
	}

	printf("HOSTCM running\n");

#if defined(DEBUG) && defined(UNIX_LIKE)
	if (ioctl(fd, TIOCMGET, &mbits) == -1) {
		error("Can't TIOCMGET on tty fd=%d", fd);
		return(-1);
	} 
	debug("mbits = 0x%x", mbits);
#endif

	/* Some USB adapters don't assert DTR by default. SuperPET UART requires it. */

#ifdef UNIX_LIKE
	mbits |= TIOCM_DTR | TIOCM_LE;
	debug("set mbits to 0x%x", mbits);
	if (ioctl(fd, TIOCMSET, &mbits) == -1) {
		error("Can't set DTR on tty fd = %d", fd);
		return(-1);
	}
#endif

#ifdef DEBUG
	if (tcgetattr(fd, &t) == -1) {
		error("Can't get tty attributes for fd %d", fd);
		return(-1);
	}

	debug("final termios settings:\n");
	debug("\tc_iflag = 0x%lx", t.c_iflag);
	debug("\tc_oflag = 0x%lx", t.c_oflag);
	debug("\tc_cflag = 0x%lx", t.c_cflag);
	debug("\tc_lflag = 0x%lx", t.c_lflag);
	debug("\tc_ispeed = %ld", t.c_ispeed);
	debug("\tc_ospeed = %ld", t.c_ospeed);

#ifdef UNIX_LIKE
	if (ioctl(fd, TIOCMGET, &mbits) == -1) {
		error("Can't TIOCMGET on tty fd=%d", fd);
		return(-1);
	} 
	debug("mbits = 0x%x", mbits);
#endif
#endif

	return(0);
}

int
receive(int fd, char *buf, int sz)
{
	int n, t = 0;

	do {
		if ((n = read(fd, &(buf[t]), sz - t)) == -1) {
			error("can't read from tty");
			return(-1);
		}

		t += n;
	} while ((n != 0) && (t < sz) && (buf[t-1] != opt.lineend));

	return(t-1);
}

int
send(int fd, char *buf, int sz)
{
	int n, t = 0;
	
	do {
		if ((n = write(fd, &buf[t], sz - t)) == -1) {
			error("can't write to tty");
			return(-1);
		}
		t += n;

	} while (t < sz);

	return(t);
}

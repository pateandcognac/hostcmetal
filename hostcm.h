/* HOSTCM. Copyright (c) 2013, Robert Ferguson (rob@bitscience.ca) */
/* All Rights Reserved. See LICENCE for details */

#include <termios.h>
#include <stdio.h>

typedef struct _HCOpts {
	speed_t 	speed;
	tcflag_t 	parity;
	tcflag_t	stopb;
	unsigned char	lineend;
	unsigned char	prompt;
	unsigned char	response;
	unsigned char	ext;
} HCOpts;	

#define DIREXT	0x1

extern HCOpts opt;

#define LOGFILE	"hostcm.log"

extern void server(int fdin, int fdout);

extern int setterm(int fd, HCOpts *po);
extern int send(int fd, char *buf, int n);
extern int receive(int fd, char *buf, int n);
extern void xtob(char *p, int n);
extern void btox(char *p, int n);

extern int hostinit(char *in, int n, char *out);
extern int hostopen(char *in, int n, char *out);
extern int hostclose(char *in, int n, char *out);
extern int hostput(char *in, int n, char *out);
extern int hostget(char *in, int n, char *out);
extern int hostdiropen(char *in, int n, char *out);
extern int hostdirread(char *in, int n, char *out);
extern int hostdirclose(char *in, int n, char *out);
extern int hostrenamefm(char *in, int n, char *out);
extern int hostrenameto(char *in, int n, char *out);
extern int hostscratch(char *in, int n, char *out);
extern int hostseek(char *in, int n, char *out);
extern void hostquit(void);
extern int hostok(char *out);
extern int hosterror(char *out, char *s, ...);
extern char checksum(char *b, int n);

extern void dump(char *s, int n);
extern void error(char *s, ...);
extern void debug(char *s, ...);

extern FILE *errorfp;

#ifdef DEBUG
extern FILE *debugfp;
#endif


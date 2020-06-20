/* HOSTCM. Copyright (c) 2013, Robert Ferguson (rob@bitscience.ca) */
/* All Rights Reserved. See LICENCE for details */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <libgen.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "hostcm.h"

/* implementions for individual parts of the protocol */

/* state for open/close/read/write ops */

#define NFILES	16

typedef struct _fent {
	FILE 	*fp;
	char	mode;
	char	format;
	char 	type;
	int	rl;
} Fent;

static Fent ftab[NFILES];

/* 
 * the hostcm initialize syntax is
 * 	v<2-digit id>
 *
 * assuming that the digits represent the protocol version, 
 * we support version "80".
 *
 * Note that the file state must be preserved across multiple
 * hostinit() calls -- PIP relies on this.
 */

int
hostinit(char *in, int n, char *out)
{
	if (strncmp(&(in[1]), "80", 2) != 0) {
		return(hosterror(out, "invalid protocol version"));
	}

	return(hostok(out));
}


/*
 * the hostcm open syntax is
 * 	o<mode><format> (<type>[:<record-length>])<file-designator>
 * 
 * open format (different than file format) is either 't' or 'b', 
 *	for text or binary.
 * 
 * modes (per Waterloo openf_ docs) can be read, write, update, append, 
 *	load or store
 * 	(load/store appear to be synonyms of read/write for prg files)
 *
 * The modes are poorly documented, not sure of the distinction between 
 * 	write and update; perhaps write implies create, and update implies 
 * 	a pre-existing file
 *
 * 	(if so, should a write of a pre-existing file or update of a 
 * 	non-existent file fail?)
 *
 * 	presumably, append opens an existing file for writing with a seek 
 * 	to the end 
 * 
 * See 'digression.txt' for more details about file handling.
 * 
 * TODO: on open for create, we should add missing extensions. Thus,
 * 	add ",seq" by default, but add ",prg" for open mode "s",
 *	and add ",rel" to fixed files.
 */

int
hostopen(char *in, int n, char *out)
{
	int count = 0;
	int i;
	char *p, *bn, *dn;
	char *flags;
	char fname[MAXPATHLEN];  /* ROMS will choke on any host path longer than 32b */

	/* find an open slot */
	for (i = 0; ((i < NFILES) && (ftab[i].fp != 0)); i++)
		; 

	if (i >= NFILES) {
		return(hosterror(out, "Too many open files"));
	}

	/* do more to check the buffer before blithely indexing into it */

	if (n < 8) {
		return(hosterror(out, "protocol error (open): command too short"));
	}

	ftab[i].mode = in[1];
	ftab[i].format = in[2];

	if ((p = memchr(in, '(', n)) == NULL) {
		return(hosterror(out, "protocol error (open): missing '('"));
		exit(-1);
	}

	ftab[i].type = *(p+1);

	/* only fixed type can have a record length */

	if (*(p+2) == ':') {
		ftab[i].rl = atoi(p+3);
	} else {
		ftab[i].rl = 0;
	}

	if ((p = memchr(in, ')', n)) == NULL) {
		return(hosterror(out, "protocol error (open); missing ')'"));
	} 

	debug("mode = %c, type = %c, format = %c, rl = %d", ftab[i].mode, ftab[i].type, ftab[i].format, ftab[i].rl);

	/* null terminate the filename */

	in[n - 1] = 0;
	debug("filename = %s", p+1);

	/* 
	 * we encode the RL in the filename on the host (prepending (f:<rl>) 
	 * to the filename), and fail the open if the RL doesn't match 
	 */

	/* 
	 * This could be much more sophisticated (dynamically setting the RL to 
	 * stored value if unspecified in the open request, allowing opens if 
	 * the RLs don't match), but it's unclear (a) if those are the right 
	 * semantics and (b) if it's worth the effort.
	 */
	  
	if (ftab[i].type == 'f') {

		bn = basename(p+1);
		dn = dirname(p+1);

		/* if we are not provided a RL, we default to 80 */ 

		if (ftab[i].rl == 0) {
			 ftab[i].rl = 80;
		}

		snprintf(fname, sizeof(fname), "%s/(f:%d)%s", dn, ftab[i].rl, bn);
	} else {
		strcpy(fname, p+1);
	}

	/* map the Superpet open modes to Unix modes */

	switch (ftab[i].mode) {
	case ('r'):
	case ('l'):
		flags = "r";
		break;
	case ('w'):
	case ('s'):
		flags = "w";
		break;
	case ('u'):
		flags = "w+";
		break;
	case ('a'):
		flags = "a";
		break;
	default:
		return(hosterror(out, "protocol error (open): unknown file mode"));
		break;
	}

	/* try what we're given before adding missing format extensions */

	if ((ftab[i].fp = fopen(fname, flags)) == NULL) {

		/* don't have a format extention, add "seq" or "prg" and try again */

		if (strchr(basename(fname), ',') == NULL) {
			if (strlen(fname) + 5 < sizeof(fname)) {
				if ((ftab[i].mode == 'l') || (ftab[i].mode == 's')) {
					strcat(fname, ",prg");
				} else {
					if (ftab[i].type == 'f') {
						strcat(fname, ",rel");
					} else {
						strcat(fname, ",seq");	
					}
				}

				if ((ftab[i].fp = fopen(fname, flags)) == NULL) {
					return(hosterror(out, "open failed (%s): %s (format assumed)", fname, strerror(errno)));
				}
			} 
		} else { 
			return(hosterror(out, "open failed (%s): %s", fname, strerror(errno)));
		}
	}

	/* translate the file descriptor to ASCII to avoid protocol conflicts */	

	out[count++] = opt.response;
	out[count++] = 'b';
	out[count++] = 'A' + i; /* file descriptor */
	out[count++] = checksum(&out[1], 2);
	out[count++] = opt.lineend;
	out[count++] = opt.prompt;

	return(count);
}


/*
 * the hostcm close syntax is
 * 	c<fileid>
 */

int
hostclose(char *in, int n, char *out)
{
	int i;

	i = in[1] - 'A';
	if ((i >= NFILES) || (i < 0)) {
		return(hosterror(out, "protocol error (close): invalid fileid %c", in[1]));
	}
	
	if (ftab[i].fp != 0) {
		if (fclose(ftab[i].fp) != 0) {
			return(hosterror(out, "close failed: %s", strerror(errno)));
		}
		ftab[i].fp = 0;
	} else {
			return(hosterror(out, "can't close file %i: not open", i));
	}

	return(hostok(out));
}


/*
 * the hostcm put syntax is
 * 	p<fileid><EOR><data>
 *
 * sends the next line/record in the file; EOR is 'z' if this data 
 * concludes a record, 'n' otherwise.
 */

int
hostput(char *in, int n, char *out)
{
	int i;
	char eor;

	i = in[1] - 'A';
	eor = in[2];

	debug("put fileid = %d, EOR = %c, data = '%.*s'", i, eor, n - 4, &in[3]);

	if ((i >= NFILES) || (i < 0)) {
		return(hosterror(out, "protocol error (put): invalid fileid %d", i));
	}

	/* translate the data */

	if (ftab[i].format == 'b') {
		xtob(&in[3], n - 4);
		n = ((n - 4) / 2) + 4;
	}

	/* 
	 * add lineend back if at EOR in certain cases
	 */

	if (eor == 'z') {
		/* text files and variable rl files have opt.lineend to mark EOR */
		if ((ftab[i].format == 't') || (ftab[i].type != 'f')) {
			in[n-1] = opt.lineend; 
			n++;
		}
	} 

	if (fwrite(&in[3], sizeof(char), n - 4, ftab[i].fp) != (n - 4)) {
		return(hosterror(out, "write failed: %s", strerror(errno)));
	}

	return(hostok(out));
}


/*
 * the hostcm get syntax is
 * 	g<fileid>[l]
 *
 * requests the next line/record of the previously opened fileid. 
 * the suffix l indicates that the last record should be retransmitted. (TODO)
 */

/* 
 * SENDBUF is the size of the SuperPET in-memory hostcm buffer, less the 
 * overhead of the protocol; value is a guess derived from reverse engineered 
 * low memory layouts
 */

#define SENDBUF	(208/2 - 6)		

int
hostget(char *in, int n, char *out)
{
	int count = 0;
	int i;
	int pos, rdcount;
	char eor;

	i = in[1] - 'A';

	if ((i >= NFILES) || (i < 0)) {
		return(hosterror(out, "protocol error (get): invalid fileid %d", i));
	}

	if ((ftab[i].fp != 0) && ((ftab[i].mode == 'r') || (ftab[i].mode == 'u') || (ftab[i].mode = 'l'))) {

		out[count++] = opt.response;

		if (ftab[i].format == 't') {
			/* get to the next EOR or to eof */

			out[count++] = 'b';
			out[count++] = 'z';

			/* should this be "lineend" from HCOpts? */

			while ((fread(&(out[count]), 1, 1, ftab[i].fp) == 1) && 
			       (out[count] != 0xd) && (count < (SENDBUF - 2))) {
				count++;
			}

			if (ferror(ftab[i].fp)) {
				return(hosterror(out, "fgets failed: %s", strerror(errno)));
			} 

			if (feof(ftab[i].fp)) {
				out[1] = 'e';
				out[2] = checksum(&out[1], 1);
				count = 3;
			} else {
				if (count >= (SENDBUF - 2)) {
					out[2] = 'n';
				}
				out[count] = checksum(&out[1], count - 1);
				count++;
			}
		} else {
			if (ftab[i].format == 'b') {

				/* 
 				 * read binary data from the file, translate to 
				 * "ascii hex", construct the checksum, and send.
				 */
				rdcount = SENDBUF;
				eor = 'n';

				if (ftab[i].rl) {

					/* 
					 * fixed rl: if the record length is less than a 
					 * buffer, read a single record at a time; 
					 * otherwise break it up
					 */

					if (ftab[i].rl < SENDBUF) {
						rdcount = ftab[i].rl;
						eor = 'z';
					} else {
						/*
						 * if the next read crosses a record boundry, 
						 * limit it to the end of the record, and set the EOR
					   	 * flag; otherwise, just read as much as we can, and
						 * indicate that there's more to come.
						 */

						if ((pos = ftell(ftab[i].fp)) == -1) {
							hosterror(out,"ftell fileid %d failed: %s", i, strerror(errno));
						}

						if ((pos % ftab[i].rl) > ((pos + rdcount) % ftab[i].rl)) {
							rdcount = ((pos + rdcount) / ftab[i].rl) * ftab[i].rl - pos;
							eor = 'z';
						} 
					}
				} 
				
				debug("reading %d chars fileid %d; rl = %d, eor = %c", rdcount, i, ftab[i].rl, eor);

				n = fread(&(out[3]), sizeof(char), rdcount, ftab[i].fp);

				if (n == 0) {
					if (ferror(ftab[i].fp)) {
						return(hosterror(out, "fread failed: %s", strerror(errno)));
					} 

					if (feof(ftab[i].fp)) {
						out[count++] = 'e';
						out[count] = checksum(&out[1], count - 1);
						count++;
					}
				} else {

					out[count++] = 'b';
					out[count++] = eor;
					btox(&out[count], n);
					count += n * 2;
					out[count] = checksum(&out[1], count - 1);
					count++;
				}
			}
		}

		out[count++] = opt.lineend;
		out[count++] = opt.prompt;

	} else {
		return(hosterror(out, "file not open, or not opened for read"));
	}

	return(count);
}


/* used to hold state for hostdir* operations */

static DIR *dirp = NULL;

/*
 * the hostcm "open directory" syntax is
 * 	d[<filename>]
 * 
 * Since there is no id returned, only a single directory can be 
 * open at any one time; works in conjunction with "directory read" 
 * and "directory close" below.
 * 
 * As an option, automatically chdir() to a directory when the name
 * ends with a '/'.
 */

int	
hostdiropen(char *in, int n, char *out)
{
	if (dirp != NULL) {
		return(hosterror(out, "protocol error (diropen): directory already open"));
	}

	if (n > 2) {	
		in[n - 1] = 0;
		debug("filename = %s", &in[1]);
	} else {
		/* current directory */
		strcpy(&in[1], ".");
		n++;
		debug("filename omitted, presuming '.'");
	}

	if ((dirp = opendir(&in[1])) == NULL) {
		return(hosterror(out, "Can't open directory %s: %s", &in[1], strerror(errno)));
	}

	/* if DIREXT and the directory name ends in a '/', chdir to it */

	if ((opt.ext & DIREXT) && (in[n-2] == '/')) {
		debug("Change directory to %s", &in[1]);
		if (chdir(&in[1]) == -1) {
			return(hosterror(out, "Can't chdir to directory %s: %s", 
				&in[1], strerror(errno)));
		}
	}

	return(hostok(out));
}


/*
 * the hostcm "read directory" syntax is
 * 	f
 * 
 * returns a string representing the next directory entry.
 * Format is not defined -- unclear if you're
 * supposed to be able to "open" the returned value as a 
 * filename, or whether it's supposed to represent a directory 
 * entry in the format of a "dir". 
 * ROMs do the latter, so do we.
 *
 * Set EOF at end of directory.
 */

			
int
hostdirread(char *in, int n, char *out)
{
	int count = 0;
	int rl = 0;
	struct dirent *dp;
	char *fname, *p, *pcwd;
	struct stat sb;
	

	if (dirp == NULL) {
		return(hosterror(out, "protocol error (dirread): no open directory"));
	}

	out[count++] = opt.response;

	/* don't return '.' or '..' */

	while (((dp = readdir(dirp)) != NULL) && (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")))
		;

	if (dp == NULL) {
		out[count++] = 'e';
	} else {
		/* remove '(f:rl)' from filenames before returning them; note the rl if present */

		fname = dp->d_name;
	
		if (strncmp("(f:", dp->d_name, 3) == 0) {
			if ((p = strchr(dp->d_name, ')')) != NULL) {
				fname = p + 1;
				rl = atoi(&dp->d_name[3]);
			} 
		}

		out[count++] = 'b';

		/* push cwd */	

		if ((pcwd = getcwd(NULL, 0)) == NULL) {
			return(hosterror(out, "Can't get working directory: %s", strerror(errno)));
		}

		if (fchdir(dirfd(dirp)) == -1) {
			free(pcwd);
			return(hosterror(out, "Can't fchdir to requested directory: %s", strerror(errno)));
		}

		if (stat(dp->d_name, &sb) == -1) {
			chdir(pcwd);
			free(pcwd);
			return(hosterror(out, "Can't stat %s: %s", dp->d_name, strerror(errno)));
		}

		/* pop cwd */

		if (chdir(pcwd) == -1) {
			free(pcwd);
			return(hosterror(out, "Can't chdir to working directory: %s", strerror(errno)));
		}

		if (rl) {
			count += snprintf(&out[count], BUFSIZ - count, "%-20s  %8llu (%d)", 
					fname, (unsigned long long)sb.st_size, rl);
		} else {
			if (sb.st_mode & S_IFDIR) {
				count += snprintf(&out[count], BUFSIZ - count, "%s/", fname);
			} else {
				count += snprintf(&out[count], BUFSIZ - count, "%-20s  %8llu", 
						fname, (unsigned long long)sb.st_size);
			}
		}
	}

	out[count] = checksum(&(out[1]), count - 1);
	count++;
	out[count++] = opt.lineend;
	out[count++] = opt.prompt;

	return(count);
}
	
/*
 * the hostcm "close directory" syntax is
 * 	k
 */

int
hostdirclose(char *in, int n, char *out)
{
	if (dirp == NULL) {
		return(hosterror(out, "protocol error (dirclose): no directory open"));
	}

	if (closedir(dirp) == -1) {
		return(hosterror(out, "failed to close directory: %s", strerror(errno)));
	}

	dirp = NULL;

	return(hostok(out));
}
	

/* state for hostrename* ops */

static char *pfn = NULL;

/*
 * the hostcm "rename from" syntax is
 * 	w<filename>
 *
 * We just store away the filename, since we'll have to wait for 
 * the "rename to" command to do the work. 
 * Check for errors now, or do it later?
 */

int
hostrenamefm(char *in, int n, char *out)
{
	if (pfn != NULL) {
		return(hosterror(out, "protocol error (renamefm): previous rename pending"));
	}

	if (n > 2) {
		in[n - 1] = 0;
		debug("filename = %s", &in[1]);
		pfn = strdup(&in[1]);
	} else {
		return(hosterror(out, "missing source filename for rename"));
	}

	return(hostok(out));
}


/*
 * the hostcm "rename to" syntax is
 * 	b<filename>
 *
 * Depends on a pending "rename from". Arguably shouldn't allow 
 * any intervening commands.
 */

int
hostrenameto(char *in, int n, char *out)
{
	if (pfn == NULL) {
		return(hosterror(out, "protocol error (renameto): no rename pending"));
	}

	if (n > 2) {
		in[n - 1] = 0;
		debug("filename = %s", &in[1]);
	} else {
		return(hosterror(out, "Missing destinationfilename for rename"));
	}

	if (rename(pfn, &in[1]) != 0) {
		return(hosterror(out, "rename (%s to %s) failed", pfn, &in[1], strerror(errno)));
	} else {
		free(pfn);
		pfn = NULL;
	}

	return(hostok(out));
}
	

/*
 * the hostcm "scratch" syntax is
 * 	y<filename>
 */

int
hostscratch(char *in, int n, char *out)
{
	if (n > 2) {
		in[n - 1] = 0;
		debug("filename = %s", &in[1]);
	} else {
		return(hosterror(out, "missing filename to scratch"));
	}

	if (unlink(&in[1]) != 0) {
		return(hosterror(out, "Can't delete file %s: %s", &in[1], strerror(errno)));
	}

	return(hostok(out));
}			
			

/* 
 * Does this make sense for files without 'fixed' type? Not clear from 
 * the minimal docs. Assume offset is record number for files with rl != 0, 
 * byte offset otherwise; implicitly limits byte seeks to a 16bit offset.
 * 
 * Also assume that the offset is meant to be absolute, not relative. 
 * 
 * Whole thing is mostly untested.
 */

/*
 * the hostcm "seek" syntax is
 * 	r<fileid><seekoffset>
 */


int
hostseek(char *in, int n, char *out)
{
	int i;
	long offset;

	i = in[1] - 'A';

	if ((i >= NFILES) || (i < 0)) {
		return(hosterror(out, "protocol error (seek): invalid fileid %d", i));
	}

	if (ftab[i].fp == 0) {
		return(hosterror(out, "protocol error (seek): fileid %d not open", i));
	}
	
	/* null terminate the string representing the offset */
	
	in[n - 1] = 0;
	debug("offset = %s", &in[2]);
	
	offset = atoi(&in[2]) * ((ftab[i].rl == 0)? 1 : ftab[i].rl);

	if (fseek(ftab[i].fp, offset, SEEK_SET) == -1) {
		return(hosterror(out, "seek to offset %ld failed for fileid %d: %s", offset, i, strerror(errno)));
	}

	return(hostok(out));
}

/*
 * the hostcm "quit" syntax is
 * 	q
 *
 * Only see this in "interactive" mode; clean up file table.
 */

void
hostquit(void)
{
	int i;

	for (i = 0; i < NFILES; i++) {
		if (ftab[i].fp && (fclose(ftab[i].fp) == EOF)) {
			error("Can't close file %d", i);
		}
	}
}

/* send OK response to client */

int
hostok(char *out)
{
	int count = 0;
	
	out[count++] = opt.response;
	out[count++] = 'b';
	out[count++] = checksum(&(out[1]), 1);
	out[count++] = opt.lineend;
	out[count++] = opt.prompt;

	return(count);
}


/* send error response to client */

int
hosterror(char *out, char *s, ...)
{
	int count = 0;
	va_list ap;

	out[count++] = opt.response;
	out[count++] = 'x'; /* anything but 'b' or 'e' will do it */

	/* 
 	 * In the ROM, the returned error message gets copied into the 
	 * SuperPET's "error message buffer" which starts at 0x300.
 	 *
	 * There doesn't appear to be any bounds checking in the ROM, 
	 * so the question is how big is the buffer? It's definitely smaller 
	 * than 0x100, since the "line buffer" starts at 0x400. 
 	 *
 	 * Be conservative and say ... 80 bytes, since that's the screen size. 
	 * Might truncate some messages.
	 */

	va_start(ap, s);
	count += vsnprintf(&out[2], 80,  s, ap); 
	va_end(ap);

	out[count] = checksum(&out[1], count - 1);
	count++;
	out[count++] = opt.lineend;
	out[count++] = opt.prompt;

	return(count);
}

SRCS = main.c term.c error.c server.c xtob.c protocol.c checksum.c
OBJS = $(SRCS:.c=.o)

# adjust as appropriate. 
# -O4 is clang specific; UNIX_LIKE indicates support for tty ioctls and non-POSIX termios extensions.

#CFLAGS = -g -Wall -DDEBUG -DUNIX_LIKE
CFLAGS = -O -Wall -DUNIX_LIKE

all: hostcm

clean:
	rm $(OBJS) hostcm

hostcm: $(OBJS)
	$(CC) $(CFLAGS) -o hostcm $(OBJS)

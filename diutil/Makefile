SRCS = di_dirent.c diskimage.c error.c 
OBJS = $(SRCS:.c=.o)

# adjust as appropriate. 
# -O4 is clang specific; UNIX_LIKE indicates support for tty ioctls and non-POSIX termios extensions.

# CFLAGS = -DDEBUG -g
CFLAGS = -O4 

all: diextract didir

clean:
	-rm $(OBJS) diexpand.o didir.o diextract.o diextract didir

diextract: $(OBJS) diextract.o
	$(CC) $(CFLAGS) -o diextract diextract.o $(OBJS)

didir: $(OBJS) didir.o
	$(CC) $(CFLAGS) -o didir didir.o $(OBJS)

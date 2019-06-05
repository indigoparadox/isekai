
# vim: ft=make noexpandtab

SOURCES=\
	action.c \
	animate.c \
	audition.c \
	b64.c \
	backlog.c \
	callback.c \
	channel.c \
	chunker.c \
	client.c \
	datafile.c \
	ezxml.c \
	files.c \
	main.c \
	mobile.c \
	plugin.c \
	rng.c \
	savestat.c \
	scaffold.c \
	server.c \
	tilemap.c \
	twindow.c \
	vm.c \
	proto/irc.c \
	ipc/netipc.c \
	hs/hsdecode.c \
	hs/hsencode.c \
	storage/sqlite.c \
	datafile/xml.c
CHECKOBJ=
OBJECTS=$(subst .c,.o,$(SOURCES))
INCDIRS= \
	-I../../libvcol/src \
	-I../../camgraph/src \
	-I../../camin/src \
	-I../../libtwin/src
CFLAGS=-Wall -pg -g \
   -ggdb3 \
   -std=gnu99 \
   -fstack-protector-all \
   -fsanitize=undefined \
   -fvar-tracking \
   -DUSE_ALLEGRO \
   -DDEBUG \
   -DUSE_NETWORK \
   -DUSE_FILE_CACHE \
   -DUSE_CHUNKS \
   -DUSE_RANDOM_PORT \
   -DUSE_CONNECT_DIALOG \
   -DDEBUG_TILES \
   -DUSE_COLORED_CONSOLE \
   -DDEBUG_FPS \
   -DENABLE_LOCAL_CLIENT \
   -DSTRICT_DEBUG \
   -DDEBUG_NETWORK \
   -DUSE_SQLITE \
	-DUSE_EZXML \
	$(INCDIRS)
LIBDIRS= \
	-L../../libvcol/bin/Debug-Linux \
	-L../../camgraph/bin/Debug-Linux-Allegro \
	-L../../camin/bin/Debug-Linux-Allegro \
	-L../../libtwin/bin/Debug-Linux-Allegro
LIBS=-lsqlite3 -lvcol -ltwin -lcamgraph -lcamin -lm -ubsan -ldl
LDFLAGS= -pg -export-dynamic $(LIBS) $(LIBDIRS) -fsanitize=undefined

BIN_LIN=../bin/Debug/isekai-Allegro-NET

MD=mkdir -v -p

$(BIN_LIN): CFLAGS += -g -DDEBUG
$(BIN_LIN): LDFLAGS += $(shell pkg-config --libs allegro)
$(BIN_LIN): OBJDIR := ../obj/Debug-Linux-NET-Allegro4

all: $(BIN_LIN)

$(BIN_LIN): $(OBJECTS)
	$(MD) ../bin/Debug
	$(CC) -o $@ $(addprefix $(OBJDIR)/,$^) $(LDFLAGS)

%.o: %.c
	$(MD) $(OBJDIR)/$(dir $@)
	$(CC) -c -o $(OBJDIR)/$@ $< $(CFLAGS)

.PHONY: clean

clean:
	rm -rf ../obj; \
	rm tests; \
	rm $(BIN_LIN);




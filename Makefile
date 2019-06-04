CC = m68k-palmos-gcc
CFLAGS = -O2
SOURCES=./src/animate.c ./src/backlog.c ./src/bstrglue.c ./src/callback.c ./src/channel.c ./src/bstrlib/bstrlib.c ./src/client.c ./src/datafile.c ./src/datafile/xml.c ./src/ezxml.c ./src/graphics.c ./src/graphics/nullg.c ./src/hashmap.c ./src/input/nulli.c ./src/ipc/shmipc.c ./src/item.c ./src/main.c ./src/mem.c ./src/mobile.c ./src/proto/shmp.c ./src/rng.c ./src/scaffold.c ./src/server.c ./src/tilemap.c ./src/ui/graphicu.c ./src/vector.c
OBJECTS=$(patsubst ./src/%.c, ./src/%.o, $(SOURCES)) procircd-sections.o
OBJECTS_DOWN=$(patsubst ./src/%.c, ./%.o, \
	$(patsubst ./src/bstrlib/%.c, ./%.o, \
	$(patsubst ./src/datafile/%.c, ./%.o, \
	$(patsubst ./src/graphics/%.c, ./%.o, \
	$(patsubst ./src/input/%.c, ./%.o, \
	$(patsubst ./src/ipc/%.c, ./%.o, \
	$(patsubst ./src/ui/%.c, ./%.o, \
	$(patsubst ./src/proto/%.c, ./%.o, $(SOURCES)))))))))
TARGET_PRC=procircd.prc

all: $(TARGET_PRC)

$(TARGET_PRC): procircd procircd.def bin.stamp
	build-prc procircd.def -o $(TARGET_PRC) "ProCIRCd" TuHe procircd *.bin

procircd: $(OBJECTS) procircd-sections.ld
	$(CC) $(CFLAGS) -o $@ $(OBJECTS_DOWN) $(LDFLAGS) procircd-sections.ld

procircd-sections.o: procircd-sections.s
	$(CC) -o procircd-sections.o -c procircd-sections.s

procircd-sections.s: procircd.def
	m68k-palmos-multigen procircd.def

%.o: %.c
	$(CC) $(CFLAGS) -c $<

bin.stamp: procircd.rcp ./src/palmos.h hello.bmp
	pilrc procircd.rcp

clean:
	-rm -f *.[oa] procircd *.bin *.stamp $(TARGET_PRC)


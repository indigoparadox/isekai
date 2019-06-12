
# vim: ft=make noexpandtab

INCDIRS= \
	-I../../src \
	-I../../../libvcol/src \
	-I../../../camgraph/src \
	-I../../../camin/src \
	-I../../../libtwin/src
CFLAGS=-Wall \
	-fPIC \
	$(INCDIRS)
LDFLAGS=-shared

MD=mkdir -v -p

OBJDIR := ../../obj/Plugins-Debug
BINDIR := ../../plugins/bin/mode

all: $(BINDIR)/libtopdown.so

$(BINDIR)/libtopdown.so: topdown.o
	$(MD) $(BINDIR)
	$(CC) -o $@ $(addprefix $(OBJDIR)/,$^) $(LDFLAGS)

%.o: %.c
	$(MD) $(OBJDIR)
	$(CC) -c -o $(OBJDIR)/$@ $< $(CFLAGS)

.PHONY: clean

clean:
	rm -rf ../obj; \
	rm tests; \
	rm $(BIN_LIN);




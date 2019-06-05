
# vim: ft=make noexpandtab

SOURCES=topdown.c
OBJECTS=$(subst .c,.o,$(SOURCES))
INCDIRS= \
	-I../../src \
	-I../../../libvcol/src \
	-I../../../camgraph/src \
	-I../../../camin/src \
	-I../../../libtwin/src
CFLAGS=-Wall -pg -g \
   -ggdb3 \
   -std=gnu99 \
	-fPIC \
   -fstack-protector-all \
   -fsanitize=undefined \
   -fvar-tracking \
	$(INCDIRS)
LDFLAGS=-shared -g -pg -fsanitize=undefined

MD=mkdir -v -p

BIN_LIN=../bin/Debug/mode/topdown.so

$(BIN_LIN): CFLAGS += -g -DDEBUG
$(BIN_LIN): LDFLAGS += $(shell pkg-config --libs allegro)
$(BIN_LIN): OBJDIR := ../obj/Debug

all: $(BIN_LIN)

$(BIN_LIN): $(OBJECTS)
	$(MD) ../bin/Debug/mode
	$(CC) -o $@ $(addprefix $(OBJDIR)/,$^) $(LDFLAGS)

%.o: %.c
	$(MD) $(OBJDIR)/$(dir $@)
	$(CC) -c -o $(OBJDIR)/$@ $< $(CFLAGS)

.PHONY: clean

clean:
	rm -rf ../obj; \
	rm tests; \
	rm $(BIN_LIN);




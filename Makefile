TARGET := read_lhalotree
SRC := main.c read_lhalotree.c utils.c 
OBJS :=  $(SRC:.c=.o)
INCL := read_lhalotree.h datatype.h utils.h

# Compiling to object
CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -Wshadow -fno-strict-aliasing -g -D_POSIX_SOURCE=200809L -D_GNU_SOURCE -D_DARWIN_C_SOURCE 
CFLAGS  += -Wformat=2  -Wpacked  -Wnested-externs -Wpointer-arith  -Wredundant-decls  -Wfloat-equal -Wcast-qual -Wpadded
CFLAGS  +=  -Wcast-align -Wmissing-declarations -Wmissing-prototypes
CFLAGS  += -Wnested-externs -Wstrict-prototypes 

OPTIMIZE := -O3 -march=native

# Linking (remove -lrt, real-time library flag, on OSX)
LDFLAGS := -lrt

%.o: %.c Makefile $(INCL)
	$(CC) $(OPTIMIZE) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS) 
	$(CC) $^ -o $@ $(LDFLAGS)

.phony: clean celan celna

clena: clean
celna: clean
celan: clean
clean:
	$(RM) $(OBJS) $(TARGET)


TARGET := read_lhalotree
SRC := read_lhalotree.c
OBJS :=  $(SRC:.c=.o)
INCL := datatype.h

# Compiling to object
CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -Wshadow -fno-strict-aliasing -g
CFLAGS  += -Wformat=2  -Wpacked  -Wnested-externs -Wpointer-arith  -Wredundant-decls  -Wfloat-equal -Wcast-qual -Wpadded
CFLAGS  +=  -Wcast-align -Wmissing-declarations -Wmissing-prototypes
CFLAGS  += -Wnested-externs -Wstrict-prototypes 

# Linking
LDFLAGS := 

%.o: %.c Makefile $(INCL)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS) 
	$(CC) $^ -o $@ $(LDFLAGS)

.phony: clean celan celna

clena: clean
celna: clean
celan: clean
clean:
	$(RM) $(OBJS) $(TARGET)


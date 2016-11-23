ROOT_DIR := ..
TARGET := find_flybys
SRC := main-flybys.c $(ROOT_DIR)/read_lhalotree.c $(ROOT_DIR)/utils.c $(ROOT_DIR)/progressbar.c
OBJS :=  $(SRC:.c=.o)
INCL := $(ROOT_DIR)/read_lhalotree.h $(ROOT_DIR)/utils.h $(ROOT_DIR)/sglib.h $(ROOT_DIR)/progressbar.h $(ROOT_DIR)/lhalotree.h

INCLUDE := -I$(ROOT_DIR)
# Compiling to object
CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -Wshadow -fno-strict-aliasing -g -D_POSIX_SOURCE=200809L -D_GNU_SOURCE -D_DARWIN_C_SOURCE 
CFLAGS  += -Wformat=2  -Wpacked  -Wnested-externs -Wpointer-arith  -Wredundant-decls  -Wfloat-equal -Wcast-qual -Wpadded
CFLAGS  +=  -Wcast-align -Wmissing-declarations -Wmissing-prototypes
CFLAGS  += -Wnested-externs -Wstrict-prototypes 

OPTIMIZE := -O3 -march=native

# Linking (remove -lrt, real-time library flag, on OSX)
LDFLAGS := -lrt -lm

all: $(TARGET) $(SRC) $(OBJS) $(INCL) Makefile

%.o: %.c $(INCL)
	$(CC) $(OPTIMIZE) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

.phony: clean celan celna

clena: clean
celna: clean
celan: clean
clean:
	$(RM) $(OBJS) $(TARGET)


CC = gcc
CFLAGS = -Wall -std=c99 -O2 -I. -D_GNU_SOURCE
LIBNAME = libpscm.a

SRCS = value.c vm.c reader.c builtin.c json.c api.c
HEADERS = value.h vm.h reader.h builtin.h json.h api.h pscm.h
OBJS = $(SRCS:.c=.o)

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	ar rcs $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIBNAME)

.PHONY: all clean

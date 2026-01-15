CC = gcc
CFLAGS = -Wall -std=c99 -O2 -I. -D_GNU_SOURCE
LIBNAME = libpscm.a

SRCS = value.c vm.c reader.c builtin.c json.c api.c
HEADERS = value.h vm.h reader.h json.h api.h pscm.h
OBJS = $(SRCS:.c=.o)

all: $(LIBNAME) pscm

$(LIBNAME): $(OBJS)
	ar rcs $@ $^

pscm: main.o $(LIBNAME)
	$(CC) $(CFLAGS) main.o -o pscm -L. -lpscm

main.o: main.c $(HEADERS)
	$(CC) $(CFLAGS) -c main.c -o main.o

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) main.o $(LIBNAME) pscm

.PHONY: all clean

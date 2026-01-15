CC = gcc
CFLAGS = -Wall -std=c99 -O2 -Isrc -D_GNU_SOURCE
LIBNAME = libpscm.a

SRCS = value.c vm.c reader.c builtin.c json.c api.c
HEADERS = value.h vm.h reader.h json.h api.h pscm.h
OBJS = $(SRCS:.c=.o)
VPATH = src

all: $(LIBNAME) pscm pscm-format

$(LIBNAME): $(OBJS)
	ar rcs $@ $^

pscm: main.o $(LIBNAME)
	$(CC) $(CFLAGS) main.o -o pscm -L. -lpscm

pscm-format: pscm-format.o $(LIBNAME)
	$(CC) $(CFLAGS) pscm-format.o -o pscm-format -L. -lpscm

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

pscm-format.o: pscm-format.c
	$(CC) $(CFLAGS) -c pscm-format.c -o pscm-format.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) main.o $(LIBNAME) pscm

.PHONY: all clean
CC = gcc
CFLAGS = -Wall -std=c99 -O2 -I. -D_GNU_SOURCE
LIBNAME = libpscm.a

SRCS = value.c vm.c reader.c builtin.c json.c api.c
HEADERS = value.h vm.h reader.h json.h api.h pscm.h
OBJS = $(SRCS:.c=.o)

all: $(LIBNAME) pscm pscm-format

$(LIBNAME): $(OBJS)
	ar rcs $@ $^

pscm: main.o $(LIBNAME)
	$(CC) $(CFLAGS) main.o -o pscm -L. -lpscm

pscm-format: pscm-format.o $(LIBNAME)
	$(CC) $(CFLAGS) pscm-format.o -o pscm-format -L. -lpscm

main.o: main.c $(HEADERS)
	$(CC) $(CFLAGS) -c main.c -o main.o

pscm-format.o: pscm-format.c $(HEADERS)
	$(CC) $(CFLAGS) -c pscm-format.c -o pscm-format.o

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) main.o $(LIBNAME) pscm

.PHONY: all clean

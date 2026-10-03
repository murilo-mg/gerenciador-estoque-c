CC = gcc
CPPFLAGS = -I.
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS =
LDLIBS =
AVISOS = -std=c11 -Wall -Wextra -Wpedantic
SANITIZADORES = -fsanitize=address,undefined -fno-omit-frame-pointer
FONTES = main.c produto.c armazenamento.c
CABECALHOS = produto.h armazenamento.h
OBJETOS = $(FONTES:%.c=build/%.o)

.PHONY: all run test clean debug

all: sistema_estoque

sistema_estoque: $(OBJETOS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: %.c $(CABECALHOS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build:
	mkdir -p $@

run: sistema_estoque
	./sistema_estoque

build/testes: testes.c produto.c armazenamento.c $(CABECALHOS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG $(LDFLAGS) testes.c produto.c armazenamento.c $(LDLIBS) -o $@

test: build/testes
	cd build && ./testes

build/debug:
	mkdir -p $@

build/debug/sistema_estoque: $(FONTES) $(CABECALHOS) | build/debug
	$(CC) $(CPPFLAGS) $(AVISOS) -g -O1 $(SANITIZADORES) $(FONTES) -o $@

build/debug/testes: testes.c produto.c armazenamento.c $(CABECALHOS) | build/debug
	$(CC) $(CPPFLAGS) $(AVISOS) -g -O1 -UNDEBUG $(SANITIZADORES) testes.c produto.c armazenamento.c -o $@

debug: build/debug/sistema_estoque build/debug/testes
	cd build/debug && ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./testes

clean:
	$(RM) -r build sistema_estoque

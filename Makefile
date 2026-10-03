CC = gcc
CPPFLAGS = -Isrc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS =
LDLIBS =
AVISOS = -std=c11 -Wall -Wextra -Wpedantic
SANITIZADORES = -fsanitize=address,undefined -fno-omit-frame-pointer
FONTES = src/main.c src/produto.c src/armazenamento.c
CABECALHOS = src/produto.h src/armazenamento.h
OBJETOS = $(FONTES:src/%.c=build/%.o)

.PHONY: all run test clean debug

all: sistema_estoque

sistema_estoque: $(OBJETOS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: src/%.c $(CABECALHOS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build:
	mkdir -p $@

run: sistema_estoque
	./sistema_estoque

build/testes: tests/testes.c src/produto.c src/armazenamento.c $(CABECALHOS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG $(LDFLAGS) tests/testes.c src/produto.c src/armazenamento.c $(LDLIBS) -o $@

build/testes_interacao: tests/testes_interacao.c $(FONTES) $(CABECALHOS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG $(LDFLAGS) tests/testes_interacao.c src/produto.c src/armazenamento.c $(LDLIBS) -o $@

build/testes_memoria: tests/testes_memoria.c src/produto.c src/armazenamento.c $(CABECALHOS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG $(LDFLAGS) tests/testes_memoria.c $(LDLIBS) -o $@

test: build/testes build/testes_interacao build/testes_memoria
	cd build && ./testes
	cd build && ./testes_interacao
	cd build && ./testes_memoria

build/debug:
	mkdir -p $@

build/debug/sistema_estoque: $(FONTES) $(CABECALHOS) | build/debug
	$(CC) $(CPPFLAGS) $(AVISOS) -g -O1 $(SANITIZADORES) $(FONTES) -o $@

build/debug/testes: tests/testes.c src/produto.c src/armazenamento.c $(CABECALHOS) | build/debug
	$(CC) $(CPPFLAGS) $(AVISOS) -g -O1 -UNDEBUG $(SANITIZADORES) tests/testes.c src/produto.c src/armazenamento.c -o $@

build/debug/testes_interacao: tests/testes_interacao.c $(FONTES) $(CABECALHOS) | build/debug
	$(CC) $(CPPFLAGS) $(AVISOS) -g -O1 -UNDEBUG $(SANITIZADORES) tests/testes_interacao.c src/produto.c src/armazenamento.c -o $@

build/debug/testes_memoria: tests/testes_memoria.c src/produto.c src/armazenamento.c $(CABECALHOS) | build/debug
	$(CC) $(CPPFLAGS) $(AVISOS) -g -O1 -UNDEBUG $(SANITIZADORES) tests/testes_memoria.c -o $@

debug: build/debug/sistema_estoque build/debug/testes build/debug/testes_interacao build/debug/testes_memoria
	cd build/debug && ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./testes
	cd build/debug && ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./testes_interacao
	cd build/debug && ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./testes_memoria

clean:
	$(RM) -r build sistema_estoque

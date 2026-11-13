# TechFix Manager - Makefile para validação em Linux/macOS com SQLite instalado
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O1 -Iinclude
LIBS = -lsqlite3 -lpthread -ldl -lm

COMUM = src/util/erro.c src/util/validacao.c src/util/io.c src/util/sha256.c \
        src/regras/regras.c src/regras/seguranca.c \
        src/dados/db.c src/dados/esquema.c src/dados/cadastros.c \
        src/dados/estoque.c src/dados/ordens.c src/dados/relatorios.c
INTERFACE = src/ui/ui_comum.c src/ui/menu_cadastros.c src/ui/menu_ordens.c \
            src/ui/menu_estoque.c src/ui/menu_admin.c src/main.c

.PHONY: all testes executar clean
all: techfix

techfix: $(COMUM) $(INTERFACE)
	$(CC) $(CFLAGS) $(COMUM) $(INTERFACE) -o $@ $(LIBS)

testes_techfix: tests/testes.c $(COMUM)
	$(CC) $(CFLAGS) tests/testes.c $(COMUM) -o $@ $(LIBS)

testes: testes_techfix
	./testes_techfix

executar: techfix
	./techfix

clean:
	rm -f techfix testes_techfix techfix.db relatorio_os.csv estoque.csv

/*
* db.h — Conexão com o banco de dados, transações e funções auxiliares.
*
* É o único módulo que conhece os detalhes da conexão com o SQLite.
* Trocar de SGBD (decisão D01) exige reescrever este módulo e o SQL dos
* arquivos da pasta src/dados, sem mexer na interface nem nas regras puras.
*/
#ifndef DB_H
#define DB_H

#include <stddef.h>
#include "sqlite3.h"
#include "erro.h"

/* Abre (ou cria) o arquivo do banco, liga as chaves estrangeiras e cria as
tabelas que ainda não existem. Use ":memory:" para um banco temporário. */
Resultado db_abrir(const char *arquivo);

/* Fecha a conexão. Pode ser chamada mesmo se o banco não estiver aberto. */
void db_fechar(void);

/* Conexão aberta (para uso interno da camada de dados). */
sqlite3 *db_conexao(void);

/* Texto completo do esquema: cópia de sql/schema.sql (gerada em src/dados/esquema.c). */
extern const char *ESQUEMA_SQL;

/* Executa um ou mais comandos SQL sem parâmetros. */
Resultado db_executar(const char *sql);

/* Prepara um comando com parâmetros "?". Quem chama deve finalizar com sqlite3_finalize(). */
Resultado db_preparar(const char *sql, sqlite3_stmt **st);

/* Converte o código devolvido por sqlite3_step() em Resultado e registra uma
mensagem compreensível (ex.: campo único repetido vira RES_DUPLICADO). */
Resultado db_falha(int codigo, const char *contexto);

/*
* Transações: "tudo ou nada". Entre iniciar e confirmar, ou todas as
* gravações acontecem, ou nenhuma. Podem ser aninhadas: só a transação mais
* externa grava de fato, e uma falha interna desfaz tudo.
*/
Resultado db_iniciar_transacao(void);
Resultado db_confirmar(void);
void db_desfazer(void);

/* Copia uma coluna de texto do resultado. NULL vira texto vazio. */
void db_coluna_texto(sqlite3_stmt *st, int coluna, char *destino, size_t tamanho);

/* Id gerado pelo último INSERT. */
long long db_ultimo_id(void);

/* Quantidade de linhas alteradas pelo último INSERT, UPDATE ou DELETE. */
int db_linhas_alteradas(void);

/* Executa uma consulta com um parâmetro inteiro que devolve um único número
(ex.: COUNT). Devolve "padrao" se não houver resultado. */
long long db_consultar_inteiro(const char *sql, long long parametro, long long padrao);

#endif

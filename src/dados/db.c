/*
* db.c — Conexão com o SQLite, criação do esquema e transações.
*/
#include <stdio.h>
#include "db.h"
#include "validacao.h"

static sqlite3 *conexao = NULL;
static int profundidade = 0; /* transações abertas */
static int desfazer_pendente = 0; /* uma transação interna falhou */

Resultado db_abrir(const char *arquivo)
{
if (sqlite3_open(arquivo, &conexao) != SQLITE_OK) {
erro_definir("Não foi possível abrir o banco '%s': %s", arquivo,
conexao ? sqlite3_errmsg(conexao) : "sem memória");
sqlite3_close(conexao);
conexao = NULL;
return RES_BANCO;
}
sqlite3_busy_timeout(conexao, 3000);
/* Chaves estrangeiras precisam ser ligadas em cada conexão no SQLite. */
if (db_executar("PRAGMA foreign_keys = ON;") != RES_OK) return RES_BANCO;
if (db_executar(ESQUEMA_SQL) != RES_OK) return RES_BANCO;
return RES_OK;
}

/* Fecha a conexão e zera o controle de transações. */
void db_fechar(void)
{
if (conexao != NULL) {
sqlite3_close(conexao);
conexao = NULL;
}
profundidade = 0;
desfazer_pendente = 0;
}

/* Devolve a conexão aberta. */
sqlite3 *db_conexao(void)
{
return conexao;
}

/* Executa SQL sem parâmetros; em caso de erro, registra a mensagem do SQLite. */
Resultado db_executar(const char *sql)
{
char *msg = NULL;
if (sqlite3_exec(conexao, sql, NULL, NULL, &msg) != SQLITE_OK) {
erro_definir("Erro no banco de dados: %s", msg ? msg : "desconhecido");
sqlite3_free(msg);
return RES_BANCO;
}
return RES_OK;
}

/* Compila um comando SQL com parâmetros "?". */
Resultado db_preparar(const char *sql, sqlite3_stmt **st)
{
if (sqlite3_prepare_v2(conexao, sql, -1, st, NULL) != SQLITE_OK) {
erro_definir("Erro ao preparar consulta: %s", sqlite3_errmsg(conexao));
*st = NULL;
return RES_BANCO;
}
return RES_OK;
}

/* Traduz o código do SQLite em Resultado e mensagem para o usuário. */
Resultado db_falha(int codigo, const char *contexto)
{
int estendido = sqlite3_extended_errcode(conexao);

if (codigo == SQLITE_DONE || codigo == SQLITE_ROW || codigo == SQLITE_OK) return RES_OK;
if (estendido == SQLITE_CONSTRAINT_UNIQUE || estendido == SQLITE_CONSTRAINT_PRIMARYKEY) {
erro_definir("%s: já existe um registro com este valor.", contexto);
return RES_DUPLICADO;
}
if (estendido == SQLITE_CONSTRAINT_FOREIGNKEY) {
erro_definir("%s: registro relacionado não existe.", contexto);
return RES_INVALIDO;
}
if (estendido == SQLITE_CONSTRAINT_CHECK || estendido == SQLITE_CONSTRAINT_NOTNULL) {
erro_definir("%s: valor não permitido.", contexto);
return RES_INVALIDO;
}
erro_definir("%s: erro no banco de dados (%s).", contexto, sqlite3_errmsg(conexao));
return RES_BANCO;
}

/* Abre a transação (BEGIN) somente no nível mais externo. */
Resultado db_iniciar_transacao(void)
{
if (profundidade == 0) {
if (db_executar("BEGIN IMMEDIATE;") != RES_OK) return RES_BANCO;
desfazer_pendente = 0;
}
profundidade++;
return RES_OK;
}

/* Grava (COMMIT) ao fechar o nível mais externo, se nenhum nível interno falhou. */
Resultado db_confirmar(void)
{
if (profundidade <= 0) return RES_OK;
profundidade--;
if (profundidade > 0) return RES_OK;
if (desfazer_pendente) {
db_executar("ROLLBACK;");
desfazer_pendente = 0;
erro_definir("A operação foi cancelada e nada foi gravado.");
return RES_BANCO;
}
if (db_executar("COMMIT;") != RES_OK) {
db_executar("ROLLBACK;");
return RES_BANCO;
}
return RES_OK;
}

/* Desfaz (ROLLBACK); em nível interno, marca a transação para ser desfeita no final. */
void db_desfazer(void)
{
if (profundidade <= 0) return;
profundidade--;
if (profundidade > 0) {
desfazer_pendente = 1;
return;
}
{
/* Preserva a mensagem de erro original. */
char *msg = NULL;
sqlite3_exec(conexao, "ROLLBACK;", NULL, NULL, &msg);
sqlite3_free(msg);
}
desfazer_pendente = 0;
}

/* Copia uma coluna de texto do resultado, tratando NULL como texto vazio. */
void db_coluna_texto(sqlite3_stmt *st, int coluna, char *destino, size_t tamanho)
{
const unsigned char *t = sqlite3_column_text(st, coluna);
texto_copiar(destino, tamanho, t ? (const char *)t : "");
}

/* Id gerado pelo último INSERT. */
long long db_ultimo_id(void)
{
return (long long)sqlite3_last_insert_rowid(conexao);
}

/* Linhas afetadas pelo último comando de alteração. */
int db_linhas_alteradas(void)
{
return sqlite3_changes(conexao);
}

/* Executa uma consulta de um único número com um parâmetro inteiro. */
long long db_consultar_inteiro(const char *sql, long long parametro, long long padrao)
{
sqlite3_stmt *st;
long long valor = padrao;

if (db_preparar(sql, &st) != RES_OK) return padrao;
sqlite3_bind_int64(st, 1, parametro);
if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL)
valor = sqlite3_column_int64(st, 0);
sqlite3_finalize(st);
return valor;
}

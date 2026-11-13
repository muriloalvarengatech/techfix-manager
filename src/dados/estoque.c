/*
* estoque.c — Cadastro de peças e movimentações de estoque.
*/
#include <string.h>
#include "db.h"
#include "estoque.h"
#include "validacao.h"

#define CAMPOS_PECA "SELECT id, codigo, descricao, preco_centavos, saldo FROM peca "

/* Copia a linha atual do resultado para a struct Peca. */
static void ler_peca(sqlite3_stmt *st, Peca *p)
{
p->id = sqlite3_column_int64(st, 0);
db_coluna_texto(st, 1, p->codigo, sizeof(p->codigo));
db_coluna_texto(st, 2, p->descricao, sizeof(p->descricao));
p->preco_centavos = sqlite3_column_int64(st, 3);
p->saldo = sqlite3_column_int(st, 4);
}

/* Grava uma entrada ou saída na tabela movimentacao. */
static Resultado registrar_movimentacao(long long peca_id, char tipo, int quantidade, const char *motivo,
long long os_id, long long usuario_id, long long *mov_id)
{
sqlite3_stmt *st;
int rc;
char t[2] = { tipo, '\0' };
Resultado r;

if (db_preparar("INSERT INTO movimentacao (peca_id, tipo, quantidade, motivo, os_id, usuario_id) "
"VALUES (?, ?, ?, ?, NULLIF(?, 0), ?);", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int64(st, 1, peca_id);
sqlite3_bind_text(st, 2, t, -1, SQLITE_TRANSIENT);
sqlite3_bind_int(st, 3, quantidade);
sqlite3_bind_text(st, 4, motivo, -1, SQLITE_TRANSIENT);
sqlite3_bind_int64(st, 5, os_id);
sqlite3_bind_int64(st, 6, usuario_id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Registro de movimentação");
if (r == RES_OK && mov_id != NULL) *mov_id = db_ultimo_id();
return r;
}

/* Grava a peça e, se houver saldo inicial, a movimentação correspondente. */
Resultado peca_cadastrar(Peca *p, int saldo_inicial, long long usuario_id)
{
sqlite3_stmt *st;
int rc;
Resultado r;

if (texto_vazio(p->codigo) || texto_vazio(p->descricao)) {
erro_definir("Código e descrição da peça são obrigatórios.");
return RES_INVALIDO;
}
if (p->preco_centavos < 0 || saldo_inicial < 0) {
erro_definir("Preço e quantidade inicial não podem ser negativos.");
return RES_INVALIDO;
}
if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
if (db_preparar("INSERT INTO peca (codigo, descricao, preco_centavos, saldo) VALUES (?, ?, ?, ?);", &st) !=RES_OK) {
db_desfazer(); return RES_BANCO;
}
sqlite3_bind_text(st, 1, p->codigo, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 2, p->descricao, -1, SQLITE_TRANSIENT);
sqlite3_bind_int64(st, 3, p->preco_centavos);
sqlite3_bind_int(st, 4, saldo_inicial);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Cadastro de peça");
if (r == RES_DUPLICADO) erro_definir("Já existe uma peça com o código '%s'.", p->codigo);
if (r != RES_OK) { db_desfazer(); return r; }
p->id = db_ultimo_id();
p->saldo = saldo_inicial;
/* O saldo inicial também vira movimentação, para que saldo = soma das movimentações. */
if (saldo_inicial > 0) {
r = registrar_movimentacao(p->id, 'E', saldo_inicial, "Saldo inicial", 0, usuario_id, NULL);
if (r != RES_OK) { db_desfazer(); return r; }
}
return db_confirmar();
}

/* Busca uma peça pelo id. */
Resultado peca_buscar_id(long long id, Peca *saida)
{
sqlite3_stmt *st;
Resultado r = RES_NAO_ENCONTRADO;

if (db_preparar(CAMPOS_PECA "WHERE id = ?;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int64(st, 1, id);
if (sqlite3_step(st) == SQLITE_ROW) { ler_peca(st, saida); r = RES_OK; }
else erro_definir("Peça %lld não encontrada.", id);
sqlite3_finalize(st);
return r;
}

/* Busca uma peça pelo código cadastrado. */
Resultado peca_buscar_codigo(const char *codigo, Peca *saida)
{
sqlite3_stmt *st;
Resultado r = RES_NAO_ENCONTRADO;

if (db_preparar(CAMPOS_PECA "WHERE lower(codigo) = lower(?);", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_text(st, 1, codigo, -1, SQLITE_TRANSIENT);
if (sqlite3_step(st) == SQLITE_ROW) { ler_peca(st, saida); r = RES_OK; }
else erro_definir("Peça com código '%s' não encontrada.", codigo);
sqlite3_finalize(st);
return r;
}

/* Busca peças por código ou parte da descrição. */
int peca_buscar(const char *termo, Peca vetor[], int maximo)
{
sqlite3_stmt *st;
int n = 0;

if (db_preparar(CAMPOS_PECA "WHERE ? = '' OR lower(codigo) = lower(?) OR instr(lower(descricao), lower(?)) >0 "
"ORDER BY descricao LIMIT ?;", &st) != RES_OK) return -1;
sqlite3_bind_text(st, 1, termo, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 2, termo, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 3, termo, -1, SQLITE_TRANSIENT);
sqlite3_bind_int(st, 4, maximo);
while (n < maximo && sqlite3_step(st) == SQLITE_ROW) ler_peca(st, &vetor[n++]);
sqlite3_finalize(st);
return n;
}

/* Altera descrição e preço da peça. */
Resultado peca_atualizar(long long id, const char *descricao, long long preco_centavos)
{
sqlite3_stmt *st;
int rc;

if (texto_vazio(descricao)) { erro_definir("A descrição é obrigatória."); return RES_INVALIDO; }
if (preco_centavos < 0) { erro_definir("O preço não pode ser negativo."); return RES_INVALIDO; }
/* A quantidade não é editável aqui: só muda por movimentação (RN10). */
if (db_preparar("UPDATE peca SET descricao = ?, preco_centavos = ? WHERE id = ?;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_text(st, 1, descricao, -1, SQLITE_TRANSIENT);
sqlite3_bind_int64(st, 2, preco_centavos);
sqlite3_bind_int64(st, 3, id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
if (rc == SQLITE_DONE && db_linhas_alteradas() == 0) { erro_definir("Peça não encontrada."); return RES_NAO_ENCONTRADO; }
return db_falha(rc, "Atualização de peça");
}

/* Soma (ou subtrai) do saldo sem nunca deixá-lo negativo. */
static Resultado alterar_saldo(long long peca_id, int delta)
{
sqlite3_stmt *st;
int rc;

/* A condição "saldo + delta >= 0" impede saldo negativo mesmo com duas
pessoas usando o sistema ao mesmo tempo. */
if (db_preparar("UPDATE peca SET saldo = saldo + ?1 WHERE id = ?2 AND saldo + ?1 >= 0;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int(st, 1, delta);
sqlite3_bind_int64(st, 2, peca_id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
if (rc != SQLITE_DONE) return db_falha(rc, "Atualização de saldo");
if (db_linhas_alteradas() == 0) {
Peca p;
if (peca_buscar_id(peca_id, &p) != RES_OK) return RES_NAO_ENCONTRADO;
erro_definir("Saldo insuficiente da peça %s: disponível %d, solicitado %d.", p.codigo, p.saldo, -delta);
return RES_REGRA;
}
return RES_OK;
}

/* Registra uma entrada: aumenta o saldo e grava a movimentação juntos. */
Resultado estoque_entrada(long long peca_id, int quantidade, const char *motivo, long long usuario_id)
{
Resultado r;

if (quantidade <= 0) { erro_definir("A quantidade deve ser maior que zero."); return RES_INVALIDO; }
if (texto_vazio(motivo)) { erro_definir("Informe o motivo da entrada (ex.: compra, devolução)."); return RES_INVALIDO; }
if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
r = alterar_saldo(peca_id, quantidade);
if (r == RES_OK) r = registrar_movimentacao(peca_id, 'E', quantidade, motivo, 0, usuario_id, NULL);
if (r != RES_OK) { db_desfazer(); return r; }
return db_confirmar();
}

/*
* Algoritmo A4 (parte 1) - saída de estoque transacional.
* Debita o saldo e grava a movimentação; se qualquer passo falhar,
* desfaz tudo e nada fica gravado.
*/
Resultado estoque_saida(long long peca_id, int quantidade, const char *motivo, long long os_id, long long usuario_id)
{
Resultado r;

if (quantidade <= 0) { erro_definir("A quantidade deve ser maior que zero."); return RES_INVALIDO; }
if (texto_vazio(motivo)) { erro_definir("Informe o motivo da saída (ex.: perda, uso na OS)."); return RES_INVALIDO; }
if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
r = alterar_saldo(peca_id, -quantidade);
if (r == RES_OK) r = registrar_movimentacao(peca_id, 'S', quantidade, motivo, os_id, usuario_id, NULL);
if (r != RES_OK) { db_desfazer(); return r; }
return db_confirmar();
}

/* Lista as movimentações de uma peça, da mais recente para a mais antiga. */
int estoque_movimentacoes(long long peca_id, Movimentacao vetor[], int maximo)
{
sqlite3_stmt *st;
int n = 0;

if (db_preparar("SELECT m.id, m.tipo, m.quantidade, m.motivo, IFNULL(m.os_id, 0), u.nome, m.registrada_em "
"FROM movimentacao m JOIN usuario u ON u.id = m.usuario_id "
"WHERE m.peca_id = ? ORDER BY m.id DESC LIMIT ?;", &st) != RES_OK) return -1;
sqlite3_bind_int64(st, 1, peca_id);
sqlite3_bind_int(st, 2, maximo);
while (n < maximo && sqlite3_step(st) == SQLITE_ROW) {
Movimentacao *m = &vetor[n++];
const unsigned char *t = sqlite3_column_text(st, 1);
m->id = sqlite3_column_int64(st, 0);
m->tipo = t ? (char)t[0] : '?';
m->quantidade = sqlite3_column_int(st, 2);
db_coluna_texto(st, 3, m->motivo, sizeof(m->motivo));
m->os_id = sqlite3_column_int64(st, 4);
db_coluna_texto(st, 5, m->usuario, sizeof(m->usuario));
db_coluna_texto(st, 6, m->data, sizeof(m->data));
}
sqlite3_finalize(st);
return n;
}

/* Soma entradas e subtrai saídas para conferir o saldo. */
long long estoque_saldo_calculado(long long peca_id)
{
return db_consultar_inteiro(
"SELECT IFNULL(SUM(CASE tipo WHEN 'E' THEN quantidade ELSE -quantidade END), 0) "
"FROM movimentacao WHERE peca_id = ?;", peca_id, 0);
}

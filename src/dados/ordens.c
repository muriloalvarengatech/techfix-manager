/*
* ordens.c — Regras e acesso a dados do fluxo da ordem de serviço.
*/
#include <stdio.h>
#include <string.h>
#include "cadastros.h"
#include "db.h"
#include "estoque.h"
#include "ordens.h"
#include "regras.h"
#include "validacao.h"

const char *FORMAS_PAGAMENTO[QTD_FORMAS_PAGAMENTO] = { "Dinheiro", "Pix", "Cartão de débito", "Cartão de crédito" };

/* =====================================================================
* Auxiliares internos
* ===================================================================== */

static Resultado carregar_status(long long os_id, int *status, long long *tecnico_id)
{
sqlite3_stmt *st;
Resultado r = RES_NAO_ENCONTRADO;

if (db_preparar("SELECT status, IFNULL(tecnico_id, 0) FROM ordem_servico WHERE id = ?;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int64(st, 1, os_id);
if (sqlite3_step(st) == SQLITE_ROW) {
*status = sqlite3_column_int(st, 0);
if (tecnico_id) *tecnico_id = sqlite3_column_int64(st, 1);
r = RES_OK;
} else {
erro_definir("Ordem de serviço %lld não encontrada.", os_id);
}
sqlite3_finalize(st);
return r;
}

/* Confere se a OS está no status esperado para a operação. */
static Resultado exigir_status(long long os_id, int esperado, long long *tecnico_id, const char *acao)
{
int status;
Resultado r = carregar_status(os_id, &status, tecnico_id);
if (r != RES_OK) return r;
if (status != esperado) {
erro_definir("Não é possível %s: a OS %lld está \"%s\" e precisa estar \"%s\".",
acao, os_id, status_nome(status), status_nome(esperado));
return RES_REGRA;
}
return RES_OK;
}

/* Somente o técnico atribuído ou um gerente executam etapas técnicas. */
static Resultado exigir_responsavel(long long tecnico_id, long long usuario_id, int perfil, const char *acao)
{
if (perfil == PERFIL_GERENTE) return RES_OK;
if (perfil == PERFIL_TECNICO && tecnico_id == usuario_id && tecnico_id != 0) return RES_OK;
if (tecnico_id == 0) erro_definir("Atribua um técnico à OS antes de %s.", acao);
else erro_definir("Somente o técnico atribuído à OS ou um gerente pode %s.", acao);
return RES_PERMISSAO;
}

/* Algoritmo A2 - aplica uma transição validada pela matriz de estados.
Deve ser chamada dentro de uma transação. */
static Resultado mudar_status(long long os_id, int para, long long usuario_id)
{
sqlite3_stmt *st;
int atual, rc;
Resultado r = carregar_status(os_id, &atual, NULL);

if (r != RES_OK) return r;
if (!status_pode_transitar(atual, para)) {
erro_definir("Mudança de \"%s\" para \"%s\" não é permitida.", status_nome(atual), status_nome(para));
return RES_REGRA;
}
if (db_preparar("UPDATE ordem_servico SET status = ?, atualizada_em = datetime('now', 'localtime') "
"WHERE id = ? AND status = ?;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int(st, 1, para);
sqlite3_bind_int64(st, 2, os_id);
sqlite3_bind_int(st, 3, atual);
rc = sqlite3_step(st);
sqlite3_finalize(st);
if (rc != SQLITE_DONE) return db_falha(rc, "Mudança de status");
if (db_linhas_alteradas() == 0) { erro_definir("A OS foi alterada por outra pessoa. Consulte e tente de novo."); return RES_REGRA; }

if (db_preparar("INSERT INTO historico_status (os_id, status_anterior, status_novo, usuario_id) VALUES (?, ?, ?, ?);", &st) != RES_OK)
return RES_BANCO;
sqlite3_bind_int64(st, 1, os_id);
sqlite3_bind_int(st, 2, atual);
sqlite3_bind_int(st, 3, para);
sqlite3_bind_int64(st, 4, usuario_id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
return db_falha(rc, "Histórico de status");
}

/* Grava um texto ligado à OS (observação ou serviço realizado). */
static Resultado inserir_texto_os(const char *sql, long long os_id, long long usuario_id, const char *texto, const char *contexto)
{
sqlite3_stmt *st;
int rc;

if (db_preparar(sql, &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int64(st, 1, os_id);
sqlite3_bind_int64(st, 2, usuario_id);
sqlite3_bind_text(st, 3, texto, -1, SQLITE_TRANSIENT);
rc = sqlite3_step(st);
sqlite3_finalize(st);
return db_falha(rc, contexto);
}

/* Finaliza uma operação: confirma se tudo deu certo ou desfaz. */
static Resultado finalizar(Resultado r)
{
if (r != RES_OK) { db_desfazer(); return r; }
return db_confirmar();
}

/* =====================================================================
* Abertura e acompanhamento
* ===================================================================== */

Resultado os_abrir(long long equipamento_id, const char *problema, long long usuario_id, long long *os_id)
{
Equipamento e;
sqlite3_stmt *st;
int rc;
Resultado r;

/* Algoritmo A1 - abertura de OS com validações (RN04). */
if (texto_vazio(problema)) { erro_definir("Descreva o problema relatado pelo cliente."); return RES_INVALIDO; }
if (equipamento_buscar_id(equipamento_id, &e) != RES_OK) return RES_NAO_ENCONTRADO;
if (!e.cliente_ativo) {
erro_definir("O cliente %s está inativo: reative o cadastro para abrir uma OS.", e.cliente_nome);
return RES_REGRA;
}

if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
if (db_preparar("INSERT INTO ordem_servico (equipamento_id, aberta_por, problema, status) VALUES (?, ?, ?, ?);", &st) != RES_OK)
return finalizar(RES_BANCO);
sqlite3_bind_int64(st, 1, equipamento_id);
sqlite3_bind_int64(st, 2, usuario_id);
sqlite3_bind_text(st, 3, problema, -1, SQLITE_TRANSIENT);
sqlite3_bind_int(st, 4, OS_ABERTA);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Abertura de OS");
if (r != RES_OK) return finalizar(r);
*os_id = db_ultimo_id();

if (db_preparar("INSERT INTO historico_status (os_id, status_anterior, status_novo, usuario_id) VALUES (?, NULL, ?, ?);", &st) != RES_OK)
return finalizar(RES_BANCO);
sqlite3_bind_int64(st, 1, *os_id);
sqlite3_bind_int(st, 2, OS_ABERTA);
sqlite3_bind_int64(st, 3, usuario_id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
return finalizar(db_falha(rc, "Histórico de status"));
}

#define CAMPOS_OS \
"SELECT o.id, o.equipamento_id, e.tipo || ' ' || e.marca || ' ' || e.modelo, e.identificador, " \
"c.id, c.nome, IFNULL(o.tecnico_id, 0), IFNULL(t.nome, ''), a.nome, o.status, o.problema, " \
"o.aberta_em, o.atualizada_em " \
"FROM ordem_servico o " \
"JOIN equipamento e ON e.id = o.equipamento_id " \
"JOIN cliente c ON c.id = e.cliente_id " \
"JOIN usuario a ON a.id = o.aberta_por " \
"LEFT JOIN usuario t ON t.id = o.tecnico_id "

/* Copia a linha atual do resultado para a struct OrdemServico. */
static void ler_os(sqlite3_stmt *st, OrdemServico *o)
{
o->id = sqlite3_column_int64(st, 0);
o->equipamento_id = sqlite3_column_int64(st, 1);
db_coluna_texto(st, 2, o->equipamento, sizeof(o->equipamento));
db_coluna_texto(st, 3, o->identificador, sizeof(o->identificador));
o->cliente_id = sqlite3_column_int64(st, 4);
db_coluna_texto(st, 5, o->cliente_nome, sizeof(o->cliente_nome));
o->tecnico_id = sqlite3_column_int64(st, 6);
db_coluna_texto(st, 7, o->tecnico_nome, sizeof(o->tecnico_nome));
db_coluna_texto(st, 8, o->aberta_por, sizeof(o->aberta_por));
o->status = sqlite3_column_int(st, 9);
db_coluna_texto(st, 10, o->problema, sizeof(o->problema));
db_coluna_texto(st, 11, o->aberta_em, sizeof(o->aberta_em));
db_coluna_texto(st, 12, o->atualizada_em, sizeof(o->atualizada_em));
}

/* Busca uma OS pelo número. */
Resultado os_buscar_id(long long os_id, OrdemServico *saida)
{
sqlite3_stmt *st;
Resultado r = RES_NAO_ENCONTRADO;

if (db_preparar(CAMPOS_OS "WHERE o.id = ?;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int64(st, 1, os_id);
if (sqlite3_step(st) == SQLITE_ROW) { ler_os(st, saida); r = RES_OK; }
else erro_definir("Ordem de serviço %lld não encontrada.", os_id);
sqlite3_finalize(st);
return r;
}

/* Lista OS aplicando os filtros informados (0 ou vazio = sem filtro). */
int os_listar(const FiltroOS *f, OrdemServico vetor[], int maximo)
{
sqlite3_stmt *st;
int n = 0;

if (db_preparar(CAMPOS_OS
"WHERE (?1 = 0 OR o.status = ?1) AND (?2 = 0 OR o.tecnico_id = ?2) "
"AND (?3 = 0 OR c.id = ?3) AND (?4 = 0 OR e.id = ?4) "
"AND (?5 = '' OR date(o.aberta_em) >= ?5) AND (?6 = '' OR date(o.aberta_em) <= ?6) "
"ORDER BY o.id DESC LIMIT ?7;", &st) != RES_OK) return -1;
sqlite3_bind_int(st, 1, f->status);
sqlite3_bind_int64(st, 2, f->tecnico_id);
sqlite3_bind_int64(st, 3, f->cliente_id);
sqlite3_bind_int64(st, 4, f->equipamento_id);
sqlite3_bind_text(st, 5, f->data_inicio, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 6, f->data_fim, -1, SQLITE_TRANSIENT);
sqlite3_bind_int(st, 7, maximo);
while (n < maximo && sqlite3_step(st) == SQLITE_ROW) ler_os(st, &vetor[n++]);
sqlite3_finalize(st);
return n;
}

/* Atribui o técnico e registra a troca como observação. */
Resultado os_atribuir_tecnico(long long os_id, long long tecnico_id, long long usuario_id)
{
Usuario t;
sqlite3_stmt *st;
int status, rc;
char texto[TAM_TEXTO];
Resultado r = carregar_status(os_id, &status, NULL);

if (r != RES_OK) return r;
if (status > OS_EM_MANUTENCAO) {
erro_definir("A OS está \"%s\": o técnico não pode mais ser trocado.", status_nome(status));
return RES_REGRA;
}
/* RN06: somente usuário ativo com perfil técnico. */
if (usuario_buscar_id(tecnico_id, &t) != RES_OK) return RES_NAO_ENCONTRADO;
if (t.perfil != PERFIL_TECNICO || !t.ativo) {
erro_definir("%s não é um técnico ativo.", t.nome);
return RES_REGRA;
}

if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
if (db_preparar("UPDATE ordem_servico SET tecnico_id = ?, atualizada_em = datetime('now', 'localtime') WHERE id = ?;", &st) != RES_OK)
return finalizar(RES_BANCO);
sqlite3_bind_int64(st, 1, tecnico_id);
sqlite3_bind_int64(st, 2, os_id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Atribuição de técnico");
if (r == RES_OK) {
snprintf(texto, sizeof(texto), "Técnico responsável: %s.", t.nome);
r = inserir_texto_os("INSERT INTO observacao (os_id, usuario_id, texto) VALUES (?, ?, ?);", os_id, usuario_id, texto, "Observação");
}
return finalizar(r);
}

/* Grava uma observação, exceto em OS encerrada. */
Resultado os_observar(long long os_id, long long usuario_id, const char *texto)
{
int status;
Resultado r = carregar_status(os_id, &status, NULL);

if (r != RES_OK) return r;
if (status == OS_ENCERRADA) { erro_definir("A OS %lld está encerrada e não aceita alterações.", os_id); return RES_REGRA; }
if (texto_vazio(texto)) { erro_definir("A observação não pode ficar em branco."); return RES_INVALIDO; }
return inserir_texto_os("INSERT INTO observacao (os_id, usuario_id, texto) VALUES (?, ?, ?);", os_id, usuario_id, texto, "Observação");
}

/* Executa uma consulta de histórico (data, usuário, texto) e preenche o vetor. */
static int listar_registros(const char *sql, long long os_id, RegistroOS vetor[], int maximo)
{
sqlite3_stmt *st;
int n = 0;

if (db_preparar(sql, &st) != RES_OK) return -1;
sqlite3_bind_int64(st, 1, os_id);
while (n < maximo && sqlite3_step(st) == SQLITE_ROW) {
db_coluna_texto(st, 0, vetor[n].data, sizeof(vetor[n].data));
db_coluna_texto(st, 1, vetor[n].usuario, sizeof(vetor[n].usuario));
db_coluna_texto(st, 2, vetor[n].texto, sizeof(vetor[n].texto));
n++;
}
sqlite3_finalize(st);
return n;
}

/* Lista as mudanças de status com o nome dos estados por extenso. */
int os_historico_status(long long os_id, RegistroOS vetor[], int maximo)
{
sqlite3_stmt *st;
int n = 0;

if (db_preparar("SELECT h.registrado_em, u.nome, IFNULL(h.status_anterior, 0), h.status_novo "
"FROM historico_status h JOIN usuario u ON u.id = h.usuario_id "
"WHERE h.os_id = ? ORDER BY h.id;", &st) != RES_OK) return -1;
sqlite3_bind_int64(st, 1, os_id);
while (n < maximo && sqlite3_step(st) == SQLITE_ROW) {
int de = sqlite3_column_int(st, 2), para = sqlite3_column_int(st, 3);
db_coluna_texto(st, 0, vetor[n].data, sizeof(vetor[n].data));
db_coluna_texto(st, 1, vetor[n].usuario, sizeof(vetor[n].usuario));
if (de == 0) snprintf(vetor[n].texto, sizeof(vetor[n].texto), "OS aberta como \"%s\"", status_nome(para));
else snprintf(vetor[n].texto, sizeof(vetor[n].texto), "\"%s\" -> \"%s\"", status_nome(de), status_nome(para));
n++;
}
sqlite3_finalize(st);
return n;
}

/* Lista as observações da OS. */
int os_observacoes(long long os_id, RegistroOS vetor[], int maximo)
{
return listar_registros("SELECT o.registrada_em, u.nome, o.texto FROM observacao o "
"JOIN usuario u ON u.id = o.usuario_id WHERE o.os_id = ? ORDER BY o.id;", os_id, vetor, maximo);
}

/* =====================================================================
* Diagnóstico
* ===================================================================== */

Resultado os_iniciar_diagnostico(long long os_id, long long usuario_id, int perfil)
{
long long tecnico_id;
Resultado r = exigir_status(os_id, OS_ABERTA, &tecnico_id, "iniciar o diagnóstico");

if (r != RES_OK) return r;
r = exigir_responsavel(tecnico_id, usuario_id, perfil, "iniciar o diagnóstico");
if (r != RES_OK) return r;
if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
return finalizar(mudar_status(os_id, OS_EM_DIAGNOSTICO, usuario_id));
}

/* Valida e grava (ou corrige) o diagnóstico e as peças previstas. */
Resultado diagnostico_registrar(long long os_id, long long usuario_id, int perfil,
const char *descricao, const char *procedimento,
const long long pecas_id[], const int quantidades[], int qtd_pecas)
{
long long tecnico_id, diag_id;
sqlite3_stmt *st;
int i, j, rc;
Peca p;
Resultado r = exigir_status(os_id, OS_EM_DIAGNOSTICO, &tecnico_id, "registrar o diagnóstico");

if (r != RES_OK) return r;
r = exigir_responsavel(tecnico_id, usuario_id, perfil, "registrar o diagnóstico");
if (r != RES_OK) return r;
if (texto_vazio(descricao) || texto_vazio(procedimento)) {
erro_definir("Diagnóstico e procedimento recomendado são obrigatórios.");
return RES_INVALIDO;
}
if (qtd_pecas < 0 || qtd_pecas > MAX_ITENS) { erro_definir("Informe no máximo %d peças.", MAX_ITENS); return RES_INVALIDO; }
for (i = 0; i < qtd_pecas; i++) {
if (quantidades[i] <= 0) { erro_definir("A quantidade de cada peça deve ser maior que zero."); return RES_INVALIDO; }
if (peca_buscar_id(pecas_id[i], &p) != RES_OK) return RES_NAO_ENCONTRADO;
for (j = 0; j < i; j++)
if (pecas_id[j] == pecas_id[i]) { erro_definir("A peça %s foi informada duas vezes.", p.codigo); return RES_INVALIDO; }
}

if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
diag_id = db_consultar_inteiro("SELECT id FROM diagnostico WHERE os_id = ?;", os_id, 0);
if (diag_id == 0) {
if (db_preparar("INSERT INTO diagnostico (tecnico_id, descricao, procedimento, os_id) VALUES (?, ?, ?, ?);", &st) != RES_OK)
return finalizar(RES_BANCO);
} else {
/* Correção antes do orçamento: substitui o diagnóstico anterior. */
if (db_preparar("UPDATE diagnostico SET tecnico_id = ?, descricao = ?, procedimento = ?, "
"registrado_em = datetime('now', 'localtime') WHERE os_id = ?;", &st) != RES_OK)
return finalizar(RES_BANCO);
}
sqlite3_bind_int64(st, 1, usuario_id);
sqlite3_bind_text(st, 2, descricao, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 3, procedimento, -1, SQLITE_TRANSIENT);
sqlite3_bind_int64(st, 4, os_id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Registro de diagnóstico");
if (r != RES_OK) return finalizar(r);
if (diag_id == 0) diag_id = db_ultimo_id();

if (db_preparar("DELETE FROM diagnostico_peca WHERE diagnostico_id = ?;", &st) != RES_OK) return finalizar(RES_BANCO);
sqlite3_bind_int64(st, 1, diag_id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Peças do diagnóstico");

for (i = 0; r == RES_OK && i < qtd_pecas; i++) {
if (db_preparar("INSERT INTO diagnostico_peca (diagnostico_id, peca_id, quantidade) VALUES (?, ?, ?);",&st) != RES_OK)
return finalizar(RES_BANCO);
sqlite3_bind_int64(st, 1, diag_id);
sqlite3_bind_int64(st, 2, pecas_id[i]);
sqlite3_bind_int(st, 3, quantidades[i]);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Peças do diagnóstico");
}
/* Peças previstas não baixam o estoque: é apenas uma previsão. */
return finalizar(r);
}

/* Executa uma consulta de peças (id, código, descrição, quantidade, preço, saldo). */
static int carregar_itens_peca(const char *sql, long long id, ItemPeca vetor[], int maximo)
{
sqlite3_stmt *st;
int n = 0;

if (db_preparar(sql, &st) != RES_OK) return 0;
sqlite3_bind_int64(st, 1, id);
while (n < maximo && sqlite3_step(st) == SQLITE_ROW) {
ItemPeca *it = &vetor[n++];
it->peca_id = sqlite3_column_int64(st, 0);
db_coluna_texto(st, 1, it->codigo, sizeof(it->codigo));
db_coluna_texto(st, 2, it->descricao, sizeof(it->descricao));
it->quantidade = sqlite3_column_int(st, 3);
it->preco_unit_centavos = sqlite3_column_int64(st, 4);
it->saldo_atual = sqlite3_column_int(st, 5);
}
sqlite3_finalize(st);
return n;
}

/* Carrega o diagnóstico e suas peças previstas. */
Resultado diagnostico_buscar(long long os_id, Diagnostico *d)
{
sqlite3_stmt *st;
long long diag_id = 0;

memset(d, 0, sizeof(*d));
if (db_preparar("SELECT d.id, d.descricao, d.procedimento, u.nome, d.registrado_em FROM diagnostico d "
"JOIN usuario u ON u.id = d.tecnico_id WHERE d.os_id = ?;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int64(st, 1, os_id);
if (sqlite3_step(st) == SQLITE_ROW) {
diag_id = sqlite3_column_int64(st, 0);
db_coluna_texto(st, 1, d->descricao, sizeof(d->descricao));
db_coluna_texto(st, 2, d->procedimento, sizeof(d->procedimento));
db_coluna_texto(st, 3, d->tecnico, sizeof(d->tecnico));
db_coluna_texto(st, 4, d->data, sizeof(d->data));
}
sqlite3_finalize(st);
if (diag_id == 0) { erro_definir("A OS %lld ainda não tem diagnóstico.", os_id); return RES_NAO_ENCONTRADO;}
d->os_id = os_id;
d->qtd_pecas = carregar_itens_peca(
"SELECT p.id, p.codigo, p.descricao, dp.quantidade, p.preco_centavos, p.saldo "
"FROM diagnostico_peca dp JOIN peca p ON p.id = dp.peca_id WHERE dp.diagnostico_id = ? ORDER BY p.codigo;",
diag_id, d->pecas, MAX_ITENS);
return RES_OK;
}

/* =====================================================================
* Orçamento e decisão do cliente
* ===================================================================== */

static long long total_do_orcamento(const Orcamento *o)
{
long long servicos[MAX_ITENS], precos[MAX_ITENS];
int quantidades[MAX_ITENS], i;

for (i = 0; i < o->qtd_servicos; i++) servicos[i] = o->servicos[i].valor_centavos;
for (i = 0; i < o->qtd_pecas; i++) {
quantidades[i] = o->pecas[i].quantidade;
precos[i] = o->pecas[i].preco_unit_centavos;
}
return orcamento_calcular_total(servicos, o->qtd_servicos, quantidades, precos, o->qtd_pecas);
}

/* Valida e grava (ou substitui) o orçamento e move a OS para aguardar a decisão. */
Resultado orcamento_elaborar(long long os_id, long long usuario_id, const ItemServico servicos[], int qtd_servicos)
{
Diagnostico d;
Orcamento previa;
sqlite3_stmt *st;
long long orc_id;
int status, i, rc;
Resultado r = carregar_status(os_id, &status, NULL);

if (r != RES_OK) return r;
if (status != OS_EM_DIAGNOSTICO && status != OS_AGUARDANDO_APROVACAO) {
erro_definir("O orçamento só pode ser elaborado ou corrigido antes da decisão do cliente (OS está \"%s\").", status_nome(status));
return RES_REGRA;
}
if (diagnostico_buscar(os_id, &d) != RES_OK) return RES_REGRA;
if (qtd_servicos < 0 || qtd_servicos > MAX_ITENS) { erro_definir("Informe no máximo %d serviços.", MAX_ITENS); return RES_INVALIDO; }
if (qtd_servicos == 0 && d.qtd_pecas == 0) { erro_definir("O orçamento precisa de pelo menos um serviço ou peça."); return RES_INVALIDO; }
for (i = 0; i < qtd_servicos; i++)
if (texto_vazio(servicos[i].descricao) || servicos[i].valor_centavos < 0) {
erro_definir("Cada serviço precisa de descrição e valor não negativo.");
return RES_INVALIDO;
}

/* Confere o total antes de gravar (valores absurdos são recusados). */
memset(&previa, 0, sizeof(previa));
previa.qtd_servicos = qtd_servicos;
for (i = 0; i < qtd_servicos; i++) previa.servicos[i] = servicos[i];
previa.qtd_pecas = d.qtd_pecas;
for (i = 0; i < d.qtd_pecas; i++) previa.pecas[i] = d.pecas[i];
if (total_do_orcamento(&previa) < 0) { erro_definir("Valores do orçamento fora do limite permitido."); return RES_INVALIDO; }

if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
orc_id = db_consultar_inteiro("SELECT id FROM orcamento WHERE os_id = ? AND decisao IS NULL;", os_id, 0);
if (orc_id != 0) {
const char *limpar[] = { "DELETE FROM orcamento_servico WHERE orcamento_id = ?;",
"DELETE FROM orcamento_peca WHERE orcamento_id = ?;",
"UPDATE orcamento SET criado_em = datetime('now', 'localtime') WHERE id = ?;" };
for (i = 0; r == RES_OK && i < 3; i++) {
if (db_preparar(limpar[i], &st) != RES_OK) return finalizar(RES_BANCO);
sqlite3_bind_int64(st, 1, orc_id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Correção do orçamento");
}
} else {
if (db_preparar("INSERT INTO orcamento (os_id, criado_por) VALUES (?, ?);", &st) != RES_OK) return finalizar(RES_BANCO);
sqlite3_bind_int64(st, 1, os_id);
sqlite3_bind_int64(st, 2, usuario_id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Criação do orçamento");
orc_id = db_ultimo_id();
}

for (i = 0; r == RES_OK && i < qtd_servicos; i++) {
if (db_preparar("INSERT INTO orcamento_servico (orcamento_id, descricao, valor_centavos) VALUES (?, ?, ?);", &st) != RES_OK)
return finalizar(RES_BANCO);
sqlite3_bind_int64(st, 1, orc_id);
sqlite3_bind_text(st, 2, servicos[i].descricao, -1, SQLITE_TRANSIENT);
sqlite3_bind_int64(st, 3, servicos[i].valor_centavos);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Serviço do orçamento");
}
for (i = 0; r == RES_OK && i < d.qtd_pecas; i++) {
/* O preço é congelado no orçamento: mudanças futuras no cadastro da
peça não alteram um orçamento já apresentado ao cliente. */
if (db_preparar("INSERT INTO orcamento_peca (orcamento_id, peca_id, quantidade, preco_unit_centavos) VALUES (?, ?, ?, ?);", &st) != RES_OK)
return finalizar(RES_BANCO);
sqlite3_bind_int64(st, 1, orc_id);
sqlite3_bind_int64(st, 2, d.pecas[i].peca_id);
sqlite3_bind_int(st, 3, d.pecas[i].quantidade);
sqlite3_bind_int64(st, 4, d.pecas[i].preco_unit_centavos);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Peça do orçamento");
}
if (r == RES_OK && status == OS_EM_DIAGNOSTICO) r = mudar_status(os_id, OS_AGUARDANDO_APROVACAO, usuario_id);
return finalizar(r);
}

/* Carrega o orçamento com serviços, peças e total. */
Resultado orcamento_buscar(long long os_id, Orcamento *o)
{
sqlite3_stmt *st;
Resultado r = RES_NAO_ENCONTRADO;

memset(o, 0, sizeof(*o));
if (db_preparar("SELECT id, IFNULL(decisao, ''), criado_em, IFNULL(decidido_em, '') FROM orcamento WHERE os_id = ?;", &st) != RES_OK)
return RES_BANCO;
sqlite3_bind_int64(st, 1, os_id);
if (sqlite3_step(st) == SQLITE_ROW) {
const unsigned char *dec = sqlite3_column_text(st, 1);
o->id = sqlite3_column_int64(st, 0);
o->decisao = (dec && dec[0]) ? (char)dec[0] : 0;
db_coluna_texto(st, 2, o->criado_em, sizeof(o->criado_em));
db_coluna_texto(st, 3, o->decidido_em, sizeof(o->decidido_em));
r = RES_OK;
} else {
erro_definir("A OS %lld ainda não tem orçamento.", os_id);
}
sqlite3_finalize(st);
if (r != RES_OK) return r;
o->os_id = os_id;

if (db_preparar("SELECT descricao, valor_centavos FROM orcamento_servico WHERE orcamento_id = ? ORDER BY id;", &st) != RES_OK)
return RES_BANCO;
sqlite3_bind_int64(st, 1, o->id);
while (o->qtd_servicos < MAX_ITENS && sqlite3_step(st) == SQLITE_ROW) {
db_coluna_texto(st, 0, o->servicos[o->qtd_servicos].descricao, TAM_TEXTO);
o->servicos[o->qtd_servicos].valor_centavos = sqlite3_column_int64(st, 1);
o->qtd_servicos++;
}
sqlite3_finalize(st);
o->qtd_pecas = carregar_itens_peca(
"SELECT p.id, p.codigo, p.descricao, op.quantidade, op.preco_unit_centavos, p.saldo "
"FROM orcamento_peca op JOIN peca p ON p.id = op.peca_id WHERE op.orcamento_id = ? ORDER BY p.codigo;",
o->id, o->pecas, MAX_ITENS);
o->total_centavos = total_do_orcamento(o);
return RES_OK;
}

/* Grava a decisão única do cliente e muda o status da OS. */
Resultado orcamento_decidir(long long os_id, long long usuario_id, int aprovado)
{
sqlite3_stmt *st;
int rc;
Resultado r = exigir_status(os_id, OS_AGUARDANDO_APROVACAO, NULL, "registrar a decisão do cliente");

if (r != RES_OK) return r;
if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
/* "decisão IS NULL" garante uma única decisão por orçamento. */
if (db_preparar("UPDATE orcamento SET decisao = ?, decidido_por = ?, decidido_em = datetime('now', 'localtime') "
"WHERE os_id = ? AND decisao IS NULL;", &st) != RES_OK) return finalizar(RES_BANCO);
sqlite3_bind_text(st, 1, aprovado ? "A" : "R", -1, SQLITE_STATIC);
sqlite3_bind_int64(st, 2, usuario_id);
sqlite3_bind_int64(st, 3, os_id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Decisão do orçamento");
if (r == RES_OK && db_linhas_alteradas() == 0) { erro_definir("O orçamento desta OS já tem uma decisão."); r= RES_REGRA; }
if (r == RES_OK) r = mudar_status(os_id, aprovado ? OS_APROVADA : OS_RECUSADA, usuario_id);
return finalizar(r);
}

/* =====================================================================
* Execução da manutenção
* ===================================================================== */

Resultado manutencao_iniciar(long long os_id, long long usuario_id, int perfil)
{
long long tecnico_id;
Resultado r = exigir_status(os_id, OS_APROVADA, &tecnico_id, "iniciar a manutenção (RN09: exige orçamento aprovado)");

if (r != RES_OK) return r;
r = exigir_responsavel(tecnico_id, usuario_id, perfil, "iniciar a manutenção");
if (r != RES_OK) return r;
if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
return finalizar(mudar_status(os_id, OS_EM_MANUTENCAO, usuario_id));
}

/* Grava um serviço realizado durante a manutenção. */
Resultado manutencao_registrar_servico(long long os_id, long long usuario_id, int perfil, const char *descricao)
{
long long tecnico_id;
Resultado r = exigir_status(os_id, OS_EM_MANUTENCAO, &tecnico_id, "registrar serviço");

if (r != RES_OK) return r;
r = exigir_responsavel(tecnico_id, usuario_id, perfil, "registrar serviço");
if (r != RES_OK) return r;
if (texto_vazio(descricao)) { erro_definir("Descreva o serviço realizado."); return RES_INVALIDO; }
return inserir_texto_os("INSERT INTO servico_realizado (os_id, usuario_id, descricao) VALUES (?, ?, ?);",
os_id, usuario_id, descricao, "Serviço realizado");
}

/*
* Algoritmo A4 - baixa de estoque transacional.
* A saída do estoque e o vínculo peça-OS são gravados juntos: se o saldo
* for insuficiente ou qualquer gravação falhar, nada é gravado.
*/
Resultado manutencao_usar_peca(long long os_id, long long peca_id, int quantidade, long long usuario_id, int perfil)
{
long long tecnico_id, mov_id;
sqlite3_stmt *st;
char motivo[64];
int rc;
Resultado r = exigir_status(os_id, OS_EM_MANUTENCAO, &tecnico_id, "registrar peça utilizada");

if (r != RES_OK) return r;
r = exigir_responsavel(tecnico_id, usuario_id, perfil, "registrar peça utilizada");
if (r != RES_OK) return r;

snprintf(motivo, sizeof(motivo), "Uso na OS %lld", os_id);
if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
r = estoque_saida(peca_id, quantidade, motivo, os_id, usuario_id);
if (r != RES_OK) return finalizar(r);
mov_id = db_ultimo_id();

if (db_preparar("INSERT INTO peca_utilizada (os_id, peca_id, quantidade, movimentacao_id) VALUES (?, ?, ?, ?);", &st) != RES_OK)
return finalizar(RES_BANCO);
sqlite3_bind_int64(st, 1, os_id);
sqlite3_bind_int64(st, 2, peca_id);
sqlite3_bind_int(st, 3, quantidade);
sqlite3_bind_int64(st, 4, mov_id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
return finalizar(db_falha(rc, "Peça utilizada"));
}

/* Grava a conclusão técnica e muda a OS para concluída. */
Resultado manutencao_concluir(long long os_id, long long usuario_id, int perfil, const char *observacao_final)
{
long long tecnico_id;
sqlite3_stmt *st;
int rc;
Resultado r = exigir_status(os_id, OS_EM_MANUTENCAO, &tecnico_id, "concluir a manutenção");

if (r != RES_OK) return r;
r = exigir_responsavel(tecnico_id, usuario_id, perfil, "concluir a manutenção");
if (r != RES_OK) return r;
if (db_consultar_inteiro("SELECT COUNT(*) FROM servico_realizado WHERE os_id = ?;", os_id, 0) == 0) {
erro_definir("Registre pelo menos um serviço realizado antes de concluir.");
return RES_REGRA;
}
if (texto_vazio(observacao_final)) { erro_definir("Informe a observação final para o cliente."); return RES_INVALIDO; }

if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
if (db_preparar("INSERT INTO conclusao_tecnica (os_id, tecnico_id, observacao_final) VALUES (?, ?, ?);", &st) != RES_OK)
return finalizar(RES_BANCO);
sqlite3_bind_int64(st, 1, os_id);
sqlite3_bind_int64(st, 2, usuario_id);
sqlite3_bind_text(st, 3, observacao_final, -1, SQLITE_TRANSIENT);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Conclusão técnica");
if (r == RES_OK) r = mudar_status(os_id, OS_CONCLUIDA, usuario_id);
return finalizar(r);
}

/* Lista os serviços realizados na OS. */
int os_servicos_realizados(long long os_id, RegistroOS vetor[], int maximo)
{
return listar_registros("SELECT s.registrado_em, u.nome, s.descricao FROM servico_realizado s "
"JOIN usuario u ON u.id = s.usuario_id WHERE s.os_id = ? ORDER BY s.id;", os_id, vetor, maximo);
}

/* Lista as peças utilizadas na OS, somadas por peça. */
int os_pecas_utilizadas(long long os_id, ItemPeca vetor[], int maximo)
{
return carregar_itens_peca(
"SELECT p.id, p.codigo, p.descricao, SUM(pu.quantidade), p.preco_centavos, p.saldo "
"FROM peca_utilizada pu JOIN peca p ON p.id = pu.peca_id WHERE pu.os_id = ? "
"GROUP BY p.id ORDER BY p.codigo;", os_id, vetor, maximo);
}

/* =====================================================================
* Pagamento, liberação, entrega e encerramento
* ===================================================================== */

Resultado os_valor_final(long long os_id, long long *centavos)
{
Orcamento o;

/* DECISÃO D11 (proposta): valor final = total do orçamento aprovado. */
if (orcamento_buscar(os_id, &o) != RES_OK) return RES_NAO_ENCONTRADO;
if (o.decisao != 'A') { erro_definir("A OS %lld não tem orçamento aprovado.", os_id); return RES_REGRA; }
*centavos = o.total_centavos;
return RES_OK;
}

/* Valida o valor e a forma e grava o pagamento integral. */
Resultado pagamento_registrar(long long os_id, long long valor_centavos, int forma, long long usuario_id)
{
sqlite3_stmt *st;
long long devido;
char devido_txt[32];
int rc;
Resultado r = exigir_status(os_id, OS_CONCLUIDA, NULL, "registrar o pagamento");

if (r != RES_OK) return r;
if (forma < 0 || forma >= QTD_FORMAS_PAGAMENTO) { erro_definir("Forma de pagamento inválida."); return RES_INVALIDO; }
r = os_valor_final(os_id, &devido);
if (r != RES_OK) return r;
if (devido <= 0) { erro_definir("O valor final desta OS é zero: não há pagamento a registrar."); return RES_REGRA; }
/* DECISÃO D12 (proposta): pagamento integral, em uma única vez. */
if (valor_centavos != devido) {
formatar_dinheiro(devido, devido_txt, sizeof(devido_txt));
erro_definir("O valor pago deve ser igual ao valor final da OS (%s).", devido_txt);
return RES_INVALIDO;
}
if (db_preparar("INSERT INTO pagamento (os_id, valor_centavos, forma, usuario_id) VALUES (?, ?, ?, ?);", &st) != RES_OK)
return RES_BANCO;
sqlite3_bind_int64(st, 1, os_id);
sqlite3_bind_int64(st, 2, valor_centavos);
sqlite3_bind_text(st, 3, FORMAS_PAGAMENTO[forma], -1, SQLITE_STATIC);
sqlite3_bind_int64(st, 4, usuario_id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Pagamento");
if (r == RES_DUPLICADO) erro_definir("O pagamento desta OS já foi registrado.");
return r;
}

/* Informa se a OS está paga, pendente ou se o pagamento não se aplica. */
int pagamento_situacao(long long os_id, char forma[], int tamanho_forma, char data[])
{
sqlite3_stmt *st;
int situacao = 0;

if (db_consultar_inteiro("SELECT COUNT(*) FROM orcamento WHERE os_id = ? AND decisao = 'A';", os_id, 0) == 0) return -1;
if (db_preparar("SELECT forma, registrado_em FROM pagamento WHERE os_id = ?;", &st) != RES_OK) return 0;
sqlite3_bind_int64(st, 1, os_id);
if (sqlite3_step(st) == SQLITE_ROW) {
if (forma) db_coluna_texto(st, 0, forma, (size_t)tamanho_forma);
if (data) db_coluna_texto(st, 1, data, TAM_DATA);
situacao = 1;
}
sqlite3_finalize(st);
if (!situacao && db_consultar_inteiro("SELECT COUNT(*) FROM orcamento o WHERE o.os_id = ? AND NOT EXISTS "
"(SELECT 1 FROM orcamento_servico s WHERE s.orcamento_id = o.id AND s.valor_centavos > 0) "
"AND NOT EXISTS (SELECT 1 FROM orcamento_peca p WHERE p.orcamento_id =o.id AND p.preco_unit_centavos > 0);",
os_id, 0) > 0)
situacao = 1; /* orçamento de valor zero (ex.: garantia) não exige pagamento */
return situacao;
}

/* Libera o equipamento quando as condições da decisão D13 são atendidas. */
Resultado os_liberar(long long os_id, long long usuario_id)
{
int status;
Resultado r = carregar_status(os_id, &status, NULL);

if (r != RES_OK) return r;
/* DECISÃO D13 (proposta): libera com conclusão técnica + pagamento,
ou quando o cliente recusou o orçamento (devolução sem reparo, D09). */
if (status == OS_CONCLUIDA) {
if (pagamento_situacao(os_id, NULL, 0, NULL) != 1) {
erro_definir("Falta registrar o pagamento para liberar o equipamento.");
return RES_REGRA;
}
} else if (status != OS_RECUSADA) {
erro_definir("Para liberar, a OS precisa estar concluída e paga, ou com orçamento recusado (atual: \"%s\").", status_nome(status));
return RES_REGRA;
}
if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
return finalizar(mudar_status(os_id, OS_LIBERADA, usuario_id));
}

/* Grava a entrega e muda a OS para entregue. */
Resultado entrega_registrar(long long os_id, const char *recebido_por, long long usuario_id)
{
sqlite3_stmt *st;
int rc;
Resultado r = exigir_status(os_id, OS_LIBERADA, NULL, "registrar a entrega");

if (r != RES_OK) return r;
if (texto_vazio(recebido_por)) { erro_definir("Informe o nome de quem recebeu o equipamento."); return RES_INVALIDO; }
if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
if (db_preparar("INSERT INTO entrega (os_id, recebido_por, usuario_id) VALUES (?, ?, ?);", &st) != RES_OK)
return finalizar(RES_BANCO);
sqlite3_bind_int64(st, 1, os_id);
sqlite3_bind_text(st, 2, recebido_por, -1, SQLITE_TRANSIENT);
sqlite3_bind_int64(st, 3, usuario_id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Entrega");
if (r == RES_OK) r = mudar_status(os_id, OS_ENTREGUE, usuario_id);
return finalizar(r);
}

/* Encerra a OS entregue. */
Resultado os_encerrar(long long os_id, long long usuario_id)
{
Resultado r = exigir_status(os_id, OS_ENTREGUE, NULL, "encerrar a OS");

if (r != RES_OK) return r;
if (db_iniciar_transacao() != RES_OK) return RES_BANCO;
return finalizar(mudar_status(os_id, OS_ENCERRADA, usuario_id));
}

/* Executa uma consulta que devolve duas colunas de texto de uma OS. */
static int buscar_dois_textos(const char *sql, long long os_id, char *a, size_t ta, char *b, size_t tb)
{
sqlite3_stmt *st;
int achou = 0;

if (db_preparar(sql, &st) != RES_OK) return 0;
sqlite3_bind_int64(st, 1, os_id);
if (sqlite3_step(st) == SQLITE_ROW) {
db_coluna_texto(st, 0, a, ta);
db_coluna_texto(st, 1, b, tb);
achou = 1;
}
sqlite3_finalize(st);
return achou;
}

/* Busca a observação final e a data da conclusão técnica. */
int os_conclusao(long long os_id, char observacao_final[TAM_TEXTO], char data[TAM_DATA])
{
return buscar_dois_textos("SELECT observacao_final, concluida_em FROM conclusao_tecnica WHERE os_id = ?;",
os_id, observacao_final, TAM_TEXTO, data, TAM_DATA);
}

/* Busca quem recebeu e a data da entrega. */
int os_entrega(long long os_id, char recebido_por[TAM_NOME], char data[TAM_DATA])
{
return buscar_dois_textos("SELECT recebido_por, registrada_em FROM entrega WHERE os_id = ?;",
os_id, recebido_por, TAM_NOME, data, TAM_DATA);
}

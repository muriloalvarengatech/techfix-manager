/*
* relatorios.c — Contagens e exportação de relatórios em CSV.
*/
#include <stdio.h>
#include <string.h>
#include "db.h"
#include "relatorios.h"
#include "validacao.h"

/* Escreve um campo entre aspas, no formato CSV. */
void csv_escrever_campo(FILE *f, const char *texto)
{
fputc('"', f);
for (; *texto; texto++) {
if (*texto == '"') fputc('"', f);
if (*texto == '\n' || *texto == '\r') { fputc(' ', f); continue; }
fputc(*texto, f);
}
fputc('"', f);
}

/* Conta as OS agrupadas por status. */
Resultado relatorio_contar_por_status(int contagem[QTD_STATUS + 1])
{
sqlite3_stmt *st;
int i;

for (i = 0; i <= QTD_STATUS; i++) contagem[i] = 0;
if (db_preparar("SELECT status, COUNT(*) FROM ordem_servico GROUP BY status;", &st) != RES_OK) return RES_BANCO;
while (sqlite3_step(st) == SQLITE_ROW) {
int s = sqlite3_column_int(st, 0);
if (status_valido(s)) contagem[s] = sqlite3_column_int(st, 1);
}
sqlite3_finalize(st);
return RES_OK;
}

/* Cria o arquivo e grava a marca UTF-8 (BOM) para as planilhas mostrarem acentos. */
static FILE *abrir_csv(const char *arquivo)
{
FILE *f = fopen(arquivo, "wb");
if (f == NULL) {
erro_definir("Não foi possível criar o arquivo '%s'. Verifique se ele está aberto em outro programa.", arquivo);
return NULL;
}
/* Marca UTF-8 (BOM) para que planilhas mostrem os acentos corretamente. */
fputs("\xEF\xBB\xBF", f);
return f;
}

/* Grava o relatório de OS em CSV, com uma linha de total no final. */
Resultado relatorio_exportar_os(const char *arquivo, const FiltroOS *filtro, int *linhas)
{
sqlite3_stmt *st;
FILE *f;
char valor[32], texto[TAM_TEXTO];
long long total_geral = 0;

*linhas = 0;
if (db_preparar(
"SELECT o.id, o.aberta_em, c.nome, e.tipo || ' ' || e.marca || ' ' || e.modelo, e.identificador, "
"o.status, IFNULL(t.nome, ''), o.problema, "
"IFNULL((SELECT SUM(s.valor_centavos) FROM orcamento_servico s JOIN orcamento r ON r.id = s.orcamento_id "
" WHERE r.os_id = o.id AND r.decisao = 'A'), 0) + "
"IFNULL((SELECT SUM(p.quantidade * p.preco_unit_centavos) FROM orcamento_peca p JOIN orcamento r ON r.id = p.orcamento_id "
" WHERE r.os_id = o.id AND r.decisao = 'A'), 0), "
"CASE WHEN EXISTS (SELECT 1 FROM pagamento g WHERE g.os_id = o.id) THEN 'Sim' ELSE 'Não' END "
"FROM ordem_servico o JOIN equipamento e ON e.id = o.equipamento_id JOIN cliente c ON c.id = e.cliente_id "
"LEFT JOIN usuario t ON t.id = o.tecnico_id "
"WHERE (?1 = 0 OR o.status = ?1) AND (?2 = 0 OR o.tecnico_id = ?2) "
"AND (?3 = '' OR date(o.aberta_em) >= ?3) AND (?4 = '' OR date(o.aberta_em) <= ?4) "
"ORDER BY o.id;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int(st, 1, filtro->status);
sqlite3_bind_int64(st, 2, filtro->tecnico_id);
sqlite3_bind_text(st, 3, filtro->data_inicio, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 4, filtro->data_fim, -1, SQLITE_TRANSIENT);

f = abrir_csv(arquivo);
if (f == NULL) { sqlite3_finalize(st); return RES_INVALIDO; }
fputs("OS;Abertura;Cliente;Equipamento;Identificador;Status;Técnico;Problema;Valor aprovado;Pago\r\n", f);
while (sqlite3_step(st) == SQLITE_ROW) {
long long valor_centavos = sqlite3_column_int64(st, 8);
fprintf(f, "%lld;", (long long)sqlite3_column_int64(st, 0));
db_coluna_texto(st, 1, texto, sizeof(texto)); csv_escrever_campo(f, texto); fputc(';', f);
db_coluna_texto(st, 2, texto, sizeof(texto)); csv_escrever_campo(f, texto); fputc(';', f);
db_coluna_texto(st, 3, texto, sizeof(texto)); csv_escrever_campo(f, texto); fputc(';', f);
db_coluna_texto(st, 4, texto, sizeof(texto)); csv_escrever_campo(f, texto); fputc(';', f);
csv_escrever_campo(f, status_nome(sqlite3_column_int(st, 5))); fputc(';', f);
db_coluna_texto(st, 6, texto, sizeof(texto)); csv_escrever_campo(f, texto); fputc(';', f);
db_coluna_texto(st, 7, texto, sizeof(texto)); csv_escrever_campo(f, texto); fputc(';', f);
/* Valor sem "R$" e com vírgula decimal, para a planilha somar. */
snprintf(valor, sizeof(valor), "%lld,%02lld", valor_centavos / 100, valor_centavos % 100);
fputs(valor, f); fputc(';', f);
db_coluna_texto(st, 9, texto, sizeof(texto)); csv_escrever_campo(f, texto);
fputs("\r\n", f);
total_geral += valor_centavos;
(*linhas)++;
}
sqlite3_finalize(st);
fprintf(f, ";;;;;;;Total;%lld,%02lld;\r\n", total_geral / 100, total_geral % 100);
if (fclose(f) != 0) { erro_definir("Erro ao gravar o arquivo '%s'.", arquivo); return RES_INVALIDO; }
return RES_OK;
}

/* Grava a posição do estoque em CSV, com o valor total no final. */
Resultado relatorio_exportar_estoque(const char *arquivo, int *linhas)
{
sqlite3_stmt *st;
FILE *f;
char texto[TAM_TEXTO];
long long total = 0;

*linhas = 0;
if (db_preparar("SELECT codigo, descricao, saldo, preco_centavos, saldo * preco_centavos FROM peca ORDER BY descricao;", &st) != RES_OK)
return RES_BANCO;
f = abrir_csv(arquivo);
if (f == NULL) { sqlite3_finalize(st); return RES_INVALIDO; }
fputs("Código;Descrição;Saldo;Preço unitário;Valor em estoque\r\n", f);
while (sqlite3_step(st) == SQLITE_ROW) {
long long preco = sqlite3_column_int64(st, 3), valor = sqlite3_column_int64(st, 4);
db_coluna_texto(st, 0, texto, sizeof(texto)); csv_escrever_campo(f, texto); fputc(';', f);
db_coluna_texto(st, 1, texto, sizeof(texto)); csv_escrever_campo(f, texto); fputc(';', f);
fprintf(f, "%d;%lld,%02lld;%lld,%02lld\r\n", sqlite3_column_int(st, 2),
preco / 100, preco % 100, valor / 100, valor % 100);
total += valor;
(*linhas)++;
}
sqlite3_finalize(st);
fprintf(f, ";;;Total;%lld,%02lld\r\n", total / 100, total % 100);
if (fclose(f) != 0) { erro_definir("Erro ao gravar o arquivo '%s'.", arquivo); return RES_INVALIDO; }
return RES_OK;
}

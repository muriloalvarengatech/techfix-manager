/*
* regras.c — Estados da OS, permissões por perfil e cálculo do orçamento.
*/
#include "regras.h"

/* ---------- Estados ---------- */

static const char *NOMES_STATUS[QTD_STATUS + 1] = {
"?",
"Aberta",
"Em diagnóstico",
"Aguardando aprovação",
"Aprovada",
"Recusada",
"Em manutenção",
"Concluída tecnicamente",
"Liberada para entrega",
"Entregue",
"Encerrada"
};

/*
* Algoritmo A2 - matriz de transições permitidas.
* TRANSICOES[de][para] == 1 quando a mudança é permitida.
* Linha e coluna 0 não são usadas (os estados começam em 1).
*
* Aberta -> Em diagnóstico -> Aguardando aprovação -> Aprovada -> Em manutenção
* -> Concluída -> Liberada -> Entregue -> Encerrada
* Aguardando aprovação -> Recusada -> Liberada (devolução sem reparo, DECISÃO D09)
*/
static const unsigned char TRANSICOES[QTD_STATUS + 1][QTD_STATUS + 1] = {
/* - ABE DIA AGU APR REC MAN CON LIB ENT ENC */
/* - */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
/* ABE */ { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
/* DIA */ { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
/* AGU */ { 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
/* APR */ { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
/* REC */ { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
/* MAN */ { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
/* CON */ { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
/* LIB */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
/* ENT */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
/* ENC */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

/* Confere se o número está entre 1 e QTD_STATUS. */
int status_valido(int status)
{
return status >= 1 && status <= QTD_STATUS;
}

/* Consulta o vetor de nomes; status inválido vira "?". */
const char *status_nome(int status)
{
return status_valido(status) ? NOMES_STATUS[status] : NOMES_STATUS[0];
}

/* Consulta a matriz TRANSICOES. */
int status_pode_transitar(int de, int para)
{
if (!status_valido(de) || !status_valido(para)) return 0;
return TRANSICOES[de][para] == 1;
}

/* ---------- Perfis e permissões ---------- */

static const char *NOMES_PERFIL[QTD_PERFIS + 1] = { "?", "Atendente", "Técnico", "Gerente" };

/*
* Matriz perfil x funcionalidade (DECISÃO D05, proposta).
* Colunas: atendente, técnico, gerente.
*/
static const unsigned char PERMISSOES[QTD_FUNCIONALIDADES][QTD_PERFIS] = {
/* F_CLIENTE_CONSULTAR */ { 1, 1, 1 },
/* F_CLIENTE_GERIR */ { 1, 0, 1 },
/* F_EQUIP_CONSULTAR */ { 1, 1, 1 },
/* F_EQUIP_GERIR */ { 1, 0, 1 },
/* F_OS_CONSULTAR */ { 1, 1, 1 },
/* F_OS_ABRIR */ { 1, 0, 1 },
/* F_OS_ATRIBUIR */ { 1, 0, 1 },
/* F_OS_OBSERVAR */ { 1, 1, 1 },
/* F_DIAGNOSTICO */ { 0, 1, 1 },
/* F_ORCAMENTO */ { 1, 1, 1 },
/* F_DECISAO */ { 1, 0, 1 },
/* F_MANUTENCAO */ { 0, 1, 1 },
/* F_PECA_CONSULTAR */ { 1, 1, 1 },
/* F_PECA_GERIR */ { 0, 1, 1 },
/* F_ESTOQUE_ENTRADA */ { 0, 1, 1 },
/* F_ESTOQUE_SAIDA */ { 0, 0, 1 },
/* F_PAGAMENTO */ { 1, 0, 1 },
/* F_ENTREGA */ { 1, 0, 1 },
/* F_ENCERRAR */ { 0, 0, 1 },
/* F_HISTORICO */ { 1, 1, 1 },
/* F_RELATORIOS */ { 0, 0, 1 },
/* F_USUARIOS */ { 0, 0, 1 }
};

/* Confere se o número está entre 1 e QTD_PERFIS. */
int perfil_valido(int perfil)
{
return perfil >= 1 && perfil <= QTD_PERFIS;
}

/* Consulta o vetor de nomes de perfil. */
const char *perfil_nome(int perfil)
{
return perfil_valido(perfil) ? NOMES_PERFIL[perfil] : NOMES_PERFIL[0];
}

/* Consulta a matriz PERMISSOES (linha = funcionalidade, coluna = perfil). */
int perfil_pode(int perfil, Funcionalidade f)
{
if (!perfil_valido(perfil) || (int)f < 0 || f >= QTD_FUNCIONALIDADES) return 0;
return PERMISSOES[f][perfil - 1] == 1;
}

/* ---------- Cálculo do orçamento ---------- */

#define LIMITE_TOTAL 99999999999LL /* R$ 999.999.999,99 */

long long orcamento_calcular_total(const long long servicos[], int qtd_servicos,
const int quantidades[], const long long precos[], int qtd_pecas)
{
long long total = 0;
int i;

if (qtd_servicos < 0 || qtd_pecas < 0) return -1;
for (i = 0; i < qtd_servicos; i++) {
if (servicos[i] < 0) return -1;
total += servicos[i];
if (total > LIMITE_TOTAL) return -1;
}
for (i = 0; i < qtd_pecas; i++) {
if (quantidades[i] <= 0 || precos[i] < 0) return -1;
if (precos[i] > LIMITE_TOTAL / quantidades[i]) return -1;
total += (long long)quantidades[i] * precos[i];
if (total > LIMITE_TOTAL) return -1;
}
return total;
}

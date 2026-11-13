/*
* regras.h — Regras de negócio "puras", que não dependem do banco de dados:
* estados da OS, perfis e permissões, e cálculo do orçamento.
*
* Por não acessarem banco nem teclado, essas regras são testadas
* diretamente em tests/testes.c.
*
* DECISÃO D04 (proposta): estados e transições da OS.
* DECISÃO D05 (proposta): perfis atendente, técnico e gerente.
*/
#ifndef REGRAS_H
#define REGRAS_H

/* ===================== Estados da ordem de serviço (RN05) ===================== */

/* Etapas pelas quais uma OS passa. Os valores (1 a 10) são gravados na
coluna ordem_servico.status e não devem ser renumerados. */
typedef enum {
OS_ABERTA = 1, /* equipamento recebido */
OS_EM_DIAGNOSTICO, /* técnico avaliando */
OS_AGUARDANDO_APROVACAO, /* orçamento apresentado ao cliente */
OS_APROVADA, /* cliente aprovou o orçamento */
OS_RECUSADA, /* cliente recusou o orçamento */
OS_EM_MANUTENCAO, /* reparo em andamento */
OS_CONCLUIDA, /* reparo concluído tecnicamente */
OS_LIBERADA, /* pronto para o cliente retirar */
OS_ENTREGUE, /* equipamento devolvido ao cliente */
OS_ENCERRADA /* atendimento finalizado; não aceita alterações */
} StatusOS;

#define QTD_STATUS 10

/* Nome do status por extenso (ex.: "Aguardando aprovação"). */
const char *status_nome(int status);

/* 1 se o número corresponde a um status existente. */
int status_valido(int status);

/* 1 se a mudança "de -> para" está prevista na matriz de transições (algoritmo A2). */
int status_pode_transitar(int de, int para);

/* ===================== Perfis e permissões (RN13) ===================== */

/* Perfis de acesso. Os valores (1 a 3) são gravados em usuario.perfil. */
typedef enum { PERFIL_ATENDENTE = 1, PERFIL_TECNICO, PERFIL_GERENTE } Perfil;
#define QTD_PERFIS 3

/* Funcionalidades controladas por permissão. Cada uma é uma linha da
matriz PERMISSOES em src/regras/regras.c. */
typedef enum {
F_CLIENTE_CONSULTAR = 0, F_CLIENTE_GERIR,
F_EQUIP_CONSULTAR, F_EQUIP_GERIR,
F_OS_CONSULTAR, F_OS_ABRIR, F_OS_ATRIBUIR, F_OS_OBSERVAR,
F_DIAGNOSTICO, F_ORCAMENTO, F_DECISAO,
F_MANUTENCAO,
F_PECA_CONSULTAR, F_PECA_GERIR, F_ESTOQUE_ENTRADA, F_ESTOQUE_SAIDA,
F_PAGAMENTO, F_ENTREGA, F_ENCERRAR,
F_HISTORICO, F_RELATORIOS, F_USUARIOS,
QTD_FUNCIONALIDADES /* não é funcionalidade: marca a quantidade */
} Funcionalidade;

/* Nome do perfil por extenso (ex.: "Técnico"). */
const char *perfil_nome(int perfil);

/* 1 se o número corresponde a um perfil existente. */
int perfil_valido(int perfil);

/* 1 se o perfil tem permissão para usar a funcionalidade. */
int perfil_pode(int perfil, Funcionalidade f);

/* ===================== Cálculo do orçamento (RN08) — algoritmo A3 ===================== */

/*
* Calcula o total de um orçamento:
* total = soma dos serviços + soma(quantidade x preço unitário das peças)
*
* servicos[] valores dos serviços, em centavos (qtd_servicos posições)
* quantidades[] quantidade de cada peça (qtd_pecas posições)
* precos[] preço unitário de cada peça, em centavos
*
* Devolve o total em centavos, ou -1 se algum valor for negativo, alguma
* quantidade for zero ou o total passar de R$ 999.999.999,99.
*/
long long orcamento_calcular_total(const long long servicos[], int qtd_servicos,
const int quantidades[], const long long precos[], int qtd_pecas);

#endif

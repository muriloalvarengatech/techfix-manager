/*
* modelos.h — Estruturas de dados (structs) do sistema.
*
* Os nomes seguem o modelo de dados (DER e sql/schema.sql), para que o
* documento do PIM, o banco e o código usem a mesma nomenclatura.
*
* Valores em dinheiro são sempre "long long" em centavos (ex.: R$ 12,50 = 1250).
* Isso evita os erros de arredondamento que aconteceriam com float ou double.
*/
#ifndef MODELOS_H
#define MODELOS_H

#include "config.h"

/* Funcionário que usa o sistema (tabela usuario). A senha não fica aqui:
ela nunca sai do banco, nem mesmo protegida. */
typedef struct {
long long id;
char login[TAM_LOGIN];
char nome[TAM_NOME];
int perfil; /* valor de Perfil (regras.h) */
int ativo; /* 1 = pode entrar; 0 = bloqueado */
} Usuario;

/* Cliente da assistência técnica (tabela cliente). */
typedef struct {
long long id;
char nome[TAM_NOME];
char telefone[TAM_TELEFONE];
char email[TAM_EMAIL]; /* opcional: texto vazio quando não informado */
int ativo; /* cliente inativo não recebe nova OS (RN01) */
char criado_em[TAM_DATA];
} Cliente;

/* Equipamento de um cliente (tabela equipamento). Os campos cliente_nome
e cliente_ativo vêm da tabela cliente, para exibição e validação. */
typedef struct {
long long id;
long long cliente_id;
char cliente_nome[TAM_NOME];
int cliente_ativo;
char tipo[TAM_CURTO];
char marca[TAM_CURTO];
char modelo[TAM_CURTO];
char identificador[TAM_CURTO]; /* número de série ou etiqueta interna; único */
char descricao[TAM_TEXTO];
} Equipamento;

/* Ordem de serviço com os dados das tabelas relacionadas já resolvidos
(equipamento, cliente, técnico e quem abriu). */
typedef struct {
long long id;
long long equipamento_id;
char equipamento[3 * TAM_CURTO]; /* "tipo marca modelo" */
char identificador[TAM_CURTO];
long long cliente_id;
char cliente_nome[TAM_NOME];
long long tecnico_id; /* 0 = nenhum técnico atribuído */
char tecnico_nome[TAM_NOME];
char aberta_por[TAM_NOME];
int status; /* valor de StatusOS (regras.h) */
char problema[TAM_TEXTO];
char aberta_em[TAM_DATA];
char atualizada_em[TAM_DATA];
} OrdemServico;

/* Peça do estoque (tabela peca). O saldo só muda por movimentação (RN10). */
typedef struct {
long long id;
char codigo[TAM_CODIGO];
char descricao[TAM_NOME];
long long preco_centavos;
int saldo;
} Peca;

/* Entrada ou saída de estoque (tabela movimentacao). */
typedef struct {
long long id;
char tipo; /* 'E' = entrada, 'S' = saída */
int quantidade;
char motivo[TAM_TEXTO];
long long os_id; /* 0 quando não está ligada a uma OS */
char usuario[TAM_NOME];
char data[TAM_DATA];
} Movimentacao;

/* Peça citada em um diagnóstico, orçamento ou lista de peças utilizadas. */
typedef struct {
long long peca_id;
char codigo[TAM_CODIGO];
char descricao[TAM_NOME];
int quantidade;
long long preco_unit_centavos;
int saldo_atual; /* saldo no estoque agora (para alertas) */
} ItemPeca;

/* Serviço (mão de obra) de um orçamento. */
typedef struct {
char descricao[TAM_TEXTO];
long long valor_centavos;
} ItemServico;

/* Diagnóstico técnico de uma OS, com as peças previstas para o reparo. */
typedef struct {
long long os_id;
char descricao[TAM_TEXTO]; /* problema identificado */
char procedimento[TAM_TEXTO]; /* reparo recomendado */
char tecnico[TAM_NOME];
char data[TAM_DATA];
ItemPeca pecas[MAX_ITENS];
int qtd_pecas;
} Diagnostico;

/* Orçamento de uma OS com seus serviços, peças e total calculado. */
typedef struct {
long long id;
long long os_id;
char decisao; /* 0 = aguardando, 'A' = aprovado, 'R' = recusado */
char criado_em[TAM_DATA];
char decidido_em[TAM_DATA];
ItemServico servicos[MAX_ITENS];
int qtd_servicos;
ItemPeca pecas[MAX_ITENS];
int qtd_pecas;
long long total_centavos;
} Orcamento;

/* Linha genérica de histórico: mudança de status, observação ou serviço realizado. */
typedef struct {
char data[TAM_DATA];
char usuario[TAM_NOME];
char texto[TAM_TEXTO];
} RegistroOS;

/* Filtro de consultas e relatórios de OS. Campo com 0 ou texto vazio = sem filtro. */
typedef struct {
int status;
long long tecnico_id;
long long cliente_id;
long long equipamento_id;
char data_inicio[11]; /* "AAAA-MM-DD" */
char data_fim[11];
} FiltroOS;

#endif

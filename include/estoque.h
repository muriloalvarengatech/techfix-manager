/*
* estoque.h — Peças e movimentações de estoque (RF09, RF10, RN10).
*
* Regra central: o saldo só muda por movimentação. Toda entrada ou saída
* grava um registro na tabela "movimentacao" na mesma transação em que o
* saldo é alterado. Assim, o saldo sempre é igual à soma das movimentações.
*/
#ifndef ESTOQUE_H
#define ESTOQUE_H

#include "erro.h"
#include "modelos.h"

/* Cadastra uma peça com código único. Se saldo_inicial > 0, registra uma
entrada "Saldo inicial". Preenche peca->id e peca->saldo. */
Resultado peca_cadastrar(Peca *peca, int saldo_inicial, long long usuario_id);

/* Busca uma peça pelo código numérico interno (id). */
Resultado peca_buscar_id(long long id, Peca *saida);

/* Busca uma peça pelo código cadastrado (ex.: "TELA-A10"), sem diferenciar maiúsculas. */
Resultado peca_buscar_codigo(const char *codigo, Peca *saida);

/* Busca por código ou parte da descrição. Termo vazio lista todas.
Devolve a quantidade encontrada ou -1 em caso de erro. */
int peca_buscar(const char *termo, Peca vetor[], int maximo);

/* Altera descrição e preço. A quantidade não é editável aqui (RN10). */
Resultado peca_atualizar(long long id, const char *descricao, long long preco_centavos);

/* Registra a chegada de peças (quantidade > 0) e soma ao saldo. */
Resultado estoque_entrada(long long peca_id, int quantidade, const char *motivo, long long usuario_id);

/* Registra uma saída e subtrai do saldo. Recusa se o saldo for insuficiente.
os_id = 0 para saídas avulsas (perda, devolução ao fornecedor). */
Resultado estoque_saida(long long peca_id, int quantidade, const char *motivo, long long os_id, long long usuario_id);

/* Últimas movimentações da peça, da mais recente para a mais antiga. */
int estoque_movimentacoes(long long peca_id, Movimentacao vetor[], int maximo);

/* Soma das movimentações (entradas menos saídas). Deve ser igual ao saldo da peça. */
long long estoque_saldo_calculado(long long peca_id);

#endif

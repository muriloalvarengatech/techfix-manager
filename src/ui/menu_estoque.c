/*
* menu_estoque.c — Telas de peças e movimentações (HU16 a HU18, HU33).
*/
#include <stdio.h>
#include <string.h>
#include "estoque.h"
#include "io.h"
#include "ui.h"
#include "validacao.h"

/* Tela de cadastro de peça. */
static void peca_nova(void)
{
Peca p;
long long saldo;

memset(&p, 0, sizeof(p));
io_titulo("Cadastrar peça");
if (!io_ler_texto("Código da peça (ex.: TELA-A10)", p.codigo, sizeof(p.codigo), 1)) return;
if (!io_ler_texto("Descrição", p.descricao, sizeof(p.descricao), 1)) return;
if (!io_ler_dinheiro("Preço unitário de venda", &p.preco_centavos)) return;
if (!io_ler_inteiro("Quantidade inicial em estoque", 0, 99999, &saldo)) return;
if (ui_resultado(peca_cadastrar(&p, (int)saldo, sessao.id), NULL))
io_ok("Peça %s cadastrada (número %lld) com saldo %d.", p.codigo, p.id, p.saldo);
}

/* Mostra a peça, o saldo e as últimas movimentações. */
static void peca_consultar(void)
{
long long id;
Peca p;
Movimentacao m[MAX_RESULTADOS];
char preco[32];
int n, i;

io_titulo("Consultar peça e movimentações");
if (!ui_selecionar_peca(&id)) return;
if (peca_buscar_id(id, &p) != RES_OK) { io_erro("%s", erro_mensagem()); return; }
formatar_dinheiro(p.preco_centavos, preco, sizeof(preco));
printf("\nCódigo: %s\nDescrição: %s\nPreço: %s\nSaldo disponível: %d\n", p.codigo, p.descricao, preco, p.saldo);
n = estoque_movimentacoes(id, m, MAX_RESULTADOS);
printf("\nÚltimas movimentações:\n");
if (n <= 0) printf(" (nenhuma)\n");
for (i = 0; i < n; i++) {
printf(" %s - %s de %d - %s", m[i].data, m[i].tipo == 'E' ? "Entrada" : "Saída", m[i].quantidade, m[i].motivo);
if (m[i].os_id) printf(" (OS %lld)", m[i].os_id);
printf(" - %s\n", m[i].usuario);
}
}

/* Tela de alteração de descrição e preço. */
static void peca_editar(void)
{
long long id, preco;
Peca p;
char descricao[TAM_NOME];

io_titulo("Atualizar peça");
if (!ui_selecionar_peca(&id)) return;
if (peca_buscar_id(id, &p) != RES_OK) { io_erro("%s", erro_mensagem()); return; }
printf("A quantidade só muda por entrada ou saída de estoque (RN10).\n");
if (!io_ler_texto("Nova descrição", descricao, sizeof(descricao), 1)) return;
if (!io_ler_dinheiro("Novo preço unitário", &preco)) return;
ui_resultado(peca_atualizar(id, descricao, preco), "Peça atualizada. Orçamentos já gravados mantêm o preço antigo.");
}

/* Tela de entrada de estoque. */
static void entrada_nova(void)
{
long long id, qtd;
char motivo[TAM_TEXTO];

io_titulo("Registrar entrada de peça");
if (!ui_selecionar_peca(&id)) return;
if (!io_ler_inteiro("Quantidade recebida", 1, 99999, &qtd)) return;
if (!io_ler_texto("Motivo (ex.: compra NF 123)", motivo, sizeof(motivo), 1)) return;
ui_resultado(estoque_entrada(id, (int)qtd, motivo, sessao.id), "Entrada registrada.");
}

/* Tela de saída avulsa de estoque. */
static void saida_nova(void)
{
long long id, qtd;
char motivo[TAM_TEXTO];

io_titulo("Registrar saída avulsa");
printf("Use para perdas ou devoluções ao fornecedor. Peças usadas em OS são registradas no menu Ordens de serviço.\n");
if (!ui_selecionar_peca(&id)) return;
if (!io_ler_inteiro("Quantidade", 1, 99999, &qtd)) return;
if (!io_ler_texto("Motivo", motivo, sizeof(motivo), 1)) return;
ui_resultado(estoque_saida(id, (int)qtd, motivo, 0, sessao.id), "Saída registrada.");
}

/* Menu do módulo de estoque. */
void menu_estoque(void)
{
static const OpcaoMenu opcoes[] = {
{ "Cadastrar peça", F_PECA_GERIR, peca_nova },
{ "Consultar peça e movimentações", F_PECA_CONSULTAR, peca_consultar },
{ "Atualizar descrição e preço", F_PECA_GERIR, peca_editar },
{ "Registrar entrada", F_ESTOQUE_ENTRADA, entrada_nova },
{ "Registrar saída avulsa", F_ESTOQUE_SAIDA, saida_nova }
};
ui_menu("Estoque de peças", opcoes, (int)(sizeof(opcoes) / sizeof(opcoes[0])));
}

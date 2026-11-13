/*
* menu_ordens.c — Telas do fluxo da ordem de serviço (HU11 a HU29).
*/
#include <stdio.h>
#include <string.h>
#include "estoque.h"
#include "io.h"
#include "ordens.h"
#include "ui.h"
#include "validacao.h"

/* Tela de abertura de OS. */
static void os_nova(void)
{
long long equip_id, os_id;
char problema[TAM_TEXTO];

io_titulo("Abrir ordem de serviço");
printf("Escolha o equipamento (cadastre antes no menu Equipamentos, se for novo).\n");
if (!ui_selecionar_equipamento(&equip_id)) return;
if (!io_ler_texto("Problema relatado pelo cliente", problema, sizeof(problema), 1)) return;
if (ui_resultado(os_abrir(equip_id, problema, sessao.id, &os_id), NULL))
io_ok("OS número %lld aberta. Informe este número ao cliente.", os_id);
}

/* Tela de consulta de OS. */
static void os_consultar(void)
{
long long id;
io_titulo("Consultar ordem de serviço");
if (ui_selecionar_os(&id)) ui_mostrar_os(id);
}

/* Tela de atribuição de técnico. */
static void os_tecnico(void)
{
long long os_id, tecnico_id;
io_titulo("Atribuir técnico");
if (!ui_selecionar_os(&os_id)) return;
if (!ui_selecionar_tecnico(&tecnico_id)) return;
ui_resultado(os_atribuir_tecnico(os_id, tecnico_id, sessao.id), "Técnico atribuído.");
}

/* Tela de registro de observação. */
static void os_observacao(void)
{
long long os_id;
char texto[TAM_TEXTO];
io_titulo("Registrar observação");
if (!ui_selecionar_os(&os_id)) return;
if (!io_ler_texto("Observação", texto, sizeof(texto), 1)) return;
ui_resultado(os_observar(os_id, sessao.id, texto), "Observação registrada.");
}

/* Tela que inicia o diagnóstico. */
static void diagnostico_iniciar(void)
{
long long os_id;
io_titulo("Iniciar diagnóstico");
if (!ui_selecionar_os(&os_id)) return;
ui_resultado(os_iniciar_diagnostico(os_id, sessao.id, sessao.perfil), "OS em diagnóstico.");
}

/* Tela de registro do diagnóstico e das peças previstas. */
static void diagnostico_novo(void)
{
long long os_id, pecas[MAX_ITENS], qtd;
int quantidades[MAX_ITENS], n = 0;
char descricao[TAM_TEXTO], procedimento[TAM_TEXTO];

io_titulo("Registrar diagnóstico");
if (!ui_selecionar_os(&os_id)) return;
if (!io_ler_texto("Problema identificado", descricao, sizeof(descricao), 1)) return;
if (!io_ler_texto("Procedimento recomendado", procedimento, sizeof(procedimento), 1)) return;
while (n < MAX_ITENS && io_confirmar("Incluir uma peça prevista para o reparo?")) {
if (!ui_selecionar_peca(&pecas[n])) break;
if (!io_ler_inteiro("Quantidade prevista", 1, 9999, &qtd)) return;
quantidades[n++] = (int)qtd;
}
if (io_fim_entrada()) return;
ui_resultado(diagnostico_registrar(os_id, sessao.id, sessao.perfil, descricao, procedimento, pecas, quantidades, n),
"Diagnóstico registrado. As peças previstas não baixam o estoque.");
}

/* Tela do orçamento: mostra as peças previstas, pede os serviços e confirma o total. */
static void orcamento_novo(void)
{
long long os_id, total;
Diagnostico d;
ItemServico servicos[MAX_ITENS];
long long valores[MAX_ITENS], precos[MAX_ITENS];
int quantidades[MAX_ITENS], n = 0, i;
char valor[32];

io_titulo("Elaborar ou corrigir orçamento");
if (!ui_selecionar_os(&os_id)) return;
if (diagnostico_buscar(os_id, &d) != RES_OK) { io_erro("%s", erro_mensagem()); return; }

printf("\nPeças previstas no diagnóstico (preço atual):\n");
if (d.qtd_pecas == 0) printf(" (nenhuma)\n");
for (i = 0; i < d.qtd_pecas; i++) {
formatar_dinheiro(d.pecas[i].preco_unit_centavos, valor, sizeof(valor));
printf(" %s %s x %d - %s cada\n", d.pecas[i].codigo, d.pecas[i].descricao, d.pecas[i].quantidade, valor);
/* RF18: alerta de estoque insuficiente. */
if (d.pecas[i].saldo_atual < d.pecas[i].quantidade)
io_aviso("Estoque insuficiente de %s: saldo %d, previsto %d.", d.pecas[i].codigo,
d.pecas[i].saldo_atual, d.pecas[i].quantidade);
quantidades[i] = d.pecas[i].quantidade;
precos[i] = d.pecas[i].preco_unit_centavos;
}

printf("\nInforme os serviços (mão de obra). Deixe a descrição em branco para terminar.\n");
while (n < MAX_ITENS) {
char linha[TAM_LINHA];
printf("Descrição do serviço %d (Enter em branco para terminar): ", n + 1);
if (!io_ler_linha(linha, sizeof(linha))) return;
texto_aparar(linha);
if (linha[0] == '\0') break;
if (strlen(linha) >= TAM_TEXTO) { io_erro("Descrição muito longa."); continue; }
texto_copiar(servicos[n].descricao, TAM_TEXTO, linha);
if (!io_ler_dinheiro("Valor do serviço", &servicos[n].valor_centavos)) return;
valores[n] = servicos[n].valor_centavos;
n++;
}
total = orcamento_calcular_total(valores, n, quantidades, precos, d.qtd_pecas);
if (total < 0) { io_erro("Valores fora do limite permitido."); return; }
formatar_dinheiro(total, valor, sizeof(valor));
printf("\nTotal do orçamento: %s\n", valor);
if (!io_confirmar("Gravar este orçamento?")) { io_aviso("Orçamento não gravado."); return; }
ui_resultado(orcamento_elaborar(os_id, sessao.id, servicos, n), "Orçamento gravado. A OS aguarda a decisão do cliente.");
}

/* Tela da decisão do cliente sobre o orçamento. */
static void orcamento_decisao(void)
{
long long os_id, opcao;
Orcamento o;
char valor[32];

io_titulo("Registrar decisão do cliente");
if (!ui_selecionar_os(&os_id)) return;
if (orcamento_buscar(os_id, &o) != RES_OK) { io_erro("%s", erro_mensagem()); return; }
formatar_dinheiro(o.total_centavos, valor, sizeof(valor));
printf("Total do orçamento: %s\n", valor);
printf("1 - Cliente APROVOU\n2 - Cliente RECUSOU\n0 - Cancelar\n");
if (!io_ler_inteiro("Decisão", 0, 2, &opcao) || opcao == 0) return;
if (!io_confirmar(opcao == 1 ? "Confirma a APROVAÇÃO? Ela não poderá ser alterada" : "Confirma a RECUSA? Elanão poderá ser alterada"))
return;
ui_resultado(orcamento_decidir(os_id, sessao.id, opcao == 1),
opcao == 1 ? "Aprovação registrada. A manutenção pode ser iniciada." : "Recusa registrada. O equipamento pode ser liberado para devolução.");
}

/* Tela que inicia a manutenção, com alerta de estoque insuficiente. */
static void manutencao_inicio(void)
{
long long os_id;
Orcamento o;
int i;

io_titulo("Iniciar manutenção");
if (!ui_selecionar_os(&os_id)) return;
if (orcamento_buscar(os_id, &o) == RES_OK)
for (i = 0; i < o.qtd_pecas; i++)
if (o.pecas[i].saldo_atual < o.pecas[i].quantidade)
io_aviso("Estoque insuficiente de %s: saldo %d, previsto %d.", o.pecas[i].codigo,
o.pecas[i].saldo_atual, o.pecas[i].quantidade);
ui_resultado(manutencao_iniciar(os_id, sessao.id, sessao.perfil), "Manutenção iniciada.");
}

/* Tela de registro de serviço realizado. */
static void manutencao_servico(void)
{
long long os_id;
char descricao[TAM_TEXTO];
io_titulo("Registrar serviço realizado");
if (!ui_selecionar_os(&os_id)) return;
if (!io_ler_texto("Serviço realizado", descricao, sizeof(descricao), 1)) return;
ui_resultado(manutencao_registrar_servico(os_id, sessao.id, sessao.perfil, descricao), "Serviço registrado.");
}

/* Tela de registro de peça utilizada (baixa no estoque). */
static void manutencao_peca(void)
{
long long os_id, peca_id, qtd;
io_titulo("Registrar peça utilizada");
printf("A peça sai do estoque na mesma operação. Se faltar saldo, nada é gravado.\n");
if (!ui_selecionar_os(&os_id)) return;
if (!ui_selecionar_peca(&peca_id)) return;
if (!io_ler_inteiro("Quantidade utilizada", 1, 9999, &qtd)) return;
ui_resultado(manutencao_usar_peca(os_id, peca_id, (int)qtd, sessao.id, sessao.perfil),
"Peça registrada e baixada do estoque.");
}

/* Tela de conclusão da manutenção. */
static void manutencao_fim(void)
{
long long os_id;
char obs[TAM_TEXTO];
io_titulo("Concluir manutenção");
if (!ui_selecionar_os(&os_id)) return;
if (!io_ler_texto("Observação final para o cliente", obs, sizeof(obs), 1)) return;
ui_resultado(manutencao_concluir(os_id, sessao.id, sessao.perfil, obs), "Manutenção concluída. Aguardando pagamento.");
}

/* Tela de pagamento: mostra o valor final e pede a forma. */
static void pagamento_novo(void)
{
long long os_id, devido, pago, forma;
char valor[32];
int i;

io_titulo("Registrar pagamento");
if (!ui_selecionar_os(&os_id)) return;
if (os_valor_final(os_id, &devido) != RES_OK) { io_erro("%s", erro_mensagem()); return; }
formatar_dinheiro(devido, valor, sizeof(valor));
printf("Valor final da OS: %s (total do orçamento aprovado).\n", valor);
printf("Formas de pagamento:\n");
for (i = 0; i < QTD_FORMAS_PAGAMENTO; i++) printf(" %d - %s\n", i + 1, FORMAS_PAGAMENTO[i]);
printf(" 0 - Cancelar\n");
if (!io_ler_inteiro("Forma", 0, QTD_FORMAS_PAGAMENTO, &forma) || forma == 0) return;
if (!io_ler_dinheiro("Confirme o valor recebido", &pago)) return;
ui_resultado(pagamento_registrar(os_id, pago, (int)forma - 1, sessao.id), "Pagamento registrado.");
}

/* Tela de liberação do equipamento. */
static void os_liberacao(void)
{
long long os_id;
io_titulo("Liberar equipamento para entrega");
if (!ui_selecionar_os(&os_id)) return;
ui_resultado(os_liberar(os_id, sessao.id), "Equipamento liberado para entrega.");
}

/* Tela de registro da entrega. */
static void os_entrega_nova(void)
{
long long os_id;
char recebido[TAM_NOME];
io_titulo("Registrar entrega");
if (!ui_selecionar_os(&os_id)) return;
if (!io_ler_texto("Nome de quem recebeu o equipamento", recebido, sizeof(recebido), 1)) return;
ui_resultado(entrega_registrar(os_id, recebido, sessao.id), "Entrega registrada.");
}

/* Tela de encerramento da OS. */
static void os_encerramento(void)
{
long long os_id;
io_titulo("Encerrar ordem de serviço");
if (!ui_selecionar_os(&os_id)) return;
if (!io_confirmar("Depois de encerrada, a OS não aceita mais alterações. Confirma?")) return;
ui_resultado(os_encerrar(os_id, sessao.id), "OS encerrada.");
}

/* Menu do módulo de ordens de serviço. */
void menu_ordens(void)
{
static const OpcaoMenu opcoes[] = {
{ "Abrir OS", F_OS_ABRIR, os_nova },
{ "Consultar OS", F_OS_CONSULTAR, os_consultar },
{ "Atribuir técnico", F_OS_ATRIBUIR, os_tecnico },
{ "Registrar observação", F_OS_OBSERVAR, os_observacao },
{ "Iniciar diagnóstico", F_DIAGNOSTICO, diagnostico_iniciar },
{ "Registrar ou corrigir diagnóstico", F_DIAGNOSTICO, diagnostico_novo },
{ "Elaborar ou corrigir orçamento", F_ORCAMENTO, orcamento_novo },
{ "Registrar decisão do cliente", F_DECISAO, orcamento_decisao },
{ "Iniciar manutenção", F_MANUTENCAO, manutencao_inicio },
{ "Registrar serviço realizado", F_MANUTENCAO, manutencao_servico },
{ "Registrar peça utilizada", F_MANUTENCAO, manutencao_peca },
{ "Concluir manutenção", F_MANUTENCAO, manutencao_fim },
{ "Registrar pagamento", F_PAGAMENTO, pagamento_novo },
{ "Liberar equipamento", F_ENTREGA, os_liberacao },
{ "Registrar entrega", F_ENTREGA, os_entrega_nova },
{ "Encerrar OS", F_ENCERRAR, os_encerramento }
};
ui_menu("Ordens de serviço", opcoes, (int)(sizeof(opcoes) / sizeof(opcoes[0])));
}

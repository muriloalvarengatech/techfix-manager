/*
* menu_cadastros.c — Telas de clientes e equipamentos (HU01 a HU06).
*/
#include <stdio.h>
#include <string.h>
#include "cadastros.h"
#include "io.h"
#include "ordens.h"
#include "ui.h"

/* ---------------------------------------------------------------------
* Clientes
* --------------------------------------------------------------------- */

static int ler_dados_cliente(Cliente *c)
{
if (!io_ler_texto("Nome", c->nome, sizeof(c->nome), 1)) return 0;
if (!io_ler_texto("Telefone com DDD", c->telefone, sizeof(c->telefone), 1)) return 0;
if (!io_ler_texto("E-mail", c->email, sizeof(c->email), 0)) return 0;
return 1;
}

/* Tela de cadastro de cliente. */
static void cliente_novo(void)
{
Cliente c;
Resultado r;

memset(&c, 0, sizeof(c));
io_titulo("Cadastrar cliente");
/* Só coletamos o necessário ao atendimento (RNF05, decisão D02). */
for (;;) {
if (!ler_dados_cliente(&c)) return;
r = cliente_cadastrar(&c);
if (r == RES_OK) { io_ok("Cliente cadastrado com o código %lld.", c.id); return; }
io_erro("%s", erro_mensagem());
if (!io_confirmar("Deseja corrigir os dados e tentar de novo?")) return;
}
}

/* Mostra os dados do cliente e seus equipamentos. */
static void cliente_detalhar(long long id)
{
Cliente c;
Equipamento eq[MAX_RESULTADOS];
int n, i;

if (cliente_buscar_id(id, &c) != RES_OK) { io_erro("%s", erro_mensagem()); return; }
io_titulo("Dados do cliente");
printf("Código: %lld\nNome: %s\nTelefone: %s\nE-mail: %s\nSituação: %s\nCadastrado em: %s\n",
c.id, c.nome, c.telefone, c.email[0] ? c.email : "(não informado)", c.ativo ? "Ativo" : "Inativo", c.criado_em);
n = equipamento_listar_cliente(c.id, eq, MAX_RESULTADOS);
printf("\nEquipamentos (%d):\n", n > 0 ? n : 0);
for (i = 0; i < n; i++)
printf(" %lld - %s %s %s (identificador %s)\n", eq[i].id, eq[i].tipo, eq[i].marca, eq[i].modelo, eq[i].identificador);
}

/* Tela de consulta de cliente. */
static void cliente_consultar(void)
{
long long id;
io_titulo("Consultar cliente");
if (ui_selecionar_cliente(&id, 0)) cliente_detalhar(id);
}

/* Tela de edição de cliente. */
static void cliente_editar(void)
{
Cliente c, novo;
long long id;

io_titulo("Editar cliente");
if (!ui_selecionar_cliente(&id, 0)) return;
if (cliente_buscar_id(id, &c) != RES_OK) { io_erro("%s", erro_mensagem()); return; }
printf("Dados atuais: %s | %s | %s\n", c.nome, c.telefone, c.email[0] ? c.email : "(sem e-mail)");
printf("Digite os dados novamente (os campos obrigatórios não podem ficar em branco).\n");
novo = c;
if (!ler_dados_cliente(&novo)) { io_aviso("Edição cancelada; nada foi alterado."); return; }
ui_resultado(cliente_atualizar(&novo), "Dados do cliente atualizados.");
}

/* Tela de inativação ou reativação de cliente. */
static void cliente_situacao(void)
{
Cliente c;
long long id;

io_titulo("Inativar ou reativar cliente");
if (!ui_selecionar_cliente(&id, 0)) return;
if (cliente_buscar_id(id, &c) != RES_OK) { io_erro("%s", erro_mensagem()); return; }
if (c.ativo) {
printf("Inativar não apaga o cliente nem o histórico de atendimentos (RN01).\n");
printf("Cliente inativo não recebe novos equipamentos nem novas OS.\n");
if (!io_confirmar("Confirma a inativação?")) return;
ui_resultado(cliente_definir_ativo(id, 0), "Cliente inativado.");
} else {
if (!io_confirmar("O cliente está inativo. Deseja reativá-lo?")) return;
ui_resultado(cliente_definir_ativo(id, 1), "Cliente reativado.");
}
}

/* Menu do módulo de clientes. */
void menu_clientes(void)
{
static const OpcaoMenu opcoes[] = {
{ "Cadastrar cliente", F_CLIENTE_GERIR, cliente_novo },
{ "Consultar cliente", F_CLIENTE_CONSULTAR, cliente_consultar },
{ "Editar cliente", F_CLIENTE_GERIR, cliente_editar },
{ "Inativar ou reativar cliente", F_CLIENTE_GERIR, cliente_situacao }
};
ui_menu("Clientes", opcoes, (int)(sizeof(opcoes) / sizeof(opcoes[0])));
}

/* ---------------------------------------------------------------------
* Equipamentos
* --------------------------------------------------------------------- */

static int ler_dados_equipamento(Equipamento *e)
{
if (!io_ler_texto("Tipo (ex.: Notebook, Celular)", e->tipo, sizeof(e->tipo), 1)) return 0;
if (!io_ler_texto("Marca", e->marca, sizeof(e->marca), 1)) return 0;
if (!io_ler_texto("Modelo", e->modelo, sizeof(e->modelo), 1)) return 0;
printf("Identificador: número de série. Se não houver, use uma etiqueta interna (ex.: TF-0001).\n");
if (!io_ler_texto("Identificador", e->identificador, sizeof(e->identificador), 1)) return 0;
if (!io_ler_texto("Descrição (cor, acessórios entregues etc.)", e->descricao, sizeof(e->descricao), 0)) return 0;
return 1;
}

/* Tela de cadastro de equipamento. */
static void equipamento_novo(void)
{
Equipamento e;
Resultado r;

memset(&e, 0, sizeof(e));
io_titulo("Cadastrar equipamento");
printf("Primeiro, escolha o cliente dono do equipamento.\n");
if (!ui_selecionar_cliente(&e.cliente_id, 1)) return;
for (;;) {
if (!ler_dados_equipamento(&e)) return;
r = equipamento_cadastrar(&e);
if (r == RES_OK) { io_ok("Equipamento cadastrado com o código %lld.", e.id); return; }
io_erro("%s", erro_mensagem());
if (!io_confirmar("Deseja corrigir os dados e tentar de novo?")) return;
}
}

/* Mostra o equipamento e as OS dele. */
static void equipamento_consultar(void)
{
Equipamento e;
OrdemServico lista[MAX_RESULTADOS];
FiltroOS f;
long long id;
int n, i;

io_titulo("Consultar equipamento");
if (!ui_selecionar_equipamento(&id)) return;
if (equipamento_buscar_id(id, &e) != RES_OK) { io_erro("%s", erro_mensagem()); return; }
printf("\nCódigo: %lld\nEquipamento: %s %s %s\nIdentificador: %s\nDescrição: %s\nCliente: %s (código %lld)\n",
e.id, e.tipo, e.marca, e.modelo, e.identificador, e.descricao[0] ? e.descricao : "(sem descrição)",
e.cliente_nome, e.cliente_id);
memset(&f, 0, sizeof(f));
f.equipamento_id = id;
n = os_listar(&f, lista, MAX_RESULTADOS);
printf("\nOrdens de serviço deste equipamento (%d):\n", n > 0 ? n : 0);
for (i = 0; i < n; i++)
printf(" OS %lld - %s - %s - %s\n", lista[i].id, lista[i].aberta_em, status_nome(lista[i].status), lista[i].problema);
}

/* Tela de edição de equipamento. */
static void equipamento_editar(void)
{
Equipamento e, novo;
long long id;

io_titulo("Editar equipamento");
if (!ui_selecionar_equipamento(&id)) return;
if (equipamento_buscar_id(id, &e) != RES_OK) { io_erro("%s", erro_mensagem()); return; }
printf("Dados atuais: %s %s %s | %s | %s\n", e.tipo, e.marca, e.modelo, e.identificador, e.descricao);
printf("O cliente vinculado não muda nesta tela. Digite os dados novamente.\n");
novo = e;
if (!ler_dados_equipamento(&novo)) { io_aviso("Edição cancelada; nada foi alterado."); return; }
ui_resultado(equipamento_atualizar(&novo), "Equipamento atualizado.");
}

/* Menu do módulo de equipamentos. */
void menu_equipamentos(void)
{
static const OpcaoMenu opcoes[] = {
{ "Cadastrar equipamento", F_EQUIP_GERIR, equipamento_novo },
{ "Consultar equipamento e histórico", F_EQUIP_CONSULTAR, equipamento_consultar },
{ "Editar equipamento", F_EQUIP_GERIR, equipamento_editar }
};
ui_menu("Equipamentos", opcoes, (int)(sizeof(opcoes) / sizeof(opcoes[0])));
}

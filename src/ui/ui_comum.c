/*
* ui_comum.c — Menus, seleção de registros e exibição de uma OS.
*/
#include <stdio.h>
#include <string.h>
#include "cadastros.h"
#include "erro.h"
#include "estoque.h"
#include "io.h"
#include "ordens.h"
#include "ui.h"
#include "validacao.h"

Usuario sessao;

/* ---------------------------------------------------------------------
* Menus
* --------------------------------------------------------------------- */

void ui_menu(const char *titulo, const OpcaoMenu opcoes[], int quantidade)
{
int visiveis[32];
int n, i;
long long escolha;

for (;;) {
io_titulo(titulo);
n = 0;
/* Só mostra o que o perfil pode usar (matriz de permissões). */
for (i = 0; i < quantidade && n < 32; i++) {
if (perfil_pode(sessao.perfil, opcoes[i].permissao)) {
visiveis[n++] = i;
printf("%2d - %s\n", n, opcoes[i].texto);
}
}
printf(" 0 - Voltar\n");
if (!io_ler_inteiro("Escolha uma opção", 0, n, &escolha)) return;
if (escolha == 0) return;
opcoes[visiveis[escolha - 1]].acao();
if (io_fim_entrada()) return;
}
}

/* ---------------------------------------------------------------------
* Formatação
* --------------------------------------------------------------------- */

void ui_coluna(const char *texto, int largura)
{
char cortado[TAM_TEXTO];
size_t n;
int i;

texto_copiar(cortado, sizeof(cortado), texto);
n = texto_caracteres(cortado);
/* Corta por caracteres (não por bytes) para não quebrar acentos. */
while ((int)n > largura) {
size_t len = strlen(cortado);
do { len--; } while (len > 0 && ((unsigned char)cortado[len] & 0xC0) == 0x80);
cortado[len] = '\0';
n--;
}
printf("%s", cortado);
for (i = (int)n; i < largura; i++) putchar(' ');
putchar(' ');
}

/* Imprime uma linha de traços. */
void ui_linha(int largura)
{
int i;
for (i = 0; i < largura; i++) putchar('-');
putchar('\n');
}

/* Exibe [OK] ou [ERRO] conforme o resultado da operação. */
int ui_resultado(int resultado, const char *mensagem_ok)
{
if (resultado == RES_OK) {
if (mensagem_ok) io_ok("%s", mensagem_ok);
return 1;
}
io_erro("%s", erro_mensagem());
return 0;
}

/* ---------------------------------------------------------------------
* Seleção de registros
* --------------------------------------------------------------------- */

/* Pede um termo; devolve 0 se o usuário digitar 0 ou a entrada terminar. */
static int pedir_termo(const char *rotulo, char *termo, size_t tamanho, int aceita_vazio)
{
for (;;) {
printf("%s: ", rotulo);
if (!io_ler_linha(termo, tamanho)) return 0;
texto_aparar(termo);
if (strcmp(termo, "0") == 0) return 0;
if (termo[0] != '\0' || aceita_vazio) return 1;
io_erro("Digite algo para buscar ou 0 para cancelar.");
}
}

/* Pede um código que esteja entre os listados. */
static int escolher_codigo(const long long codigos[], int n, long long *id)
{
long long escolha;
int i;

for (;;) {
if (!io_ler_inteiro("Digite o código desejado (0 para cancelar)", 0, 999999999LL, &escolha)) return 0;
if (escolha == 0) return 0;
for (i = 0; i < n; i++)
if (codigos[i] == escolha) { *id = escolha; return 1; }
io_erro("Código %lld não está na lista acima.", escolha);
}
}

/* Busca clientes, mostra a lista e pede o código escolhido. */
int ui_selecionar_cliente(long long *id, int somente_ativos)
{
Cliente lista[MAX_RESULTADOS];
long long codigos[MAX_RESULTADOS];
char termo[TAM_NOME];
int n, i;

for (;;) {
if (!pedir_termo("Código ou parte do nome do cliente (0 para cancelar)", termo, sizeof(termo), 0)) return 0;
n = cliente_buscar(termo, lista, MAX_RESULTADOS);
if (n <= 0) { io_erro("Nenhum cliente encontrado para \"%s\".", termo); continue; }
printf("\n");
ui_coluna("Código", 7); ui_coluna("Nome", 30); ui_coluna("Telefone", 18); ui_coluna("Situação", 8); printf("\n");
ui_linha(66);
for (i = 0; i < n; i++) {
char cod[16];
snprintf(cod, sizeof(cod), "%lld", lista[i].id);
ui_coluna(cod, 7); ui_coluna(lista[i].nome, 30); ui_coluna(lista[i].telefone, 18);
ui_coluna(lista[i].ativo ? "Ativo" : "Inativo", 8); printf("\n");
codigos[i] = lista[i].id;
}
if (n == MAX_RESULTADOS) io_aviso("Mostrando os %d primeiros. Refine a busca se não encontrar.", MAX_RESULTADOS);
if (!escolher_codigo(codigos, n, id)) return 0;
for (i = 0; i < n; i++)
if (lista[i].id == *id && somente_ativos && !lista[i].ativo) {
io_erro("Este cliente está inativo. Reative o cadastro no menu Clientes.");
return 0;
}
return 1;
}
}

/* Busca equipamentos, mostra a lista e pede o código escolhido. */
int ui_selecionar_equipamento(long long *id)
{
Equipamento lista[MAX_RESULTADOS];
long long codigos[MAX_RESULTADOS];
char termo[TAM_NOME];
int n, i;

for (;;) {
if (!pedir_termo("Identificador, modelo, código ou nome do cliente (0 para cancelar)", termo, sizeof(termo), 0)) return 0;
n = equipamento_buscar(termo, lista, MAX_RESULTADOS);
if (n <= 0) { io_erro("Nenhum equipamento encontrado para \"%s\".", termo); continue; }
printf("\n");
ui_coluna("Código", 7); ui_coluna("Equipamento", 30); ui_coluna("Identificador", 16); ui_coluna("Cliente", 22); printf("\n");
ui_linha(78);
for (i = 0; i < n; i++) {
char cod[16], desc[3 * TAM_CURTO];
snprintf(cod, sizeof(cod), "%lld", lista[i].id);
snprintf(desc, sizeof(desc), "%s %s %s", lista[i].tipo, lista[i].marca, lista[i].modelo);
ui_coluna(cod, 7); ui_coluna(desc, 30); ui_coluna(lista[i].identificador, 16); ui_coluna(lista[i].cliente_nome, 22);
printf("\n");
codigos[i] = lista[i].id;
}
return escolher_codigo(codigos, n, id);
}
}

/* Tabela resumida de OS (número, data, cliente, equipamento, status). */
static void listar_os_resumo(const OrdemServico lista[], int n)
{
int i;
printf("\n");
ui_coluna("OS", 6); ui_coluna("Aberta em", 16); ui_coluna("Cliente", 22); ui_coluna("Equipamento", 24); ui_coluna("Status", 22);
printf("\n");
ui_linha(94);
for (i = 0; i < n; i++) {
char cod[16];
snprintf(cod, sizeof(cod), "%lld", lista[i].id);
ui_coluna(cod, 6); ui_coluna(lista[i].aberta_em, 16); ui_coluna(lista[i].cliente_nome, 22);
ui_coluna(lista[i].equipamento, 24); ui_coluna(status_nome(lista[i].status), 22); printf("\n");
}
}

/* Pede o número da OS; Enter em branco lista as OS em andamento. */
int ui_selecionar_os(long long *id)
{
OrdemServico lista[MAX_RESULTADOS], tmp;
FiltroOS filtro;
char termo[32];
long long numero;
int n, i, k;

for (;;) {
if (!pedir_termo("Número da OS (Enter para listar as OS em andamento, 0 para cancelar)", termo, sizeof(termo), 1)) return 0;
if (termo[0] == '\0') {
memset(&filtro, 0, sizeof(filtro));
n = os_listar(&filtro, lista, MAX_RESULTADOS);
/* Remove as encerradas da lista exibida. */
for (i = 0, k = 0; i < n; i++)
if (lista[i].status != OS_ENCERRADA) lista[k++] = lista[i];
if (k == 0) { io_aviso("Não há OS em andamento."); continue; }
listar_os_resumo(lista, k);
continue;
}
if (!converter_inteiro(termo, &numero) || numero <= 0) { io_erro("Digite o número da OS."); continue; }
if (os_buscar_id(numero, &tmp) != RES_OK) { io_erro("%s", erro_mensagem()); continue; }
printf("OS %lld - %s - %s - %s\n", tmp.id, tmp.cliente_nome, tmp.equipamento, status_nome(tmp.status));
*id = numero;
return 1;
}
}

/* Busca peças, mostra a lista e pede o número escolhido. */
int ui_selecionar_peca(long long *id)
{
Peca lista[MAX_RESULTADOS];
long long codigos[MAX_RESULTADOS];
char termo[TAM_NOME], preco[32];
int n, i;

for (;;) {
if (!pedir_termo("Código ou parte da descrição da peça (Enter lista todas, 0 cancela)", termo, sizeof(termo), 1)) return 0;
n = peca_buscar(termo, lista, MAX_RESULTADOS);
if (n <= 0) { io_erro("Nenhuma peça encontrada."); continue; }
printf("\n");
ui_coluna("Nº", 5); ui_coluna("Código", 12); ui_coluna("Descrição", 30); ui_coluna("Preço", 14); ui_coluna("Saldo", 6); printf("\n");
ui_linha(71);
for (i = 0; i < n; i++) {
char cod[16];
snprintf(cod, sizeof(cod), "%lld", lista[i].id);
formatar_dinheiro(lista[i].preco_centavos, preco, sizeof(preco));
ui_coluna(cod, 5); ui_coluna(lista[i].codigo, 12); ui_coluna(lista[i].descricao, 30); ui_coluna(preco, 14);
printf("%d\n", lista[i].saldo);
codigos[i] = lista[i].id;
}
printf("(Use o número da primeira coluna.)\n");
return escolher_codigo(codigos, n, id);
}
}

/* Lista os técnicos ativos e pede o código escolhido. */
int ui_selecionar_tecnico(long long *id)
{
Usuario lista[MAX_RESULTADOS];
long long codigos[MAX_RESULTADOS];
int n, i;

n = usuario_listar(lista, MAX_RESULTADOS, 1);
if (n <= 0) { io_erro("Não há técnicos ativos cadastrados. Peça a um gerente para cadastrar."); return 0; }
printf("\nTécnicos ativos:\n");
for (i = 0; i < n; i++) {
printf(" %lld - %s\n", lista[i].id, lista[i].nome);
codigos[i] = lista[i].id;
}
return escolher_codigo(codigos, n, id);
}

/* ---------------------------------------------------------------------
* Exibição completa de uma OS
* --------------------------------------------------------------------- */

static void mostrar_registros(const char *titulo, const RegistroOS r[], int n)
{
int i;
printf("\n%s:\n", titulo);
if (n <= 0) { printf(" (nenhum registro)\n"); return; }
for (i = 0; i < n; i++) printf(" %s - %s: %s\n", r[i].data, r[i].usuario, r[i].texto);
}

/* Exibe todos os dados de uma OS, etapa por etapa. */
void ui_mostrar_os(long long os_id)
{
OrdemServico o;
Diagnostico d;
Orcamento orc;
RegistroOS regs[MAX_RESULTADOS];
ItemPeca usadas[MAX_ITENS];
char valor[32], forma[TAM_CURTO] = "", data[TAM_DATA] = "";
int n, i, pago;

if (os_buscar_id(os_id, &o) != RES_OK) { io_erro("%s", erro_mensagem()); return; }
io_titulo("Ordem de serviço");
printf("Número: %lld\nStatus: %s\n", o.id, status_nome(o.status));
printf("Cliente: %s (código %lld)\n", o.cliente_nome, o.cliente_id);
printf("Equipamento: %s - identificador %s\n", o.equipamento, o.identificador);
printf("Problema relatado: %s\n", o.problema);
printf("Aberta em %s por %s\n", o.aberta_em, o.aberta_por);
printf("Técnico: %s\n", o.tecnico_id ? o.tecnico_nome : "(não atribuído)");

if (diagnostico_buscar(os_id, &d) == RES_OK) {
printf("\nDiagnóstico (%s, %s):\n %s\n Procedimento: %s\n", d.tecnico, d.data, d.descricao, d.procedimento);
for (i = 0; i < d.qtd_pecas; i++)
printf(" Peça prevista: %s %s x %d (saldo atual %d)\n", d.pecas[i].codigo, d.pecas[i].descricao,
d.pecas[i].quantidade, d.pecas[i].saldo_atual);
}
if (orcamento_buscar(os_id, &orc) == RES_OK) {
printf("\nOrçamento (%s): %s\n", orc.criado_em,
orc.decisao == 'A' ? "APROVADO" : orc.decisao == 'R' ? "RECUSADO" : "aguardando decisão do cliente");
for (i = 0; i < orc.qtd_servicos; i++) {
formatar_dinheiro(orc.servicos[i].valor_centavos, valor, sizeof(valor));
printf(" Serviço: %s - %s\n", orc.servicos[i].descricao, valor);
}
for (i = 0; i < orc.qtd_pecas; i++) {
formatar_dinheiro((long long)orc.pecas[i].quantidade * orc.pecas[i].preco_unit_centavos, valor, sizeof(valor));
printf(" Peça: %s %s x %d - %s\n", orc.pecas[i].codigo, orc.pecas[i].descricao, orc.pecas[i].quantidade, valor);
}
formatar_dinheiro(orc.total_centavos, valor, sizeof(valor));
printf(" TOTAL: %s\n", valor);
}
n = os_servicos_realizados(os_id, regs, MAX_RESULTADOS);
if (n > 0) mostrar_registros("Serviços realizados", regs, n);
n = os_pecas_utilizadas(os_id, usadas, MAX_ITENS);
if (n > 0) {
printf("\nPeças utilizadas:\n");
for (i = 0; i < n; i++) printf(" %s %s x %d\n", usadas[i].codigo, usadas[i].descricao, usadas[i].quantidade);
}
{
char obs[TAM_TEXTO], quando[TAM_DATA];
if (os_conclusao(os_id, obs, quando)) printf("\nConclusão técnica (%s): %s\n", quando, obs);
}
pago = pagamento_situacao(os_id, forma, sizeof(forma), data);
if (pago == 1 && forma[0]) printf("\nPagamento: concluído (%s, %s)\n", forma, data);
else if (pago == 1) printf("\nPagamento: não exigido (orçamento sem valor)\n");
else if (pago == 0) printf("\nPagamento: pendente\n");

{
char recebido[TAM_NOME], quando[TAM_DATA];
if (os_entrega(os_id, recebido, quando)) printf("Entrega: %s, recebido por %s\n", quando, recebido);
}
n = os_observacoes(os_id, regs, MAX_RESULTADOS);
mostrar_registros("Observações", regs, n);
n = os_historico_status(os_id, regs, MAX_RESULTADOS);
mostrar_registros("Histórico de status", regs, n);
}

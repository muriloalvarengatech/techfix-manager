/*
* menu_admin.c — Login, usuários, consultas gerenciais e relatórios
* (HU07 a HU10, HU30 a HU32, HU34).
*/
#include <stdio.h>
#include <string.h>
#include "cadastros.h"
#include "io.h"
#include "ordens.h"
#include "relatorios.h"
#include "ui.h"
#include "validacao.h"

/* ---------------------------------------------------------------------
* Login e primeiro acesso
* --------------------------------------------------------------------- */

static int ler_nova_senha(char *senha, size_t tamanho)
{
char confirmacao[TAM_SENHA];

for (;;) {
printf("A senha deve ter pelo menos %d caracteres, com letras e números.\n", SENHA_MIN);
if (!io_ler_senha("Senha", senha, tamanho)) return 0;
if (!io_ler_senha("Repita a senha", confirmacao, sizeof(confirmacao))) return 0;
if (strcmp(senha, confirmacao) != 0) { io_erro("As senhas não conferem."); continue; }
return 1;
}
}

/* Cria o primeiro gerente quando o banco ainda não tem usuários. */
int ui_primeiro_acesso(void)
{
char login[TAM_LOGIN], nome[TAM_NOME], senha[TAM_SENHA];

io_titulo("Primeiro acesso");
printf("Nenhum usuário cadastrado. Crie agora o primeiro GERENTE do sistema.\n");
for (;;) {
if (!io_ler_texto("Login (sem espaços)", login, sizeof(login), 1)) return 0;
if (!io_ler_texto("Nome completo", nome, sizeof(nome), 1)) return 0;
if (!ler_nova_senha(senha, sizeof(senha))) return 0;
if (ui_resultado(usuario_criar(login, nome, PERFIL_GERENTE, senha, NULL), "Gerente criado. Faça o login.")) return 1;
}
}

/* Pede login e senha, com até 3 tentativas. */
int ui_login(void)
{
char login[TAM_LOGIN], senha[TAM_SENHA];
int tentativa;

io_titulo("Entrar no sistema");
for (tentativa = 1; tentativa <= 3; tentativa++) {
if (!io_ler_texto("Login", login, sizeof(login), 1)) return 0;
if (!io_ler_senha("Senha", senha, sizeof(senha))) return 0;
if (usuario_autenticar(login, senha, &sessao) == RES_OK) {
io_ok("Bem-vindo(a), %s. Perfil: %s.", sessao.nome, perfil_nome(sessao.perfil));
return 1;
}
io_erro("%s Tentativa %d de 3.", erro_mensagem(), tentativa);
}
io_erro("Número de tentativas excedido. O programa será encerrado.");
return 0;
}

/* Tela de troca da própria senha. */
void ui_trocar_senha(void)
{
char atual[TAM_SENHA], nova[TAM_SENHA];

io_titulo("Trocar minha senha");
if (!io_ler_senha("Senha atual", atual, sizeof(atual))) return;
if (!ler_nova_senha(nova, sizeof(nova))) return;
ui_resultado(usuario_trocar_senha(sessao.id, atual, nova), "Senha alterada.");
}

/* ---------------------------------------------------------------------
* Usuários
* --------------------------------------------------------------------- */

static int escolher_perfil(int *perfil)
{
long long p;
printf("Perfis: 1 - Atendente | 2 - Técnico | 3 - Gerente | 0 - Cancelar\n");
if (!io_ler_inteiro("Perfil", 0, 3, &p) || p == 0) return 0;
*perfil = (int)p;
return 1;
}

/* Tela de cadastro de usuário. */
static void usuario_novo(void)
{
char login[TAM_LOGIN], nome[TAM_NOME], senha[TAM_SENHA];
int perfil;
long long id;

io_titulo("Cadastrar usuário");
if (!io_ler_texto("Login (sem espaços)", login, sizeof(login), 1)) return;
if (!io_ler_texto("Nome completo", nome, sizeof(nome), 1)) return;
if (!escolher_perfil(&perfil)) return;
if (!ler_nova_senha(senha, sizeof(senha))) return;
if (ui_resultado(usuario_criar(login, nome, perfil, senha, &id), NULL))
io_ok("Usuário %s criado (código %lld, perfil %s).", login, id, perfil_nome(perfil));
}

/* Tabela com todos os usuários. */
static void usuario_lista(void)
{
Usuario u[MAX_RESULTADOS];
int n, i;

io_titulo("Usuários");
n = usuario_listar(u, MAX_RESULTADOS, 0);
ui_coluna("Código", 7); ui_coluna("Login", 16); ui_coluna("Nome", 28); ui_coluna("Perfil", 10); ui_coluna("Situação", 9);
printf("\n");
ui_linha(74);
for (i = 0; i < n; i++) {
char cod[16];
snprintf(cod, sizeof(cod), "%lld", u[i].id);
ui_coluna(cod, 7); ui_coluna(u[i].login, 16); ui_coluna(u[i].nome, 28); ui_coluna(perfil_nome(u[i].perfil), 10);
ui_coluna(u[i].ativo ? "Ativo" : "Bloqueado", 9); printf("\n");
}
}

/* Tela de alteração de perfil. */
static void usuario_perfil(void)
{
long long id;
int perfil;

io_titulo("Alterar perfil");
usuario_lista();
if (!io_ler_inteiro("Código do usuário (0 para cancelar)", 0, 999999999LL, &id) || id == 0) return;
if (!escolher_perfil(&perfil)) return;
ui_resultado(usuario_alterar_perfil(sessao.id, id, perfil), "Perfil alterado. Vale a partir do próximo login.");
}

/* Tela de bloqueio ou desbloqueio de usuário. */
static void usuario_bloqueio(void)
{
long long id;
Usuario u;

io_titulo("Bloquear ou desbloquear usuário");
usuario_lista();
if (!io_ler_inteiro("Código do usuário (0 para cancelar)", 0, 999999999LL, &id) || id == 0) return;
if (usuario_buscar_id(id, &u) != RES_OK) { io_erro("%s", erro_mensagem()); return; }
if (u.ativo) {
if (!io_confirmar("Bloquear este usuário? O histórico dele é mantido")) return;
ui_resultado(usuario_definir_ativo(sessao.id, id, 0), "Usuário bloqueado.");
} else {
if (!io_confirmar("Desbloquear este usuário?")) return;
ui_resultado(usuario_definir_ativo(sessao.id, id, 1), "Usuário desbloqueado.");
}
}

/* Menu do módulo de usuários (somente gerente). */
void menu_usuarios(void)
{
static const OpcaoMenu opcoes[] = {
{ "Cadastrar usuário", F_USUARIOS, usuario_novo },
{ "Listar usuários", F_USUARIOS, usuario_lista },
{ "Alterar perfil", F_USUARIOS, usuario_perfil },
{ "Bloquear ou desbloquear", F_USUARIOS, usuario_bloqueio }
};
ui_menu("Usuários", opcoes, (int)(sizeof(opcoes) / sizeof(opcoes[0])));
}

/* ---------------------------------------------------------------------
* Consultas e relatórios
* --------------------------------------------------------------------- */

/* Le uma data no formato AAAA-MM-DD ou DD/MM/AAAA (vazio = sem filtro). */
static int ler_data(const char *rotulo, char destino[11])
{
char linha[32];
int d, m, a;

for (;;) {
printf("%s (DD/MM/AAAA, Enter para não filtrar): ", rotulo);
if (!io_ler_linha(linha, sizeof(linha))) return 0;
texto_aparar(linha);
if (linha[0] == '\0') { destino[0] = '\0'; return 1; }
if ((sscanf(linha, "%d/%d/%d", &d, &m, &a) == 3 || (sscanf(linha, "%d-%d-%d", &a, &m, &d) == 3 && a > 31))
&& a >= 2000 && a <= 2100 && m >= 1 && m <= 12 && d >= 1 && d <= 31) {
snprintf(destino, 11, "%04d-%02d-%02d", a, m, d);
return 1;
}
io_erro("Data inválida. Exemplo: 21/10/2026.");
}
}

/* Pergunta status, técnico e período para filtrar as OS. */
static int ler_filtro(FiltroOS *f)
{
long long s;
int i;

memset(f, 0, sizeof(*f));
printf("Status (0 = todos):\n");
for (i = 1; i <= QTD_STATUS; i++) printf(" %2d - %s\n", i, status_nome(i));
if (!io_ler_inteiro("Status", 0, QTD_STATUS, &s)) return 0;
f->status = (int)s;
if (io_confirmar("Filtrar por técnico?") && !ui_selecionar_tecnico(&f->tecnico_id)) return 0;
if (!ler_data("Abertas a partir de", f->data_inicio)) return 0;
if (!ler_data("Abertas até", f->data_fim)) return 0;
return 1;
}

/* Tela de consulta de OS com filtros. */
static void consulta_filtros(void)
{
FiltroOS f;
OrdemServico lista[MAX_RESULTADOS];
int n, i;

io_titulo("Consultar OS com filtros");
if (!ler_filtro(&f)) return;
n = os_listar(&f, lista, MAX_RESULTADOS);
if (n <= 0) { io_aviso("Nenhuma OS encontrada com esses filtros."); return; }
printf("\n");
ui_coluna("OS", 6); ui_coluna("Aberta em", 20); ui_coluna("Cliente", 22); ui_coluna("Técnico", 18); ui_coluna("Status", 22); printf("\n");
ui_linha(92);
for (i = 0; i < n; i++) {
char cod[16];
snprintf(cod, sizeof(cod), "%lld", lista[i].id);
ui_coluna(cod, 6); ui_coluna(lista[i].aberta_em, 20); ui_coluna(lista[i].cliente_nome, 22);
ui_coluna(lista[i].tecnico_id ? lista[i].tecnico_nome : "-", 18); ui_coluna(status_nome(lista[i].status), 22); printf("\n");
}
printf("%d OS listadas%s.\n", n, n == MAX_RESULTADOS ? " (limite da tela; use o relatório em arquivo para ver todas)" : "");
}

/* Lista todas as OS de um cliente. */
static void consulta_historico_cliente(void)
{
long long id;
FiltroOS f;
OrdemServico lista[MAX_RESULTADOS];
int n, i;

io_titulo("Histórico do cliente");
if (!ui_selecionar_cliente(&id, 0)) return;
memset(&f, 0, sizeof(f));
f.cliente_id = id;
n = os_listar(&f, lista, MAX_RESULTADOS);
if (n <= 0) { io_aviso("Este cliente ainda não tem OS."); return; }
for (i = 0; i < n; i++)
printf("OS %lld | %s | %s | %s\n Problema: %s\n", lista[i].id, lista[i].aberta_em, lista[i].equipamento,
status_nome(lista[i].status), lista[i].problema);
}

/* Mostra quantas OS existem em cada status. */
static void consulta_resumo(void)
{
int contagem[QTD_STATUS + 1], i, total = 0;

io_titulo("Resumo por status");
if (relatorio_contar_por_status(contagem) != RES_OK) { io_erro("%s", erro_mensagem()); return; }
for (i = 1; i <= QTD_STATUS; i++) {
ui_coluna(status_nome(i), 26);
printf("%d\n", contagem[i]);
total += contagem[i];
}
ui_coluna("Total", 26);
printf("%d\n", total);
}

/* Tela de exportação do relatório de OS. */
static void relatorio_os(void)
{
FiltroOS f;
char arquivo[TAM_NOME];
int linhas;

io_titulo("Exportar relatório de OS (CSV)");
if (!ler_filtro(&f)) return;
if (!io_ler_texto("Nome do arquivo (ex.: relatorio_os.csv)", arquivo, sizeof(arquivo), 1)) return;
if (ui_resultado(relatorio_exportar_os(arquivo, &f, &linhas), NULL))
io_ok("%d OS exportadas para %s. Abra o arquivo em uma planilha.", linhas, arquivo);
}

/* Tela de exportação do relatório de estoque. */
static void relatorio_estoque(void)
{
char arquivo[TAM_NOME];
int linhas;

io_titulo("Exportar relatório de estoque (CSV)");
if (!io_ler_texto("Nome do arquivo (ex.: estoque.csv)", arquivo, sizeof(arquivo), 1)) return;
if (ui_resultado(relatorio_exportar_estoque(arquivo, &linhas), NULL))
io_ok("%d peças exportadas para %s.", linhas, arquivo);
}

/* Menu de consultas e relatórios. */
void menu_consultas(void)
{
static const OpcaoMenu opcoes[] = {
{ "Consultar OS com filtros", F_HISTORICO, consulta_filtros },
{ "Histórico do cliente", F_HISTORICO, consulta_historico_cliente },
{ "Resumo de OS por status", F_RELATORIOS, consulta_resumo },
{ "Exportar relatório de OS (CSV)", F_RELATORIOS, relatorio_os },
{ "Exportar relatório de estoque (CSV)", F_RELATORIOS, relatorio_estoque }
};
ui_menu("Consultas e relatórios", opcoes, (int)(sizeof(opcoes) / sizeof(opcoes[0])));
}

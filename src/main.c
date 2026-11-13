/*
* main.c — Ponto de entrada do TechFix Manager.
*
* Fluxo: abre o banco -> primeiro acesso (se não houver usuários) -> login
* -> menu principal com as opções permitidas ao perfil.
*/
#include <stdio.h>
#include "cadastros.h"
#include "config.h"
#include "db.h"
#include "io.h"
#include "ui.h"

typedef struct {
const char *texto;
Funcionalidade permissao; /* opção aparece se o perfil puder usar */
void (*acao)(void);
} ItemPrincipal;

/* Menu principal: mostra os módulos permitidos ao perfil e a troca de senha. */
static void menu_principal(void)
{
static const ItemPrincipal itens[] = {
{ "Ordens de serviço", F_OS_CONSULTAR, menu_ordens },
{ "Clientes", F_CLIENTE_CONSULTAR, menu_clientes },
{ "Equipamentos", F_EQUIP_CONSULTAR, menu_equipamentos },
{ "Estoque de peças", F_PECA_CONSULTAR, menu_estoque },
{ "Consultas e relatórios", F_HISTORICO, menu_consultas },
{ "Usuários", F_USUARIOS, menu_usuarios }
};
const int qtd = (int)(sizeof(itens) / sizeof(itens[0]));
int visiveis[8], n, i;
long long escolha;

for (;;) {
io_titulo("Menu principal");
printf("Usuário: %s (%s)\n", sessao.nome, perfil_nome(sessao.perfil));
for (i = 0, n = 0; i < qtd; i++)
if (perfil_pode(sessao.perfil, itens[i].permissao)) {
visiveis[n++] = i;
printf("%2d - %s\n", n, itens[i].texto);
}
printf("%2d - Trocar minha senha\n", n + 1);
printf(" 0 - Sair\n");
if (!io_ler_inteiro("Escolha uma opção", 0, n + 1, &escolha) || escolha == 0) return;
if (escolha == n + 1) ui_trocar_senha();
else itens[visiveis[escolha - 1]].acao();
if (io_fim_entrada()) return;
}
}

/* Abre o banco, faz o primeiro acesso se necessário, autentica e abre o menu. */
int main(void)
{
io_iniciar();
printf("%s %s - Gestão de assistência técnica\n", SISTEMA_NOME, SISTEMA_VERSAO);

if (db_abrir(ARQUIVO_BANCO) != RES_OK) {
io_erro("%s", erro_mensagem());
io_erro("Verifique se a pasta do programa permite gravar arquivos.");
return 1;
}
if (usuario_contar() == 0 && !ui_primeiro_acesso()) {
db_fechar();
return 0;
}
if (ui_login()) menu_principal();

db_fechar();
printf("\nSessão encerrada. Até logo!\n");
return 0;
}

/*
* ui.h — Camada de interface (menus e telas do terminal).
*
* A interface apenas conversa com o usuário: pergunta, confere o formato do
* que foi digitado e chama as funções das camadas de regras e de dados.
* Ela não executa SQL nem decide regras de negócio.
*/
#ifndef UI_H
#define UI_H

#include "modelos.h"
#include "regras.h"

/* Usuário autenticado na sessão atual (preenchido no login). */
extern Usuario sessao;

/* Uma opção de menu. Ela só aparece se o perfil logado tiver a permissão (RN13).
"acao" é um ponteiro para a função executada quando a opção é escolhida. */
typedef struct {
const char *texto;
Funcionalidade permissao;
void (*acao)(void);
} OpcaoMenu;

/* Exibe o menu repetidamente até o usuário escolher 0 (Voltar). */
void ui_menu(const char *titulo, const OpcaoMenu opcoes[], int quantidade);

/* Seleção de registros: o usuário busca, vê uma lista e escolhe o código.
Devolvem 1 e preenchem "id", ou 0 se o usuário cancelar. */
int ui_selecionar_cliente(long long *id, int somente_ativos);
int ui_selecionar_equipamento(long long *id);
int ui_selecionar_os(long long *id);
int ui_selecionar_peca(long long *id);
int ui_selecionar_tecnico(long long *id);

/* Escreve o texto em uma coluna de largura fixa, contando caracteres (e não
bytes), para alinhar tabelas com acentos. */
void ui_coluna(const char *texto, int largura);

/* Linha separadora de tabela. */
void ui_linha(int largura);

/* Mostra [OK] com a mensagem informada ou [ERRO] com o motivo registrado.
Devolve 1 se a operação deu certo. */
int ui_resultado(int resultado, const char *mensagem_ok);

/* Exibe todos os detalhes de uma OS (diagnóstico, orçamento, execução, pagamento...). */
void ui_mostrar_os(long long os_id);

/* Menus de cada módulo (um arquivo em src/ui/ para cada grupo). */
void menu_clientes(void);
void menu_equipamentos(void);
void menu_ordens(void);
void menu_estoque(void);
void menu_consultas(void);
void menu_usuarios(void);

/* Telas de acesso. */
void ui_trocar_senha(void);
int ui_primeiro_acesso(void); /* cria o primeiro gerente; 1 se criou */
int ui_login(void); /* até 3 tentativas; 1 se entrou */

#endif

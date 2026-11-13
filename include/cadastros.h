/*
* cadastros.h — Usuários, clientes e equipamentos (RF01, RF02, RF16, RF17).
*
* Cada função valida os dados antes de gravar e devolve um Resultado.
* Em caso de falha, nada é gravado e erro_mensagem() explica o motivo.
*
* Funções de busca que preenchem um vetor devolvem a quantidade de
* registros encontrados (no máximo "maximo") ou -1 em caso de erro.
*/
#ifndef CADASTROS_H
#define CADASTROS_H

#include "erro.h"
#include "modelos.h"

/* ===================== Usuários ===================== */

/* Quantidade de usuários cadastrados (0 indica o primeiro acesso ao sistema). */
long long usuario_contar(void);

/* Cria um usuário. Valida o login (3 a 30 caracteres sem acento), o perfil e a
força da senha. Se "id" não for nulo, recebe o código gerado. */
Resultado usuario_criar(const char *login, const char *nome, int perfil, const char *senha, long long *id);

/* Confere login e senha de um usuário ativo e preenche "saida". Para não revelar
quais logins existem, qualquer falha recebe a mesma mensagem. */
Resultado usuario_autenticar(const char *login, const char *senha, Usuario *saida);

/* Busca um usuário pelo código. */
Resultado usuario_buscar_id(long long id, Usuario *saida);

/* Lista usuários em ordem de nome. Com somente_tecnicos_ativos = 1, lista apenas
técnicos que podem ser atribuídos a uma OS. */
int usuario_listar(Usuario vetor[], int maximo, int somente_tecnicos_ativos);

/* Muda o perfil de um usuário. Um gerente não pode alterar o próprio perfil. */
Resultado usuario_alterar_perfil(long long gerente_id, long long id, int perfil);

/* Bloqueia (ativo = 0) ou desbloqueia (ativo = 1). Ninguém bloqueia a si mesmo. */
Resultado usuario_definir_ativo(long long gerente_id, long long id, int ativo);

/* Troca a senha, exigindo a senha atual. */
Resultado usuario_trocar_senha(long long id, const char *senha_atual, const char *senha_nova);

/* ===================== Clientes ===================== */

/* Cadastra o cliente e preenche cliente->id. Exige nome e telefone válido;
e-mail é opcional (decisão D02: só os dados necessários ao atendimento). */
Resultado cliente_cadastrar(Cliente *cliente);

/* Busca um cliente pelo código. */
Resultado cliente_buscar_id(long long id, Cliente *saida);

/* Busca por código ou por parte do nome (sem diferenciar maiúsculas). */
int cliente_buscar(const char *termo, Cliente vetor[], int maximo);

/* Atualiza nome, telefone e e-mail, com as mesmas validações do cadastro. */
Resultado cliente_atualizar(const Cliente *cliente);

/* Inativa (0) ou reativa (1). Clientes nunca são excluídos, para preservar o histórico (RN01). */
Resultado cliente_definir_ativo(long long id, int ativo);

/* ===================== Equipamentos ===================== */

/* Cadastra o equipamento de um cliente ativo e preenche equip->id. O
identificador (número de série ou etiqueta interna) é único (RN02, RN03). */
Resultado equipamento_cadastrar(Equipamento *equip);

/* Busca um equipamento pelo código. */
Resultado equipamento_buscar_id(long long id, Equipamento *saida);

/* Busca por código, identificador exato, parte do modelo ou parte do nome do cliente. */
int equipamento_buscar(const char *termo, Equipamento vetor[], int maximo);

/* Lista os equipamentos de um cliente. */
int equipamento_listar_cliente(long long cliente_id, Equipamento vetor[], int maximo);

/* Atualiza tipo, marca, modelo, identificador e descrição. O cliente não muda. */
Resultado equipamento_atualizar(const Equipamento *equip);

#endif

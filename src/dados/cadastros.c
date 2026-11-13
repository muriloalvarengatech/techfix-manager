/*
* cadastros.c — Regras e acesso a dados de usuários, clientes e equipamentos.
*/
#include <ctype.h>
#include <string.h>
#include "cadastros.h"
#include "db.h"
#include "regras.h"
#include "seguranca.h"
#include "validacao.h"

/* =====================================================================
* Usuários
* ===================================================================== */

static int login_valido(const char *login)
{
size_t n = strlen(login), i;
if (n < 3 || n >= TAM_LOGIN) return 0;
for (i = 0; i < n; i++) {
unsigned char c = (unsigned char)login[i];
if (!isalnum(c) && c != '.' && c != '_') return 0;
}
return 1;
}

/* Conta os usuários cadastrados. */
long long usuario_contar(void)
{
return db_consultar_inteiro("SELECT COUNT(*) FROM usuario WHERE id > ?;", 0, 0);
}

/* Valida os dados, protege a senha (sal + resumo) e grava o usuário. */
Resultado usuario_criar(const char *login, const char *nome, int perfil, const char *senha, long long *id)
{
sqlite3_stmt *st;
char sal[TAM_SAL_HEX], hash[TAM_HASH_HEX];
int rc;
Resultado r;

if (!login_valido(login)) {
erro_definir("Login inválido: use de 3 a %d letras sem acento, números, ponto ou sublinhado.", TAM_LOGIN- 1);
return RES_INVALIDO;
}
if (texto_caracteres(nome) < 2) { erro_definir("Informe o nome do usuário."); return RES_INVALIDO; }
if (!perfil_valido(perfil)) { erro_definir("Perfil inválido."); return RES_INVALIDO; }
if (!senha_aceitavel(senha)) {
erro_definir("Senha fraca: use pelo menos %d caracteres, com letras e números.", SENHA_MIN);
return RES_INVALIDO;
}

senha_gerar_sal(sal);
senha_calcular_hash(senha, sal, hash);
if (db_preparar("INSERT INTO usuario (login, nome, perfil, senha_hash, senha_sal) VALUES (?, ?, ?, ?, ?);",&st) != RES_OK)
return RES_BANCO;
sqlite3_bind_text(st, 1, login, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 2, nome, -1, SQLITE_TRANSIENT);
sqlite3_bind_int(st, 3, perfil);
sqlite3_bind_text(st, 4, hash, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 5, sal, -1, SQLITE_TRANSIENT);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Cadastro de usuário");
if (r == RES_DUPLICADO) erro_definir("Já existe um usuário com o login '%s'.", login);
if (r == RES_OK && id != NULL) *id = db_ultimo_id();
return r;
}

/* Copia a linha atual do resultado para a struct Usuario. */
static void ler_usuario(sqlite3_stmt *st, Usuario *u)
{
u->id = sqlite3_column_int64(st, 0);
db_coluna_texto(st, 1, u->login, sizeof(u->login));
db_coluna_texto(st, 2, u->nome, sizeof(u->nome));
u->perfil = sqlite3_column_int(st, 3);
u->ativo = sqlite3_column_int(st, 4);
}

/* Confere login e senha; qualquer falha recebe a mesma mensagem genérica. */
Resultado usuario_autenticar(const char *login, const char *senha, Usuario *saida)
{
sqlite3_stmt *st;
char sal[TAM_SAL_HEX] = "0", hash[TAM_HASH_HEX] = "";
int encontrado = 0, confere;

if (db_preparar("SELECT id, login, nome, perfil, ativo, senha_sal, senha_hash FROM usuario WHERE login = ?;", &st) != RES_OK)
return RES_BANCO;
sqlite3_bind_text(st, 1, login, -1, SQLITE_TRANSIENT);
if (sqlite3_step(st) == SQLITE_ROW) {
ler_usuario(st, saida);
db_coluna_texto(st, 5, sal, sizeof(sal));
db_coluna_texto(st, 6, hash, sizeof(hash));
encontrado = 1;
}
sqlite3_finalize(st);

/* Calcula o resumo mesmo quando o login não existe, para que o tempo de
resposta não revele quais logins estão cadastrados. */
confere = senha_confere(senha, sal, encontrado ? hash : "x");
if (!encontrado || !confere || !saida->ativo) {
erro_definir("Usuário ou senha inválidos.");
return RES_PERMISSAO;
}
return RES_OK;
}

/* Busca um usuário pelo código. */
Resultado usuario_buscar_id(long long id, Usuario *saida)
{
sqlite3_stmt *st;
Resultado r = RES_NAO_ENCONTRADO;

if (db_preparar("SELECT id, login, nome, perfil, ativo FROM usuario WHERE id = ?;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int64(st, 1, id);
if (sqlite3_step(st) == SQLITE_ROW) { ler_usuario(st, saida); r = RES_OK; }
else erro_definir("Usuário %lld não encontrado.", id);
sqlite3_finalize(st);
return r;
}

/* Lista usuários (todos ou apenas técnicos ativos) em um vetor. */
int usuario_listar(Usuario vetor[], int maximo, int somente_tecnicos_ativos)
{
sqlite3_stmt *st;
int n = 0;
const char *sql = somente_tecnicos_ativos
? "SELECT id, login, nome, perfil, ativo FROM usuario WHERE perfil = 2 AND ativo = 1 ORDER BY nome;"
: "SELECT id, login, nome, perfil, ativo FROM usuario ORDER BY nome;";

if (db_preparar(sql, &st) != RES_OK) return -1;
while (n < maximo && sqlite3_step(st) == SQLITE_ROW) ler_usuario(st, &vetor[n++]);
sqlite3_finalize(st);
return n;
}

/* Executa um UPDATE de um campo inteiro do usuário e confere se ele existe. */
static Resultado atualizar_usuario_campo(const char *sql, long long valor, long long id)
{
sqlite3_stmt *st;
int rc;
if (db_preparar(sql, &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int64(st, 1, valor);
sqlite3_bind_int64(st, 2, id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
if (rc == SQLITE_DONE && db_linhas_alteradas() == 0) {
erro_definir("Usuário %lld não encontrado.", id);
return RES_NAO_ENCONTRADO;
}
return db_falha(rc, "Atualização de usuário");
}

/* Altera o perfil, impedindo que o gerente altere o próprio. */
Resultado usuario_alterar_perfil(long long gerente_id, long long id, int perfil)
{
if (!perfil_valido(perfil)) { erro_definir("Perfil inválido."); return RES_INVALIDO; }
if (gerente_id == id) {
erro_definir("Você não pode alterar o próprio perfil (evita ficar sem acesso de gerente).");
return RES_REGRA;
}
return atualizar_usuario_campo("UPDATE usuario SET perfil = ? WHERE id = ?;", perfil, id);
}

/* Bloqueia ou desbloqueia, impedindo que o gerente bloqueie a si mesmo. */
Resultado usuario_definir_ativo(long long gerente_id, long long id, int ativo)
{
if (gerente_id == id && !ativo) {
erro_definir("Você não pode bloquear o próprio usuário.");
return RES_REGRA;
}
return atualizar_usuario_campo("UPDATE usuario SET ativo = ? WHERE id = ?;", ativo ? 1 : 0, id);
}

/* Confere a senha atual e grava a nova com um sal novo. */
Resultado usuario_trocar_senha(long long id, const char *senha_atual, const char *senha_nova)
{
sqlite3_stmt *st;
char sal[TAM_SAL_HEX] = "", hash[TAM_HASH_HEX] = "";
int rc;

if (db_preparar("SELECT senha_sal, senha_hash FROM usuario WHERE id = ?;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int64(st, 1, id);
if (sqlite3_step(st) == SQLITE_ROW) {
db_coluna_texto(st, 0, sal, sizeof(sal));
db_coluna_texto(st, 1, hash, sizeof(hash));
}
sqlite3_finalize(st);
if (!senha_confere(senha_atual, sal, hash)) { erro_definir("Senha atual incorreta."); return RES_PERMISSAO;}
if (!senha_aceitavel(senha_nova)) {
erro_definir("Senha fraca: use pelo menos %d caracteres, com letras e números.", SENHA_MIN);
return RES_INVALIDO;
}
senha_gerar_sal(sal);
senha_calcular_hash(senha_nova, sal, hash);
if (db_preparar("UPDATE usuario SET senha_hash = ?, senha_sal = ? WHERE id = ?;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_text(st, 1, hash, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 2, sal, -1, SQLITE_TRANSIENT);
sqlite3_bind_int64(st, 3, id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
return db_falha(rc, "Troca de senha");
}

/* =====================================================================
* Clientes
* ===================================================================== */

static Resultado validar_cliente(const Cliente *c)
{
if (texto_caracteres(c->nome) < 2) { erro_definir("Informe o nome do cliente (pelo menos 2 letras)."); return RES_INVALIDO; }
if (!telefone_valido(c->telefone)) {
erro_definir("Telefone inválido: informe de 8 a 15 dígitos, por exemplo (11) 91234-5678.");
return RES_INVALIDO;
}
if (!texto_vazio(c->email) && !email_valido(c->email)) {
erro_definir("E-mail inválido: use o formato nome@dominio.com ou deixe em branco.");
return RES_INVALIDO;
}
return RES_OK;
}

/* Valida e grava um novo cliente. */
Resultado cliente_cadastrar(Cliente *c)
{
sqlite3_stmt *st;
int rc;
Resultado r = validar_cliente(c);

if (r != RES_OK) return r;
if (db_preparar("INSERT INTO cliente (nome, telefone, email) VALUES (?, ?, NULLIF(?, ''));", &st) != RES_OK)return RES_BANCO;
sqlite3_bind_text(st, 1, c->nome, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 2, c->telefone, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 3, c->email, -1, SQLITE_TRANSIENT);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Cadastro de cliente");
if (r == RES_OK) { c->id = db_ultimo_id(); c->ativo = 1; }
return r;
}

/* Copia a linha atual do resultado para a struct Cliente. */
static void ler_cliente(sqlite3_stmt *st, Cliente *c)
{
c->id = sqlite3_column_int64(st, 0);
db_coluna_texto(st, 1, c->nome, sizeof(c->nome));
db_coluna_texto(st, 2, c->telefone, sizeof(c->telefone));
db_coluna_texto(st, 3, c->email, sizeof(c->email));
c->ativo = sqlite3_column_int(st, 4);
db_coluna_texto(st, 5, c->criado_em, sizeof(c->criado_em));
}

#define CAMPOS_CLIENTE "SELECT id, nome, telefone, email, ativo, criado_em FROM cliente "

/* Busca um cliente pelo código. */
Resultado cliente_buscar_id(long long id, Cliente *saida)
{
sqlite3_stmt *st;
Resultado r = RES_NAO_ENCONTRADO;

if (db_preparar(CAMPOS_CLIENTE "WHERE id = ?;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int64(st, 1, id);
if (sqlite3_step(st) == SQLITE_ROW) { ler_cliente(st, saida); r = RES_OK; }
else erro_definir("Cliente %lld não encontrado.", id);
sqlite3_finalize(st);
return r;
}

/* Busca clientes por código ou parte do nome. */
int cliente_buscar(const char *termo, Cliente vetor[], int maximo)
{
sqlite3_stmt *st;
long long codigo = 0;
int n = 0;

if (!converter_inteiro(termo, &codigo)) codigo = -1;
if (db_preparar(CAMPOS_CLIENTE "WHERE id = ? OR instr(lower(nome), lower(?)) > 0 ORDER BY nome LIMIT ?;", &st) != RES_OK)
return -1;
sqlite3_bind_int64(st, 1, codigo);
sqlite3_bind_text(st, 2, termo, -1, SQLITE_TRANSIENT);
sqlite3_bind_int(st, 3, maximo);
while (n < maximo && sqlite3_step(st) == SQLITE_ROW) ler_cliente(st, &vetor[n++]);
sqlite3_finalize(st);
return n;
}

/* Valida e grava a alteração dos dados do cliente. */
Resultado cliente_atualizar(const Cliente *c)
{
sqlite3_stmt *st;
int rc;
Resultado r = validar_cliente(c);

if (r != RES_OK) return r;
if (db_preparar("UPDATE cliente SET nome = ?, telefone = ?, email = NULLIF(?, '') WHERE id = ?;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_text(st, 1, c->nome, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 2, c->telefone, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 3, c->email, -1, SQLITE_TRANSIENT);
sqlite3_bind_int64(st, 4, c->id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
if (rc == SQLITE_DONE && db_linhas_alteradas() == 0) { erro_definir("Cliente não encontrado."); return RES_NAO_ENCONTRADO; }
return db_falha(rc, "Atualização de cliente");
}

/* Inativa ou reativa o cliente (nunca exclui). */
Resultado cliente_definir_ativo(long long id, int ativo)
{
sqlite3_stmt *st;
int rc;

if (db_preparar("UPDATE cliente SET ativo = ? WHERE id = ?;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int(st, 1, ativo ? 1 : 0);
sqlite3_bind_int64(st, 2, id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
if (rc == SQLITE_DONE && db_linhas_alteradas() == 0) { erro_definir("Cliente não encontrado."); return RES_NAO_ENCONTRADO; }
return db_falha(rc, "Alteração de situação do cliente");
}

/* =====================================================================
* Equipamentos
* ===================================================================== */

static Resultado validar_equipamento(const Equipamento *e)
{
Cliente c;

if (texto_vazio(e->tipo) || texto_vazio(e->marca) || texto_vazio(e->modelo)) {
erro_definir("Tipo, marca e modelo são obrigatórios.");
return RES_INVALIDO;
}
if (texto_vazio(e->identificador)) {
erro_definir("Informe o número de série ou uma etiqueta interna (ex.: TF-0001).");
return RES_INVALIDO;
}
if (cliente_buscar_id(e->cliente_id, &c) != RES_OK) return RES_NAO_ENCONTRADO;
if (!c.ativo) {
erro_definir("O cliente %lld está inativo: reative o cadastro antes de vincular equipamentos.", c.id);
return RES_REGRA;
}
return RES_OK;
}

/* Valida e grava um novo equipamento. */
Resultado equipamento_cadastrar(Equipamento *e)
{
sqlite3_stmt *st;
int rc;
Resultado r = validar_equipamento(e);

if (r != RES_OK) return r;
if (db_preparar("INSERT INTO equipamento (cliente_id, tipo, marca, modelo, identificador, descricao) "
"VALUES (?, ?, ?, ?, ?, NULLIF(?, ''));", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int64(st, 1, e->cliente_id);
sqlite3_bind_text(st, 2, e->tipo, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 3, e->marca, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 4, e->modelo, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 5, e->identificador, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 6, e->descricao, -1, SQLITE_TRANSIENT);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Cadastro de equipamento");
if (r == RES_DUPLICADO) erro_definir("Já existe um equipamento com o identificador '%s'.", e->identificador);
if (r == RES_OK) e->id = db_ultimo_id();
return r;
}

#define CAMPOS_EQUIP \
"SELECT e.id, e.cliente_id, c.nome, c.ativo, e.tipo, e.marca, e.modelo, e.identificador, e.descricao " \
"FROM equipamento e JOIN cliente c ON c.id = e.cliente_id "

/* Copia a linha atual do resultado para a struct Equipamento. */
static void ler_equipamento(sqlite3_stmt *st, Equipamento *e)
{
e->id = sqlite3_column_int64(st, 0);
e->cliente_id = sqlite3_column_int64(st, 1);
db_coluna_texto(st, 2, e->cliente_nome, sizeof(e->cliente_nome));
e->cliente_ativo = sqlite3_column_int(st, 3);
db_coluna_texto(st, 4, e->tipo, sizeof(e->tipo));
db_coluna_texto(st, 5, e->marca, sizeof(e->marca));
db_coluna_texto(st, 6, e->modelo, sizeof(e->modelo));
db_coluna_texto(st, 7, e->identificador, sizeof(e->identificador));
db_coluna_texto(st, 8, e->descricao, sizeof(e->descricao));
}

/* Busca um equipamento pelo código. */
Resultado equipamento_buscar_id(long long id, Equipamento *saida)
{
sqlite3_stmt *st;
Resultado r = RES_NAO_ENCONTRADO;

if (db_preparar(CAMPOS_EQUIP "WHERE e.id = ?;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_int64(st, 1, id);
if (sqlite3_step(st) == SQLITE_ROW) { ler_equipamento(st, saida); r = RES_OK; }
else erro_definir("Equipamento %lld não encontrado.", id);
sqlite3_finalize(st);
return r;
}

/* Busca equipamentos por código, identificador, modelo ou nome do cliente. */
int equipamento_buscar(const char *termo, Equipamento vetor[], int maximo)
{
sqlite3_stmt *st;
long long codigo;
int n = 0;

if (!converter_inteiro(termo, &codigo)) codigo = -1;
if (db_preparar(CAMPOS_EQUIP "WHERE e.id = ? OR lower(e.identificador) = lower(?) "
"OR instr(lower(e.modelo), lower(?)) > 0 OR instr(lower(c.nome), lower(?)) > 0 "
"ORDER BY c.nome, e.id LIMIT ?;", &st) != RES_OK) return -1;
sqlite3_bind_int64(st, 1, codigo);
sqlite3_bind_text(st, 2, termo, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 3, termo, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 4, termo, -1, SQLITE_TRANSIENT);
sqlite3_bind_int(st, 5, maximo);
while (n < maximo && sqlite3_step(st) == SQLITE_ROW) ler_equipamento(st, &vetor[n++]);
sqlite3_finalize(st);
return n;
}

/* Lista os equipamentos de um cliente. */
int equipamento_listar_cliente(long long cliente_id, Equipamento vetor[], int maximo)
{
sqlite3_stmt *st;
int n = 0;

if (db_preparar(CAMPOS_EQUIP "WHERE e.cliente_id = ? ORDER BY e.id;", &st) != RES_OK) return -1;
sqlite3_bind_int64(st, 1, cliente_id);
while (n < maximo && sqlite3_step(st) == SQLITE_ROW) ler_equipamento(st, &vetor[n++]);
sqlite3_finalize(st);
return n;
}

/* Valida e grava a alteração dos dados do equipamento. */
Resultado equipamento_atualizar(const Equipamento *e)
{
sqlite3_stmt *st;
int rc;
Resultado r;

if (texto_vazio(e->tipo) || texto_vazio(e->marca) || texto_vazio(e->modelo) || texto_vazio(e->identificador)) {
erro_definir("Tipo, marca, modelo e identificador são obrigatórios.");
return RES_INVALIDO;
}
if (db_preparar("UPDATE equipamento SET tipo = ?, marca = ?, modelo = ?, identificador = ?, "
"descricao = NULLIF(?, '') WHERE id = ?;", &st) != RES_OK) return RES_BANCO;
sqlite3_bind_text(st, 1, e->tipo, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 2, e->marca, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 3, e->modelo, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 4, e->identificador, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(st, 5, e->descricao, -1, SQLITE_TRANSIENT);
sqlite3_bind_int64(st, 6, e->id);
rc = sqlite3_step(st);
sqlite3_finalize(st);
r = db_falha(rc, "Atualização de equipamento");
if (r == RES_DUPLICADO) erro_definir("Já existe outro equipamento com o identificador '%s'.", e->identificador);
if (r == RES_OK && db_linhas_alteradas() == 0) { erro_definir("Equipamento não encontrado."); return RES_NAO_ENCONTRADO; }
return r;
}

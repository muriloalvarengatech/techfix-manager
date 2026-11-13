/*
* testes.c - Testes automatizados do TechFix Manager.
*
* Executar a partir da pasta raiz do repositorio (para encontrar
* sql/schema.sql). Usa um banco em memória: não altera techfix.db.
* Cada teste exibe [PASSOU] ou [FALHOU] e o total aparece no final.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cadastros.h"
#include "db.h"
#include "estoque.h"
#include "ordens.h"
#include "regras.h"
#include "relatorios.h"
#include "seguranca.h"
#include "sha256.h"
#include "validacao.h"

static int aprovados = 0, reprovados = 0;

#define VERIFICA(condicao, descricao) verificar((condicao) ? 1 : 0, descricao, __LINE__)

/* Conta e exibe o resultado de uma verificação. */
static void verificar(int ok, const char *descricao, int linha)
{
if (ok) { aprovados++; printf("[PASSOU] %s\n", descricao); }
else { reprovados++; printf("[FALHOU] %s (linha %d) - ultimo erro: %s\n", descricao, linha, erro_mensagem()); }
}

/* Título de um grupo de testes. */
static void secao(const char *nome) { printf("\n--- %s ---\n", nome); }

/* Resumo SHA-256 de um texto, em hexadecimal. */
static void hex_de(const char *texto, char saida[65])
{
unsigned char r[32];
sha256(texto, strlen(texto), r);
bytes_para_hex(r, 32, saida);
}

/* ===================================================================== */

static void testar_sha256(void)
{
char h[65];
secao("SHA-256 (vetores oficiais FIPS 180-4)");
hex_de("", h);
VERIFICA(strcmp(h, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") == 0, "Resumo do texto vazio");
hex_de("abc", h);
VERIFICA(strcmp(h, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0, "Resumo de \"abc\"");
hex_de("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", h);
VERIFICA(strcmp(h, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1") == 0, "Resumo de texto com 56 bytes (dois blocos)");
}

/* Validação e conversão de dados. */
static void testar_validacao(void)
{
long long v;
char buf[64];
char t[] = " texto com espacos \n";

secao("Validacao e conversao (TT04)");
texto_aparar(t);
VERIFICA(strcmp(t, "texto com espacos") == 0, "Remove espacos das pontas");
VERIFICA(texto_vazio(" ") && !texto_vazio(" a "), "Detecta texto vazio");
VERIFICA(texto_caracteres("João") == 4, "Conta caracteres acentuados como um");
texto_copiar(buf, 3, "Jo\xC3\xA3o"); /* "João" */
VERIFICA(strcmp(buf, "Jo") == 0, "Corta texto sem quebrar caractere acentuado");
VERIFICA(telefone_valido("(11) 91234-5678"), "Telefone valido com mascara");
VERIFICA(!telefone_valido("123") && !telefone_valido("11 9abc12345"), "Telefone curto ou com letras e recusado");
VERIFICA(email_valido("ana@techfix.com.br"), "E-mail valido");
VERIFICA(!email_valido("ana@") && !email_valido("ana.com") && !email_valido("a b@x.com"), "E-mails invalidosrecusados");
VERIFICA(converter_dinheiro("12", &v) && v == 1200, "\"12\" vira 1200 centavos");
VERIFICA(converter_dinheiro("12,5", &v) && v == 1250, "\"12,5\" vira 1250 centavos");
VERIFICA(converter_dinheiro("1.234,56", &v) && v == 123456, "\"1.234,56\" vira 123456 centavos");
VERIFICA(converter_dinheiro("R$ 89,90", &v) && v == 8990, "Aceita prefixo R$");
VERIFICA(converter_dinheiro("12.50", &v) && v == 1250, "Ponto como separador decimal");
VERIFICA(!converter_dinheiro("-5", &v) && !converter_dinheiro("12,345", &v) && !converter_dinheiro("abc", &v)
&& !converter_dinheiro("", &v), "Valores negativos, com 3 casas ou nao numericos recusados");
formatar_dinheiro(123456, buf, sizeof(buf));
VERIFICA(strcmp(buf, "R$ 1.234,56") == 0, "Formata R$ 1.234,56");
formatar_dinheiro(5, buf, sizeof(buf));
VERIFICA(strcmp(buf, "R$ 0,05") == 0, "Formata R$ 0,05");
VERIFICA(converter_inteiro(" 42 ", &v) && v == 42 && !converter_inteiro("4x", &v), "Conversao de inteiro");
}

/* Geração de sal, resumo e conferência de senhas. */
static void testar_senha(void)
{
char sal[TAM_SAL_HEX], sal2[TAM_SAL_HEX], hash[TAM_HASH_HEX], hash2[TAM_HASH_HEX];

secao("Senhas (RN12, RNF04)");
senha_gerar_sal(sal);
senha_gerar_sal(sal2);
VERIFICA(strlen(sal) == 32 && strcmp(sal, sal2) != 0, "Sal aleatorio diferente a cada geracao");
senha_calcular_hash("Senha123", sal, hash);
senha_calcular_hash("Senha123", sal2, hash2);
VERIFICA(strcmp(hash, hash2) != 0, "Mesma senha com sais diferentes gera resumos diferentes");
VERIFICA(strstr(hash, "Senha123") == NULL, "Resumo nao contem a senha");
VERIFICA(senha_confere("Senha123", sal, hash), "Senha correta confere");
VERIFICA(!senha_confere("senha123", sal, hash), "Senha errada nao confere");
VERIFICA(senha_aceitavel("abc123") && !senha_aceitavel("abcdef") && !senha_aceitavel("12345678") && !senha_aceitavel("a1"),
"Exige minimo de caracteres, letra e numero");
}

/* Matriz de estados, permissões e cálculo do orçamento. */
static void testar_regras(void)
{
long long servicos[] = { 8000, 2000 };
int quantidades[] = { 1, 3 };
long long precos[] = { 25000, 1050 };
long long negativo[] = { -1 };

secao("Matriz de estados da OS (A2, RN05)");
VERIFICA(status_pode_transitar(OS_ABERTA, OS_EM_DIAGNOSTICO), "Aberta -> Em diagnostico permitido");
VERIFICA(status_pode_transitar(OS_AGUARDANDO_APROVACAO, OS_RECUSADA), "Aguardando -> Recusada permitido");
VERIFICA(status_pode_transitar(OS_RECUSADA, OS_LIBERADA), "Recusada -> Liberada permitido (devolucao)");
VERIFICA(!status_pode_transitar(OS_ABERTA, OS_EM_MANUTENCAO), "Aberta -> Em manutencao recusado");
VERIFICA(!status_pode_transitar(OS_RECUSADA, OS_EM_MANUTENCAO), "Recusada -> Em manutencao recusado");
VERIFICA(!status_pode_transitar(OS_ENCERRADA, OS_ABERTA) && !status_pode_transitar(0, 1) && !status_pode_transitar(1, 99),
"Encerrada nao muda; status invalidos recusados");

secao("Permissoes por perfil (RN13)");
VERIFICA(!perfil_pode(PERFIL_ATENDENTE, F_DIAGNOSTICO), "Atendente nao registra diagnostico");
VERIFICA(!perfil_pode(PERFIL_TECNICO, F_DECISAO) && !perfil_pode(PERFIL_TECNICO, F_PAGAMENTO), "Tecnico naoregistra decisao nem pagamento");
VERIFICA(!perfil_pode(PERFIL_ATENDENTE, F_USUARIOS) && !perfil_pode(PERFIL_TECNICO, F_USUARIOS), "Somente gerente gerencia usuarios");
{
int f, todas = 1;
for (f = 0; f < QTD_FUNCIONALIDADES; f++) if (!perfil_pode(PERFIL_GERENTE, (Funcionalidade)f)) todas = 0;
VERIFICA(todas, "Gerente acessa todas as funcionalidades");
}
VERIFICA(!perfil_pode(0, F_OS_CONSULTAR) && !perfil_pode(9, F_OS_CONSULTAR), "Perfil invalido nao acessa nada");

secao("Calculo do orcamento (A3, RN08)");
VERIFICA(orcamento_calcular_total(servicos, 2, quantidades, precos, 2) == 8000 + 2000 + 25000 + 3 * 1050,
"Total = servicos + quantidade x preco (R$ 381,50)");
VERIFICA(orcamento_calcular_total(servicos, 1, NULL, NULL, 0) == 8000, "Orcamento so com servico");
VERIFICA(orcamento_calcular_total(NULL, 0, quantidades, precos, 1) == 25000, "Orcamento so com peca");
VERIFICA(orcamento_calcular_total(negativo, 1, NULL, NULL, 0) == -1, "Valor negativo e recusado");
}

/* ===================================================================== */

static char *ler_arquivo(const char *caminho)
{
FILE *f = fopen(caminho, "rb");
long n;
char *s;
if (!f) return NULL;
fseek(f, 0, SEEK_END);
n = ftell(f);
fseek(f, 0, SEEK_SET);
s = (char *)malloc((size_t)n + 1);
if (s && fread(s, 1, (size_t)n, f) == (size_t)n) s[n] = '\0';
fclose(f);
return s;
}

/* Quantidade de linhas de uma tabela. */
static long long contar(const char *tabela)
{
char sql[128];
snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM %s WHERE rowid > ?;", tabela);
return db_consultar_inteiro(sql, 0, -1);
}

/* Banco em memória: cadastros, estoque e o fluxo completo da OS. */
static void testar_banco(void)
{
long long gerente, atendente, tecnico, tecnico2, os1, os2, os3;
Cliente cli, cli2;
Equipamento eq, eq2, eq3;
Peca tela, cabo;
Usuario u;
Orcamento orc;
ItemServico servico;
long long peca_ids[1];
int qtds[1], linhas;
long long movs_antes, usadas_antes, valor;
char *arquivo_sql;
OrdemServico o;

secao("Banco de dados e esquema (TT05, RNF03)");
VERIFICA(db_abrir(":memory:") == RES_OK, "Cria o esquema em banco vazio");
VERIFICA(db_executar(ESQUEMA_SQL) == RES_OK, "Esquema pode ser executado de novo sem erro");
arquivo_sql = ler_arquivo("sql/schema.sql");
VERIFICA(arquivo_sql != NULL && strcmp(arquivo_sql, ESQUEMA_SQL) == 0,
"src/dados/esquema.c igual a sql/schema.sql (rode tools/gerar_esquema.py se falhar)");
free(arquivo_sql);
VERIFICA(db_executar("INSERT INTO equipamento (cliente_id, tipo, marca, modelo, identificador) VALUES (999,'a', 'b', 'c', 'd');") != RES_OK,
"Banco recusa equipamento com cliente inexistente (chave estrangeira)");

secao("Usuarios e login (HU07, HU08, HU10)");
VERIFICA(usuario_criar("gerente", "Gerente Teste", PERFIL_GERENTE, "Senha123", &gerente) == RES_OK, "Cria gerente");
VERIFICA(usuario_criar("atend", "Atendente Teste", PERFIL_ATENDENTE, "Senha123", &atendente) == RES_OK, "Cria atendente");
VERIFICA(usuario_criar("tec", "Tecnico Teste", PERFIL_TECNICO, "Senha123", &tecnico) == RES_OK, "Cria tecnico");
VERIFICA(usuario_criar("tec2", "Outro Tecnico", PERFIL_TECNICO, "Senha123", &tecnico2) == RES_OK, "Cria segundo tecnico");
VERIFICA(usuario_criar("tec", "Duplicado", PERFIL_TECNICO, "Senha123", NULL) == RES_DUPLICADO, "Login duplicado recusado");
VERIFICA(usuario_criar("x y", "Login com espaco", PERFIL_TECNICO, "Senha123", NULL) == RES_INVALIDO, "Logincom espaco recusado");
VERIFICA(usuario_criar("fraca", "Senha fraca", PERFIL_TECNICO, "123", NULL) == RES_INVALIDO, "Senha fraca recusada");
VERIFICA(usuario_autenticar("atend", "Senha123", &u) == RES_OK && u.perfil == PERFIL_ATENDENTE, "Login correto devolve o perfil");
{
char m1[TAM_MSG], m2[TAM_MSG];
usuario_autenticar("atend", "errada1", &u); strcpy(m1, erro_mensagem());
usuario_autenticar("naoexiste", "Senha123", &u); strcpy(m2, erro_mensagem());
VERIFICA(strcmp(m1, m2) == 0, "Senha errada e usuario inexistente recebem a mesma mensagem");
}
VERIFICA(usuario_definir_ativo(gerente, gerente, 0) == RES_REGRA, "Gerente nao bloqueia a si mesmo");
VERIFICA(usuario_alterar_perfil(gerente, gerente, PERFIL_TECNICO) == RES_REGRA, "Gerente nao altera o proprio perfil");
VERIFICA(usuario_definir_ativo(gerente, tecnico2, 0) == RES_OK && usuario_autenticar("tec2", "Senha123", &u)!= RES_OK,
"Usuario bloqueado nao entra");
VERIFICA(usuario_trocar_senha(atendente, "Senha123", "NovaSenha9") == RES_OK && usuario_autenticar("atend","NovaSenha9", &u) == RES_OK,
"Troca de senha funciona");
VERIFICA(db_consultar_inteiro("SELECT COUNT(*) FROM usuario WHERE (senha_hash LIKE '%Senha%' OR senha_sal LIKE '%Senha%') AND id > ?;", 0, -1) == 0,
"Nenhuma senha legivel no banco");

secao("Clientes e equipamentos (HU01 a HU06)");
memset(&cli, 0, sizeof(cli));
strcpy(cli.nome, "João da Silva"); strcpy(cli.telefone, "123");
VERIFICA(cliente_cadastrar(&cli) == RES_INVALIDO && contar("cliente") == 0, "Telefone invalido: cliente naoe gravado");
strcpy(cli.telefone, "(11) 91234-5678");
VERIFICA(cliente_cadastrar(&cli) == RES_OK && cli.id > 0, "Cadastra cliente valido");
{
Cliente achados[5];
VERIFICA(cliente_buscar("silva", achados, 5) == 1 && cliente_buscar("1", achados, 5) == 1, "Busca por parte do nome e por codigo");
}
memset(&cli2, 0, sizeof(cli2));
strcpy(cli2.nome, "Maria Souza"); strcpy(cli2.telefone, "11 98888-7777"); strcpy(cli2.email, "maria@exemplo.com");
VERIFICA(cliente_cadastrar(&cli2) == RES_OK, "Cadastra segundo cliente com e-mail");

memset(&eq, 0, sizeof(eq));
eq.cliente_id = cli.id;
strcpy(eq.tipo, "Celular"); strcpy(eq.marca, "Samsung"); strcpy(eq.modelo, "Galaxy A10"); strcpy(eq.identificador, "SN-1");
VERIFICA(equipamento_cadastrar(&eq) == RES_OK, "Cadastra equipamento");
eq2 = eq; eq2.cliente_id = cli2.id;
VERIFICA(equipamento_cadastrar(&eq2) == RES_DUPLICADO, "Identificador duplicado recusado");
strcpy(eq2.identificador, "SN-2");
VERIFICA(equipamento_cadastrar(&eq2) == RES_OK, "Cadastra equipamento do segundo cliente");
eq3 = eq; eq3.cliente_id = 999; strcpy(eq3.identificador, "SN-3");
VERIFICA(equipamento_cadastrar(&eq3) == RES_NAO_ENCONTRADO, "Equipamento sem cliente existente recusado");

secao("Estoque (HU16 a HU18, RN10)");
memset(&tela, 0, sizeof(tela));
strcpy(tela.codigo, "TELA-A10"); strcpy(tela.descricao, "Tela A10"); tela.preco_centavos = 25000;
VERIFICA(peca_cadastrar(&tela, 2, gerente) == RES_OK && tela.saldo == 2, "Cadastra peca com saldo inicial 2");
memset(&cabo, 0, sizeof(cabo));
strcpy(cabo.codigo, "CABO-FLEX"); strcpy(cabo.descricao, "Cabo flex"); cabo.preco_centavos = 1500;
VERIFICA(peca_cadastrar(&cabo, 0, gerente) == RES_OK, "Cadastra peca sem saldo");
VERIFICA(peca_cadastrar(&cabo, 0, gerente) == RES_DUPLICADO, "Codigo de peca duplicado recusado");
VERIFICA(estoque_entrada(tela.id, 3, "Compra NF 10", gerente) == RES_OK, "Entrada soma ao saldo");
movs_antes = contar("movimentacao");
VERIFICA(estoque_saida(tela.id, 99, "Perda", 0, gerente) == RES_REGRA && contar("movimentacao") == movs_antes,
"Saida maior que o saldo e recusada e nada e gravado");
VERIFICA(estoque_saida(tela.id, 1, "Perda no transporte", 0, gerente) == RES_OK, "Saida avulsa valida");
peca_buscar_id(tela.id, &tela);
VERIFICA(tela.saldo == 4 && estoque_saldo_calculado(tela.id) == 4, "Saldo (4) igual a soma das movimentacoes");
VERIFICA(estoque_entrada(tela.id, 0, "x", gerente) == RES_INVALIDO, "Quantidade zero recusada");

secao("Fluxo principal da OS (HU11 a HU29)");
VERIFICA(os_abrir(eq.id, "", atendente, &os1) == RES_INVALIDO, "OS sem problema relatado recusada");
VERIFICA(os_abrir(eq.id, "Tela quebrada", atendente, &os1) == RES_OK, "Abre OS");
VERIFICA(os_buscar_id(os1, &o) == RES_OK && o.status == OS_ABERTA, "OS nasce com status Aberta");
VERIFICA(os_iniciar_diagnostico(os1, tecnico, PERFIL_TECNICO) == RES_PERMISSAO, "Diagnostico exige tecnico atribuido");
VERIFICA(os_atribuir_tecnico(os1, atendente, atendente) == RES_REGRA, "Atendente nao pode ser atribuido comotecnico");
VERIFICA(os_atribuir_tecnico(os1, tecnico2, atendente) == RES_REGRA, "Tecnico bloqueado nao pode ser atribuido");
VERIFICA(os_atribuir_tecnico(os1, tecnico, atendente) == RES_OK, "Atribui tecnico ativo");
VERIFICA(orcamento_decidir(os1, atendente, 1) == RES_REGRA, "Decisao antes do orcamento recusada (transicaoinvalida)");
VERIFICA(os_iniciar_diagnostico(os1, tecnico, PERFIL_TECNICO) == RES_OK, "Tecnico atribuido inicia diagnostico");
peca_ids[0] = 777; qtds[0] = 1;
VERIFICA(diagnostico_registrar(os1, tecnico, PERFIL_TECNICO, "Display trincado", "Trocar tela", peca_ids, qtds, 1) == RES_NAO_ENCONTRADO,
"Diagnostico com peca inexistente recusado");
peca_ids[0] = tela.id;
VERIFICA(diagnostico_registrar(os1, tecnico, PERFIL_TECNICO, "Display trincado", "Trocar tela", peca_ids, qtds, 1) == RES_OK,
"Registra diagnostico com peca prevista");
peca_buscar_id(tela.id, &tela);
VERIFICA(tela.saldo == 4, "Peca prevista nao baixa o estoque");
memset(&servico, 0, sizeof(servico));
strcpy(servico.descricao, "Mao de obra"); servico.valor_centavos = 8000;
VERIFICA(orcamento_elaborar(os1, atendente, &servico, 1) == RES_OK, "Elabora orcamento");
VERIFICA(orcamento_buscar(os1, &orc) == RES_OK && orc.total_centavos == 33000, "Total do orcamento = R$ 330,00");
servico.valor_centavos = 9000;
VERIFICA(orcamento_elaborar(os1, atendente, &servico, 1) == RES_OK && orcamento_buscar(os1, &orc) == RES_OK&& orc.total_centavos == 34000,
"Orcamento pode ser corrigido antes da decisao");
VERIFICA(manutencao_iniciar(os1, tecnico, PERFIL_TECNICO) == RES_REGRA, "Manutencao sem aprovacao recusada (RN09)");
VERIFICA(orcamento_decidir(os1, atendente, 1) == RES_OK, "Registra aprovacao");
VERIFICA(orcamento_decidir(os1, atendente, 0) == RES_REGRA, "Segunda decisao recusada");
VERIFICA(orcamento_elaborar(os1, atendente, &servico, 1) == RES_REGRA, "Orcamento decidido nao pode ser alterado (RN07)");
VERIFICA(pagamento_registrar(os1, 34000, 1, atendente) == RES_REGRA, "Pagamento antes da conclusao recusado");
VERIFICA(manutencao_iniciar(os1, tecnico2, PERFIL_TECNICO) == RES_PERMISSAO, "Outro tecnico nao executa a OS");
VERIFICA(manutencao_iniciar(os1, tecnico, PERFIL_TECNICO) == RES_OK, "Inicia manutencao");
VERIFICA(manutencao_concluir(os1, tecnico, PERFIL_TECNICO, "ok") == RES_REGRA, "Conclusao sem servico registrado recusada");

movs_antes = contar("movimentacao");
usadas_antes = contar("peca_utilizada");
VERIFICA(manutencao_usar_peca(os1, tela.id, 50, tecnico, PERFIL_TECNICO) == RES_REGRA, "Peca utilizada acimado saldo recusada");
VERIFICA(contar("movimentacao") == movs_antes && contar("peca_utilizada") == usadas_antes,
"Falha nao deixa vinculo sem movimentacao (A4 transacional)");
VERIFICA(manutencao_usar_peca(os1, tela.id, 1, tecnico, PERFIL_TECNICO) == RES_OK, "Registra peca utilizada");
peca_buscar_id(tela.id, &tela);
VERIFICA(tela.saldo == 3 && estoque_saldo_calculado(tela.id) == 3 && contar("peca_utilizada") == usadas_antes + 1,
"Baixa de estoque e vinculo gravados juntos");
VERIFICA(manutencao_registrar_servico(os1, tecnico, PERFIL_TECNICO, "Tela substituida") == RES_OK, "Registraservico realizado");
VERIFICA(manutencao_concluir(os1, tecnico, PERFIL_TECNICO, "Testado") == RES_OK, "Conclui manutencao");
VERIFICA(os_liberar(os1, atendente) == RES_REGRA, "Liberacao sem pagamento recusada (D13)");
VERIFICA(os_valor_final(os1, &valor) == RES_OK && valor == 34000, "Valor final = orcamento aprovado (D11)");
VERIFICA(pagamento_registrar(os1, 1000, 1, atendente) == RES_INVALIDO, "Pagamento com valor diferente recusado (D12)");
VERIFICA(pagamento_registrar(os1, 34000, 9, atendente) == RES_INVALIDO, "Forma de pagamento invalida recusada");
VERIFICA(pagamento_registrar(os1, 34000, 1, atendente) == RES_OK && pagamento_situacao(os1, NULL, 0, NULL) == 1, "Registra pagamento");
VERIFICA(pagamento_registrar(os1, 34000, 1, atendente) != RES_OK, "Pagamento em dobro recusado");
VERIFICA(entrega_registrar(os1, "Joao", atendente) == RES_REGRA, "Entrega antes da liberacao recusada");
VERIFICA(os_liberar(os1, atendente) == RES_OK, "Libera equipamento");
VERIFICA(entrega_registrar(os1, "", atendente) == RES_INVALIDO, "Entrega sem nome de quem recebeu recusada");
VERIFICA(entrega_registrar(os1, "Joao da Silva", atendente) == RES_OK, "Registra entrega");
VERIFICA(os_encerrar(os1, gerente) == RES_OK, "Encerra OS");
VERIFICA(os_observar(os1, gerente, "teste") == RES_REGRA && os_atribuir_tecnico(os1, tecnico, gerente) == RES_REGRA,
"OS encerrada nao aceita alteracoes");
VERIFICA(contar("historico_status") == 9, "Historico registra as 9 etapas (abertura + 8 mudancas)");

secao("Fluxo de recusa e regras de cliente inativo");
VERIFICA(os_abrir(eq2.id, "Nao liga", atendente, &os2) == RES_OK, "Abre segunda OS");
os_atribuir_tecnico(os2, tecnico, atendente);
os_iniciar_diagnostico(os2, gerente, PERFIL_GERENTE);
VERIFICA(diagnostico_registrar(os2, gerente, PERFIL_GERENTE, "Placa queimada", "Trocar placa", NULL, NULL, 0) == RES_OK,
"Gerente tambem pode registrar diagnostico");
VERIFICA(orcamento_elaborar(os2, atendente, &servico, 1) == RES_OK, "Orcamento da segunda OS");
VERIFICA(orcamento_decidir(os2, atendente, 0) == RES_OK, "Cliente recusa o orcamento");
VERIFICA(manutencao_iniciar(os2, tecnico, PERFIL_TECNICO) == RES_REGRA, "Recusada nao entra em manutencao");
VERIFICA(pagamento_situacao(os2, NULL, 0, NULL) == -1, "Pagamento nao se aplica a orcamento recusado");
VERIFICA(os_liberar(os2, atendente) == RES_OK, "Recusada pode ser liberada para devolucao (D09)");
VERIFICA(cliente_definir_ativo(cli2.id, 0) == RES_OK, "Inativa cliente");
VERIFICA(os_abrir(eq2.id, "Outro problema", atendente, &os3) == RES_REGRA, "Cliente inativo nao recebe novaOS");
{
FiltroOS f;
OrdemServico lista[5];
memset(&f, 0, sizeof(f));
f.cliente_id = cli2.id;
VERIFICA(os_listar(&f, lista, 5) == 1, "Historico do cliente inativo continua consultavel (RN01)");
f.cliente_id = 0; f.status = OS_ENCERRADA;
VERIFICA(os_listar(&f, lista, 5) == 1, "Filtro por status");
}
VERIFICA(os_buscar_id(os2, &o) == RES_OK && o.status == OS_LIBERADA, "Segunda OS esta liberada");

secao("Relatorios em arquivo (HU32, manipulacao de arquivos)");
{
FiltroOS f;
char *csv;
memset(&f, 0, sizeof(f));
VERIFICA(relatorio_exportar_os("teste_relatorio_os.csv", &f, &linhas) == RES_OK && linhas == 2, "Exporta2 OS para CSV");
csv = ler_arquivo("teste_relatorio_os.csv");
VERIFICA(csv != NULL && strstr(csv, "OS;Abertura;Cliente") != NULL && strstr(csv, "340,00") != NULL,
"CSV tem cabecalho e valor aprovado");
free(csv);
remove("teste_relatorio_os.csv");
VERIFICA(relatorio_exportar_estoque("teste_estoque.csv", &linhas) == RES_OK && linhas == 2, "Exporta estoque para CSV");
remove("teste_estoque.csv");
}
VERIFICA(db_consultar_inteiro("SELECT COUNT(*) FROM pragma_foreign_key_check WHERE 1 > ?;", 0, -1) == 0,
"Nenhuma chave estrangeira quebrada ao final");
db_fechar();
}

/* Abre o banco, faz o primeiro acesso se necessário, autentica e abre o menu. */
int main(void)
{
printf("TechFix Manager - testes automatizados\n");
testar_sha256();
testar_validacao();
testar_senha();
testar_regras();
testar_banco();
printf("\n==== Resultado: %d aprovados, %d reprovados ====\n", aprovados, reprovados);
return reprovados == 0 ? 0 : 1;
}

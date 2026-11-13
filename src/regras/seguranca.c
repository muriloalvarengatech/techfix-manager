/*
* seguranca.c — Sal, resumo e conferência de senhas.
*/
#include <ctype.h>
#include <string.h>
#include "sqlite3.h"
#include "seguranca.h"
#include "sha256.h"
#include "validacao.h"

/* Gera 16 bytes aleatórios e os converte para hexadecimal. */
void senha_gerar_sal(char sal_hex[TAM_SAL_HEX])
{
unsigned char sal[16];
/* O gerador do SQLite usa fontes de aleatoriedade do sistema operacional. */
sqlite3_randomness((int)sizeof(sal), sal);
bytes_para_hex(sal, sizeof(sal), sal_hex);
}

/* Resumo de sal + senha, repetido HASH_ITERACOES vezes. */
void senha_calcular_hash(const char *senha, const char *sal_hex, char hash_hex[TAM_HASH_HEX])
{
unsigned char resumo[32];
Sha256 ctx;
int i;

sha256_iniciar(&ctx);
sha256_adicionar(&ctx, sal_hex, strlen(sal_hex));
sha256_adicionar(&ctx, senha, strlen(senha));
sha256_finalizar(&ctx, resumo);

for (i = 1; i < HASH_ITERACOES; i++) {
sha256_iniciar(&ctx);
sha256_adicionar(&ctx, resumo, sizeof(resumo));
sha256_adicionar(&ctx, sal_hex, strlen(sal_hex));
sha256_finalizar(&ctx, resumo);
}
bytes_para_hex(resumo, sizeof(resumo), hash_hex);
}

/* Recalcula o resumo e compara todos os caracteres, sem parar no primeiro diferente. */
int senha_confere(const char *senha, const char *sal_hex, const char *hash_hex)
{
char calculado[TAM_HASH_HEX];
unsigned char diferenca = 0;
size_t i;

if (strlen(hash_hex) != TAM_HASH_HEX - 1) return 0;
senha_calcular_hash(senha, sal_hex, calculado);
/* Compara todos os caracteres, sem parar no primeiro diferente. */
for (i = 0; i < TAM_HASH_HEX - 1; i++)
diferenca |= (unsigned char)(calculado[i] ^ hash_hex[i]);
return diferenca == 0;
}

/* Exige tamanho mínimo, ao menos uma letra e ao menos um número. */
int senha_aceitavel(const char *senha)
{
int letra = 0, numero = 0;
const char *p;

if (texto_caracteres(senha) < SENHA_MIN) return 0;
for (p = senha; *p; p++) {
if (isdigit((unsigned char)*p)) numero = 1;
else if (isalpha((unsigned char)*p) || ((unsigned char)*p & 0x80)) letra = 1;
}
return letra && numero;
}

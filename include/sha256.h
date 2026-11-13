/*
* sha256.h — Função de resumo criptográfico SHA-256 (norma FIPS 180-4).
*
* Um resumo ("hash") transforma qualquer texto em 32 bytes. É fácil de
* calcular e, na prática, impossível de reverter: por isso é usado para
* proteger senhas (ver seguranca.h).
*
* Uso em etapas (para dados grandes ou em partes):
* Sha256 ctx;
* sha256_iniciar(&ctx);
* sha256_adicionar(&ctx, parte1, tamanho1);
* sha256_adicionar(&ctx, parte2, tamanho2);
* sha256_finalizar(&ctx, resumo);
*/
#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>

/* Estado intermediário do cálculo. */
typedef struct {
uint32_t estado[8]; /* oito palavras de 32 bits do resumo parcial */
uint64_t total_bits; /* tamanho total já adicionado, em bits */
unsigned char bloco[64]; /* bloco de 512 bits em preenchimento */
size_t usados; /* bytes já ocupados em "bloco" */
} Sha256;

void sha256_iniciar(Sha256 *ctx);
void sha256_adicionar(Sha256 *ctx, const void *dados, size_t tamanho);
void sha256_finalizar(Sha256 *ctx, unsigned char resumo[32]);

/* Atalho: calcula o resumo de um bloco de memória de uma só vez. */
void sha256(const void *dados, size_t tamanho, unsigned char resumo[32]);

/* Converte n bytes em texto hexadecimal. O destino precisa de 2*n + 1 posições. */
void bytes_para_hex(const unsigned char *bytes, size_t n, char *destino);

#endif

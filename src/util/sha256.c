/*
* sha256.c — Implementação do SHA-256 conforme a norma FIPS 180-4.
* Conferida pelos vetores de teste oficiais em tests/testes.c.
*/
#include <string.h>
#include "sha256.h"

static const uint32_t K[64] = {
0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROTD(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

/* Processa um bloco de 64 bytes: expande para 64 palavras e aplica as 64 rodadas. */
static void processar_bloco(Sha256 *ctx, const unsigned char *b)
{
uint32_t w[64], a, bb, c, d, e, f, g, h, t1, t2;
int i;

for (i = 0; i < 16; i++)
w[i] = ((uint32_t)b[i * 4] << 24) | ((uint32_t)b[i * 4 + 1] << 16) |
((uint32_t)b[i * 4 + 2] << 8) | (uint32_t)b[i * 4 + 3];
for (i = 16; i < 64; i++) {
uint32_t s0 = ROTD(w[i - 15], 7) ^ ROTD(w[i - 15], 18) ^ (w[i - 15] >> 3);
uint32_t s1 = ROTD(w[i - 2], 17) ^ ROTD(w[i - 2], 19) ^ (w[i - 2] >> 10);
w[i] = w[i - 16] + s0 + w[i - 7] + s1;
}
a = ctx->estado[0]; bb = ctx->estado[1]; c = ctx->estado[2]; d = ctx->estado[3];
e = ctx->estado[4]; f = ctx->estado[5]; g = ctx->estado[6]; h = ctx->estado[7];
for (i = 0; i < 64; i++) {
uint32_t S1 = ROTD(e, 6) ^ ROTD(e, 11) ^ ROTD(e, 25);
uint32_t ch = (e & f) ^ (~e & g);
uint32_t S0 = ROTD(a, 2) ^ ROTD(a, 13) ^ ROTD(a, 22);
uint32_t maj = (a & bb) ^ (a & c) ^ (bb & c);
t1 = h + S1 + ch + K[i] + w[i];
t2 = S0 + maj;
h = g; g = f; f = e; e = d + t1; d = c; c = bb; bb = a; a = t1 + t2;
}
ctx->estado[0] += a; ctx->estado[1] += bb; ctx->estado[2] += c; ctx->estado[3] += d;
ctx->estado[4] += e; ctx->estado[5] += f; ctx->estado[6] += g; ctx->estado[7] += h;
}

/* Carrega os valores iniciais definidos pela norma. */
void sha256_iniciar(Sha256 *ctx)
{
static const uint32_t inicial[8] = {
0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};
memcpy(ctx->estado, inicial, sizeof(inicial));
ctx->total_bits = 0;
ctx->usados = 0;
}

/* Acumula dados no bloco e processa cada bloco completo de 64 bytes. */
void sha256_adicionar(Sha256 *ctx, const void *dados, size_t tamanho)
{
const unsigned char *p = (const unsigned char *)dados;

ctx->total_bits += (uint64_t)tamanho * 8;
while (tamanho > 0) {
size_t copiar = 64 - ctx->usados;
if (copiar > tamanho) copiar = tamanho;
memcpy(ctx->bloco + ctx->usados, p, copiar);
ctx->usados += copiar;
p += copiar;
tamanho -= copiar;
if (ctx->usados == 64) {
processar_bloco(ctx, ctx->bloco);
ctx->usados = 0;
}
}
}

/* Acrescenta o preenchimento (bit 1, zeros e tamanho) e extrai o resumo. */
void sha256_finalizar(Sha256 *ctx, unsigned char resumo[32])
{
uint64_t bits = ctx->total_bits;
int i;

ctx->bloco[ctx->usados++] = 0x80;
if (ctx->usados > 56) {
while (ctx->usados < 64) ctx->bloco[ctx->usados++] = 0;
processar_bloco(ctx, ctx->bloco);
ctx->usados = 0;
}
while (ctx->usados < 56) ctx->bloco[ctx->usados++] = 0;
for (i = 7; i >= 0; i--) {
ctx->bloco[56 + (7 - i)] = (unsigned char)(bits >> (i * 8));
}
processar_bloco(ctx, ctx->bloco);
for (i = 0; i < 8; i++) {
resumo[i * 4] = (unsigned char)(ctx->estado[i] >> 24);
resumo[i * 4 + 1] = (unsigned char)(ctx->estado[i] >> 16);
resumo[i * 4 + 2] = (unsigned char)(ctx->estado[i] >> 8);
resumo[i * 4 + 3] = (unsigned char)(ctx->estado[i]);
}
}

/* Calcula o resumo de um bloco de memória em uma única chamada. */
void sha256(const void *dados, size_t tamanho, unsigned char resumo[32])
{
Sha256 ctx;
sha256_iniciar(&ctx);
sha256_adicionar(&ctx, dados, tamanho);
sha256_finalizar(&ctx, resumo);
}

/* Escreve cada byte como dois dígitos hexadecimais. */
void bytes_para_hex(const unsigned char *bytes, size_t n, char *destino)
{
static const char hex[] = "0123456789abcdef";
size_t i;
for (i = 0; i < n; i++) {
destino[i * 2] = hex[bytes[i] >> 4];
destino[i * 2 + 1] = hex[bytes[i] & 0x0F];
}
destino[n * 2] = '\0';
}

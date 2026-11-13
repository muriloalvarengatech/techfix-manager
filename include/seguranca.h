/*
* seguranca.h — Armazenamento seguro de senhas (RN12, RNF04).
*
* A senha nunca é gravada. Grava-se apenas:
* - um "sal": 16 bytes aleatórios, diferentes para cada usuário;
* - o resumo SHA-256 de (sal + senha), recalculado HASH_ITERACOES vezes.
*
* O sal faz duas senhas iguais gerarem resumos diferentes. As repetições
* tornam lento testar milhões de senhas caso o banco seja copiado.
*
* Limitação documentada: sistemas profissionais usam algoritmos feitos
* para senhas (bcrypt, Argon2). A repetição do SHA-256 é uma aproximação
* didática, sem bibliotecas externas.
*/
#ifndef SEGURANCA_H
#define SEGURANCA_H

#include "config.h"

/* Gera um sal aleatório e o devolve em hexadecimal (32 caracteres). */
void senha_gerar_sal(char sal_hex[TAM_SAL_HEX]);

/* Calcula o resumo da senha com o sal informado (64 caracteres hexadecimais). */
void senha_calcular_hash(const char *senha, const char *sal_hex, char hash_hex[TAM_HASH_HEX]);

/* 1 se a senha digitada gera o mesmo resumo que está gravado. */
int senha_confere(const char *senha, const char *sal_hex, const char *hash_hex);

/* 1 se a senha tem pelo menos SENHA_MIN caracteres, com ao menos uma letra e um número. */
int senha_aceitavel(const char *senha);

#endif

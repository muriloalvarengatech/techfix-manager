/*
* validacao.h — Validação e conversão de dados digitados pelo usuário.
*
* São funções "puras": não leem do teclado nem acessam o banco. Por isso
* podem ser testadas isoladamente (tests/testes.c).
*
* Todo texto do sistema está em UTF-8, em que letras acentuadas ocupam
* mais de um byte. As funções abaixo levam isso em conta.
*/
#ifndef VALIDACAO_H
#define VALIDACAO_H

#include <stddef.h>

/* Remove espaços e quebras de linha do início e do fim (altera o próprio texto). */
void texto_aparar(char *texto);

/* 1 se o texto for nulo, vazio ou tiver apenas espaços. */
int texto_vazio(const char *texto);

/* Quantidade de caracteres (e não de bytes): "João" tem 4 caracteres e 5 bytes. */
size_t texto_caracteres(const char *texto);

/* Copia no máximo (tamanho - 1) bytes, sem cortar uma letra acentuada ao meio.
O destino sempre termina com '\0'. Origem nula vira texto vazio. */
void texto_copiar(char *destino, size_t tamanho, const char *origem);

/* 1 se tiver entre 8 e 15 dígitos. Aceita espaços, parênteses, '+' e '-'. */
int telefone_valido(const char *telefone);

/* Validação simples: um '@', texto antes dele, um ponto depois e nenhum espaço. */
int email_valido(const char *email);

/* Converte texto em inteiro. Devolve 1 se o texto inteiro for um número válido. */
int converter_inteiro(const char *texto, long long *valor);

/* Converte um valor em reais para centavos. Aceita "12", "12,5", "12,50",
"12.50", "1.234,56" e "R$ 89,90". Recusa negativos e mais de 2 casas decimais. */
int converter_dinheiro(const char *texto, long long *centavos);

/* Formata centavos como texto em reais: 123456 -> "R$ 1.234,56". */
void formatar_dinheiro(long long centavos, char *destino, size_t tamanho);

#endif

/*
* validacao.c — Validação e conversão de dados digitados pelo usuário.
*/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "validacao.h"

/* Corta espaços do fim e depois desloca o texto para tirar os do início. */
void texto_aparar(char *texto)
{
size_t inicio = 0, fim;

if (texto == NULL) return;
fim = strlen(texto);
while (fim > 0 && isspace((unsigned char)texto[fim - 1])) fim--;
texto[fim] = '\0';
while (texto[inicio] != '\0' && isspace((unsigned char)texto[inicio])) inicio++;
if (inicio > 0) memmove(texto, texto + inicio, fim - inicio + 1);
}

/* Percorre o texto procurando qualquer caractere que não seja espaço. */
int texto_vazio(const char *texto)
{
if (texto == NULL) return 1;
for (; *texto; texto++)
if (!isspace((unsigned char)*texto)) return 0;
return 1;
}

/* Conta caracteres UTF-8 ignorando os bytes de continuação. */
size_t texto_caracteres(const char *texto)
{
size_t n = 0;
/* Em UTF-8, bytes de continuação têm o formato 10xxxxxx. */
for (; *texto; texto++)
if (((unsigned char)*texto & 0xC0) != 0x80) n++;
return n;
}

/* Copia com limite, recuando o corte para não partir uma letra acentuada. */
void texto_copiar(char *destino, size_t tamanho, const char *origem)
{
size_t n;

if (tamanho == 0) return;
if (origem == NULL) origem = "";
n = strlen(origem);
if (n >= tamanho) {
n = tamanho - 1;
/* Recua até o início de um caractere para não deixar UTF-8 inválido. */
while (n > 0 && ((unsigned char)origem[n] & 0xC0) == 0x80) n--;
}
memcpy(destino, origem, n);
destino[n] = '\0';
}

/* Aceita dígitos e símbolos de máscara; exige de 8 a 15 dígitos. */
int telefone_valido(const char *telefone)
{
int digitos = 0;

if (texto_vazio(telefone)) return 0;
for (; *telefone; telefone++) {
unsigned char c = (unsigned char)*telefone;
if (isdigit(c)) digitos++;
else if (c != ' ' && c != '(' && c != ')' && c != '-' && c != '+') return 0;
}
return digitos >= 8 && digitos <= 15;
}

/* Confere a estrutura básica nome@dominio.ext. */
int email_valido(const char *email)
{
const char *arroba, *ponto;

if (texto_vazio(email) || strchr(email, ' ') != NULL) return 0;
arroba = strchr(email, '@');
if (arroba == NULL || arroba == email || strchr(arroba + 1, '@') != NULL) return 0;
ponto = strrchr(arroba, '.');
return ponto != NULL && ponto > arroba + 1 && ponto[1] != '\0';
}

/* Converte com strtoll e exige que o texto inteiro seja numérico. */
int converter_inteiro(const char *texto, long long *valor)
{
char copia[32];
char *fim;
long long v;

if (texto == NULL || strlen(texto) >= sizeof(copia)) return 0;
strcpy(copia, texto);
texto_aparar(copia);
if (copia[0] == '\0') return 0;

errno = 0;
v = strtoll(copia, &fim, 10);
if (errno != 0 || *fim != '\0') return 0;
*valor = v;
return 1;
}

/* Interpreta valores em reais nos formatos brasileiro e com ponto decimal. */
int converter_dinheiro(const char *texto, long long *centavos)
{
char limpo[32];
size_t i, j = 0;
int tem_virgula, casas = 0, digitos_inteiros = 0, depois_separador = 0;
long long inteiro = 0, fracao = 0;

if (texto == NULL) return 0;

/* Remove "R$" e espaços. */
for (i = 0; texto[i] != '\0'; i++) {
if (texto[i] == ' ' || texto[i] == 'R' || texto[i] == '$') continue;
if (j >= sizeof(limpo) - 1) return 0;
limpo[j++] = texto[i];
}
limpo[j] = '\0';
if (limpo[0] == '\0') return 0;

/* Com vírgula, o ponto é separador de milhar ("1.234,56"); sem vírgula,
o ponto é o separador decimal ("1234.56"). */
tem_virgula = strchr(limpo, ',') != NULL;
for (i = 0; limpo[i] != '\0'; i++) {
char c = limpo[i];
if (isdigit((unsigned char)c)) {
if (depois_separador) {
if (++casas > 2) return 0;
fracao = fracao * 10 + (c - '0');
} else {
if (++digitos_inteiros > 9) return 0; /* até 999.999.999 */
inteiro = inteiro * 10 + (c - '0');
}
} else if (c == ',' || (c == '.' && !tem_virgula)) {
if (depois_separador) return 0;
depois_separador = 1;
} else if (c == '.' && tem_virgula && !depois_separador) {
continue; /* separador de milhar */
} else {
return 0;
}
}
if (digitos_inteiros == 0 && casas == 0) return 0;
if (casas == 1) fracao *= 10;
*centavos = inteiro * 100 + fracao;
return 1;
}

/* Insere o separador de milhar e escreve sempre 2 casas decimais. */
void formatar_dinheiro(long long centavos, char *destino, size_t tamanho)
{
char digitos[32], com_milhar[48];
long long reais;
int negativo = centavos < 0, n, i, k = 0;

if (negativo) centavos = -centavos;
reais = centavos / 100;
n = snprintf(digitos, sizeof(digitos), "%lld", reais);
for (i = 0; i < n; i++) {
if (i > 0 && (n - i) % 3 == 0) com_milhar[k++] = '.';
com_milhar[k++] = digitos[i];
}
com_milhar[k] = '\0';
snprintf(destino, tamanho, "%sR$ %s,%02lld", negativo ? "-" : "", com_milhar, centavos % 100);
}

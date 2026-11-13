/*
* io.c — Leitura do teclado e mensagens padronizadas.
*
* No Windows a leitura usa ReadConsoleW e converte para UTF-8. Assim nomes
* com acento (ex.: "João") funcionam em qualquer versão do console. Quando a
* entrada vem de um arquivo (testes), usa fgets normalmente.
*/
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "io.h"
#include "validacao.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

static int entrada_terminou = 0;

/* Configura o console. No Windows, entrada e saída passam a usar UTF-8. */
void io_iniciar(void)
{
#ifdef _WIN32
SetConsoleOutputCP(CP_UTF8);
SetConsoleCP(CP_UTF8);
#endif
/* Não usamos setvbuf: no Windows, _IOLBF com tamanho 0 encerra o programa.
A saída é descarregada (fflush) antes de cada leitura em io_ler_linha. */
}

/* Informa se a entrada já terminou (os menus param quando isso acontece). */
int io_fim_entrada(void)
{
return entrada_terminou;
}

/* Descarta o resto de uma linha longa demais. */
static void descartar_resto_da_linha(void)
{
int c;
while ((c = getchar()) != '\n' && c != EOF) { }
}

#ifdef _WIN32
/* Devolve 1 se leu, 0 no fim da entrada, -1 se a entrada não é um console. */
static int ler_console_windows(char *destino, size_t tamanho)
{
HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
DWORD modo, lidos, i;
wchar_t bloco[256], linha[TAM_LINHA];
size_t pos = 0;
int fim_linha = 0;

if (!GetConsoleMode(h, &modo)) return -1;
while (!fim_linha) {
if (!ReadConsoleW(h, bloco, 255, &lidos, NULL) || lidos == 0) return 0;
for (i = 0; i < lidos; i++) {
wchar_t c = bloco[i];
if (c == 0x1A) return 0; /* Ctrl+Z = fim da entrada */
if (c == L'\r') continue;
if (c == L'\n') { fim_linha = 1; break; }
if (pos < TAM_LINHA - 1) linha[pos++] = c;
}
}
linha[pos] = L'\0';
if (WideCharToMultiByte(CP_UTF8, 0, linha, -1, destino, (int)tamanho, NULL, NULL) == 0)
destino[0] = '\0'; /* texto maior que o destino */
return 1;
}
#endif

/* Lê uma linha do console do Windows ou de stdin, sem a quebra de linha final. */
int io_ler_linha(char *destino, size_t tamanho)
{
size_t n;

fflush(stdout);
if (entrada_terminou) { destino[0] = '\0'; return 0; }
#ifdef _WIN32
{
int r = ler_console_windows(destino, tamanho);
if (r == 0) { entrada_terminou = 1; destino[0] = '\0'; return 0; }
if (r == 1) return 1;
}
#endif
if (fgets(destino, (int)tamanho, stdin) == NULL) {
entrada_terminou = 1;
destino[0] = '\0';
return 0;
}
n = strlen(destino);
if (n > 0 && destino[n - 1] == '\n') {
destino[--n] = '\0';
if (n > 0 && destino[n - 1] == '\r') destino[--n] = '\0';
} else if (n == tamanho - 1) {
descartar_resto_da_linha();
}
return 1;
}

/* Pergunta um texto, conferindo tamanho máximo e obrigatoriedade. */
int io_ler_texto(const char *rotulo, char *destino, size_t tamanho, int obrigatorio)
{
char linha[TAM_LINHA];

for (;;) {
printf("%s%s: ", rotulo, obrigatorio ? "" : " (opcional, Enter para deixar em branco)");
if (!io_ler_linha(linha, sizeof(linha))) return 0;
texto_aparar(linha);
if (strlen(linha) >= tamanho) {
io_erro("Texto muito longo: use no máximo %u caracteres.", (unsigned)(tamanho - 1));
continue;
}
if (obrigatorio && linha[0] == '\0') {
io_erro("Este campo é obrigatório.");
if (!io_confirmar("Deseja tentar de novo?")) return 0;
continue;
}
texto_copiar(destino, tamanho, linha);
return 1;
}
}

/* Pergunta um número até receber um valor válido dentro da faixa. */
int io_ler_inteiro(const char *rotulo, long long minimo, long long maximo, long long *valor)
{
char linha[TAM_LINHA];
long long v;

for (;;) {
printf("%s: ", rotulo);
if (!io_ler_linha(linha, sizeof(linha))) return 0;
if (!converter_inteiro(linha, &v)) {
io_erro("Digite apenas números.");
continue;
}
if (v < minimo || v > maximo) {
io_erro("Digite um número entre %lld e %lld.", minimo, maximo);
continue;
}
*valor = v;
return 1;
}
}

/* Pergunta um valor em reais até receber um formato válido. */
int io_ler_dinheiro(const char *rotulo, long long *centavos)
{
char linha[TAM_LINHA];

for (;;) {
printf("%s (ex.: 150,00): ", rotulo);
if (!io_ler_linha(linha, sizeof(linha))) return 0;
if (converter_dinheiro(linha, centavos)) return 1;
io_erro("Valor inválido. Use números com até 2 casas decimais, por exemplo 89,90.");
}
}

/* Pergunta s/n até receber uma das duas respostas. */
int io_confirmar(const char *pergunta)
{
char linha[TAM_LINHA];

for (;;) {
printf("%s (s/n): ", pergunta);
if (!io_ler_linha(linha, sizeof(linha))) return 0;
texto_aparar(linha);
if (linha[0] == 's' || linha[0] == 'S') return 1;
if (linha[0] == 'n' || linha[0] == 'N') return 0;
io_erro("Responda s para sim ou n para não.");
}
}

/* Desliga o eco do terminal enquanto a senha é digitada e religa em seguida. */
int io_ler_senha(const char *rotulo, char *destino, size_t tamanho)
{
int ok;
#ifdef _WIN32
HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
DWORD modo;
int console = GetConsoleMode(h, &modo);
printf("%s: ", rotulo);
if (console) SetConsoleMode(h, modo & ~ENABLE_ECHO_INPUT);
ok = io_ler_linha(destino, tamanho);
if (console) SetConsoleMode(h, modo);
#else
struct termios antigo, novo;
int terminal = isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &antigo) == 0;
printf("%s: ", rotulo);
if (terminal) {
novo = antigo;
novo.c_lflag &= ~(tcflag_t)ECHO;
tcsetattr(STDIN_FILENO, TCSANOW, &novo);
}
ok = io_ler_linha(destino, tamanho);
if (terminal) tcsetattr(STDIN_FILENO, TCSANOW, &antigo);
#endif
printf("\n");
return ok;
}

/* Imprime o título de uma tela. */
void io_titulo(const char *titulo)
{
printf("\n=== %s ===\n", titulo);
}

/* Imprime uma mensagem com prefixo de texto ([OK], [ERRO] ou [AVISO]). */
static void mensagem(const char *prefixo, const char *formato, va_list args)
{
printf("%s ", prefixo);
vprintf(formato, args);
printf("\n");
}

/* Mensagem de sucesso. */
void io_ok(const char *formato, ...)
{
va_list a; va_start(a, formato); mensagem("[OK]", formato, a); va_end(a);
}

/* Mensagem de erro. */
void io_erro(const char *formato, ...)
{
va_list a; va_start(a, formato); mensagem("[ERRO]", formato, a); va_end(a);
}

/* Mensagem de aviso. */
void io_aviso(const char *formato, ...)
{
va_list a; va_start(a, formato); mensagem("[AVISO]", formato, a); va_end(a);
}

/* Espera o usuário pressionar Enter. */
void io_pausar(void)
{
char linha[8];
printf("Pressione Enter para continuar...");
io_ler_linha(linha, sizeof(linha));
}

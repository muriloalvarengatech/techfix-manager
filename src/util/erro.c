/*
* erro.c — Guarda a mensagem do último erro ocorrido.
*/
#include <stdarg.h>
#include <stdio.h>
#include "config.h"
#include "erro.h"

static char mensagem[TAM_MSG] = "";

/* Monta a mensagem com vsnprintf (mesmo formato do printf) e guarda para exibição. */
void erro_definir(const char *formato, ...)
{
va_list args;
va_start(args, formato);
vsnprintf(mensagem, sizeof(mensagem), formato, args);
va_end(args);
}

/* Devolve a última mensagem registrada ou um texto padrão. */
const char *erro_mensagem(void)
{
return mensagem[0] ? mensagem : "Erro desconhecido.";
}

/* Apaga a mensagem atual. */
void erro_limpar(void)
{
mensagem[0] = '\0';
}

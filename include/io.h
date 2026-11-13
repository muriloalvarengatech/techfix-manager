/*
* io.h — Entrada e saída no terminal, conforme docs/guia-interface.md.
*
* Padrão das mensagens (nunca depende de cor, para funcionar com leitores de tela):
* [OK] operação concluída
* [ERRO] algo impediu a operação; a mensagem diz como corrigir
* [AVISO] informação importante que não impede a operação
*
* Todas as funções de leitura devolvem 1 quando obtêm um valor e 0 quando
* a entrada termina (fim de arquivo, Ctrl+Z) ou o usuário cancela.
* Ao receber 0, quem chamou deve voltar ao menu.
*/
#ifndef IO_H
#define IO_H

#include <stddef.h>

/* Prepara o console. No Windows, ativa UTF-8 para exibir acentos. Chamar no início do main. */
void io_iniciar(void);

/* Lê uma linha inteira, sem a quebra de linha final. */
int io_ler_linha(char *destino, size_t tamanho);

/* Pergunta um texto. Se obrigatório, insiste até receber algo (ou o usuário
desistir); se opcional, aceita Enter em branco. */
int io_ler_texto(const char *rotulo, char *destino, size_t tamanho, int obrigatorio);

/* Pergunta um número inteiro entre mínimo e máximo; repete até ser válido. */
int io_ler_inteiro(const char *rotulo, long long minimo, long long maximo, long long *valor);

/* Pergunta um valor em reais e devolve em centavos. */
int io_ler_dinheiro(const char *rotulo, long long *centavos);

/* Pergunta de sim ou não. Devolve 1 para "s" e 0 para "n" ou fim da entrada. */
int io_confirmar(const char *pergunta);

/* Lê uma senha sem mostrar o que é digitado (quando a entrada é um terminal). */
int io_ler_senha(const char *rotulo, char *destino, size_t tamanho);

/* Título de tela: "=== Título ===". */
void io_titulo(const char *titulo);

/* Mensagens padronizadas (mesmo formato do printf). */
void io_ok(const char *formato, ...);
void io_erro(const char *formato, ...);
void io_aviso(const char *formato, ...);

/* Espera o usuário pressionar Enter. */
void io_pausar(void);

/* 1 depois que a entrada terminou; os menus usam isso para encerrar. */
int io_fim_entrada(void);

#endif

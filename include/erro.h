/*
* erro.h — Códigos de resultado e mensagem do último erro.
*
* Convenção usada em todo o sistema:
* - as funções das camadas de regras e de dados devolvem um Resultado;
* - quando falham, registram uma mensagem legível com erro_definir();
* - a interface exibe essa mensagem ao usuário com erro_mensagem().
*
* Assim, a regra de negócio explica o problema, e a interface só o mostra.
*/
#ifndef ERRO_H
#define ERRO_H

typedef enum {
RES_OK = 0, /* operação concluída */
RES_INVALIDO, /* dado informado não passou na validação */
RES_NAO_ENCONTRADO, /* registro inexistente */
RES_DUPLICADO, /* violaria um campo único (ex.: login, código da peça) */
RES_REGRA, /* operação proibida por regra de negócio */
RES_PERMISSAO, /* perfil ou usuário sem permissão */
RES_BANCO /* falha do banco de dados */
} Resultado;

/* Registra a mensagem do erro atual (mesmo formato do printf). */
void erro_definir(const char *formato, ...);

/* Devolve a mensagem do último erro registrado. */
const char *erro_mensagem(void);

/* Apaga a mensagem registrada. */
void erro_limpar(void);

#endif

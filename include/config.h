/*
* config.h — Constantes gerais do TechFix Manager.
*
* Centraliza tamanhos de campos, limites e parâmetros de segurança, para que
* uma mudança seja feita em um único lugar.
*
* Valores marcados com "DECISÃO Dxx" implementam a PROPOSTA registrada em
* docs/decisoes/README.md. Se o Product Owner decidir diferente, altere aqui.
*
* Compilação com MinGW (Dev-C++ ou VS Code no Windows): o projeto define
* __USE_MINGW_ANSI_STDIO=1 nas opções do compilador para que printf aceite
* o formato "%lld" (long long).
*/
#ifndef CONFIG_H
#define CONFIG_H

#define SISTEMA_NOME "TechFix Manager"
#define SISTEMA_VERSAO "0.1.0"

/* DECISÃO D01 (proposta): SQLite, com o banco em um arquivo na pasta do programa. */
#define ARQUIVO_BANCO "techfix.db"

/* Tamanho máximo dos campos de texto, em bytes, já contando o '\0' final.
Letras acentuadas ocupam 2 bytes em UTF-8. */
#define TAM_NOME 81
#define TAM_TELEFONE 21
#define TAM_EMAIL 81
#define TAM_LOGIN 31
#define TAM_SENHA 65
#define TAM_CURTO 41 /* tipo, marca, modelo, identificador */
#define TAM_CODIGO 21 /* código de peça */
#define TAM_TEXTO 401 /* descrições longas e observações */
#define TAM_DATA 20 /* "AAAA-MM-DD HH:MM:SS" */
#define TAM_LINHA 512 /* uma linha digitada no terminal */
#define TAM_MSG 256 /* mensagem de erro */

/* Quantidade máxima de registros devolvidos por uma consulta (tamanho dos vetores). */
#define MAX_RESULTADOS 50
#define MAX_ITENS 30 /* serviços ou peças em um diagnóstico ou orçamento */

/* Senhas — DECISÃO D07 (proposta): SHA-256 com sal aleatório e repetições. */
#define SENHA_MIN 6
#define TAM_SAL_HEX 33 /* 16 bytes em hexadecimal + '\0' */
#define TAM_HASH_HEX 65 /* 32 bytes em hexadecimal + '\0' */
#define HASH_ITERACOES 20000

/* DECISÃO D12 (proposta, pendente do PO): formas de pagamento aceitas.
O vetor é definido em src/dados/ordens.c. */
#define QTD_FORMAS_PAGAMENTO 4
extern const char *FORMAS_PAGAMENTO[QTD_FORMAS_PAGAMENTO];

#endif

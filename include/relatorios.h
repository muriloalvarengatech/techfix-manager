/*
* relatorios.h — Informações gerenciais e exportação para arquivo (RF14, RF15).
*
* Os relatórios são gravados em CSV (valores separados por ';', o padrão das
* planilhas em português) com fopen, fprintf e fclose: é o uso de
* manipulação de arquivos pedido na Etapa 5 do PIM.
*/
#ifndef RELATORIOS_H
#define RELATORIOS_H

#include <stdio.h>
#include "erro.h"
#include "modelos.h"
#include "regras.h"

/* Conta as OS por status: contagem[s] recebe a quantidade no status s (1..QTD_STATUS). */
Resultado relatorio_contar_por_status(int contagem[QTD_STATUS + 1]);

/* Exporta as OS que atendem ao filtro, com o valor aprovado e a situação do
pagamento. "linhas" recebe a quantidade de OS exportadas. */
Resultado relatorio_exportar_os(const char *arquivo, const FiltroOS *filtro, int *linhas);

/* Exporta a posição do estoque: saldo, preço e valor de cada peça. */
Resultado relatorio_exportar_estoque(const char *arquivo, int *linhas);

/* Escreve um campo de texto no CSV entre aspas, duplicando aspas internas
e trocando quebras de linha por espaço. */
void csv_escrever_campo(FILE *f, const char *texto);

#endif

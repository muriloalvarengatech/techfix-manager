/*
* ordens.h — Fluxo da ordem de serviço (OS), da abertura ao encerramento
* (RF03 a RF08 e RF11 a RF13).
*
* Caminho principal de uma OS:
* Aberta -> Em diagnóstico -> Aguardando aprovação -> Aprovada
* -> Em manutenção -> Concluída -> Liberada -> Entregue -> Encerrada
* Caminho de recusa:
* Aguardando aprovação -> Recusada -> Liberada (devolução sem reparo)
*
* Toda mudança de status é validada pela matriz de transições (regras.h)
* e registrada em historico_status com data, hora e usuário.
*
* Parâmetros "usuario_id" e "perfil" identificam quem executa a operação,
* para registro no histórico e verificação de responsabilidade.
*/
#ifndef ORDENS_H
#define ORDENS_H

#include "erro.h"
#include "modelos.h"

/* ===================== Abertura e acompanhamento ===================== */

/* Abre uma OS para um equipamento de cliente ativo, com o problema relatado
(algoritmo A1, RN04). Preenche "os_id" com o número gerado. */
Resultado os_abrir(long long equipamento_id, const char *problema, long long usuario_id, long long *os_id);

/* Busca a OS com cliente, equipamento e técnico já resolvidos. */
Resultado os_buscar_id(long long os_id, OrdemServico *saida);

/* Lista OS que atendem ao filtro, da mais recente para a mais antiga.
Devolve a quantidade encontrada ou -1 em caso de erro. */
int os_listar(const FiltroOS *filtro, OrdemServico vetor[], int maximo);

/* Atribui (ou troca) o técnico responsável. Só aceita técnico ativo (RN06)
e registra a troca como observação. */
Resultado os_atribuir_tecnico(long long os_id, long long tecnico_id, long long usuario_id);

/* Adiciona uma observação datada. OS encerrada não aceita observações. */
Resultado os_observar(long long os_id, long long usuario_id, const char *texto);

/* Histórico de mudanças de status, em ordem cronológica. */
int os_historico_status(long long os_id, RegistroOS vetor[], int maximo);

/* Observações da OS, em ordem cronológica. */
int os_observacoes(long long os_id, RegistroOS vetor[], int maximo);

/* ===================== Diagnóstico ===================== */

/* Passa a OS de "Aberta" para "Em diagnóstico". Exige técnico atribuído. */
Resultado os_iniciar_diagnostico(long long os_id, long long usuario_id, int perfil);

/* Registra (ou corrige) o diagnóstico e as peças previstas. Só o técnico
atribuído ou um gerente pode registrar. As peças previstas NÃO baixam o
estoque: são apenas uma previsão para o orçamento. */
Resultado diagnostico_registrar(long long os_id, long long usuario_id, int perfil,
const char *descricao, const char *procedimento,
const long long pecas_id[], const int quantidades[], int qtd_pecas);

/* Busca o diagnóstico com as peças previstas e o saldo atual de cada uma. */
Resultado diagnostico_buscar(long long os_id, Diagnostico *saida);

/* ===================== Orçamento e decisão do cliente ===================== */

/* Monta o orçamento com os serviços informados e as peças previstas no
diagnóstico, pelo preço atual (que fica congelado no orçamento).
Se já houver orçamento aguardando decisão, ele é substituído: é a
correção permitida antes da decisão do cliente (RN07). */
Resultado orcamento_elaborar(long long os_id, long long usuario_id, const ItemServico servicos[], int qtd_servicos);

/* Busca o orçamento com serviços, peças e total calculado. */
Resultado orcamento_buscar(long long os_id, Orcamento *saida);

/* Registra a decisão do cliente: aprovado = 1 (aprovação) ou 0 (recusa).
Só existe uma decisão por orçamento, e depois dela o orçamento não muda. */
Resultado orcamento_decidir(long long os_id, long long usuario_id, int aprovado);

/* ===================== Execução da manutenção ===================== */

/* Passa de "Aprovada" para "Em manutenção" (RN09: exige orçamento aprovado). */
Resultado manutencao_iniciar(long long os_id, long long usuario_id, int perfil);

/* Registra um serviço realizado durante a manutenção. */
Resultado manutencao_registrar_servico(long long os_id, long long usuario_id, int perfil, const char *descricao);

/* Registra uma peça utilizada e dá baixa no estoque na mesma transação
(algoritmo A4). Se o saldo for insuficiente, nada é gravado. */
Resultado manutencao_usar_peca(long long os_id, long long peca_id, int quantidade, long long usuario_id, int perfil);

/* Conclui a manutenção. Exige ao menos um serviço registrado e uma observação final. */
Resultado manutencao_concluir(long long os_id, long long usuario_id, int perfil, const char *observacao_final);

/* Serviços realizados, em ordem cronológica. */
int os_servicos_realizados(long long os_id, RegistroOS vetor[], int maximo);

/* Peças utilizadas, somadas por peça. */
int os_pecas_utilizadas(long long os_id, ItemPeca vetor[], int maximo);

/* ===================== Pagamento, liberação, entrega e encerramento ===================== */

/* Valor a cobrar. DECISÃO D11 (proposta): total do orçamento aprovado. */
Resultado os_valor_final(long long os_id, long long *centavos);

/* Registra o pagamento de uma OS concluída. DECISÃO D12 (proposta): pagamento
integral e único; "forma" é o índice em FORMAS_PAGAMENTO (config.h). */
Resultado pagamento_registrar(long long os_id, long long valor_centavos, int forma, long long usuario_id);

/* Situação do pagamento: 1 = pago (ou orçamento sem valor), 0 = pendente,
-1 = não se aplica (sem orçamento aprovado). Se pago, preenche forma e data
(podem ser nulos). */
int pagamento_situacao(long long os_id, char forma[], int tamanho_forma, char data[]);

/* Libera o equipamento para retirada. DECISÃO D13 (proposta): OS concluída e
paga, ou orçamento recusado (devolução sem reparo, D09). */
Resultado os_liberar(long long os_id, long long usuario_id);

/* Registra a entrega ao cliente, com o nome de quem recebeu. */
Resultado entrega_registrar(long long os_id, const char *recebido_por, long long usuario_id);

/* Encerra a OS entregue. Depois disso, ela não aceita alterações. */
Resultado os_encerrar(long long os_id, long long usuario_id);

/* ===================== Consultas auxiliares ===================== */

/* Observação final e data da conclusão técnica. Devolve 1 se existir. */
int os_conclusao(long long os_id, char observacao_final[TAM_TEXTO], char data[TAM_DATA]);

/* Quem recebeu e data da entrega. Devolve 1 se existir. */
int os_entrega(long long os_id, char recebido_por[TAM_NOME], char data[TAM_DATA]);

#endif

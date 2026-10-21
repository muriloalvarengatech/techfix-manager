# Regras de Negócio | TechFix Manager

## 1. Objetivo

Registrar as regras operacionais que deverão orientar o funcionamento do TechFix Manager.

As regras abaixo constituem uma proposta inicial e deverão ser validadas pela equipe e confrontadas com os requisitos oficiais do PIM antes de serem consideradas definitivas.

## 2. Clientes e equipamentos

- **RN01:** Todo cliente deverá possuir um identificador único.
- **RN02:** Todo equipamento cadastrado deverá possuir um identificador único.
- **RN03:** Um equipamento deverá estar associado a um cliente cadastrado.
- **RN04:** Os dados obrigatórios deverão ser validados antes da confirmação do cadastro.

## 3. Ordens de serviço

- **RN05:** Toda ordem de serviço deverá possuir um identificador único.
- **RN06:** Uma ordem de serviço deverá estar associada a um cliente e a um equipamento.
- **RN07:** Uma ordem de serviço deverá possuir um status válido definido pelo sistema.
- **RN08:** As mudanças de status deverão respeitar as transições permitidas pelo fluxo operacional.
- **RN09:** O histórico da ordem de serviço deverá permitir identificar as principais alterações registradas.
- **RN10:** Uma ordem de serviço não deverá ser finalizada enquanto houver pendências obrigatórias previstas no fluxo aprovado.

## 4. Diagnósticos e orçamentos

- **RN11:** Um diagnóstico deverá estar associado a uma ordem de serviço existente.
- **RN12:** Um orçamento deverá estar associado à ordem de serviço correspondente.
- **RN13:** A aprovação ou recusa de um orçamento deverá ser registrada.
- **RN14:** A execução do reparo deverá respeitar as condições de aprovação definidas pelo processo da assistência técnica.
- **RN15:** Alterações relevantes de orçamento deverão ser registradas de acordo com as regras de histórico estabelecidas.

## 5. Estoque

- **RN16:** Cada item de estoque deverá possuir um identificador único.
- **RN17:** As saídas de estoque deverão respeitar a disponibilidade registrada, salvo se o grupo definir explicitamente outro comportamento.
- **RN18:** Cada movimentação deverá identificar o item, o tipo de movimento e a quantidade movimentada.
- **RN19:** O saldo de estoque deverá ser atualizado de maneira consistente após cada movimentação confirmada.
- **RN20:** Movimentações inválidas não deverão alterar o saldo registrado.

## 6. Pagamentos

- **RN21:** Cada registro de pagamento deverá estar associado a uma ordem de serviço existente.
- **RN22:** O sistema deverá distinguir os estados de pagamento definidos pelo projeto.
- **RN23:** Um registro de pagamento não deverá ser considerado concluído apenas porque os dados foram digitados; a operação deverá ser confirmada e registrada.
- **RN24:** As regras para pagamentos parciais, estornos, descontos e cancelamentos deverão ser definidas antes da implementação dessas funcionalidades.

## 7. Entregas

- **RN25:** Cada registro de entrega deverá estar associado a uma ordem de serviço existente.
- **RN26:** A entrega deverá respeitar os requisitos de conclusão definidos para a ordem de serviço.
- **RN27:** O sistema deverá registrar as informações necessárias para consultar posteriormente a entrega realizada.

## 8. Persistência e integridade

- **RN28:** Os registros deverão possuir identificadores que permitam distingui-los de outros registros.
- **RN29:** O sistema deverá validar os vínculos entre registros relacionados antes de confirmar uma operação.
- **RN30:** Operações que falharem não deverão deixar os dados em um estado inconsistente.
- **RN31:** As regras de exclusão e alteração de registros relacionados deverão ser definidas para evitar perda acidental de informações.

## 9. Acesso e responsabilidades

- **RN32:** Os perfis de funcionário e gerente deverão ter suas responsabilidades e permissões definidas antes da implementação do controle de acesso.
- **RN33:** As operações administrativas que exigirem restrições deverão respeitar as permissões aprovadas pelo grupo.
- **RN34:** O sistema deverá apresentar mensagens claras quando uma operação não puder ser executada por violar uma regra de negócio.

## 10. Pendências de validação

Antes da implementação definitiva, a equipe deverá decidir:

- Quais status uma ordem de serviço poderá assumir e quais transições serão permitidas.
- Se o sistema permitirá cancelar ordens de serviço e em quais condições.
- Quais condições autorizam a entrega de um equipamento.
- Como serão tratados pagamentos parciais, estornos e cancelamentos.
- Como serão registradas as garantias de serviço.
- Quais operações poderão ser realizadas por funcionários e quais serão restritas ao gerente.
- Como serão tratados registros duplicados, exclusões e correções.


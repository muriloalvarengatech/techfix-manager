# Requisitos do Sistema | TechFix Manager

## 1. Objetivo

Definir os requisitos funcionais e não funcionais iniciais do TechFix Manager para orientar o desenvolvimento, o planejamento das Sprints e os testes de aceitação.

## 2. Requisitos funcionais

### Módulo de clientes

- **RF01:** O sistema deverá permitir cadastrar clientes.
- **RF02:** O sistema deverá permitir consultar clientes cadastrados.
- **RF03:** O sistema deverá permitir atualizar os dados de um cliente.
- **RF04:** O sistema deverá permitir consultar os atendimentos relacionados a um cliente.

### Módulo de equipamentos

- **RF05:** O sistema deverá permitir registrar equipamentos recebidos para assistência técnica.
- **RF06:** O sistema deverá permitir consultar os equipamentos cadastrados.
- **RF07:** O sistema deverá associar cada equipamento a um cliente.
- **RF08:** O sistema deverá permitir atualizar os dados de um equipamento, respeitando as regras de integridade dos registros.

### Módulo de ordens de serviço

- **RF09:** O sistema deverá permitir abrir ordens de serviço para equipamentos recebidos.
- **RF10:** O sistema deverá gerar ou atribuir um identificador único a cada ordem de serviço.
- **RF11:** O sistema deverá permitir consultar uma ordem de serviço pelo identificador.
- **RF12:** O sistema deverá permitir acompanhar o status de uma ordem de serviço.
- **RF13:** O sistema deverá manter o vínculo entre a ordem de serviço, o cliente e o equipamento correspondente.
- **RF14:** O sistema deverá registrar o histórico de alterações relevantes da ordem de serviço.

### Módulo de diagnósticos

- **RF15:** O sistema deverá permitir registrar o diagnóstico de um equipamento.
- **RF16:** O sistema deverá associar o diagnóstico à ordem de serviço correspondente.
- **RF17:** O sistema deverá permitir consultar os diagnósticos registrados.

### Módulo de orçamentos

- **RF18:** O sistema deverá permitir elaborar orçamentos associados a ordens de serviço.
- **RF19:** O sistema deverá permitir registrar os valores previstos de mão de obra e peças, quando aplicáveis.
- **RF20:** O sistema deverá permitir registrar o estado do orçamento, incluindo aprovação ou recusa pelo cliente.
- **RF21:** O sistema deverá permitir consultar os orçamentos de uma ordem de serviço.

### Módulo de estoque

- **RF22:** O sistema deverá permitir cadastrar peças e itens de estoque.
- **RF23:** O sistema deverá permitir consultar a disponibilidade dos itens.
- **RF24:** O sistema deverá registrar movimentações de entrada e saída.
- **RF25:** O sistema deverá manter o saldo de estoque consistente com as movimentações registradas.
- **RF26:** O sistema deverá permitir consultar o histórico de movimentações.

### Módulo de pagamentos

- **RF27:** O sistema deverá permitir registrar pagamentos relacionados a ordens de serviço.
- **RF28:** O sistema deverá permitir consultar os registros de pagamento.
- **RF29:** O sistema deverá manter o estado do pagamento registrado, de acordo com as regras definidas pelo projeto.

### Módulo de entregas

- **RF30:** O sistema deverá permitir registrar a entrega de um equipamento.
- **RF31:** O sistema deverá associar a entrega à ordem de serviço correspondente.
- **RF32:** O sistema deverá permitir consultar as informações de entrega.

### Consultas e histórico

- **RF33:** O sistema deverá permitir consultar o histórico de serviços de um cliente.
- **RF34:** O sistema deverá permitir consultar o histórico de uma ordem de serviço.
- **RF35:** O sistema deverá apresentar mensagens claras para operações concluídas ou que apresentem erros.

### Persistência e integridade

- **RF36:** O sistema deverá armazenar os registros em um banco de dados.
- **RF37:** O sistema deverá recuperar os registros armazenados quando for executado novamente.
- **RF38:** O sistema deverá validar os dados antes de realizar operações de cadastro e atualização.
- **RF39:** O sistema deverá impedir operações que violem as regras de integridade definidas.

## 3. Requisitos não funcionais

- **RNF01 - Usabilidade:** A interface de terminal deverá apresentar menus, instruções e mensagens compreensíveis.
- **RNF02 - Integridade:** Os dados relacionados deverão manter vínculos consistentes no banco de dados.
- **RNF03 - Persistência:** Os registros confirmados deverão continuar disponíveis após o encerramento e a reabertura do programa, conforme as garantias do banco de dados.
- **RNF04 - Manutenibilidade:** O código deverá ser organizado em módulos com responsabilidades definidas.
- **RNF05 - Portabilidade:** O projeto deverá documentar o ambiente e os procedimentos necessários para compilação e execução.
- **RNF06 - Tratamento de erros:** Falhas de entrada, banco de dados e operações inválidas deverão ser tratadas de maneira controlada.
- **RNF07 - Testabilidade:** As funcionalidades deverão possuir critérios verificáveis e ser submetidas a testes.
- **RNF08 - Segurança:** Os dados de acesso ao banco deverão ser tratados de maneira adequada ao ambiente acadêmico, evitando exposição desnecessária de credenciais.
- **RNF09 - Documentação:** As decisões técnicas e os procedimentos de configuração, compilação e execução deverão ser documentados.

## 4. Diretrizes de validação

Cada requisito deverá ser revisado para verificar:

- Se corresponde ao escopo aprovado do PIM.
- Se está descrito de maneira clara e sem ambiguidades.
- Se possui critérios de aceitação verificáveis.
- Se depende de outro requisito ou módulo.
- Se pode ser desenvolvido e testado dentro de uma Sprint.

## 5. Pendências

Os requisitos relativos a permissões, exclusão de registros, cancelamento de ordens, garantias, formas de pagamento, relatórios e notificações deverão ser detalhados após a revisão das orientações acadêmicas e das regras de negócio.


# Visão do Produto | TechFix Manager

## 1. Identificação

- **Projeto:** TechFix Manager
- **Contexto:** Projeto Integrado Multidisciplinar (PIM)
- **Área:** Análise e Desenvolvimento de Sistemas
- **Tipo de sistema:** Sistema de gerenciamento de assistência técnica
- **Linguagem:** C
- **Ambiente de desenvolvimento:** Dev-C++
- **Interface:** Terminal (CLI)
- **Armazenamento:** Banco de dados relacional, com SGBD a definir

## 2. Contexto e problema

Uma assistência técnica precisa organizar informações de clientes, equipamentos, ordens de serviço, diagnósticos, orçamentos, peças, pagamentos e entregas.

Quando essas informações ficam dispersas ou são registradas sem um processo padronizado, torna-se mais difícil acompanhar o andamento dos serviços, consultar históricos, controlar peças e identificar inconsistências nos registros.

O TechFix Manager propõe centralizar essas informações e organizar o fluxo operacional da empresa em um único sistema.

## 3. Visão do produto

Desenvolver um sistema de gerenciamento em C, executado pelo terminal e integrado a um banco de dados, que permita aos funcionários e ao gerente da TechFix registrar, consultar e acompanhar os serviços de assistência técnica com organização, agilidade e confiabilidade.

## 4. Público-alvo

### Funcionários da assistência técnica

Precisam registrar clientes, equipamentos, ordens de serviço, diagnósticos, orçamentos e outras informações relacionadas ao atendimento.

### Gerente ou administrador

Precisa acompanhar os serviços, consultar históricos, verificar informações operacionais e supervisionar os registros da empresa.

As permissões específicas de cada perfil serão definidas durante o detalhamento dos requisitos.

## 5. Objetivo geral

Desenvolver um sistema que centralize os registros da assistência técnica, facilite o acompanhamento dos serviços e contribua para a redução de inconsistências, retrabalho e perda de informações.

## 6. Objetivos específicos

- Cadastrar e consultar clientes.
- Registrar equipamentos recebidos para manutenção.
- Criar e acompanhar ordens de serviço.
- Registrar diagnósticos e orçamentos.
- Controlar peças e movimentações de estoque.
- Registrar pagamentos e entregas.
- Consultar o histórico dos atendimentos.
- Facilitar a localização das informações.
- Manter os registros persistidos em banco de dados.
- Apoiar o acompanhamento operacional pelo gerente.

## 7. Escopo inicial

O sistema deverá contemplar os seguintes módulos:

1. Clientes.
2. Equipamentos.
3. Ordens de serviço.
4. Diagnósticos.
5. Orçamentos.
6. Estoque.
7. Pagamentos.
8. Entregas.
9. Histórico de operações e consultas.

A ordem de implementação será definida pelo Product Owner em conjunto com a equipe, considerando dependências técnicas, prioridades e capacidade de entrega.

## 8. Fora do escopo inicial

Salvo exigência acadêmica ou decisão posterior do grupo, não fazem parte da versão inicial:

- Interface web ou aplicativo mobile.
- Aplicativo com interface gráfica.
- Integração com sistemas bancários para executar pagamentos.
- Emissão fiscal integrada a serviços governamentais.
- Integrações externas não previstas nos requisitos acadêmicos.

## 9. Indicadores de sucesso

O produto será considerado bem encaminhado quando:

- Os módulos previstos estiverem implementados ou adequadamente tratados no escopo aprovado.
- Os registros puderem ser armazenados e recuperados do banco de dados.
- As operações principais forem testadas.
- As informações relacionadas aos serviços puderem ser consultadas de maneira consistente.
- A equipe conseguir compilar, executar e testar o projeto seguindo a documentação.
- Os requisitos acadêmicos forem atendidos.

## 10. Equipe e organização

- **Murilo Alvarenga:** Product Owner.
- **Clara Henrique:** Scrum Master.
- **João Pedro:** Desenvolvedor.
- **Rafael:** Desenvolvedor.
- **Gustavo:** Desenvolvedor.

O projeto será organizado em Sprints semanais, com tarefas acompanhadas no Trello, documentação e versionamento no GitHub e reuniões semanais de acompanhamento.

## 11. Pendências de definição

- Escolha do sistema gerenciador de banco de dados (SGBD).
- Definição dos perfis e permissões de acesso.
- Definição detalhada das regras de negócio.
- Validação dos requisitos com as orientações oficiais do PIM.
- Revisão e priorização do backlog.


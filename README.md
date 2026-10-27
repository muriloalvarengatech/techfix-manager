# TechFix Manager

Sistema de gestão de assistência técnica de equipamentos eletrônicos, desenvolvido em linguagem C como parte do Projeto Integrado Multidisciplinar II (PIM II) do curso de Análise e Desenvolvimento de Sistemas.

## Objetivo

Centralizar e organizar o ciclo de atendimento da TechFix, desde o cadastro de clientes e equipamentos até o acompanhamento das ordens de serviço, diagnósticos, orçamentos, estoque, pagamentos e entregas.

## Funcionalidades previstas

- Cadastro e consulta de clientes.
- Cadastro de equipamentos.
- Abertura e acompanhamento de ordens de serviço.
- Registro de diagnósticos e orçamentos.
- Controle de peças e movimentações de estoque.
- Registro de pagamentos.
- Controle de conclusão e entrega.
- Consulta de histórico e relatórios operacionais.

> **Status do projeto:** em desenvolvimento. As funcionalidades listadas representam o escopo planejado e serão marcadas como concluídas após implementação e testes.

## Tecnologias

- Linguagem C.
- Dev-C++ ou Code::Blocks como ambiente de desenvolvimento.
- SQLite como opção de banco de dados, sujeita à validação da integração.
- Git e GitHub para controle de versão e documentação.

## Estrutura do repositório

```text
TechFixManager/
├── README.md
├── docs/
│   ├── 01-especificacao-do-sistema.md
│   ├── 02-backlog.md
│   ├── 03-modelagem-uml.md
│   ├── 04-modelagem-do-banco.md
│   ├── 05-arquitetura-e-seguranca.md
│   └── 06-plano-de-testes.md
├── src/
├── database/
└── tests/
```

As pastas e os arquivos serão adicionados conforme os respectivos conteúdos forem produzidos.

## Documentação

A especificação inicial está disponível em [`docs/01-especificacao-do-sistema.md`](docs/01-especificacao-do-sistema.md).

Ela apresenta o problema, os objetivos, o escopo, os atores, os requisitos, as regras de negócio e os modelos conceituais do sistema.

## Ambiente de desenvolvimento

O código será preparado para compiladores C compatíveis com o ambiente selecionado. A integração com SQLite será documentada após sua configuração e validação.

As instruções definitivas para compilação e execução serão incluídas quando a primeira versão executável estiver disponível.

## Contexto acadêmico

Projeto desenvolvido para fins acadêmicos no âmbito do PIM II. A organização TechFix é fictícia e utilizada para contextualizar a solução proposta.

## Situação atual

- [x] Escopo inicial definido.
- [x] Requisitos iniciais documentados.
- [x] Atores e regras de negócio identificados.
- [x] Modelos conceituais iniciais elaborados.
- [ ] Estrutura do projeto em C.
- [ ] Integração e validação do SQLite.
- [ ] Implementação dos módulos.
- [ ] Testes funcionais.
- [ ] Documentação final e evidências.

O status será atualizado conforme o desenvolvimento avançar.

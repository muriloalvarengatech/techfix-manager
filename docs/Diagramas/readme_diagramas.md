## Arquivos
- `01_UML/01_casos_de_uso.puml`: casos de uso associados aos perfis conforme a matriz `PERMISSOES` de `src/regras/regras.c`.
- `01_UML/02_modelo_conceitual_classes.puml`: modelo conceitual de entidades do domínio. O projeto é C procedural; o diagrama de classes é uma notação conceitual UML.
- `02_Banco_de_Dados/03_DER_completo.mmd`: DER lógico de todas as tabelas do esquema SQL.
- `03_Processos/04_atividade_fluxo_os.mmd`: atividades do ciclo de atendimento da OS.
- `03_Processos/05_estados_os.puml`: estados e transições implementados em `src/regras/regras.c`.
- `03_Processos/06_sequencia_os.puml`: sequência de alto nível entre os atores e as camadas.
- `04_Arquitetura/07_componentes.mmd`: módulos reais do projeto.

## Decisões de modelagem
- O banco contém as tabelas `usuario`, `cliente`, `equipamento`, `ordem_servico`, `historico_status`, `observacao`, `peca`, `movimentacao`, `diagnostico`, `diagnostico_peca`, `orcamento`, `orcamento_servico`, `orcamento_peca`, `servico_realizado`, `peca_utilizada`, `conclusao_tecnica`, `pagamento` e `entrega`.
- O campo `ordem_servico.status` usa os dez estados declarados em `include/regras.h`.
- A matriz de permissões reproduzida no caso de uso é a implementada em `src/regras/regras.c`: Atendente, Técnico e Gerente. O Gerente tem todas as permissões e é modelado como especialização de Atendente e Técnico.
- Dinheiro é persistido em centavos (`INTEGER`); datas são texto local `AAAA-MM-DD HH:MM:SS`.
- A tabela `diagnostico` tem relação 1:1 com OS (`os_id UNIQUE`); `orcamento` também tem relação 1:1; `pagamento` e `entrega` são únicos por OS.
- `conclusao_tecnica.os_id` é chave primária e FK para OS.
- `movimentacao.os_id` é opcional; a movimentação sempre referencia peça e usuário.
- `peca_utilizada.movimentacao_id` é único e referencia uma movimentação.

## Imagens PNG
Cada arquivo-fonte tem um `.png` de mesmo nome na mesma pasta (PlantUML 1.2024.7 e Mermaid CLI).

## Correções da v3 em relação à v2
1. **Casos de uso**: o Técnico tem `F_PECA_GERIR` e `F_ESTOQUE_ENTRADA` em `regras.c` (a v2 mostrava só o Gerente). Incluídos os casos que existem nos menus e faltavam: Iniciar diagnóstico, Liberar equipamento para entrega, Trocar a própria senha, Criar primeiro gerente. "Executar manutenção" foi dividido nas quatro opções reais (iniciar, serviço, peça utilizada, concluir). Consultas e relatórios separados conforme `F_HISTORICO` e `F_RELATORIOS`.
2. **Modelo conceitual**: a OS é opcional na Movimentação (`0..1`, não `1`). Adicionadas as enumerações `Perfil` e `StatusOS` e as formas de pagamento de `ordens.c`.
3. **DER**: a v2 não renderizava (Mermaid não aceita `FK_UK`/`PK_FK`); corrigido para `FK, UK` e `PK, FK`. O conteúdo das tabelas já conferia com `sql/schema.sql`.
4. **Atividades**: incluída a etapa "Iniciar diagnóstico" (Aberta → Em diagnóstico); incluídos o pagamento integral (só quando o valor é maior que zero, sem mudar status) e a liberação como ação própria, também na recusa; correção do orçamento antes da decisão; regra de ao menos um serviço antes de concluir; rollback quando falta saldo; falha no login após 3 tentativas.
5. **Estados**: transições rotuladas com as funções de `ordens.c` e suas condições (técnico atribuído, ≥ 1 serviço, pagamento ou valor zero para liberar).
6. **Sequência**: atribuir técnico não muda status (grava observação); pagamento não muda status; adicionados "Iniciar diagnóstico", "Liberar equipamento", a verificação de senha em `seguranca.c`, a validação de transição feita pela camada de dados (`mudar_status` → `status_pode_transitar`) e o ator Gerente declarado.
7. **Componentes**: removidas dependências inexistentes (`src/ui` → `seguranca.c`, `regras.c` → `src/util`); adicionadas as reais (`ordens.c` → `regras.c`, `cadastros.c`, `estoque.c`; `cadastros.c` → `seguranca.c`/`regras.c`; `main.c` → `cadastros.c`/`db.c`; `seguranca.c` → SQLite para gerar o sal).

## Como visualizar
- Mermaid: arquivos `.mmd` podem ser visualizados no GitHub com extensão compatível ou copiados para um bloco ```mermaid``` em Markdown.
- PlantUML: arquivos `.puml` podem ser renderizados por extensão PlantUML ou serviço compatível.

## Limites
- A sequência é uma visão de alto nível das chamadas e responsabilidades, não uma captura literal de todas as funções.
- O diagrama de implantação não foi incluído nesta revisão porque a configuração final de distribuição/instalação do programa não foi verificada.
- Para entregar o PIM completo, acrescentar texto explicativo sobre modelo conceitual, lógico e físico, normalização e justificativa das chaves/relacionamentos conforme o manual acadêmico.
- A validação definitiva deve ser feita comparando estes arquivos com a versão final do repositório e as regras aprovadas pelo professor.

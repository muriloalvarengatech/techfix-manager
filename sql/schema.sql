-- TechFix Manager - esquema físico do banco (SQLite)
-- Valores em dinheiro são gravados em centavos (INTEGER) para evitar erros de arredondamento.
-- Datas são gravadas como texto "AAAA-MM-DD HH:MM:SS" no horario local.
-- Este arquivo é executado pelo programa ao abrir o banco (src/dados/esquema.c).

CREATE TABLE IF NOT EXISTS usuario (
id INTEGER PRIMARY KEY AUTOINCREMENT,
login TEXT NOT NULL UNIQUE,
nome TEXT NOT NULL,
perfil INTEGER NOT NULL CHECK (perfil BETWEEN 1 AND 3),
senha_hash TEXT NOT NULL,
senha_sal TEXT NOT NULL,
ativo INTEGER NOT NULL DEFAULT 1 CHECK (ativo IN (0, 1)),
criado_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);

CREATE TABLE IF NOT EXISTS cliente (
id INTEGER PRIMARY KEY AUTOINCREMENT,
nome TEXT NOT NULL,
telefone TEXT NOT NULL,
email TEXT,
ativo INTEGER NOT NULL DEFAULT 1 CHECK (ativo IN (0, 1)),
criado_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);
CREATE INDEX IF NOT EXISTS ix_cliente_nome ON cliente (nome);

CREATE TABLE IF NOT EXISTS equipamento (
id INTEGER PRIMARY KEY AUTOINCREMENT,
cliente_id INTEGER NOT NULL REFERENCES cliente (id),
tipo TEXT NOT NULL,
marca TEXT NOT NULL,
modelo TEXT NOT NULL,
identificador TEXT NOT NULL UNIQUE,
descricao TEXT,
criado_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);
CREATE INDEX IF NOT EXISTS ix_equipamento_cliente ON equipamento (cliente_id);

CREATE TABLE IF NOT EXISTS ordem_servico (
id INTEGER PRIMARY KEY AUTOINCREMENT,
equipamento_id INTEGER NOT NULL REFERENCES equipamento (id),
aberta_por INTEGER NOT NULL REFERENCES usuario (id),
tecnico_id INTEGER REFERENCES usuario (id),
problema TEXT NOT NULL,
status INTEGER NOT NULL CHECK (status BETWEEN 1 AND 10),
aberta_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
atualizada_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);
CREATE INDEX IF NOT EXISTS ix_os_status ON ordem_servico (status);
CREATE INDEX IF NOT EXISTS ix_os_equipamento ON ordem_servico (equipamento_id);
CREATE INDEX IF NOT EXISTS ix_os_tecnico ON ordem_servico (tecnico_id);

CREATE TABLE IF NOT EXISTS historico_status (
id INTEGER PRIMARY KEY AUTOINCREMENT,
os_id INTEGER NOT NULL REFERENCES ordem_servico (id),
status_anterior INTEGER,
status_novo INTEGER NOT NULL,
usuario_id INTEGER NOT NULL REFERENCES usuario (id),
registrado_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);
CREATE INDEX IF NOT EXISTS ix_historico_os ON historico_status (os_id);

CREATE TABLE IF NOT EXISTS observacao (
id INTEGER PRIMARY KEY AUTOINCREMENT,
os_id INTEGER NOT NULL REFERENCES ordem_servico (id),
usuario_id INTEGER NOT NULL REFERENCES usuario (id),
texto TEXT NOT NULL,
registrada_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);

CREATE TABLE IF NOT EXISTS peca (
id INTEGER PRIMARY KEY AUTOINCREMENT,
codigo TEXT NOT NULL UNIQUE,
descricao TEXT NOT NULL,
preco_centavos INTEGER NOT NULL CHECK (preco_centavos >= 0),
saldo INTEGER NOT NULL DEFAULT 0 CHECK (saldo >= 0),
criado_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);

CREATE TABLE IF NOT EXISTS movimentacao (
id INTEGER PRIMARY KEY AUTOINCREMENT,
peca_id INTEGER NOT NULL REFERENCES peca (id),
tipo TEXT NOT NULL CHECK (tipo IN ('E', 'S')),
quantidade INTEGER NOT NULL CHECK (quantidade > 0),
motivo TEXT NOT NULL,
os_id INTEGER REFERENCES ordem_servico (id),
usuario_id INTEGER NOT NULL REFERENCES usuario (id),
registrada_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);
CREATE INDEX IF NOT EXISTS ix_movimentacao_peca ON movimentacao (peca_id);

CREATE TABLE IF NOT EXISTS diagnostico (
id INTEGER PRIMARY KEY AUTOINCREMENT,
os_id INTEGER NOT NULL UNIQUE REFERENCES ordem_servico (id),
tecnico_id INTEGER NOT NULL REFERENCES usuario (id),
descricao TEXT NOT NULL,
procedimento TEXT NOT NULL,
registrado_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);

CREATE TABLE IF NOT EXISTS diagnostico_peca (
diagnostico_id INTEGER NOT NULL REFERENCES diagnostico (id),
peca_id INTEGER NOT NULL REFERENCES peca (id),
quantidade INTEGER NOT NULL CHECK (quantidade > 0),
PRIMARY KEY (diagnostico_id, peca_id)
);

CREATE TABLE IF NOT EXISTS orcamento (
id INTEGER PRIMARY KEY AUTOINCREMENT,
os_id INTEGER NOT NULL UNIQUE REFERENCES ordem_servico (id),
criado_por INTEGER NOT NULL REFERENCES usuario (id),
criado_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
decisao TEXT CHECK (decisao IN ('A', 'R')),
decidido_por INTEGER REFERENCES usuario (id),
decidido_em TEXT
);

CREATE TABLE IF NOT EXISTS orcamento_servico (
id INTEGER PRIMARY KEY AUTOINCREMENT,
orcamento_id INTEGER NOT NULL REFERENCES orcamento (id),
descricao TEXT NOT NULL,
valor_centavos INTEGER NOT NULL CHECK (valor_centavos >= 0)
);

CREATE TABLE IF NOT EXISTS orcamento_peca (
orcamento_id INTEGER NOT NULL REFERENCES orcamento (id),
peca_id INTEGER NOT NULL REFERENCES peca (id),
quantidade INTEGER NOT NULL CHECK (quantidade > 0),
preco_unit_centavos INTEGER NOT NULL CHECK (preco_unit_centavos >= 0),
PRIMARY KEY (orcamento_id, peca_id)
);

CREATE TABLE IF NOT EXISTS servico_realizado (
id INTEGER PRIMARY KEY AUTOINCREMENT,
os_id INTEGER NOT NULL REFERENCES ordem_servico (id),
usuario_id INTEGER NOT NULL REFERENCES usuario (id),
descricao TEXT NOT NULL,
registrado_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);

CREATE TABLE IF NOT EXISTS peca_utilizada (
id INTEGER PRIMARY KEY AUTOINCREMENT,
os_id INTEGER NOT NULL REFERENCES ordem_servico (id),
peca_id INTEGER NOT NULL REFERENCES peca (id),
quantidade INTEGER NOT NULL CHECK (quantidade > 0),
movimentacao_id INTEGER NOT NULL UNIQUE REFERENCES movimentacao (id),
registrada_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);

CREATE TABLE IF NOT EXISTS conclusao_tecnica (
os_id INTEGER PRIMARY KEY REFERENCES ordem_servico (id),
tecnico_id INTEGER NOT NULL REFERENCES usuario (id),
observacao_final TEXT NOT NULL,
concluida_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);

CREATE TABLE IF NOT EXISTS pagamento (
id INTEGER PRIMARY KEY AUTOINCREMENT,
os_id INTEGER NOT NULL UNIQUE REFERENCES ordem_servico (id),
valor_centavos INTEGER NOT NULL CHECK (valor_centavos > 0),
forma TEXT NOT NULL,
usuario_id INTEGER NOT NULL REFERENCES usuario (id),
registrado_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);

CREATE TABLE IF NOT EXISTS entrega (
id INTEGER PRIMARY KEY AUTOINCREMENT,
os_id INTEGER NOT NULL UNIQUE REFERENCES ordem_servico (id),
recebido_por TEXT NOT NULL,
usuario_id INTEGER NOT NULL REFERENCES usuario (id),
registrada_em TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);

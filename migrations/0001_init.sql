PRAGMA foreign_keys=ON;
CREATE TABLE IF NOT EXISTS metadata(key TEXT PRIMARY KEY, value TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS regiao_funcional(id TEXT PRIMARY KEY, numero INTEGER NOT NULL UNIQUE);
CREATE TABLE IF NOT EXISTS corede(id TEXT PRIMARY KEY, nome TEXT NOT NULL, regiao_funcional_id TEXT NOT NULL REFERENCES regiao_funcional(id));
CREATE TABLE IF NOT EXISTS bioma(id TEXT PRIMARY KEY, nome TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS municipio(
 id INTEGER PRIMARY KEY, codigo_ibge TEXT UNIQUE, nome TEXT NOT NULL UNIQUE, nome_busca TEXT NOT NULL,
 uf TEXT NOT NULL CHECK(uf='RS'), corede_id TEXT NOT NULL REFERENCES corede(id),
 regiao_funcional_id TEXT NOT NULL REFERENCES regiao_funcional(id), bioma_predominante_id TEXT REFERENCES bioma(id),
 biomas_presentes_json TEXT, classificacao_bioma_status TEXT NOT NULL);
CREATE INDEX IF NOT EXISTS idx_municipio_corede ON municipio(corede_id);
CREATE INDEX IF NOT EXISTS idx_municipio_rf ON municipio(regiao_funcional_id);
CREATE INDEX IF NOT EXISTS idx_municipio_busca ON municipio(nome_busca);
CREATE TABLE IF NOT EXISTS source(id INTEGER PRIMARY KEY, payload_json TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS audit_event(id INTEGER PRIMARY KEY, event TEXT NOT NULL, created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);

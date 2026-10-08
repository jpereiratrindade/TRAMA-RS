# TERRITÓRIO-RS-001 — Constituição do Serviço de Referência Territorial e Ambiental

**Versão:** 0.1.0 — `design-candidate`  
**Data:** 2026-10-08  
**Autoria institucional sugerida, a confirmar:** NACDA / Embrapa Pecuária Sul  
**Lema de engenharia:** **Sempre pronto. Sempre incompleto.**

## Missão para o Gemini

Desenvolva uma aplicação **C++26** (com fallback documentado para C++23 quando a toolchain não implementar C++26 plenamente), **offline-first**, multiplataforma Linux x86_64 e Raspberry Pi 5/aarch64, que ofereça uma **API REST somente-leitura** de referência territorial do Rio Grande do Sul. A primeira versão deve entregar resultados úteis mesmo sem acesso à rede e incluir uma página web de exploração local. Não criar listas de municípios ou biomas por inferência. Consumir os JSON existentes e substituir o conjunto preliminar por arquivos integrados das fontes oficiais quando disponíveis.

## 1. Propósito e fronteira

Um **catálogo geoterritorial compartilhado**, não uma nova plataforma institucional, nem um motor de IA, nem uma fonte primária. Deve servir ELO e demais aplicações por API HTTP local e futura biblioteca C++ incorporável. Não duplicar lógica regional nas aplicações consumidoras.

* 497 municípios (a unidade de identificação é o **geocódigo IBGE, string com 7 dígitos**, quando confirmado).
* 28 COREDEs; 9 Regiões Funcionais de Planejamento.
* 2 biomas terrestres no estado: **Pampa** e **Mata Atlântica**.
* COREDE → Região Funcional: muitos-para-um.
* Município → COREDE: um-para-um na versão temporal adotada.
* Município ↔ biomas presentes: muitos-para-muitos.
* Município → bioma predominante: um-para-um quando o IBGE 2024 estiver efetivamente importado.
* Municípios sem identificador IBGE confirmado ou sem bioma verificado devem conservar valores **`null`**. **`null` não significa ausência de bioma.**

O catálogo NÃO define que toda a extensão municipal possua o bioma predominante, NÃO considera COREDE como unidade ecológica e NÃO reclassifica biomas por coordenadas da sede municipal.

## 2. Fontes oficiais e versionamento

| Entidade/relação | Fonte primária | Referência | Nota |
|---|---|---|---|
| Município / COREDE / RF | Atlas Socioeconômico RS (SPGG) | https://atlassocioeconomico.rs.gov.br/upload/arquivos/202010/09172616-tabela-dos-municipios-por-corede-e-regiao-funcional-de-planejamento.xlsx | Relação territorial divulgada em 2020; cotejar com atualizações |
| COREDE / RF | Atlas RS | https://atlassocioeconomico.rs.gov.br/regioes-funcionais-de-planejamento | 28 COREDEs em 9 RFs |
| Bioma predominante | IBGE 2024 | https://geoftp.ibge.gov.br/informacoes_ambientais/estudos_ambientais/biomas/documentos/Bioma_Predominante_por_Municipio_2024.csv | Predominante corresponde à maior área do território |
| Biomas presentes | IBGE 2019 (1:250 000) | https://geoftp.ibge.gov.br/informacoes_ambientais/estudos_ambientais/biomas/documentos/Lista_Municipio_Bioma_250mil.xls | Relação multivalorada; ano/metodologia distintos de 2024 |
| Explicação da diferença entre produtos | IBGE, nota técnica | https://geoftp.ibge.gov.br/informacoes_ambientais/estudos_ambientais/biomas/documentos/LEIA_ME_Sobre_a_relacao_entre_municipios_e_biomas.pdf | Ler antes de combinar os dados |

**Atenção:** o pacote acompanha uma lista preliminar de 497 vínculos município-COREDE transcrita de uma compilação secundária (CC BY-SA 4.0); os códigos IBGE não foram preenchidos por estimativa e apenas dois exemplos de bioma foram previamente conferidos com a publicação IBGE 2024. Essa lista é apropriada para prototipação, mas não deve ser apresentada como conjunto oficialmente homologado. Executar importação e reconciliação com arquivos oficiais antes de uso científico final ou publicação.

## 3. Arquivos JSON disponíveis

- `data/coredes_regioes_funcionais.json`: 9 RFs + 28 COREDEs, identificadores `slug` internos.
- `data/municipios_coredes_preliminar.json`: 497 municípios atribuídos preliminarmente a COREDE/RF; `codigo_ibge: null` onde não validado; biome nulo onde não conferido.
- `data/biomas_rs.json`: vocabulário de biomas e critérios distintos de classificação.
- `data/catalogo_fontes.json`: fontes, estado da curadoria, convenções e invariantes.
- `tools/municipios_seed.txt`: insumo legível de 28 grupos; documentar licença de compilação secundária.
- `tools/prepare_seed.py`: reprodutor determinístico dos JSON preliminares.
- `tools/integrar_oficiais.py`: importador conservador das planilhas oficiais e do CSV IBGE, com falha fechada se faltar município ou não reconhecer layout.

### Exemplo do contrato de município

```json
{
  "codigo_ibge": "4303301",
  "nome": "Caibaté",
  "uf": "RS",
  "corede_id": "missoes",
  "regiao_funcional_id": "RF7",
  "bioma_predominante_id": "pampa",
  "biomas_presentes_ids": ["pampa", "mata-atlantica"],
  "classificacao_bioma_status": "exemplo_ibge_2024_interbiomas"
}
```

O exemplo é verificável no apêndice **Interbiomas, IBGE 2024**. As bases combinadas completas são derivadas SOMENTE após ingestão validada. A presença de dois biomas não deve ser confundida com frações territoriais conhecidas.

### Resultado da integração oficial

`data/gerados/municipios_rs.json` — 497 municípios com códigos IBGE únicos, COREDE, RF, bioma predominante IBGE 2024; se importada a planilha de 2019, também biomas presentes com indicação de referência temporal. Também produzir:

- `data/gerados/municipios_por_bioma_predominante_2024.json`
- `data/gerados/municipios_por_bioma_presenca_2019.json`, apenas se a fonte 2019 foi importada

O importador deve preservar a evidência de diferenças entre fontes (não silenciar divergências) e registrar SHA-256, URL, data de obtenção, licença, versão, esquema e horário do processamento de cada fonte primária.

## 4. Arquitetura inicial C++26

```
fontes IBGE / Governo RS (ingestão sob demanda, não durante GET)
                |
                v
  raw/ -> ETL validado -> JSON versionado -> SQLite3 (WAL)
                                             |
                             dominio + repositorio C++26
                                             |
                              HTTP REST (cpp-httplib)
                                             |
                    dashboard local / ELO / pesquisas / scripts
```

- C++26 para domínio, persistência e HTTP; `std::expected`/`std::filesystem` conforme disponibilidade, sem recursos artificiais ou dependentes de compiler bleeding edge.
- SQLite3 com `PRAGMA journal_mode=WAL`, `PRAGMA foreign_keys=ON`, transações ACID e migrações versionadas.
- API JSON: `nlohmann/json`, `cpp-httplib`, bibliotecas de versões conhecidas, travadas por release/commit e com licenças verificadas; CMake, CTest e testes de integração.
- Dados em tabelas normalizadas: `regiao_funcional`, `corede`, `municipio`, `bioma`, `municipio_bioma_presenca`, `fonte`, `importacao`, `revisao`.
- Chaves de município: `codigo_ibge TEXT` (admite NULL no seed preliminar; obrigatório após homologação); **jamais número de ponto flutuante**.
- ID de COREDE (`slug`) é identificador interno imutável do serviço, não alegar que é código oficial.
- O banco SQLite materializa uma versão do catálogo; importação é atômica (`staging` + validação + swap lógico), sem alterar o catálogo publicado no meio de uma consulta.
- Aplicação deve oferecer modo `--read-only`/somente consulta, sem dependência da internet para iniciar, com log estruturado, métricas básicas e `GET /health`.
- Publicar no loopback por padrão (`127.0.0.1`); acesso na rede local só por opção explícita, autenticação se houver exposição, CORS restrito e limites de consultas.

## 5. API REST v1 — contratos

| Método / endpoint | Propósito |
|---|---|
| `GET /v1/health` | Versão, integridade, status do banco |
| `GET /v1/catalogo` | Versão do esquema e dos dados, fontes, status de validação |
| `GET /v1/regioes-funcionais` | Nove regiões funcionais |
| `GET /v1/regioes-funcionais/RF7/coredes` | COREDEs de uma RF |
| `GET /v1/coredes` | Lista dos 28 COREDEs |
| `GET /v1/coredes/missoes/municipios` | Municípios de um COREDE |
| `GET /v1/municipios?corede_id=missoes` | Filtra por COREDE |
| `GET /v1/municipios?regiao_funcional_id=RF7` | Filtra por RF |
| `GET /v1/municipios?bioma_id=pampa&criterio=predominante` | Predominante IBGE 2024 |
| `GET /v1/municipios?bioma_id=pampa&criterio=presenca` | Presença IBGE 2019; `409/503` quando indisponível, nunca inventar |
| `GET /v1/municipios/4303301` | Identificação municipal por geocódigo IBGE |
| `GET /v1/biomas` | Vocabulário Pampa e Mata Atlântica |
| `GET /v1/biomas/pampa/municipios?criterio=presenca` | Relação por bioma, conforme critério |
| `GET /v1/estatisticas` | Quantidades, RFs, COREDEs e completude |
| `GET /` | Página local simples que explora município, COREDE, RF e bioma |

Parâmetros comuns: `limit` (1..500, padrão 100), `offset`, `q` (busca sem acento), `campos` opcional. Respostas com envelope `{ "data": ..., "meta": { "schema_version", "dataset_version", "status_validacao", "fonte", "criterio", "total" } }`.

**Regra obrigatória de resposta:** filtros de bioma jamais retornar conjuntos incompletos como se fossem completos. Em banco preliminar, `bioma_id` deve falhar explicitamente com mensagem `dados_bioma_incompletos` e opção de consultar apenas amostras (`/v1/amostras`).

## 6. Invariantes e testes de aceitação

1. **497 municípios distintos** e sem município atribuível a dois COREDEs na mesma vigência.
2. **28 COREDEs, 9 RFs**, todos com vínculo válido; totais por RF do conjunto preliminar: **70, 59, 49, 21, 22, 20, 77, 49, 130** (RF1..RF9).
3. Todos os 497 municípios oficiais com **geocódigo único de 7 dígitos iniciado por 43**, sem incluir as áreas estaduais operacionais **Lagoa Mirim e Lagoa dos Patos**.
4. Bioma predominante preenchido apenas por fonte **IBGE 2024**, sem cálculo a partir de COREDE ou sede.
5. `biomas_presentes_ids` só completo com fonte de presença importada; interbiomas podem pertencer a mais de um bioma.
6. Consulta de bioma indisponível retorna erro explicitamente tipado, não zero nem lista parcial.
7. Testar municípios nos limites: **Caibaté** (Pampa e Mata Atlântica, pred. Pampa) e **Campina das Missões** (ambos, pred. Mata Atlântica).
8. Verificar inconsistência temporal nas fontes e disponibilizar `fonte`/`ano`/`metodo` em toda resposta derivada.
9. JSON sintaticamente válido e UTF-8; `Content-Type: application/json; charset=utf-8`.
10. Testes de API, SQLite WAL e execução offline em x86_64 e Raspberry Pi 5. Não usar atualização de rede no caminho de leitura.

## 7. Entregas em incrementos

**v0.1.0 — Always ready:** carregar os quatro JSON preliminares, SQLite WAL, API de RF/COREDE/municípios e dashboard; distinguir respostas preliminares; testes básicos; README e script de instalação.

**v0.2.0 — Homologação de dados:** importar tabela Atlas e CSV IBGE; completar geocódigos/bioma predominante, auditar divergências, liberar consultas por bioma predominante, identificar versão/validade das fontes.

**v0.3.0 — Interbiomas:** importar lista IBGE 2019 ou interseção de geometrias homologada; consultas por presença; nunca misturar 2019 e 2024 sem ressalva.

**v0.4.0 — Ciência territorial:** proporções por bioma calculadas a partir de malhas vetoriais compatíveis, CRS/área/metadados registrados; opcional GeoJSON; filtros e análise por unidade experimental.

**v1.0.0 — Integrações:** SDK C++ do serviço; cache e índices; documentação OpenAPI; clientes ELO e sistemas de pesquisa; pacote RPi com systemd, checagem de integridade e atualização assinada de dados.

## 8. Como preparar os arquivos oficiais

1. Baixe os arquivos da tabela do Atlas e do CSV IBGE 2024 nas URLs oficiais e salve em `raw/` (não versionar raws pesados desnecessariamente).
2. Rode: `python3 tools/integrar_oficiais.py --atlas-xlsx raw/coredes.xlsx --ibge-csv raw/biomas_2024.csv`.
3. Para incluir todos os biomas presentes a partir da lista IBGE 2019, instale `xlrd` e acrescente `--presenca-xls raw/biomas_2019.xls`.
4. Se o parser reclamar de cabeçalhos/layout, adapte-o ao arquivo **inspecionado de fato**, sem relaxar as verificações 497/28/9, sem inferir bioma e sem mascarar erros.
5. Importe o JSON completo gerado para SQLite numa transação e habilite o endpoint correspondente.

## 9. Critério de definição de pronto

A primeira aplicação está pronta quando inicia sem internet, oferece `GET /v1/health`, `GET /v1/regioes-funcionais`, `GET /v1/coredes` e `GET /v1/municipios`, identifica a natureza preliminar dos dados, mostra uma interface web funcional, passa nos testes e pode ser executada em Raspberry Pi 5. Novos dados e relações são extensões versionadas, não mudanças silenciosas do contrato.

**Não escrever "dados oficiais completos" enquanto a importação e a reconciliação não tiverem sido executadas com sucesso.**

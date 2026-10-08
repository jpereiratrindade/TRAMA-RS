# TERRITÓRIO-RS-002 — Conversão integral para C++26

**Versão:** 0.2.0 — especificação de implementação  
**Data:** 2026-10-08  
**Relação com a constituição anterior:** complementa e, nos pontos conflitantes, substitui `GEMINI_IMPLEMENTACAO.md` (v0.1.0)  
**Lema:** **Sempre pronto. Sempre incompleto.**

> **Instrução ao Gemini:** transformar este pacote de dados e scripts em um repositório **executável nativamente em C++26**. A entrega requer código-fonte, build, testes, importador real, API e dashboard mínimo; não apenas documentos ou pseudocódigo. Manter os JSON existentes como fixtures e dados de partida. Não afirmar que os dados preliminares são oficialmente homologados.

## 1. Decisão arquitetural

Criar **uma biblioteca compartilhável de domínio (`territorio-core`)** e três aplicações (`territorio-rsd`, `territorio-rs`, `territorio-rs-verify`) usando C++26. Todos os processos de **execução, ingestão, geração de dados, auditoria, testes e empacotamento** devem funcionar sem interpretador Python, notebooks nem serviço externo. O painel pode utilizar HTML/CSS/JavaScript estáticos, servidos pelo processo C++.

Manter o Python original **somente como material histórico** em `legacy/python/`, excluído do build, da rotina de testes e das operações de produção. Não converter JSON em código-fonte nem embutir listas fixas de municípios no C++.

**Importante:** o projeto anterior já descrevia API C++26. O que muda nesta versão é **a substituição completa dos scripts ETL, de empacotamento e dos testes Python por executáveis e testes nativos C++26**, além de concretizar a biblioteca de domínio reutilizável.

## 2. Saídas obrigatórias

| Alvo CMake | Tipo | Responsabilidade |
|---|---|---|
| `territorio-core` | biblioteca | entidades, regras, tipagens fortes, consultas, erros, normalização de nomes |
| `territorio-data` | biblioteca | SQLite, WAL, migrações, leitura e escrita transacional, origem de cada relação |
| `territorio-import` | biblioteca | adaptadores para fontes CSV, XLSX, XLS, JSON e validação de lote |
| `territorio-http` | biblioteca | rotas REST, contratos JSON, tratamento de erros, limites de consulta |
| `territorio-rsd` | executável | daemon HTTP com dashboard estático, somente leitura na API pública |
| `territorio-rs` | executável CLI | `init`, `seed`, `import`, `validate`, `catalog`, `export`, `pack`, `sources`, `migrate` |
| `territorio-rs-verify` | executável | validação independente e relatórios de integridade/diferenças |
| `territorio-tests` | CTest | casos unitários, fixtures, importação, SQL e testes HTTP |

Usar apenas C++ para regras, CLI, ETL e servidor, mantendo JSON/CSV como formatos de intercâmbio.

## 3. Toolchain e dependências

- Linguagem: `C++26` com `CMAKE_CXX_STANDARD 26`, `CMAKE_CXX_STANDARD_REQUIRED ON`, `CMAKE_CXX_EXTENSIONS OFF`; CMake **3.30+** recomendado. Testar na toolchain real antes de assumir uma biblioteca específica de C++26.
- GCC/Clang: detectar suporte ao modo C++26 (`-std=c++26` ou `-std=c++2c` conforme compilador). Se a toolchain não o suportar, **falhar com diagnóstico claro**. Oferecer modo C++23 apenas como configuração explícita de portabilidade, sem alterar a decisão principal do projeto.
- JSON: `nlohmann/json`, versão fixada em gerenciador de dependências (`FetchContent`/vcpkg/Conan), sem download obrigatório durante a execução.
- HTTP: `cpp-httplib` para servidor simples, versão fixada; preferir arquitetura desacoplada do framework para futura substituição.
- Banco: SQLite3, WAL configurado na fase de preparação/escrita; foreign keys ON; transações ACID; prepared statements; busy_timeout. Suportar `SQLITE_OPEN_READONLY` no servidor.
- XLSX (Atlas): `libxlsxio_read` ou adaptador robusto `libzip` + parser XML, com testes para células esparsas, strings compartilhadas e abas múltiplas.
- XLS (IBGE 2019): adaptador opcional `libxls`. Quando não compilado, `import --presenca-xls` deve falhar expressamente com `FORMATO_XLS_NAO_SUPORTADO`; alternativa operacional nativa: converter/importar um CSV previamente exportado, com metadados da transformação. **Não inventar vínculos.**
- CSV: parser de aspas, quebras de linha e separadores; BOM UTF-8 e codificações legadas controladas (iconv/ICU conforme disponibilidade). Normalização Unicode com biblioteca explícita (p.ex. `utf8proc`/ICU), pois `lower()`/SQLite `NOCASE` não resolvem corretamente todos os acentos.
- Hash: biblioteca de SHA-256 definida e versionada; preservação dos manifestos e hashes dos **arquivos originais**.
- Testes: CTest + framework C++ (Catch2 ou doctest) fixado por versão.
- Linux x86_64 e aarch64/Raspberry Pi 5; build em Fedora/Silverblue sem modificações permanentes no sistema host (toolbox/Podman quando conveniente).

**Evitar complexidade artificial:** usar recursos C++23/26 realmente suportados na toolchain (`std::expected`, `std::filesystem`, `std::span`, RAII) e não exigir módulos, reflexão ou bibliotecas experimentais para a primeira versão.

## 4. Estrutura pretendida do repositório

```text
territorio-rs/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── GEMINI_CPP26_CONVERSAO.md
├── include/territorio/{domain,application,storage,import,http}/
├── src/{domain,application,storage,import,http}/
├── apps/
│   ├── territorio-rsd/main.cpp
│   ├── territorio-rs/main.cpp
│   └── territorio-rs-verify/main.cpp
├── migrations/0001_init.sql
├── web/{index.html,assets/}
├── data/                     # JSON preliminar preservado
├── raw/                      # arquivos oficiais recebidos, sem inferências
├── docs/{API_CONTRATO_V1.json,MATRIZ_MIGRACAO.json}
├── tests/{fixtures/,unit/,integration/,golden/}
├── deploy/systemd/
└── legacy/python/            # scripts históricos, NÃO necessários para executar
```

## 5. Modelo territorial e proveniência

Entidades mínimas: `Municipio {codigo_ibge: string|null, nome, uf}`, `Corede {id, nome}`, `RegiaoFuncional {id, numero}`, `Bioma {id, nome}`, `Fonte {id, orgao, documento, versao, ano_referencia, url, sha256, licenca, acesso_em}`, `DatasetVersao`, `Importacao`, `VinculoMunicipioCorede`, `ClassificacaoBiomaPredominante`, `PresencaMunicipioBioma`.

- Município -> COREDE: 1:1 **para a vigência declarada**, guardar fonte e dataset; não pressupor imutabilidade histórica.
- COREDE -> Região Funcional: N:1 **para a vigência declarada**.
- Município -> bioma predominante: 1:1 quando a base IBGE 2024 foi integralmente validada.
- Município <-> biomas presentes: N:N com origem IBGE 2019, explícita e independente da coluna `predominante`.
- Valores desconhecidos: `null`, nunca vetor vazio ou bioma presumido.
- A agregação de municípios por COREDE não dá uma classificação ecológica a um COREDE inteiro.
- Futuras proporções territoriais exigem geometria, projeção adequada, fonte e método; não estimar percentuais apenas pelos nomes de municípios.

### SQLite sugerido

Tabelas `municipio`, `corede`, `regiao_funcional`, `bioma`, `dataset_version`, `source`, `municipio_corede`, `corede_regiao`, `municipio_bioma_predominante`, `municipio_bioma_presenca`, `import_batch`, `audit_event`.

Todas as tabelas de relacionamento importadas devem armazenar `dataset_version_id` e referência de origem. `codigo_ibge` é **TEXT**. Manter chaves estrangeiras, índices nos campos de consulta e integridade por lote/versionamento. A API responde com snapshot ativo; novo snapshot é validado em staging antes de publicar. Em SQLite WAL, fazer checkpoint/fechamento apropriado antes de qualquer troca de arquivo de banco.

## 6. Migração dos scripts e paridade de comportamento

O mapeamento completo está em `docs/MATRIZ_MIGRACAO.json`.

1. `tools/prepare_seed.py` -> `territorio-rs seed --input tools/municipios_seed.txt --output data/`. Ler 28 blocos, validar 497 nomes únicos e preservar a marca `PRELIMINAR_NAO_HOMOLOGADO`.
2. `tools/integrar_oficiais.py` -> `territorio-rs import --atlas-xlsx raw/coredes.xlsx --ibge-csv raw/biomas_2024.csv [--presenca-xls raw/biomas_2019.xls]`. Suportar também `--presenca-csv` com proveniência da exportação. Nada publicar se alguma validação falhar.
3. `tools/empacotar_catalogo.py` -> `territorio-rs pack --data-dir data --output data/catalogo_territorio_rs_preliminar.json --manifest data/manifest.json`. Serialização JSON UTF-8, determinística, hash em bytes e versão.
4. `tests/test_seed.py` -> testes C++ no CTest e fixtures JSON existentes; preservar e ampliar os cinco tipos de teste.

**A importação oficial não deve usar como verdade os dados preliminares de origem secundária**: usar os arquivos oficiais como autoridade, reconciliando-os com a lista preliminar para detectar inclusão, omissão, grafias divergentes e mudança de atribuição. Emitir `relatorio_conciliacao.json` com toda divergência; não corrigir automaticamente vínculos duvidosos.

## 7. API e UI

Implementar todos os endpoints de `GEMINI_IMPLEMENTACAO.md`, com contrato adicional em `docs/API_CONTRATO_V1.json`.

Exemplos:

```http
GET /v1/health
GET /v1/catalogo
GET /v1/regioes-funcionais/RF7/coredes
GET /v1/coredes/missoes/municipios
GET /v1/municipios?corede_id=missoes
GET /v1/municipios?regiao_funcional_id=RF7
GET /v1/municipios/4303301
GET /v1/biomas/pampa/municipios?criterio=presenca
GET /v1/estatisticas
```

O JSON de resposta usa `data` + `meta` (`schema_version`, `dataset_version`, `status_validacao`, `criterio`, `fonte`, `total`, `limit`, `offset`). Listas paginadas, ordenação estável, limites e códigos HTTP tipados. `GET /v1/health` distingue processo saudável de dataset incompleto.

**Com dataset preliminar, qualquer consulta populacional por bioma deve retornar HTTP 409** com `code=dados_bioma_incompletos`, explicando exatamente o que falta. Não publicar os dois exemplos conhecidos como classificação do estado inteiro. Se for desejável demonstrá-los, criar `/v1/amostras` devidamente identificado. Quando uma importação completa é ativada, a consulta por bioma passa a funcionar normalmente.

Dashboard mínimo: busca por nome/código IBGE; filtros COREDE/RF/bioma conforme completude; painel de fontes e versões; avisos sobre 2019/2024; indicadores de 497/28/9; exportar JSON. HTML acessível, mobile-friendly e arquivos servidos localmente pelo daemon. **Não exige Qt** porque a interface é web.

Padrões: `127.0.0.1:8080`, publicação externa apenas via configuração explícita, CORS fechado, limites de tamanho/paginação, logs estruturados sem dados pessoais, sem update remoto automático no caminho de GET. Sem rotas públicas de mutação: importação e ativação de snapshot exclusivamente pela CLI local.

## 8. Invariantes e critérios de aceite

**Dataset preliminar:** 497 municípios distintos, 28 COREDEs, 9 Regiões Funcionais, duas definições de bioma e estado `PRELIMINAR_NAO_HOMOLOGADO`. Contagens preliminares por RF: 70, 59, 49, 21, 22, 20, 77, 49, 130. Dois registros municipais com bioma exemplo; **495 sem classificação verificada**.

**Dataset oficial importado:** 497 códigos IBGE únicos válidos no RS (sete caracteres começando por `43`); nenhuma município duplicado, nenhuma atribuição conflituosa na vigência, referências oficiais e hashes presentes. Não incluir Lagoa Mirim/Lagoa dos Patos como municípios. IBGE 2024: predominância integral; IBGE 2019: presença integral somente quando importada. Campos desconhecidos seguem nulos.

Casos de regressão: Caibaté e Campina das Missões; verificar os exemplos existentes, mas **não adotá-los como prova de completude**. Validar UTF-8 e acentos (`Caibaté`, `Missões`, `Jacuí`), consultas ignorando acentos, `codigo_ibge` como string, caminhos com espaços e fontes legadas.

- `cmake --build` produz as bibliotecas e três executáveis, de forma reprodutível.
- `ctest --output-on-failure` passa sem invocar Python, rede ou arquivos não fornecidos.
- `territorio-rs seed` reproduz os invariantes do pacote original.
- `territorio-rs validate` detecta relações inválidas, duplicações, catálogo incompleto e diferenças temporais.
- `territorio-rs pack` escreve os JSON e o manifesto SHA-256 deterministicamente para a mesma entrada.
- `territorio-rsd` inicia sem rede e oferece as rotas listadas, inclusive dashboard em `GET /`.
- Banco em WAL, FK habilitadas, consultas somente leitura, importação transacional com rollback em qualquer falha.
- Executa em Fedora Linux x86_64 e compila para aarch64/Raspberry Pi 5.
- O relatório de testes indica o que foi testado de verdade; **não apresentar como implementado o que não passou**.

## 9. Ordem de execução para o Gemini

**Marco A — Fundação:** CMake, toolchain C++26, RAII, tipos e testes de modelo; usar fixtures JSON existentes.  
**Marco B — Persistência:** schema SQLite, migrações, seed, consulta local, auditoria e snapshots.  
**Marco C — API e UI:** servidor, rotas e dashboard funcional com dataset preliminar.  
**Marco D — ETL nativo:** CSV do IBGE, XLSX Atlas, XLS opcional, reconciliação, relatórios, publicação atômica.  
**Marco E — Empacotamento:** exportação JSON/manifest, systemd, documentação, build Fedora e Raspberry Pi.

**Concluir cada marco com código compilável e testes antes de prosseguir.** Se faltar arquivo primário para testar ETL, escrever testes com fixtures sintéticas e registrar que a homologação real está pendente. Não inferir dados ausentes nem baixar fontes sem documentar data, URL, SHA-256 e licenciamento.

## 10. Comandos esperados ao término

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure

./build/bin/territorio-rs init --db ./data/territorio.sqlite
./build/bin/territorio-rs seed --db ./data/territorio.sqlite --data-dir ./data
./build/bin/territorio-rs validate --db ./data/territorio.sqlite
./build/bin/territorio-rsd --db ./data/territorio.sqlite --host 127.0.0.1 --port 8080

# Quando fontes originais estiverem disponíveis:
./build/bin/territorio-rs import --db ./data/territorio.sqlite \
  --atlas-xlsx ./raw/coredes.xlsx \
  --ibge-csv ./raw/biomas_2024.csv \
  --presenca-xls ./raw/biomas_2019.xls
```

**Entrega:** repositório C++26 pronto para compilação e uso, com testes reais, build multiplataforma, bibliotecas reaproveitáveis, servidor REST e dados oficiais somente quando efetivamente validados. Não substituir essa entrega por um novo roteiro teórico.

# TRAMA-RS

Primeira versão operacional do serviço offline-first **Territórios, Regiões, Ambientes, Municípios e Análises**. O catálogo incluído é `PRELIMINAR_NAO_HOMOLOGADO`: preserva 497 municípios, 28 COREDEs e nove Regiões Funcionais, mas somente duas classificações municipais de bioma são exemplos verificados. As outras 495 permanecem `null`.

## Compilar e testar

Requisitos: CMake 3.25+, compilador com C++26, SQLite3 e nlohmann/json 3.11. O fallback C++23 é opt-in com `-DTRAMA_CXX23_FALLBACK=ON`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Executar

```sh
./build/bin/trama init --db ./data/trama.sqlite
./build/bin/trama seed --db ./data/trama.sqlite --data-dir ./data
./build/bin/trama validate --db ./data/trama.sqlite
./build/bin/trama export --db ./data/trama.sqlite --output ./data/export.json
./build/bin/trama-rsd --db ./data/trama.sqlite --host 127.0.0.1 --port 8080
curl -fsS http://127.0.0.1:8080/v1/health
```

Abra `http://127.0.0.1:8080/` para o dashboard. A API é somente leitura e usa envelopes `data`/`meta`. Consultas por bioma retornam HTTP 409 enquanto a cobertura estiver incompleta; os dois registros disponíveis ficam isolados em `/v1/amostras`.

## Componentes

- `trama-core`: tipos e regras de domínio;
- `trama-data`: SQLite, WAL, migração, consultas e exportação;
- `trama-import`: inspeção dos dados de entrada;
- `trama-http`: API REST e arquivos locais do dashboard;
- `trama`, `trama-rsd` e `trama-verify`: CLI, daemon e auditor independente.

Os scripts em `legacy/python/` são históricos e nunca participam do build ou da operação. A importação de fontes oficiais XLSX/CSV ainda falha de forma fechada: ela somente será liberada depois que arquivos primários forem recebidos e fixtures sintéticas cobrirem reconciliação e rollback. Nenhuma informação ausente é inferida.

Documentos originais e precedência estão preservados em [`docs/INICIAR_AQUI_GEMINI.md`](docs/INICIAR_AQUI_GEMINI.md).

## Licença

O código-fonte do TRAMA-RS é distribuído sob a [GNU General Public License, versão 3](LICENSE) (`GPL-3.0-only`).

Os conjuntos de dados e documentos de terceiros preservam sua proveniência, autoria e licença próprias, registradas nos respectivos metadados. A licença do software não altera nem substitui essas condições.

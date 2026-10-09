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
./build/bin/trama import --db ./data/trama.sqlite --ibge-csv ./raw/Bioma_Predominante_por_Municipio_2024.csv
./build/bin/trama-rsd --db ./data/trama.sqlite --host 0.0.0.0 --port 8080
curl -fsS http://127.0.0.1:8080/v1/health
```

O endereço padrão do daemon é `0.0.0.0:8080`, permitindo acesso pela rede local. Na própria máquina, abra `http://127.0.0.1:8080/`; em outro equipamento, use `http://IP_DA_MAQUINA:8080/`. A API é somente leitura e usa envelopes `data`/`meta`. Consultas por bioma retornam HTTP 409 enquanto a cobertura estiver incompleta; os dois registros disponíveis ficam isolados em `/v1/amostras`.

Se o Fedora estiver com o `firewalld` ativo, libere a porta na zona da rede confiável conforme a política do ambiente. Para restringir novamente ao acesso local, informe `--host 127.0.0.1`.

### Mapas

O serviço fornece camadas GeoJSON simplificadas em EPSG:4326:

```text
/v1/mapa/municipios.geojson
/v1/mapa/coredes.geojson
/v1/mapa/regioes-funcionais.geojson
/v1/mapa/biomas.geojson
```

Municípios e biomas vêm das malhas oficiais IBGE 2025. COREDEs e Regiões Funcionais são uniões dos municípios conforme a regionalização preliminar registrada e, portanto, mantêm o estado `PRELIMINAR_NAO_HOMOLOGADO`. Consulte [data/map/README.md](data/map/README.md) para fontes, hashes e método.

## Componentes

- `trama-core`: tipos e regras de domínio;
- `trama-data`: SQLite, WAL, migração, consultas e exportação;
- `trama-import`: inspeção dos dados de entrada;
- `trama-http`: API REST e arquivos locais do dashboard;
- `trama`, `trama-rsd` e `trama-verify`: CLI, daemon e auditor independente.

Os scripts em `legacy/python/` são históricos e nunca participam do build ou da operação. O importador C++ aceita o CSV oficial de bioma predominante do IBGE 2024, reconhece delimitador e cabeçalho, concilia nomes sem acentos e exige os 497 geocódigos únicos do RS antes de alterar o banco. A atribuição é atômica: qualquer ausência, duplicação ou bioma desconhecido cancela toda a operação. A relação multivalorada de presença de biomas (IBGE 2019) continua indisponível até que seu importador próprio seja implementado; ela não é inferida a partir do bioma predominante.

Documentos originais e precedência estão preservados em [`docs/INICIAR_AQUI_GEMINI.md`](docs/INICIAR_AQUI_GEMINI.md).

## Licença

O código-fonte do TRAMA-RS é distribuído sob a [GNU General Public License, versão 3](LICENSE) (`GPL-3.0-only`).

Os conjuntos de dados e documentos de terceiros preservam sua proveniência, autoria e licença próprias, registradas nos respectivos metadados. A licença do software não altera nem substitui essas condições.

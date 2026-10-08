# TRAMA-RS-003 — Instrução de implementação para Gemini

**Nome público:** TRAMA-RS  
**Expansão:** Territórios, Regiões, Ambientes, Municípios e Análises  
**Repositório:** `trama-rs`  
**Foco:** serviço independente de referência territorial e ambiental do Rio Grande do Sul  
**Lema:** Sempre pronto. Sempre incompleto.  
**Estado:** especificação para desenvolvimento; NÃO é código C++ compilável nem dataset integral homologado.

## Ordem de leitura e precedência

1. **Este arquivo**: define nome, produto e critérios de entrega; prevalece sobre os documentos anteriores em questões de nomenclatura e apresentação.
2. `especificacao_anterior/GEMINI_CPP26_CONVERSAO.md`: requisitos técnicos normativos e marcos da implementação.
3. `especificacao_anterior/GEMINI_IMPLEMENTACAO.md`: contexto e constituição anterior, desde que não conflite com os dois documentos acima.
4. `especificacao_anterior/docs/API_CONTRATO_V1.json`, `MATRIZ_MIGRACAO.json` e `data/*.json`: contratos, migração e dados preliminares.

## Instrução executiva

Implemente agora o projeto real em **C++26**, com bibliotecas, aplicativos, testes, build e dashboard local. Não devolva somente outra especificação ou pseudocódigo. Use CMake e bibliotecas versionadas, SQLite3 WAL, API HTTP REST, JSON UTF-8 e CLI. Nenhuma etapa operacional deve depender de Python (scripts antigos são exclusivamente arquivos históricos para auditoria). Os dados continuam externos ao código-fonte; não criar listas fixas em C++.

Adote o nome **TRAMA-RS** em interface, documentação e distribuição. Sugestões de nomes internos:

- `trama-core`, `trama-data`, `trama-import`, `trama-http` (bibliotecas)
- `trama-rsd` (daemon HTTP e dashboard)
- `trama` (CLI com `init`, `seed`, `import`, `validate`, `export`, `pack`)
- `trama-verify` (auditoria e validação)
- `trama-tests` (CTest)
- Namespace C++: `trama`

Os nomes `territorio-*` nas especificações anteriores são **referências históricas**, não produtos adicionais. Atualize os exemplos de linha de comando, README, CMake, arquivos de implantação e nomes de executáveis para TRAMA. Preserve o significado sem quebrar o contrato REST `/v1/...`, salvo correção documentada.

## Domínio e confiabilidade dos dados

O sistema integra **497 municípios, 28 COREDEs, nove Regiões Funcionais e os biomas Pampa/Mata Atlântica**. A relação município–COREDE e COREDE–Região Funcional é temporal e requer identificação da fonte. Modelo por bioma contém **duas relações distintas**: (a) bioma predominante por município, (b) presença de um ou mais biomas por município. Não deduzir uma a partir da outra.

**O conjunto incluído é preliminar e não homologado**: há apenas **dois exemplos municipais com bioma preenchido; 495 estão pendentes**. Não completar campos usando inferência ou proximidade geográfica; manter valores desconhecidos como `null` e indicar cobertura real nos endpoints. Priorizar importação posterior de fontes oficiais estaduais e IBGE, com versão, URL, acesso, licença, SHA-256 e relatórios de reconciliação. Não afirmar que importações oficiais foram executadas se os arquivos não estiverem disponíveis.

A primeira versão deve responder com o catálogo preliminar corretamente identificado, sem transformar desconhecimento em falsa certeza.

## Marcos de entrega com evidência

1. Fundação: repositório `trama-rs`, CMake, C++26, entidades e testes.
2. Persistência: migrações SQLite WAL, transações e CLI de seed/validate.
3. API REST: endpoints do contrato, JSON, paginação, erros, health e dashboard funcional.
4. Importadores C++: CSV, XLSX, XLS opcional documentado, conciliação e snapshot validado.
5. Distribuição: build em Fedora x86_64 e, quando disponível, Raspberry Pi 5 aarch64, documentação e serviços.

Para cada marco, **entregue arquivos reais e compiláveis**, relate exatamente os comandos executados e seus resultados, indique claramente os testes não executados e justifique bloqueios. Não declare como concluído um marco que apenas está descrito.

### Comandos de aceitação desejados

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/bin/trama init --db ./data/trama.sqlite
./build/bin/trama seed --db ./data/trama.sqlite --data-dir ./data
./build/bin/trama validate --db ./data/trama.sqlite
./build/bin/trama-rsd --db ./data/trama.sqlite --host 127.0.0.1 --port 8080
curl -fsS http://127.0.0.1:8080/v1/health
```

Se o ambiente não suportar `-std=c++26`, registrar objetivamente e oferecer modo C++23 **apenas como fallback explícito**, sem afirmar ter compilado em C++26. Evitar dependências de funcionalidades ainda não suportadas pela toolchain.

## Escopo deste envio

Este ZIP já inclui dados JSON, contratos, versão anterior de especificação e scripts históricos. **Não precisa enviar outro ZIP para começar.** Fontes oficiais brutas (XLS/XLSX/CSV) poderão ser acrescentadas quando disponíveis; sua ausência não impede iniciar a fundação e a API com a classificação preliminar explicitada.

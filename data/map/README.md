# Camadas cartográficas

GeoJSON em EPSG:4326 simplificado para visualização web. A simplificação reduz vértices, não altera a classificação territorial e não deve ser usada para medições de área.

## Fontes

- Municípios: IBGE, Malha Municipal Digital 2025, `RS_Municipios_2025.zip`.
  - URL: `https://geoftp.ibge.gov.br/organizacao_do_territorio/malhas_territoriais/malhas_municipais/municipio_2025/UFs/RS/RS_Municipios_2025.zip`
  - SHA-256: `d70d47ccd1c2722e78a6dcc7fa4a00715679684785aa137ed2bf11999de28513`
  - CRS original: SIRGAS 2000, EPSG:4674.
  - As áreas operacionais Lagoa Mirim e Lagoa dos Patos foram excluídas. Restaram 497 municípios.
- Biomas: IBGE, Biomas e Sistema Costeiro-Marinho do Brasil 1:250.000, versão 2025.
  - URL: `https://geoftp.ibge.gov.br/informacoes_ambientais/estudos_ambientais/biomas/vetores/2025_Biomas-e-Sistema-Costeiro-Marinho-do-Brasil-1-250000_shp.zip`
  - SHA-256: `247b15c427070805044ebe02012f15552850d535fbfecaddb359875cdda1ea8f`
  - CRS original: SIRGAS 2000, EPSG:4674.
  - Recorte espacial aplicado pela união dos 497 polígonos municipais do RS.

COREDEs e Regiões Funcionais são geometrias derivadas por união dos polígonos municipais conforme os vínculos do catálogo preliminar TRAMA-RS. Portanto, preservam o estado `PRELIMINAR_NAO_HOMOLOGADO` até a homologação da regionalização.

Arquivos brutos não são necessários em execução e não estão incluídos no repositório. A preparação foi realizada com GDAL 3.12 (`ogr2ogr`) e serialização GeoJSON RFC 7946.

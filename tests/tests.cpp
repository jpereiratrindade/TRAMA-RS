#include "trama/database.hpp"
#include "trama/domain.hpp"
#include "trama/importer.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

namespace {void check(bool x,const char* m){if(!x)throw std::runtime_error(m);}}
int main(){try{
  check(trama::normalize_for_search("Caibaté") == "caibate","normalização UTF-8");
  check(trama::normalize_for_search("Missões") == "missoes","normalização Missões");
  check(trama::valid_ibge_code("4303301"),"código IBGE válido");check(!trama::valid_ibge_code("43033"),"código IBGE inválido");
  auto source=std::filesystem::path(TRAMA_SOURCE_DIR);auto inspected=trama::inspect_seed(source/"data");check(inspected["apto_para_seed"],"fixture seed");
  auto geo=[&](const char* name){std::ifstream f(source/"data/map"/name);check(static_cast<bool>(f),"camada GeoJSON disponível");return nlohmann::json::parse(f);};
  auto municipios_geo=geo("municipios.geojson"),coredes_geo=geo("coredes.geojson"),rf_geo=geo("regioes-funcionais.geojson"),biomas_geo=geo("biomas.geojson");
  check(municipios_geo["features"].size()==497 && !municipios_geo["features"][0]["geometry"].is_null(),"497 geometrias municipais");
  check(coredes_geo["features"].size()==28 && !coredes_geo["features"][0]["geometry"].is_null(),"28 geometrias COREDE");
  check(rf_geo["features"].size()==9 && !rf_geo["features"][0]["geometry"].is_null(),"9 geometrias RF");
  check(biomas_geo["features"].size()==2 && !biomas_geo["features"][0]["geometry"].is_null(),"2 geometrias de bioma");
  auto dbpath=std::filesystem::temp_directory_path()/("trama-test-"+std::to_string(getpid())+".sqlite");
  auto csvpath=std::filesystem::temp_directory_path()/("trama-biomas-"+std::to_string(getpid())+".csv");
  {trama::Database db(dbpath);db.seed(source/"data");auto v=db.validate();check(v["ok"],"integridade");check(v["contagens"]["municipios"]==497,"497 municípios");check(v["contagens"]["coredes"]==28,"28 COREDEs");check(v["contagens"]["regioes_funcionais"]==9,"9 RFs");check(v["contagens"]["bioma_pendente"]==495,"495 biomas pendentes");check(!db.predominant_biomes_complete(),"biomas preliminares incompletos");check(db.municipalities("caibate","","",100,0).size()==1,"busca sem acento");check(db.municipality("4303301")["nome"]=="Caibaté","consulta por IBGE");check(db.samples().size()==2,"somente duas amostras");sqlite3_stmt* s{};sqlite3_prepare_v2(db.handle(),"PRAGMA journal_mode",-1,&s,nullptr);sqlite3_step(s);check(std::string(reinterpret_cast<const char*>(sqlite3_column_text(s,0)))=="wal","WAL");sqlite3_finalize(s);
    std::ifstream mf(source/"data/municipios_coredes_preliminar.json");auto municipalities=nlohmann::json::parse(mf)["municipios"];std::ofstream csv(csvpath);csv<<"Código IBGE;Nome do Município;UF;Bioma Predominante\n";int i=0;for(const auto& m:municipalities){auto code=std::to_string(4390000+i++);csv<<code<<';'<<m["nome"].get<std::string>()<<";RS;"<<(i%2?"Pampa":"Mata Atlântica")<<'\n';}csv.close();
    auto imported=trama::import_ibge_predominant_biomes(db,csvpath);check(imported["contagens"]["bioma_pendente"]==0,"497 biomas atribuídos");check(db.predominant_biomes_complete(),"cobertura predominante completa");auto pampa=db.municipalities("","","",500,0,"pampa","predominante");check(!pampa.empty()&&pampa.size()<497,"filtro por bioma predominante");check(db.municipality("4390000")["classificacao_bioma_status"]=="predominante_ibge_2024_verificado","proveniência da atribuição");
  }
  std::filesystem::remove(dbpath);std::filesystem::remove(dbpath.string()+"-wal");std::filesystem::remove(dbpath.string()+"-shm");
  std::filesystem::remove(csvpath);
  std::cout<<"todos os testes passaram\n";return 0;
}catch(const std::exception&e){std::cerr<<"falha: "<<e.what()<<'\n';return 1;}}

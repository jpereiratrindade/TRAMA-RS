#include "trama/database.hpp"
#include "trama/domain.hpp"
#include "trama/importer.hpp"
#include <fstream>
#include <stdexcept>
#include <unordered_map>

namespace trama {
using json=nlohmann::json;
namespace {
struct Statement {
  sqlite3_stmt* p{};
  Statement(sqlite3* db, const std::string& sql) { if(sqlite3_prepare_v2(db,sql.c_str(),-1,&p,nullptr)!=SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(db)); }
  ~Statement(){ sqlite3_finalize(p); }
};
json read_json(const std::filesystem::path& p) { std::ifstream f(p); if(!f) throw std::runtime_error("arquivo não encontrado: "+p.string()); return json::parse(f); }
void bind_value(sqlite3_stmt* s,int n,const std::string& v){ auto rc=sqlite3_bind_text(s,n,v.c_str(),-1,SQLITE_TRANSIENT);if(rc!=SQLITE_OK)throw std::runtime_error("sqlite bind falhou: "+std::to_string(rc)); }
std::string col(sqlite3_stmt* s,int n){ auto p=sqlite3_column_text(s,n); return p?reinterpret_cast<const char*>(p):""; }
json municipio_row(sqlite3_stmt* s) {
  json j={{"nome",col(s,1)},{"uf",col(s,2)},{"corede_id",col(s,3)},{"regiao_funcional_id",col(s,4)},
          {"classificacao_bioma_status",col(s,7)}};
  j["codigo_ibge"]=sqlite3_column_type(s,0)==SQLITE_NULL?json(nullptr):json(col(s,0));
  j["bioma_predominante_id"]=sqlite3_column_type(s,5)==SQLITE_NULL?json(nullptr):json(col(s,5));
  j["biomas_presentes_ids"]=sqlite3_column_type(s,6)==SQLITE_NULL?json(nullptr):json::parse(col(s,6));
  return j;
}
}

Database::Database(const std::filesystem::path& path,bool ro) {
  if(!ro && path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
  int flags=ro?SQLITE_OPEN_READONLY:(SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
  if(sqlite3_open_v2(path.string().c_str(),&db_,flags,nullptr)!=SQLITE_OK) { auto e=std::string(sqlite3_errmsg(db_)); sqlite3_close(db_); db_=nullptr; throw std::runtime_error(e); }
  sqlite3_busy_timeout(db_,5000); exec("PRAGMA foreign_keys=ON");
  if(!ro) exec("PRAGMA journal_mode=WAL");
}
Database::~Database(){ if(db_) sqlite3_close(db_); }
void Database::exec(const std::string& sql) const { char* e=nullptr; if(sqlite3_exec(db_,sql.c_str(),nullptr,nullptr,&e)!=SQLITE_OK){std::string m=e?e:"sqlite error";sqlite3_free(e);throw std::runtime_error(m);} }
void Database::migrate(){
  std::ifstream f(std::filesystem::path(TRAMA_SOURCE_DIR)/"migrations/0001_init.sql");
  if(!f) throw std::runtime_error("migração 0001_init.sql não encontrada");
  exec(std::string(std::istreambuf_iterator<char>(f),{}));
  exec("INSERT OR REPLACE INTO metadata VALUES('schema_version','1.0.0')");
}
void Database::seed(const std::filesystem::path& dir){
  migrate(); auto regional=read_json(dir/"coredes_regioes_funcionais.json"); auto mun=read_json(dir/"municipios_coredes_preliminar.json"); auto bio=read_json(dir/"biomas_rs.json"); auto fontes=read_json(dir/"catalogo_fontes.json");
  if(mun.at("municipios").size()!=497 || regional.at("coredes").size()!=28 || regional.at("regioes_funcionais").size()!=9) throw std::runtime_error("fixture não satisfaz invariantes 497/28/9");
  exec("BEGIN IMMEDIATE");
  try {
    exec("DELETE FROM municipio;DELETE FROM corede;DELETE FROM regiao_funcional;DELETE FROM bioma;DELETE FROM source;");
    Statement rf(db_,"INSERT INTO regiao_funcional(id,numero) VALUES(?,?)");
    for(auto& x:regional["regioes_funcionais"]){bind_value(rf.p,1,x["id"].get<std::string>());sqlite3_bind_int(rf.p,2,x["numero"].get<int>());if(sqlite3_step(rf.p)!=SQLITE_DONE)throw std::runtime_error(sqlite3_errmsg(db_));sqlite3_reset(rf.p);}
    Statement co(db_,"INSERT INTO corede(id,nome,regiao_funcional_id) VALUES(?,?,?)");
    for(auto& x:regional["coredes"]){bind_value(co.p,1,x["id"].get<std::string>());bind_value(co.p,2,x["nome"].get<std::string>());bind_value(co.p,3,x["regiao_funcional_id"].get<std::string>());if(sqlite3_step(co.p)!=SQLITE_DONE){auto expanded=sqlite3_expanded_sql(co.p);std::string detail=expanded?expanded:"";sqlite3_free(expanded);throw std::runtime_error(std::string(sqlite3_errmsg(db_))+": "+detail);}sqlite3_reset(co.p);}
    Statement bi(db_,"INSERT INTO bioma(id,nome) VALUES(?,?)");
    for(auto& x:bio["biomas"]){bind_value(bi.p,1,x["id"].get<std::string>());bind_value(bi.p,2,x["nome"].get<std::string>());if(sqlite3_step(bi.p)!=SQLITE_DONE)throw std::runtime_error(sqlite3_errmsg(db_));sqlite3_reset(bi.p);}
    Statement ms(db_,"INSERT INTO municipio(codigo_ibge,nome,nome_busca,uf,corede_id,regiao_funcional_id,bioma_predominante_id,biomas_presentes_json,classificacao_bioma_status) VALUES(?,?,?,?,?,?,?,?,?)");
    for(auto& x:mun["municipios"]){
      if(x["codigo_ibge"].is_null())sqlite3_bind_null(ms.p,1);else bind_value(ms.p,1,x["codigo_ibge"].get<std::string>()); bind_value(ms.p,2,x["nome"].get<std::string>());bind_value(ms.p,3,normalize_for_search(x["nome"].get<std::string>()));bind_value(ms.p,4,x["uf"].get<std::string>());bind_value(ms.p,5,x["corede_id"].get<std::string>());bind_value(ms.p,6,x["regiao_funcional_id"].get<std::string>());
      if(x["bioma_predominante_id"].is_null())sqlite3_bind_null(ms.p,7);else bind_value(ms.p,7,x["bioma_predominante_id"].get<std::string>());
      if(x["biomas_presentes_ids"].is_null())sqlite3_bind_null(ms.p,8);else bind_value(ms.p,8,x["biomas_presentes_ids"].dump());bind_value(ms.p,9,x["classificacao_bioma_status"].get<std::string>());
      if(sqlite3_step(ms.p)!=SQLITE_DONE)throw std::runtime_error(sqlite3_errmsg(db_));sqlite3_reset(ms.p);sqlite3_clear_bindings(ms.p);
    }
    Statement so(db_,"INSERT INTO source(payload_json) VALUES(?)"); for(auto& x:fontes["fontes"]){bind_value(so.p,1,x.dump());if(sqlite3_step(so.p)!=SQLITE_DONE)throw std::runtime_error(sqlite3_errmsg(db_));sqlite3_reset(so.p);}
    exec("INSERT OR REPLACE INTO metadata VALUES('dataset_version','0.1.0');INSERT OR REPLACE INTO metadata VALUES('status_validacao','PRELIMINAR_NAO_HOMOLOGADO');INSERT INTO audit_event(event) VALUES('seed_preliminar');COMMIT");
  } catch(...) { exec("ROLLBACK"); throw; }
}
json Database::validate() const {
  auto scalar=[&](const char* q){Statement s(db_,q);sqlite3_step(s.p);return sqlite3_column_int(s.p,0);};
  int m=scalar("SELECT count(*) FROM municipio"),c=scalar("SELECT count(*) FROM corede"),r=scalar("SELECT count(*) FROM regiao_funcional"),known=scalar("SELECT count(*) FROM municipio WHERE bioma_predominante_id IS NOT NULL");
  json errors=json::array(); if(m!=497)errors.push_back("total_municipios_deve_ser_497");if(c!=28)errors.push_back("total_coredes_deve_ser_28");if(r!=9)errors.push_back("total_regioes_deve_ser_9");
  return {{"ok",errors.empty()},{"status",preliminary_status},{"contagens",{{"municipios",m},{"coredes",c},{"regioes_funcionais",r},{"bioma_predominante_conhecido",known},{"bioma_pendente",m-known}}},{"erros",errors}};
}
json Database::catalog() const { json sources=json::array();Statement s(db_,"SELECT payload_json FROM source ORDER BY id");while(sqlite3_step(s.p)==SQLITE_ROW)sources.push_back(json::parse(col(s.p,0)));return {{"nome","TRAMA-RS"},{"schema_version",schema_version},{"dataset_version","0.1.0"},{"status_validacao",preliminary_status},{"fontes",sources}}; }
json Database::regions() const {json a=json::array();Statement s(db_,"SELECT id,numero FROM regiao_funcional ORDER BY numero");while(sqlite3_step(s.p)==SQLITE_ROW)a.push_back({{"id",col(s.p,0)},{"numero",sqlite3_column_int(s.p,1)}});return a;}
json Database::coredes(const std::string& region) const {json a=json::array();Statement s(db_,"SELECT id,nome,regiao_funcional_id FROM corede WHERE (?='' OR regiao_funcional_id=?) ORDER BY nome");bind_value(s.p,1,region);bind_value(s.p,2,region);while(sqlite3_step(s.p)==SQLITE_ROW)a.push_back({{"id",col(s.p,0)},{"nome",col(s.p,1)},{"regiao_funcional_id",col(s.p,2)}});return a;}
json Database::municipalities(const std::string& q,const std::string& corede,const std::string& region,int limit,int offset,const std::string& biome,const std::string& criterion) const {json a=json::array();std::string biome_clause;if(!biome.empty()){if(criterion=="predominante")biome_clause=" AND bioma_predominante_id=?";else if(criterion=="presenca")biome_clause=" AND EXISTS (SELECT 1 FROM json_each(biomas_presentes_json) WHERE value=?)";else throw std::invalid_argument("critério de bioma inválido");}Statement s(db_,"SELECT codigo_ibge,nome,uf,corede_id,regiao_funcional_id,bioma_predominante_id,biomas_presentes_json,classificacao_bioma_status FROM municipio WHERE (?='' OR nome_busca LIKE '%'||?||'%') AND (?='' OR corede_id=?) AND (?='' OR regiao_funcional_id=?)"+biome_clause+" ORDER BY nome LIMIT ? OFFSET ?");auto nq=normalize_for_search(q);int n=1;bind_value(s.p,n++,nq);bind_value(s.p,n++,nq);bind_value(s.p,n++,corede);bind_value(s.p,n++,corede);bind_value(s.p,n++,region);bind_value(s.p,n++,region);if(!biome.empty())bind_value(s.p,n++,biome);sqlite3_bind_int(s.p,n++,limit);sqlite3_bind_int(s.p,n,offset);while(sqlite3_step(s.p)==SQLITE_ROW)a.push_back(municipio_row(s.p));return a;}
json Database::municipality(const std::string& ibge) const {Statement s(db_,"SELECT codigo_ibge,nome,uf,corede_id,regiao_funcional_id,bioma_predominante_id,biomas_presentes_json,classificacao_bioma_status FROM municipio WHERE codigo_ibge=?");bind_value(s.p,1,ibge);return sqlite3_step(s.p)==SQLITE_ROW?municipio_row(s.p):json(nullptr);}
json Database::biomes() const {json a=json::array();Statement s(db_,"SELECT id,nome FROM bioma ORDER BY id");while(sqlite3_step(s.p)==SQLITE_ROW)a.push_back({{"id",col(s.p,0)},{"nome",col(s.p,1)}});return a;}
json Database::samples() const {json a=json::array();Statement s(db_,"SELECT codigo_ibge,nome,uf,corede_id,regiao_funcional_id,bioma_predominante_id,biomas_presentes_json,classificacao_bioma_status FROM municipio WHERE bioma_predominante_id IS NOT NULL ORDER BY nome");while(sqlite3_step(s.p)==SQLITE_ROW)a.push_back(municipio_row(s.p));return a;}
json Database::statistics() const {return validate()["contagens"];}
bool Database::predominant_biomes_complete() const {Statement s(db_,"SELECT count(*)=497 AND count(bioma_predominante_id)=497 AND count(codigo_ibge)=497 FROM municipio");return sqlite3_step(s.p)==SQLITE_ROW&&sqlite3_column_int(s.p,0)==1;}
void Database::assign_predominant_biomes(const std::vector<BiomeAssignment>& assignments,const std::filesystem::path& source){
  if(assignments.size()!=497)throw std::runtime_error("a importação exige exatamente 497 atribuições");
  std::unordered_map<std::string,BiomeAssignment> by_name;for(const auto& x:assignments){auto key=normalize_for_search(x.municipio);if(key.empty()||!by_name.emplace(key,x).second)throw std::runtime_error("IBGE 2024: nome municipal ausente ou duplicado: "+x.municipio);}
  Statement current(db_,"SELECT nome FROM municipio ORDER BY nome");std::vector<std::pair<std::string,BiomeAssignment>> reconciled;while(sqlite3_step(current.p)==SQLITE_ROW){auto name=col(current.p,0);auto it=by_name.find(normalize_for_search(name));if(it==by_name.end())throw std::runtime_error("IBGE 2024: município não conciliado: "+name);reconciled.emplace_back(name,it->second);}
  if(reconciled.size()!=497)throw std::runtime_error("catálogo local não contém 497 municípios");
  exec("BEGIN IMMEDIATE");try{exec("UPDATE municipio SET codigo_ibge=NULL");Statement update(db_,"UPDATE municipio SET codigo_ibge=?,bioma_predominante_id=?,classificacao_bioma_status='predominante_ibge_2024_verificado' WHERE nome=?");for(const auto& [name,x]:reconciled){bind_value(update.p,1,x.codigo_ibge);bind_value(update.p,2,x.bioma_id);bind_value(update.p,3,name);if(sqlite3_step(update.p)!=SQLITE_DONE||sqlite3_changes(db_)!=1)throw std::runtime_error("falha ao atribuir bioma a "+name);sqlite3_reset(update.p);sqlite3_clear_bindings(update.p);}Statement src(db_,"INSERT INTO source(payload_json) VALUES(?)");json payload={{"tipo","bioma_predominante_ibge"},{"ano_referencia",2024},{"arquivo",source.string()}};bind_value(src.p,1,payload.dump());if(sqlite3_step(src.p)!=SQLITE_DONE)throw std::runtime_error(sqlite3_errmsg(db_));exec("INSERT OR REPLACE INTO metadata VALUES('dataset_version','0.2.0');INSERT OR REPLACE INTO metadata VALUES('bioma_predominante_status','COMPLETO_IBGE_2024');INSERT INTO audit_event(event) VALUES('import_bioma_predominante_ibge_2024');COMMIT");}catch(...){exec("ROLLBACK");throw;}
}
void Database::export_json(const std::filesystem::path& out) const {json j={{"schema_version",schema_version},{"dataset_version","0.1.0"},{"status",preliminary_status},{"municipios",municipalities("","","",500,0)},{"regionalizacao",{{"regioes_funcionais",regions()},{"coredes",coredes()}}},{"biomas",biomes()},{"proveniencia",catalog()["fontes"]}};std::ofstream f(out);if(!f)throw std::runtime_error("não foi possível escrever "+out.string());f<<j.dump(2)<<'\n';}
}

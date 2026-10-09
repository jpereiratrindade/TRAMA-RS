#include "trama/importer.hpp"
#include "trama/database.hpp"
#include "trama/domain.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
namespace trama {
nlohmann::json inspect_seed(const std::filesystem::path& d){std::ifstream f(d/"municipios_coredes_preliminar.json");if(!f)throw std::runtime_error("dataset de municípios ausente");auto j=nlohmann::json::parse(f);return {{"municipios",j.at("municipios").size()},{"estado",j.at("estado")},{"apto_para_seed",j.at("municipios").size()==497}};}

namespace {
std::vector<std::string> csv_row(const std::string& line, char delimiter) {
  std::vector<std::string> row; std::string field; bool quoted=false;
  for(std::size_t i=0;i<line.size();++i){char c=line[i];if(c=='"'){if(quoted&&i+1<line.size()&&line[i+1]=='"'){field+='"';++i;}else quoted=!quoted;}else if(c==delimiter&&!quoted){row.push_back(field);field.clear();}else if(c!='\r')field+=c;}
  if(quoted)throw std::runtime_error("IBGE 2024: aspas não fechadas no CSV");row.push_back(field);return row;
}
std::string digits(std::string s){s.erase(std::remove_if(s.begin(),s.end(),[](unsigned char c){return !std::isdigit(c);}),s.end());return s;}
bool contains(const std::string& value,const std::string& needle){return value.find(needle)!=std::string::npos;}
bool valid_utf8(const std::string& s){for(std::size_t i=0;i<s.size();){auto c=static_cast<unsigned char>(s[i]);std::size_t n=c<0x80?1:(c&0xe0)==0xc0?2:(c&0xf0)==0xe0?3:(c&0xf8)==0xf0?4:0;if(n==0||i+n>s.size())return false;for(std::size_t j=1;j<n;++j)if((static_cast<unsigned char>(s[i+j])&0xc0)!=0x80)return false;i+=n;}return true;}
std::string latin1_to_utf8(const std::string& s){std::string out;for(unsigned char c:s){if(c<0x80)out+=static_cast<char>(c);else{out+=static_cast<char>(0xc0|(c>>6));out+=static_cast<char>(0x80|(c&0x3f));}}return out;}
}

std::vector<BiomeAssignment> parse_ibge_predominant_biomes(const std::filesystem::path& path){
  std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("IBGE 2024: arquivo CSV não encontrado: "+path.string());std::string raw(std::istreambuf_iterator<char>(file),{});if(!valid_utf8(raw))raw=latin1_to_utf8(raw);std::istringstream in(raw);
  std::vector<std::string> lines;std::string line;while(std::getline(in,line))if(line.find_first_not_of(" \t\r")!=std::string::npos)lines.push_back(line);
  if(lines.empty())throw std::runtime_error("IBGE 2024: CSV vazio");
  char delimiter=';';std::size_t best=0;for(char candidate:{';',',','\t'}){std::size_t score=0;for(std::size_t i=0;i<std::min<std::size_t>(30,lines.size());++i)score+=std::count(lines[i].begin(),lines[i].end(),candidate);if(score>best){best=score;delimiter=candidate;}}
  std::size_t header_line=lines.size();std::vector<std::string> headers;
  for(std::size_t i=0;i<lines.size();++i){auto row=csv_row(lines[i],delimiter);std::string joined;for(auto& cell:row)joined+=' '+normalize_for_search(cell);if((contains(joined,"geocodigo")||contains(joined,"codigo do municipio")||contains(joined,"codmun")||contains(joined,"codigo ibge"))&&contains(joined,"bioma")){header_line=i;headers=std::move(row);break;}}
  if(header_line==lines.size())throw std::runtime_error("IBGE 2024: cabeçalho não identificado");
  int code=-1,name=-1,biome=-1,uf=-1;for(std::size_t i=0;i<headers.size();++i){auto h=normalize_for_search(headers[i]);if(code<0&&(contains(h,"geocodigo")||contains(h,"codigo ibge")||contains(h,"codmun")||contains(h,"codigo do municipio")))code=i;if(name<0&&(contains(h,"nome do municipio")||h=="municipio"||contains(h,"nome_mun")))name=i;if(biome<0&&contains(h,"bioma"))biome=i;if(uf<0&&(h=="uf"||contains(h,"sigla da uf")))uf=i;}
  if(code<0||name<0||biome<0)throw std::runtime_error("IBGE 2024: colunas de código, município e bioma são obrigatórias");
  std::vector<BiomeAssignment> result;std::unordered_set<std::string> seen;
  for(std::size_t i=header_line+1;i<lines.size();++i){auto row=csv_row(lines[i],delimiter);auto needed=std::max({code,name,biome,uf});if(static_cast<int>(row.size())<=needed)continue;auto ibge=digits(row[code]);if(!valid_ibge_code(ibge)||ibge=="4300001"||ibge=="4300002")continue;if(uf>=0){auto state=normalize_for_search(row[uf]);if(state!="rs"&&state!="rio grande do sul")continue;}auto b=normalize_for_search(row[biome]);std::string id;if(contains(b,"pampa"))id="pampa";else if(contains(b,"mata atlantica"))id="mata-atlantica";else throw std::runtime_error("IBGE 2024: bioma não reconhecido para "+ibge+": "+row[biome]);if(!seen.insert(ibge).second)throw std::runtime_error("IBGE 2024: geocódigo duplicado: "+ibge);result.push_back({ibge,row[name],id});}
  if(result.size()!=497)throw std::runtime_error("IBGE 2024: extração incompleta ("+std::to_string(result.size())+"/497 municípios)");return result;
}
nlohmann::json import_ibge_predominant_biomes(Database& db,const std::filesystem::path& csv){auto assignments=parse_ibge_predominant_biomes(csv);db.assign_predominant_biomes(assignments,csv);return db.validate();}
}

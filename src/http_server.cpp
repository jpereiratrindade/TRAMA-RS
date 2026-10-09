#include "trama/http_server.hpp"
#include "trama/database.hpp"
#include "trama/domain.hpp"
#include <arpa/inet.h>
#include <csignal>
#include <fstream>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace trama {
using json=nlohmann::json;
namespace {
std::string decode(std::string s){for(size_t p=0;(p=s.find('+',p))!=std::string::npos;)s[p++]=' ';for(size_t p=0;(p=s.find('%',p))!=std::string::npos&&p+2<s.size();){try{s.replace(p,3,1,static_cast<char>(std::stoi(s.substr(p+1,2),nullptr,16)));}catch(...){++p;}}return s;}
std::map<std::string,std::string> query(const std::string& raw){std::map<std::string,std::string> q;std::stringstream ss(raw);std::string x;while(std::getline(ss,x,'&')){auto p=x.find('=');q[decode(x.substr(0,p))]=p==std::string::npos?"":decode(x.substr(p+1));}return q;}
json meta(const json& data,const std::string& criterion=""){return {{"schema_version",schema_version},{"dataset_version","0.1.0"},{"status_validacao",preliminary_status},{"criterio",criterion.empty()?json(nullptr):json(criterion)},{"fonte",nullptr},{"total",data.is_array()?json(data.size()):json(nullptr)}};}
std::string file(const std::filesystem::path& p){std::ifstream f(p);if(!f)return {};return {std::istreambuf_iterator<char>(f),{}};}
struct Reply{int status=200;std::string type="application/json; charset=utf-8";std::string body;};
bool send_all(int socket,const std::string& data){std::size_t sent=0;while(sent<data.size()){auto n=send(socket,data.data()+sent,data.size()-sent,0);if(n<=0)return false;sent+=static_cast<std::size_t>(n);}return true;}
Reply api(Database& db,std::string target){
  auto qm=target.find('?');auto path=target.substr(0,qm);auto q=query(qm==std::string::npos?"":target.substr(qm+1));
  auto ok=[&](json d,std::string c=""){return Reply{200,"application/json; charset=utf-8",json{{"data",d},{"meta",meta(d,c)}}.dump()};};
  auto err=[](int status,std::string code,std::string message){return Reply{status,"application/json; charset=utf-8",json{{"error",{{"code",code},{"message",message}}},{"meta",meta(nullptr)}}.dump()};};
  if(path=="/")return {200,"text/html; charset=utf-8",file(std::filesystem::path(TRAMA_SOURCE_DIR)/"web/index.html")};
  if(path=="/v1/health")return ok({{"service","TRAMA-RS"},{"version",version},{"healthy",true},{"bioma_predominante_completo",db.predominant_biomes_complete()},{"status",preliminary_status}});
  if(path=="/v1/catalogo")return ok(db.catalog());
  if(path=="/v1/regioes-funcionais")return ok(db.regions());
  if(path=="/v1/coredes")return ok(db.coredes());
  if(path=="/v1/biomas")return ok(db.biomes());
  if(path=="/v1/estatisticas")return ok(db.statistics());
  if(path=="/v1/amostras")return ok(db.samples(),"amostras_nao_representativas");
  if(path.starts_with("/v1/mapa/")&&path.ends_with(".geojson")){
    auto layer=path.substr(9,path.size()-9-8);
    if(layer!="municipios"&&layer!="coredes"&&layer!="regioes-funcionais"&&layer!="biomas")return err(404,"recurso_nao_encontrado","camada cartográfica não encontrada");
    auto raw=file(std::filesystem::path(TRAMA_SOURCE_DIR)/"data/map"/(layer+".geojson"));if(raw.empty())return err(503,"mapa_indisponivel","arquivo cartográfico não instalado");
    auto geo=json::parse(raw);geo["metadata"]={{"layer",layer},{"crs","EPSG:4326"},{"simplified_for_web",true},{"source",layer=="biomas"?"IBGE Biomas e Sistema Costeiro-Marinho 1:250.000, 2025":"IBGE Malha Municipal Digital 2025"},{"regionalization_source",(layer=="coredes"||layer=="regioes-funcionais")?json("TRAMA-RS PRELIMINAR_NAO_HOMOLOGADO"):json(nullptr)},{"status_validacao",layer=="biomas"?"OFICIAL_IBGE_GEOMETRIA":preliminary_status}};
    return {200,"application/geo+json; charset=utf-8",geo.dump()};
  }
  if(path.starts_with("/v1/regioes-funcionais/")&&path.ends_with("/coredes")){constexpr std::string_view prefix="/v1/regioes-funcionais/";constexpr std::string_view suffix="/coredes";auto id=path.substr(prefix.size(),path.size()-prefix.size()-suffix.size());return ok(db.coredes(id));}
  if(path.starts_with("/v1/coredes/")&&path.ends_with("/municipios")){auto id=path.substr(12,path.size()-12-11);auto d=db.municipalities("",id,"",500,0);return ok(d);}
  if(path.starts_with("/v1/biomas/")&&path.ends_with("/municipios")){auto id=path.substr(12,path.size()-12-11);auto criterion=q.contains("criterio")?q["criterio"]:"predominante";if(criterion=="presenca")return err(409,"dados_bioma_incompletos","a relação de presença IBGE 2019 ainda não foi importada");if(criterion!="predominante")return err(400,"parametro_invalido","criterio deve ser predominante ou presenca");if(!db.predominant_biomes_complete())return err(409,"dados_bioma_incompletos","a classificação predominante ainda não cobre os 497 municípios");return ok(db.municipalities("","","",500,0,id,criterion),criterion);}
  if(path=="/v1/municipios"){
    auto criterion=q.contains("criterio")?q["criterio"]:"predominante";if(q.contains("bioma_id")&&criterion=="presenca")return err(409,"dados_bioma_incompletos","a relação de presença IBGE 2019 ainda não foi importada");if(q.contains("bioma_id")&&criterion!="predominante")return err(400,"parametro_invalido","criterio deve ser predominante ou presenca");if(q.contains("bioma_id")&&!db.predominant_biomes_complete())return err(409,"dados_bioma_incompletos","a classificação predominante ainda não cobre os 497 municípios");
    int limit=100,offset=0;try{if(q.contains("limit"))limit=std::stoi(q["limit"]);if(q.contains("offset"))offset=std::stoi(q["offset"]);}catch(...){return err(400,"parametro_invalido","limit e offset devem ser inteiros");}
    if(limit<1||limit>500||offset<0)return err(400,"parametro_invalido","limit deve estar entre 1 e 500 e offset não pode ser negativo");
    return ok(db.municipalities(q["q"],q["corede_id"],q["regiao_funcional_id"],limit,offset,q["bioma_id"],criterion),q.contains("bioma_id")?criterion:"");
  }
  if(path.starts_with("/v1/municipios/")){auto d=db.municipality(path.substr(15));return d.is_null()?err(404,"recurso_nao_encontrado","município não encontrado"):ok(d);}
  return err(404,"recurso_nao_encontrado","rota não encontrada");
}
}
HttpServer::HttpServer(std::filesystem::path db,std::string host,int port):db_(std::move(db)),host_(std::move(host)),port_(port){}
void HttpServer::run(){
  Database db(db_,true);int server=socket(AF_INET,SOCK_STREAM,0);if(server<0)throw std::runtime_error("socket falhou");int one=1;setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_port=htons(port_);if(inet_pton(AF_INET,host_.c_str(),&addr.sin_addr)!=1)throw std::runtime_error("host IPv4 inválido");if(bind(server,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))<0)throw std::runtime_error("bind falhou");if(listen(server,32)<0)throw std::runtime_error("listen falhou");std::cout<<"{\"event\":\"listening\",\"host\":\""<<host_<<"\",\"port\":"<<port_<<"}\n"<<std::flush;
  std::signal(SIGPIPE,SIG_IGN);
  while(true){int client=accept(server,nullptr,nullptr);if(client<0)continue;char buf[16384];auto n=read(client,buf,sizeof(buf)-1);if(n>0){buf[n]=0;std::istringstream in(std::string(buf,n));std::string method,target,proto;in>>method>>target>>proto;Reply r=method=="GET"?api(db,target):Reply{405,"application/json; charset=utf-8",json{{"error",{{"code","metodo_nao_permitido"}}}}.dump()};std::string reason=r.status==200?"OK":r.status==400?"Bad Request":r.status==404?"Not Found":r.status==409?"Conflict":"Error";std::ostringstream out;out<<"HTTP/1.1 "<<r.status<<' '<<reason<<"\r\nContent-Type: "<<r.type<<"\r\nContent-Length: "<<r.body.size()<<"\r\nConnection: close\r\nX-Content-Type-Options: nosniff\r\n\r\n"<<r.body;send_all(client,out.str());}close(client);}
}
}

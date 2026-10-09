#include "trama/database.hpp"
#include "trama/importer.hpp"
#include <iostream>
#include <map>
#include <stdexcept>

namespace {std::map<std::string,std::string> args(int argc,char** argv,int from){std::map<std::string,std::string> m;for(int i=from;i<argc;++i){std::string k=argv[i];if(k.starts_with("--")&&i+1<argc)m[k]=argv[++i];}return m;}void usage(){std::cerr<<"uso: trama <init|seed|validate|export|import> --db ARQUIVO [--data-dir DIR] [--output JSON] [--ibge-csv CSV]\n";}}
int main(int argc,char** argv){try{if(argc<2){usage();return 2;}std::string cmd=argv[1];auto a=args(argc,argv,2);if(!a.contains("--db")){usage();return 2;}trama::Database db(a["--db"]);if(cmd=="init"){db.migrate();std::cout<<"banco inicializado: "<<a["--db"]<<'\n';}else if(cmd=="seed"){auto d=a.contains("--data-dir")?a["--data-dir"]:"data";db.seed(d);std::cout<<db.validate().dump(2)<<'\n';}else if(cmd=="validate"){auto r=db.validate();std::cout<<r.dump(2)<<'\n';return r["ok"].get<bool>()?0:1;}else if(cmd=="export"){if(!a.contains("--output"))throw std::runtime_error("--output é obrigatório");db.export_json(a["--output"]);std::cout<<"JSON exportado: "<<a["--output"]<<'\n';}else if(cmd=="import"){if(!a.contains("--ibge-csv"))throw std::runtime_error("--ibge-csv é obrigatório");std::cout<<trama::import_ibge_predominant_biomes(db,a["--ibge-csv"]).dump(2)<<'\n';}else{usage();return 2;}return 0;}catch(const std::exception& e){std::cerr<<"erro: "<<e.what()<<'\n';return 1;}}

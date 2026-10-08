#include "trama/database.hpp"
#include <iostream>
int main(int argc,char**argv){if(argc!=2){std::cerr<<"uso: trama-verify BANCO\n";return 2;}try{trama::Database db(argv[1],true);auto r=db.validate();std::cout<<r.dump(2)<<'\n';return r["ok"].get<bool>()?0:1;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

#include "trama/http_server.hpp"
#include <iostream>
#include <map>
#include <stdexcept>
int main(int argc,char** argv){try{std::map<std::string,std::string>a;for(int i=1;i+1<argc;i+=2)a[argv[i]]=argv[i+1];if(!a.contains("--db"))throw std::runtime_error("uso: trama-rsd --db ARQUIVO [--host 127.0.0.1] [--port 8080]");trama::HttpServer s(a["--db"],a.contains("--host")?a["--host"]:"127.0.0.1",a.contains("--port")?std::stoi(a["--port"]):8080);s.run();}catch(const std::exception&e){std::cerr<<"erro: "<<e.what()<<'\n';return 1;}}

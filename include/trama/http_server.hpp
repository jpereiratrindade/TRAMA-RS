#pragma once
#include <filesystem>
#include <string>

namespace trama {
class HttpServer {
 public:
  HttpServer(std::filesystem::path db, std::string host, int port);
  void run();
 private:
  std::filesystem::path db_;
  std::string host_;
  int port_;
};
}

#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <string>

namespace trama {
struct BiomeAssignment;
class Database {
 public:
  explicit Database(const std::filesystem::path& path, bool read_only = false);
  ~Database();
  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;
  void migrate();
  void seed(const std::filesystem::path& data_dir);
  nlohmann::json validate() const;
  nlohmann::json catalog() const;
  nlohmann::json regions() const;
  nlohmann::json coredes(const std::string& region = {}) const;
  nlohmann::json municipalities(const std::string& q = {}, const std::string& corede = {},
                                const std::string& region = {}, int limit = 100, int offset = 0,
                                const std::string& biome = {}, const std::string& criterion = "predominante") const;
  nlohmann::json municipality(const std::string& ibge) const;
  nlohmann::json biomes() const;
  nlohmann::json samples() const;
  nlohmann::json statistics() const;
  bool predominant_biomes_complete() const;
  void assign_predominant_biomes(const std::vector<BiomeAssignment>& assignments,
                                 const std::filesystem::path& source);
  void export_json(const std::filesystem::path& output) const;
  sqlite3* handle() const { return db_; }
 private:
  sqlite3* db_{};
  void exec(const std::string& sql) const;
};
}

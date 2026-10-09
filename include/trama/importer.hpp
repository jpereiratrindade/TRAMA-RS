#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace trama {
class Database;
struct BiomeAssignment {
  std::string codigo_ibge;
  std::string municipio;
  std::string bioma_id;
};

nlohmann::json inspect_seed(const std::filesystem::path& data_dir);
std::vector<BiomeAssignment> parse_ibge_predominant_biomes(const std::filesystem::path& csv);
nlohmann::json import_ibge_predominant_biomes(Database& db, const std::filesystem::path& csv);
}

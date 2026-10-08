#include "trama/importer.hpp"
#include <fstream>
#include <stdexcept>
namespace trama {
nlohmann::json inspect_seed(const std::filesystem::path& d){std::ifstream f(d/"municipios_coredes_preliminar.json");if(!f)throw std::runtime_error("dataset de municípios ausente");auto j=nlohmann::json::parse(f);return {{"municipios",j.at("municipios").size()},{"estado",j.at("estado")},{"apto_para_seed",j.at("municipios").size()==497}};}
}

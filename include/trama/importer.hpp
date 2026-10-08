#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>

namespace trama {
nlohmann::json inspect_seed(const std::filesystem::path& data_dir);
}

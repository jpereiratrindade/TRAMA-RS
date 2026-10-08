#pragma once
#include <optional>
#include <string>

namespace trama {
inline constexpr const char* version = "0.1.0";
inline constexpr const char* schema_version = "1.0.0";
inline constexpr const char* preliminary_status = "PRELIMINAR_NAO_HOMOLOGADO";

struct Municipio {
  std::optional<std::string> codigo_ibge;
  std::string nome;
  std::string uf{"RS"};
  std::string corede_id;
  std::string regiao_funcional_id;
  std::optional<std::string> bioma_predominante_id;
  std::string classificacao_bioma_status;
};

std::string normalize_for_search(std::string text);
bool valid_ibge_code(const std::string& code);
}

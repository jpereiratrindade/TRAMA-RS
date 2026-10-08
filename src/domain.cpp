#include "trama/domain.hpp"
#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace trama {
std::string normalize_for_search(std::string s) {
  static const std::unordered_map<std::string, char> map = {
    {"á",'a'},{"à",'a'},{"â",'a'},{"ã",'a'},{"ä",'a'},{"Á",'a'},{"À",'a'},{"Â",'a'},{"Ã",'a'},
    {"é",'e'},{"ê",'e'},{"É",'e'},{"Ê",'e'},{"í",'i'},{"Í",'i'},
    {"ó",'o'},{"ô",'o'},{"õ",'o'},{"Ó",'o'},{"Ô",'o'},{"Õ",'o'},
    {"ú",'u'},{"ü",'u'},{"Ú",'u'},{"Ü",'u'},{"ç",'c'},{"Ç",'c'}};
  std::string out;
  for (std::size_t i=0; i<s.size();) {
    bool matched=false;
    for (const auto& [from,to] : map) if (s.compare(i,from.size(),from)==0) {
      out += to; i += from.size(); matched=true; break;
    }
    if (!matched) { unsigned char c=s[i++]; out += static_cast<char>(std::tolower(c)); }
  }
  return out;
}
bool valid_ibge_code(const std::string& c) {
  return c.size()==7 && c.starts_with("43") && std::ranges::all_of(c, [](unsigned char x){return std::isdigit(x);});
}
}

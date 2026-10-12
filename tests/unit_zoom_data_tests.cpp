#include "halo2/content.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
int main(int argc, char **argv) {
  try {
    if (argc != 3 && argc != 4) throw FormatError("Requires map, original zoom fixture and optional update XBE");
    for (int i = 1; i < argc; ++i) if (!std::filesystem::exists(argv[i])) return 77;
    std::shared_ptr<const OriginalData> source;
    if (argc == 4) source = OriginalData::load_xbe(argv[3]);
    const Cache cache(argv[1], std::filesystem::path(argv[1]).parent_path(), source);
    const auto content = cache.load_content(0, true);
    const auto weapon = std::find_if(content.weapons.begin(), content.weapons.end(),
        [](const auto &row) { return row.second.name == "objects\\weapons\\rifle\\smg\\smg"; });
    if (weapon == content.weapons.end() || !content.binocular_zoom)
      throw FormatError("Actual imported SMG/binocular metadata absent");
    std::ifstream input(argv[2]);
    bool found{};
    for (std::string line; std::getline(input, line);) {
      std::istringstream row(line);
      std::vector<std::string> fields;
      for (std::string field; row >> field;) fields.push_back(field);
      if (fields.size() < 15 || fields[0] != "A" || fields[1] != "stock_admission") continue;
      const auto bits = [](const std::string &field) { return std::uint32_t(std::stoul(field, nullptr, 16)); };
      const auto &w = weapon->second.zoom;
      const auto &b = *content.binocular_zoom;
      if (w.levels != std::stoi(fields[4]) || w.flags != bits(fields[5]) ||
          std::bit_cast<std::uint32_t>(w.minimum) != bits(fields[6]) ||
          std::bit_cast<std::uint32_t>(w.maximum) != bits(fields[7]) ||
          b.levels != std::stoi(fields[8]) ||
          std::bit_cast<std::uint32_t>(b.minimum) != bits(fields[9]) ||
          std::bit_cast<std::uint32_t>(b.maximum) != bits(fields[10]))
        throw FormatError("Live Cache zoom metadata differs from actual original stock context");
      found = true;
      break;
    }
    if (!found) throw FormatError("Original stock context missing");
    std::cout << "Actual Cache SMG/binocular data matches original runtime context; camera/action consumers remain separate\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}

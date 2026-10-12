#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/residency.hpp"
#include <fstream>
#include <iostream>
#include <set>

int main(int argc, char **argv) {
  try {
    if (argc != 3) throw std::runtime_error("map directory and fixture required");
    if (!std::filesystem::is_regular_file(std::filesystem::path(argv[1]) / "01b_spacestation.map") ||
        !std::filesystem::is_regular_file(std::filesystem::path(argv[1]) / "03a_oldmombasa.map") ||
        !std::filesystem::is_regular_file(std::filesystem::path(argv[1]) / "08b_deltacontrol.map"))
      return 77;
    std::ifstream input(argv[2]);
    if (!input) throw std::runtime_error("authored resource fixture unavailable");
    std::string map, datum_text;
    std::size_t count, cases = 0, records = 0;
    while (input >> map >> datum_text >> count) {
      const auto datum = std::stoul(datum_text, nullptr, 16);
      halo2::Cache cache(std::filesystem::path(argv[1]) / map, argv[1]);
      const auto actual = cache.object_resources(datum);
      if (actual.size() != count) throw std::runtime_error("authored list count changed");
      std::set<std::pair<std::uint32_t,int>> bitmaps;
      std::set<std::uint32_t> models;
      for (std::size_t i = 0; i < count; ++i) {
        int type, index;
        std::string resource_text;
        if (!(input >> type >> index >> resource_text)) throw std::runtime_error("truncated fixture");
        const auto resource = std::stoul(resource_text, nullptr, 16);
        if (actual[i].type != type || actual[i].index != index || actual[i].datum != resource)
          throw std::runtime_error("authored resource dispatch changed");
        if (type == 0) bitmaps.emplace(resource, index);
        if (type == 2) models.insert(resource);
      }
      halo2::NativeResidency residency(cache);
      residency.object(datum);
      if (residency.model_count() != models.size() || residency.bitmap_count() < bitmaps.size())
        throw std::runtime_error("object resources were not retained");
      const auto before = residency.bitmap_count();
      residency.object(datum);
      residency.object(halo2::null_datum);
      if (residency.bitmap_count() != before || !cache.object_resources(halo2::null_datum).empty())
        throw std::runtime_error("duplicate/NONE residency changed");
      bool rejected = false;
      try { cache.object_resources(datum ^ 0x10000u); }
      catch (const halo2::FormatError &) { rejected = true; }
      if (!rejected) throw std::runtime_error("stale object tag salt accepted");
      std::cout << map << ' ' << datum_text << " records=" << count
                << " models=" << residency.model_count()
                << " bitmaps=" << residency.bitmap_count() << '\n';
      ++cases;
      records += count;
    }
    if (cases != 11 || records != 475) throw std::runtime_error("incomplete authored coverage");
    std::cout << "11 authored object predictions / 475 resources retained; NONE and stale salts checked\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}

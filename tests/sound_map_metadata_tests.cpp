#include "halo2/source_tag_metadata_store.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>

namespace {
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
std::uint32_t number(const std::string &value) { return std::stoul(value, nullptr, 16); }
std::vector<std::byte> raw(const std::string &value) {
  check(!value.empty() && value.size()%2 == 0, "Incomplete owned sound-map metadata");
  std::vector<std::byte> bytes(value.size()/2);
  for (std::size_t i = 0; i < bytes.size(); ++i) bytes[i] = std::byte(number(value.substr(i*2, 2)));
  return bytes;
}
}
int main(int argc, char **argv) {
  try {
    check(argc == 3, "Supply complete sound-map fixture and project root");
    if (!std::filesystem::exists(argv[1])) return 77;
    std::ifstream input(argv[1]);
    halo2::SourceTagMetadataProviders provider;
    std::function<std::uint32_t(std::uint16_t)> indexed_address;
    std::weak_ptr<const halo2::Cache> cache_lifetime;
    std::size_t maps = 0, records = 0, bytes = 0;
    std::set<std::string> groups;
    auto finish = [&] {
      if (!maps) return;
      check(groups == std::set<std::string>{"matg", "sncl", "ugh!"}, "Incomplete owned sound-map groups");
      provider = {};
      check(!cache_lifetime.expired(), "Literal tag-table callback lost its owned cache");
      indexed_address = {};
      check(cache_lifetime.expired(), "Released sound-map callbacks retained their cache");
      groups.clear();
    };
    for (std::string line; std::getline(input, line);) {
      std::istringstream row(line); char kind; row >> kind;
      if (kind == 'M') {
        finish(); std::string path; unsigned expected;
        check(bool(row >> std::quoted(path) >> expected) && expected == 3, "Incomplete owned sound-map row");
        const auto source = std::filesystem::path(path).is_absolute() ? std::filesystem::path(path) : std::filesystem::path(argv[2])/path;
        if (!std::filesystem::exists(source)) return 77;
        {
          auto cache = std::make_shared<halo2::Cache>(source); cache_lifetime = cache;
          halo2::SourceTagMetadataStore store(cache, {"matg", "sncl", "ugh!"});
          check(store.tag_count() == expected, "Actual resident sound-map payload count differs");
          provider = store.providers();
          indexed_address = store.index_address_provider();
          bool excluded = false;
          for (std::size_t i=0;i<cache->tags().size();++i) {
            const auto &tag=cache->tags()[i];
            if (tag.group=="matg" || tag.group=="sncl" || tag.group=="ugh!") continue;
            try { indexed_address(std::uint16_t(i)); }
            catch (const halo2::FormatError &) { excluded=true; }
            check(excluded,"Unselected literal tag-table backing was implicitly provided");
            break;
          }
          check(excluded,"Missing unselected tag-table boundary coverage");
          if (cache->tags().size()<65536) {
            bool unavailable=false;
            try { indexed_address(std::uint16_t(cache->tags().size())); }
            catch (const halo2::FormatError &) { unavailable=true; }
            check(unavailable,"Out-of-table slot was silently resolved");
          }
        }
        check(!cache_lifetime.expired(), "Opaque sound-map callbacks lost their owned backing");
        ++maps;
      } else if (kind == 'T') {
        std::string datum, group, address, payload;
        check(bool(row >> datum >> group >> address >> payload), "Incomplete original sound-map payload");
        check(groups.emplace(group).second, "Duplicate original sound-map group");
        const auto expected = raw(payload); const auto id = number(datum), va = number(address);
        const auto &actual = provider.definition(id);
        check(actual.datum() == id && actual.group() == group && actual.source_address() == va &&
              actual.size() == expected.size() && provider.address(id) == va,
              "Original sound-map datum/address/extent differs");
        check(indexed_address(std::uint16_t(id)) == va,
              "Literal original low16 tag-table address differs");
        const auto direct = actual.bytes_at(va, expected.size()), forwarded = provider.read(va, expected.size());
        check(std::equal(direct.begin(), direct.end(), expected.begin(), expected.end()) &&
              std::equal(forwarded.begin(), forwarded.end(), expected.begin(), expected.end()),
              "Complete owned sound-map metadata bytes differ");
        ++records; bytes += expected.size();
      } else throw std::runtime_error("Unknown owned sound-map fixture record");
    }
    finish();
    check(maps == 23 && records == 69 && bytes == 5244048, "Incomplete authenticated sound-map coverage");
    std::cout << "ownedMaps=" << maps << " completeOpaqueSoundMapPayloads=" << records << " exactBytes=" << bytes
              << " literalTagTableSlots=69 retainedBacking=passed sourceMapActivationCacheAndSoundAdmission=unverified\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}

#include "halo2/custom_slayer.hpp"
#include "halo2/frontend.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <set>
#ifdef __linux__
#include <unistd.h>
#endif
namespace halo2 {
CustomSlayer custom_slayer(const UiVariant &variant) {
  unsigned kind=0;
  for(const auto &[id,value]:variant.values)if(id>=native_slayer_ruleset_parameter) {
    if(id!=native_slayer_ruleset_parameter || value<0 || value>2 || variant.engine!=0)
      throw FormatError("Unsupported native Slayer ruleset");
    kind=unsigned(value);
  }
  return CustomSlayer(kind);
}
std::uint32_t source_starting_weapon(std::uint32_t fallback,unsigned option,
    std::uint32_t excluded,std::span<const std::uint32_t,18> refs,std::uint32_t &seed) {
  constexpr std::array<unsigned,18> permutation{3,0,1,6,7,5,2,4,12,9,10,13,11,15,14,8,16,17};
  if(option==1)return 0xffffffffu;
  if(option==2) {
    // A malformed external reference bank must not hang the native process.
    if(std::none_of(permutation.begin(),permutation.begin()+16,[&](auto i){return refs[i]!=0xffffffffu && (excluded==0xffffffffu || refs[i]!=excluded);}))
      throw FormatError("Random starting weapon has no eligible authored definition");
    for(;;) {
      seed=seed*1664525u+1013904223u;
      const auto result=refs[permutation[seed>>28]];
      if(result!=0xffffffffu && (excluded==0xffffffffu || result!=excluded))return result;
    }
  }
  return option>=3 && option<=20 ? refs[permutation[option-3]] : fallback;
}
std::uint64_t custom_slayer_fingerprint(std::uint64_t base,const UiVariant &variant) {
  if(custom_slayer(variant)==CustomSlayer::none)return base;
  // Domain/version and every native extension write fence ordinary Slayer and
  // future ruleset revisions at the existing loading/UDP admission boundary.
  for(const auto &[id,value]:variant.values)if(id>=native_slayer_ruleset_parameter)
    for(auto word:{std::uint64_t(id),std::uint64_t(unsigned(value))})
      for(unsigned i=0;i<8;++i)base=(base^((word>>(i*8))&255u))*1099511628211ull;
  return base;
}
std::vector<UiVariant> load_custom_slayer_variants(const std::filesystem::path &directory) {
  std::vector<UiVariant> result;
  if(!std::filesystem::exists(directory))return result;
  std::vector<std::filesystem::path> files;
  for(const auto &entry:std::filesystem::directory_iterator(directory))
    if(entry.is_regular_file() && entry.path().extension()==".json")files.push_back(entry.path());
  std::sort(files.begin(),files.end());
  if(files.size()>32)throw FormatError("Too many bundled Slayer variants");
  std::set<std::string> names,ids;
  for(const auto &path:files) {
    if(std::filesystem::file_size(path)>16384)throw FormatError("Bundled Slayer variant is too large");
    std::ifstream input(path);auto json=nlohmann::json::parse(input);
    if(!json.is_object() || json.size()!=8 || json.at("schema")!="halo2-native-slayer-variant-v1" || json.at("engine")!="slayer")
      throw FormatError("Invalid bundled Slayer variant schema");
    UiVariant variant;variant.name=json.at("name").get<std::string>();variant.engine=0;
    if(variant.name.empty() || variant.name.size()>32 || !names.insert(variant.name).second ||
        !ids.insert(json.at("id").get<std::string>()).second)throw FormatError("Invalid bundled Slayer variant identity");
    const auto kind=json.at("native_ruleset").get<int>();
    if(kind<1 || kind>2 || !json.at("settings").is_object())throw FormatError("Invalid custom Slayer ruleset");
    for(auto &[key,value]:json.at("settings").items()) {
      if(key.empty() || key.find_first_not_of("0123456789")!=std::string::npos)throw FormatError("Invalid Slayer setting ID");
      const auto id=std::stoul(key);
      if(id>=112 || !value.is_number_integer())throw FormatError("Invalid Slayer setting value");
      variant.values.append(unsigned(id),value.get<int>());
    }
    variant.values.append(native_slayer_ruleset_parameter,kind);
    if(!source_game_variant_valid(source_game_variant(variant)))throw FormatError("Bundled Slayer settings fail original validation");
    result.push_back(std::move(variant));
  }
  return result;
}
std::vector<UiVariant> bundled_slayer_variants() {
#ifdef __linux__
  std::array<char,4096> path{};const auto count=readlink("/proc/self/exe",path.data(),path.size()-1);
  if(count>0) {
    const auto directory=std::filesystem::path(std::string(path.data(),std::size_t(count))).parent_path()/"data/tags/variants";
    if(std::filesystem::exists(directory))return load_custom_slayer_variants(directory);
  }
#endif
  if(std::filesystem::exists("data/tags/variants"))return load_custom_slayer_variants("data/tags/variants");
#ifdef HALO2_VARIANT_TAG_DIR
  return load_custom_slayer_variants(HALO2_VARIANT_TAG_DIR);
#else
  return {};
#endif
}
}

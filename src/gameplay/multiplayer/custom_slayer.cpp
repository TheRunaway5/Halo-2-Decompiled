#include "halo2/custom_slayer.hpp"
#include "halo2/bomb_defusal.hpp"
#include "halo2/frontend.hpp"
#include "halo2/cache.hpp"
#include "halo2/math.hpp"
#include "halo2/macos/bundle.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <set>
#include <limits>
#ifdef __linux__
#include <unistd.h>
#endif
namespace halo2 {
namespace {
constexpr const char *defusal_layout_file="bomb-defusal-sites-v1.json";
std::filesystem::path bundled_tag_path(const std::filesystem::path &name) {
  if(const auto contents=macos_bundle_contents();!contents.empty())return contents/"Resources/data/tags"/name;
#ifdef __linux__
  std::array<char,4096> path{};const auto count=readlink("/proc/self/exe",path.data(),path.size()-1);
  if(count>0) {
    const auto directory=std::filesystem::path(std::string(path.data(),std::size_t(count))).parent_path()/"data/tags";
    if(std::filesystem::exists(directory))return directory/name;
  }
#endif
  if(std::filesystem::exists("data/tags"))return std::filesystem::path("data/tags")/name;
#ifdef HALO2_VARIANT_TAG_DIR
  return std::filesystem::path(HALO2_VARIANT_TAG_DIR).parent_path()/name;
#else
  return std::filesystem::path("data/tags")/name;
#endif
}
std::string layout_bytes(const std::filesystem::path &path) {
  if(!std::filesystem::is_regular_file(path) || std::filesystem::file_size(path)>65536)
    throw FormatError("Missing or oversized Bomb Defusal map layouts; rerun setup with the updated assets");
  std::ifstream input(path,std::ios::binary);
  return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}
}
CustomSlayer custom_slayer(const UiVariant &variant) {
  unsigned kind=0;
  for(const auto &[id,value]:variant.values)if(id>=native_slayer_ruleset_parameter) {
    if(bomb_defusal_parameter(id) && bomb_defusal_variant(variant))continue;
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
  if(custom_slayer(variant)==CustomSlayer::none && !bomb_defusal_variant(variant))return base;
  // Domain/version and every native extension write fence ordinary Slayer and
  // future ruleset revisions at the existing loading/UDP admission boundary.
  for(const auto &[id,value]:variant.values)if(id>=native_slayer_ruleset_parameter)
    for(auto word:{std::uint64_t(id),std::uint64_t(unsigned(value))})
      for(unsigned i=0;i<8;++i)base=(base^((word>>(i*8))&255u))*1099511628211ull;
  if(bomb_defusal_variant(variant))for(const unsigned char byte:layout_bytes(bundled_tag_path(defusal_layout_file)))
    base=(base^byte)*1099511628211ull;
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
    const bool defusal=json.value("schema","")=="halo2-native-assault-variant-v1" && json.value("engine","")=="assault";
    if(!json.is_object() || json.size()!=8 || (!defusal && (json.at("schema")!="halo2-native-slayer-variant-v1" || json.at("engine")!="slayer")))
      throw FormatError("Invalid bundled Slayer variant schema");
    UiVariant variant;variant.name=json.at("name").get<std::string>();variant.engine=defusal?5:0;
    if(variant.name.empty() || variant.name.size()>32 || !names.insert(variant.name).second ||
        !ids.insert(json.at("id").get<std::string>()).second)throw FormatError("Invalid bundled Slayer variant identity");
    const auto kind=json.at("native_ruleset").get<int>();
    if(kind<1 || kind>(defusal?1:2) || !json.at("settings").is_object())throw FormatError("Invalid custom multiplayer ruleset");
    for(auto &[key,value]:json.at("settings").items()) {
      if(key.empty() || key.find_first_not_of("0123456789")!=std::string::npos)throw FormatError("Invalid Slayer setting ID");
      const auto id=std::stoul(key);
      if(id>std::numeric_limits<unsigned>::max() || (id>=112 && !(defusal && id!=native_defusal_parameter && bomb_defusal_parameter(unsigned(id)))) || !value.is_number_integer())throw FormatError("Invalid multiplayer setting value");
      variant.values.append(unsigned(id),value.get<int>());
    }
    variant.values.append(defusal?native_defusal_parameter:native_slayer_ruleset_parameter,kind);
    if(defusal)bomb_defusal_settings(variant);
    if(!source_game_variant_valid(source_game_variant(variant)))throw FormatError("Bundled Slayer settings fail original validation");
    result.push_back(std::move(variant));
  }
  return result;
}
std::vector<UiVariant> bundled_slayer_variants() {
  return load_custom_slayer_variants(bundled_tag_path("variants"));
}
std::optional<BombDefusalLayout> load_bomb_defusal_layout(const std::filesystem::path &path,
    std::string_view scenario,std::span<const NetgameFlag> markers) {
  const auto json=nlohmann::json::parse(layout_bytes(path));
  if(!json.is_object() || json.value("schema","")!="halo2-native-bomb-defusal-sites-v1" ||
      !json.at("maps").is_array() || json.at("maps").size()>32)throw FormatError("Invalid Bomb Defusal layout catalog");
  const nlohmann::json *selected=nullptr;std::set<std::string> scenarios;
  for(const auto &row:json.at("maps")) {
    const auto identity=row.at("scenario").get<std::string>();
    if(!scenarios.insert(identity).second)throw FormatError("Duplicate Bomb Defusal scenario layout");
    if(identity==scenario)selected=&row;
  }
  if(!selected)return {};
  const auto &row=*selected;BombDefusalLayout result;
  result.name=row.at("name").get<std::string>();result.recommended_team_size=row.at("team_size").get<unsigned>();
  if(result.name.empty() || result.name.size()>48 || (result.recommended_team_size!=4 && result.recommended_team_size!=8) ||
      row.at("sites").size()!=2 || row.at("spawns").size()!=2)throw FormatError("Invalid Bomb Defusal A/B layout");
  const auto marker=[&](const nlohmann::json &pin)->const NetgameFlag& {
    const auto id=pin.at("index").get<unsigned>();
    const auto found=std::ranges::find(markers,id,&NetgameFlag::source_index);
    const auto &p=pin.at("position");if(!p.is_array() || p.size()!=3)throw FormatError("Invalid Bomb Defusal marker pin");
    const Vec3 expected{p[0].get<float>(),p[1].get<float>(),p[2].get<float>()};
    if(!std::isfinite(expected.x) || !std::isfinite(expected.y) || !std::isfinite(expected.z) ||
        found==markers.end() || found->type!=pin.at("type").get<int>() ||
        found->selector12!=pin.at("team").get<int>() || length(found->position-expected)>.015f)
      throw FormatError("Bomb Defusal layout does not match this map's authored markers: "+result.name);
    return *found;
  };
  for(unsigned i=0;i<2;++i) {
    const auto &site=marker(row.at("sites").at(i).at("marker"));
    result.sites[i]={site.source_index,site.position};
    result.spawn_anchors[i]=marker(row.at("spawns").at(i)).position;
  }
  if(length(result.sites[0].position-result.sites[1].position)<6.f ||
      length(result.spawn_anchors[0]-result.spawn_anchors[1])<6.f)throw FormatError("Bomb Defusal sites or spawns are not separated");
  return result;
}
std::optional<BombDefusalLayout> bundled_bomb_defusal_layout(std::string_view scenario,std::span<const NetgameFlag> markers) {
  return load_bomb_defusal_layout(bundled_tag_path(defusal_layout_file),scenario,markers);
}
}

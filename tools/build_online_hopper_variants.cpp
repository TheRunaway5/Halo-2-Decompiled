// Offline authoring helper. Use the actual original-option implementation;
// no second parameter writer or name-based runtime rule lookup.
#include "halo2/frontend.hpp"
#include "halo2/source_game_variant.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

int main() try {
  using Json=nlohmann::json;
  Json input;std::cin>>input;Json output=Json::array();
  constexpr char hex[]="0123456789abcdef";
  for(const auto &row:input) {
    halo2::UiVariant variant{row.at("name").get<std::string>(),row.at("engine").get<unsigned>(),{}};
    if(row.contains("settings")) {
      const auto raw=row.at("settings").get<std::string>();
      if(raw.size()!=608||raw.find_first_not_of(hex)!=std::string::npos)throw std::runtime_error("Invalid base settings");
      variant.source_settings.emplace();
      for(unsigned i=0;i<304;++i)variant.source_settings->bytes[i]=std::uint8_t(std::stoul(raw.substr(i*2,2),nullptr,16));
    }
    for(const auto &write:row.value("writes",Json::array()))variant.values.append(write[0].get<unsigned>(),write[1].get<int>());
    auto settings=halo2::source_game_variant(variant);
    if(!row.value("preserve_title",false)) {
      if(variant.name.size()>31||variant.name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 -")!=std::string::npos)
        throw std::runtime_error("Invalid authored option title");
      halo2::source_game_variant_set_title(settings,std::u16string(variant.name.begin(),variant.name.end()));
    }
    if(!halo2::source_game_variant_valid(settings))throw std::runtime_error("Invalid original option buffer: "+variant.name);
    std::string encoded;for(auto byte:settings.bytes){encoded+=hex[byte>>4];encoded+=hex[byte&15];}
    output.push_back({{"name",variant.name},{"engine",variant.engine},{"settings",encoded},
      {"team",halo2::source_game_variant_get(settings,91)!=0}});
  }
  std::cout<<output.dump()<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

#include "halo2/mcc_loading_converter_metadata.hpp"
#include "halo2/macos/loading_converter_metadata.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>

using Json=nlohmann::json;
namespace profile=halo2::macos::loading_converter_metadata;
namespace {
const char* codebook="external/ww2ogg/packed_codebooks_aoTuV_603.bin";
const char* codebook_sha="00a93eab267d281401b1efd54e888a2e183299b9e6c446c48d09f701a89d9d27";
Json file() {return {{"bytes",64},{"sha256",std::string(64,'0')}};}
Json audio(bool mac) {
  const std::string executable=mac?std::string(profile::executable):"bin/halo2_mcc_loading_audio";
  Json files=Json::object();
  if(mac)for(auto name:profile::files)files[std::string(name)]=file();
  else for(const auto* name:{"bin/halo2_mcc_loading_audio","bin/halo2_mcc_loading_audio.real",
                            "lib/ld-linux-x86-64.so.2","lib/libvorbis.so.0",codebook})files[name]=file();
  files[codebook]["sha256"]=codebook_sha;
  Json runtime={{"schema",mac?std::string(profile::schema):"H2_MCC_LOADING_CONVERTER_RUNTIME_V1"},
    {"files",files},{"manifest",{{"path","resources/mcc-loading-converter.json"},
                                  {"bytes",512},{"sha256",std::string(64,'1')}}}};
  if(mac)runtime["architectures"]={"arm64","x86_64"};
  return {{"converter",{{"path",executable},{"bytes",64},{"sha256",std::string(64,'0')},{"runtime",runtime}}},
          {"codebook",{{"path",codebook},{"bytes",64},{"sha256",codebook_sha}}}};
}
void reject(Json value) {
  try {halo2::validate_mcc_loading_converter_metadata(value);}
  catch(const std::exception&){return;}
  throw std::runtime_error("Malformed converter provenance was admitted");
}
}
int main()try {
  unsigned checks{};
  for(bool mac:{false,true}) {
    auto value=audio(mac);halo2::validate_mcc_loading_converter_metadata(value);++checks;
    auto direct=value;direct["converter"].erase("runtime");
    halo2::validate_mcc_loading_converter_metadata(direct);++checks;
    for(const char* field:{"path","bytes","sha256"}) {
      auto bad=value;
      if(std::string_view(field)=="bytes")bad["converter"][field]=1;
      else bad["converter"][field]="foreign";
      reject(bad);++checks;
    }
    for(auto bytes:{Json(0),Json(true),Json(64*1024*1024),Json(-1)}) {
      auto bad=value;bad["converter"]["runtime"]["files"][codebook]["bytes"]=bytes;
      reject(bad);++checks;
    }
    for(const auto* name:{"../escape","lib/a/b","lib/a\nb","lib/"}) {
      auto bad=value;bad["converter"]["runtime"]["files"][name]=file();
      reject(bad);++checks;
    }
    auto bad=value;bad["converter"]["runtime"]["manifest"]["path"]="../manifest.json";
    reject(bad);++checks;
  }
  for(const Json arches:{Json::array(),Json::array({"arm64","arm64"}),Json::array({"arm64","ppc"}),
                         Json::array({"arm64",1}),Json::array({"arm64","x86_64","arm64"})}) {
    auto bad=audio(true);bad["converter"]["runtime"]["architectures"]=arches;reject(bad);++checks;
  }
  for(auto name:profile::files) {
    auto bad=audio(true);bad["converter"]["runtime"]["files"].erase(std::string(name));reject(bad);++checks;
  }
  auto bad=audio(true);bad["converter"]["runtime"]["files"]["lib/ld-linux-x86-64.so.2"]=file();
  reject(bad);++checks;
  std::cout<<"PASS "<<checks<<" production converter provenance controls (Linux and macOS layouts)\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

#include "halo2/mcc_loading_converter_metadata.hpp"
#include "halo2/macos/loading_converter_metadata.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <string>

namespace halo2 {
namespace {
using Json=nlohmann::json;
bool hex(const Json& value,std::size_t count) {
  if(!value.is_string())return false;
  const auto& text=value.get_ref<const std::string&>();
  return text.size()==count&&text.find_first_not_of("0123456789abcdef")==std::string::npos;
}
bool natural(const Json& value,bool positive=false) {
  return value.is_number_integer()&&value.get<std::int64_t>()>=(positive?1:0);
}
} // namespace

void validate_mcc_loading_converter_metadata(const nlohmann::json& audio) {
  const auto& decoder=audio.at("converter");
  if(!decoder.contains("runtime"))return; // Direct development decoder owner.
  const auto& runtime=decoder.at("runtime");
  const auto& files=runtime.at("files");const auto& manifest=runtime.at("manifest");
  constexpr const char* wrapper="bin/halo2_mcc_loading_audio";
  constexpr const char* codebook="external/ww2ogg/packed_codebooks_aoTuV_603.bin";
  const bool mac=runtime.at("schema")==macos::loading_converter_metadata::schema;
  const bool linux_runtime=runtime.at("schema")=="H2_MCC_LOADING_CONVERTER_RUNTIME_V1";
  const std::string executable=mac?std::string(macos::loading_converter_metadata::executable):wrapper;
  const auto required=[&](const std::string& name) {
    if(mac)return std::find(macos::loading_converter_metadata::files.begin(),
        macos::loading_converter_metadata::files.end(),name)!=macos::loading_converter_metadata::files.end();
    return name==wrapper || name=="bin/halo2_mcc_loading_audio.real" ||
      name=="lib/ld-linux-x86-64.so.2" || name==codebook;
  };
  if((!mac&&!linux_runtime) ||
      !files.is_object() || files.size()<4 || files.size()>256 ||
      !files.contains(executable) || !files.contains(codebook) ||
      (!mac&&(!files.contains("bin/halo2_mcc_loading_audio.real") ||
              !files.contains("lib/ld-linux-x86-64.so.2"))) ||
      manifest.at("path")!="resources/mcc-loading-converter.json" ||
      !natural(manifest.at("bytes"),true) || manifest.at("bytes").get<std::uint64_t>()>65536 ||
      !hex(manifest.at("sha256"),64))
    throw FormatError("MCC V3 packaged converter provenance is incomplete");
  if(mac) {
    const auto& arches=runtime.at("architectures");
    if(files.size()!=macos::loading_converter_metadata::files.size() || !arches.is_array() ||
        arches.empty() || arches.size()>2 ||
        std::any_of(arches.begin(),arches.end(),[](const Json& value){return value!="arm64"&&value!="x86_64";}) ||
        (arches.size()==2&&arches[0]==arches[1]) ||
        std::any_of(macos::loading_converter_metadata::files.begin(),macos::loading_converter_metadata::files.end(),
            [&](std::string_view name){return !files.contains(std::string(name));}))
      throw FormatError("MCC V3 packaged converter provenance is incomplete");
  }
  std::uint64_t total{};
  for(const auto& [name,row]:files.items()) {
    const auto path=std::filesystem::path(name);
    if(name.empty() || path.is_absolute() || path.lexically_normal().generic_string()!=name ||
        name.find('\\')!=std::string::npos ||
        std::any_of(name.begin(),name.end(),[](unsigned char c){return c<0x20 || c==0x7f;}) ||
        (!required(name) && (mac || !name.starts_with("lib/") || name.size()<=4 || name.back()=='/' ||
                            name.find('/',4)!=std::string::npos)) ||
        !row.is_object() || row.size()!=2 || !natural(row.at("bytes"),true) ||
        row.at("bytes").get<std::uint64_t>()>=64*1024*1024 || !hex(row.at("sha256"),64))
      throw FormatError("MCC V3 packaged converter file identity is invalid");
    for(const auto& part:path)if(part==".." || part==".")
      throw FormatError("MCC V3 packaged converter path escapes its owner");
    total+=row.at("bytes").get<std::uint64_t>();
  }
  if(total>256*1024*1024 || decoder.at("path")!=executable ||
      files.at(executable)!=Json{{"bytes",decoder.at("bytes")},{"sha256",decoder.at("sha256")}} ||
      files.at(codebook)!=Json{{"bytes",audio.at("codebook").at("bytes")},
                              {"sha256",audio.at("codebook").at("sha256")}})
    throw FormatError("MCC V3 packaged converter provenance owners disagree");
}
} // namespace halo2


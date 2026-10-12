#include "halo2/vista_announcer_events.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <fstream>
namespace halo2 {
VistaAnnouncerResponses load_vista_announcer_responses(const Cache& cache){
  if(!cache.is_vista())throw FormatError("Vista announcer responses require actual PC cache owners");
  const Tag* globals=nullptr;
  for(const auto& tag:cache.select_tags({.group=tag_groups::globals, .require_address=true})){
    if(globals)throw FormatError("Vista selected globals owner is ambiguous");
    globals=&tag;
  }
  if(!globals)throw FormatError("Vista selected globals owner is absent");
  const auto matg=cache.load_source_tag_metadata(globals->datum);
  const Bytes header(matg.bytes_at(matg.source_address(),0x170));
  const auto mulg=cache.load_source_tag_metadata(header.u32(0x16c));
  if(mulg.group()!="mulg")throw FormatError("Vista globals announcer owner has wrong group");
  const Bytes top(mulg.bytes_at(mulg.source_address(),16));
  if(top.u32(8)!=1)throw FormatError("Vista multiplayer runtime owner is not unique");
  const auto runtime=top.u32(12);const Bytes body(mulg.bytes_at(runtime,0xd0));VistaAnnouncerResponses out;
  auto load=[&](unsigned at,std::vector<VistaAnnouncerResponse>& bank){
    const auto count=body.u32(at);if(count>4096)throw FormatError("Excessive Vista announcer response bank");
    if(!count)return;
    const auto bytes=mulg.bytes_at(body.u32(at+4),std::size_t(count)*0xa8);
    bank.resize(count);for(unsigned i=0;i<count;++i)std::copy_n(bytes.begin()+std::ptrdiff_t(i*0xa8),0xa8,bank[i].begin());
  };
  load(0x98,out.general);load(0xa8,out.slayer);load(0xc8,out.king);return out;
}
VistaAnnouncerAssets open_vista_announcer_assets(const std::filesystem::path& maps,
    const std::string& scenario,std::shared_ptr<const OriginalData> constants){
  if(scenario.empty() || !std::filesystem::is_directory(maps))
    throw FormatError("Vista announcer requires actual PC maps and selected scenario");
  std::optional<std::filesystem::path> selected;
  for(const auto& item:std::filesystem::directory_iterator(maps)){
    if(!item.is_regular_file() || item.path().extension()!=".map")continue;
    std::array<std::byte,2048> header{};std::ifstream file(item.path(),std::ios::binary);
    if(!file.read(reinterpret_cast<char*>(header.data()),header.size()))continue;
    const Bytes b(header);
    if(b.u32(0)!=0x68656164 || b.u32(4)!=8 || b.u32(2044)!=0x666f6f74)continue;
    if(b.string(300,32)!="11081.07.04.30.0934.main" || b.string(456,256)!=scenario)continue;
    if(selected)throw FormatError("Actual selected Vista scenario has multiple cache owners");
    selected=item.path();
  }
  if(!selected)throw FormatError("Actual selected scenario has no supplied Vista cache owner");
  auto cache=std::make_shared<Cache>(*selected,maps,std::move(constants));
  if(!cache->is_vista() || cache->scenario()!=scenario)throw FormatError("Vista announcer cache owner changed during opening");
  auto responses=load_vista_announcer_responses(*cache);
  return {*selected,std::move(cache),std::move(responses)};
}
} // namespace halo2

// Export the ordinary game's prepared HUD metadata for custom-map authoring.
// Build/run under tools/run_test.py; no renderer or second HUD implementation.
#include "halo2/cache.hpp"
#include "halo2/mcc_ordinary_pointer.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
using Json=nlohmann::json;
int main(int argc,char **argv)try {
  if(argc!=4)throw FormatError("Usage: export_dust2_hud MAP SHARED_MAPS OUTPUT_JSON");
  Cache cache(argv[1],argv[2]);
  Json out{{"schema","H2_DUST2_MAIN_GAME_HUD_V1"},{"source_map",cache.path().string()},
           {"definitions",Json::array()}};
  for(const auto &tag:cache.tags())if(tag.group=="nhdt" && tag.address && tag.size) {
    const auto read=[&](auto at,auto size){return cache.read_tag_bytes(at,size);};
    const auto fields=ordinary_mcc_pointer_fields(tag.type(),tag.address,read);
    if(!fields)throw FormatError("Missing HUD pointer schema");
    struct Range {std::uint32_t source,size,target;};
    std::vector<Range> ranges{{tag.address,tag.size,0}};
    std::vector<std::byte> bytes(cache.tag_payload(tag.handle()).begin(),cache.tag_payload(tag.handle()).end());
    const auto locate=[&](std::uint32_t at,std::size_t size)->std::uint32_t {
      for(const auto &r:ranges)if(at>=r.source && std::uint64_t(at)-r.source+size<=r.size)
        return r.target+at-r.source;
      throw FormatError("HUD pointer field escapes its declared closure");
    };
    for(const auto &field:*fields)if(field.bytes) {
      bool present=false;
      for(const auto &r:ranges)if(field.address>=r.source &&
          std::uint64_t(field.address)-r.source+field.bytes<=r.size)present=true;
      if(!present) {
        bytes.resize((bytes.size()+3)&~std::size_t(3));
        const auto at=std::uint32_t(bytes.size());const auto data=read(field.address,field.bytes);
        bytes.insert(bytes.end(),data.begin(),data.end());ranges.push_back({field.address,field.bytes,at});
      }
    }
    Json pointers=Json::array(),references=Json::array(),strings=Json::array();
    for(const auto &f:*fields) {
      const auto at=locate(f.field_address,4);
      if(f.bytes)pointers.push_back({at,locate(f.address,f.bytes)});
      else for(unsigned n=0;n<4;++n)bytes[at+n]=std::byte{0};
    }
    const auto ref=[&](std::uint32_t at) {
      const Bytes data(read(at,8));const TagHandle handle{data.u32(4)};
      if(handle.is_null())return;
      const auto *target=cache.find_tag(handle,TagGroup{data.u32(0)});
      if(!target)throw FormatError("Unresolved main-game HUD reference");
      references.push_back({locate(at+4,4),target->group,target->name});
    };
    const auto sid=[&](std::uint32_t at) {
      const auto value=Bytes(read(at,4)).u32(0);
      strings.push_back({locate(at,4),cache.string_id(value)});
    };
    ref(tag.address);
    for(unsigned kind=0;kind<3;++kind) {
      const unsigned descriptor=kind==0?8:kind==1?16:32,stride=kind==0?100:kind==1?84:80;
      const Bytes block(read(tag.address+descriptor,8));
      for(unsigned i=0;i<block.u32(0);++i) {
        const auto row=block.u32(4)+i*stride;sid(row);
        if(kind!=1)ref(row+32);
        if(kind<2)ref(row+(kind==0?40:32));
        else for(const auto at:{40u,48u,56u})ref(row+at);
        if(kind==1)sid(row+40);
        const auto ep=kind==0?88u:kind==1?76u:72u;
        const Bytes effects(read(row+ep,8));
        for(unsigned e=0;e<effects.u32(0);++e)for(unsigned f=0;f<5;++f) {
          const auto function=effects.u32(4)+e*104+4+f*20;sid(function);sid(function+4);
        }
      }
    }
    constexpr char digits[]="0123456789abcdef";std::string hex;
    for(const auto b:bytes){const auto v=std::to_integer<unsigned>(b);hex+=digits[v>>4];hex+=digits[v&15];}
    out["definitions"].push_back({{"name",tag.name},{"hex",hex},{"pointers",pointers},
                                   {"references",references},{"strings",strings}});
  }
  std::ofstream stream(argv[3]);stream<<out.dump(2)<<'\n';
  if(!stream)throw FormatError("Could not write HUD export");
  std::cout<<"Exported "<<out["definitions"].size()<<" main-game HUD definitions\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

#include "halo2/source_tag_metadata_store.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
using namespace halo2;
namespace {void check(bool v,const char* why){if(!v)throw FormatError(why);}}
int main(int argc,char** argv)try{
  check(argc==3,"Supply ORIGINAL_DISC_MAPS ORIGINAL_MAP_PACK_MAPS");
  struct Case {const char* map;bool pack;std::uint32_t alias,owner;};
  // Literal original cache-directory pairs. Their address identity and
  // 84-byte owner extents were read independently by audit_rendering.Cache.
  const Case cases[]{
    {"08a_deltacliffs",false,0xf4681235u,0xf30010cdu},
    {"08b_deltacontrol",false,0xf7ec1676u,0xec830b0eu},
    {"08b_deltacontrol",false,0xf93117bbu,0xec830b0eu},
    {"containment",true,0xf0ec0f65u,0xf04f0ec8u}
  };
  for(const auto& c:cases){
    const auto path=std::filesystem::path(argv[c.pack?2:1])/(std::string(c.map)+".map");
    if(!std::filesystem::exists(path))return 77;
    auto cache=std::make_shared<Cache>(path,std::filesystem::path(argv[1]));
    const auto alias=cache->query_tag({.handle=TagHandle{c.alias}}),owner=cache->query_tag({.handle=TagHandle{c.owner}});
    check(alias && owner && alias->group=="shad" && owner->group=="shad" && alias->address==owner->address && alias->size==0 && owner->size==84,
        "Original vehicle shader alias directory identity differs");
    const auto a=cache->tag_payload({c.alias}),b=cache->tag_payload({c.owner});
    check(a.size()==84 && std::ranges::equal(a,b),"Original same-address shader payload differs");
    SourceTagMetadataStore store(cache,{"shad"});const auto services=store.providers();
    check(services.definition(c.alias).datum()==c.alias && services.definition(c.alias).size()==84,
        "Shader alias lost its original datum or owned extent");
    check(std::ranges::equal(services.read(alias->address,84),b),"Shader alias metadata lookup is ambiguous");
    const auto material=cache->load_material(c.alias),canonical=cache->load_material(c.owner);
    check(material.source_shader_datum==c.alias && material.name==alias->name && material.passes.size()==canonical.passes.size(),
        "Vehicle shader alias became a fallback material");
    for(unsigned i=0;i<material.passes.size();++i){
      const auto& x=material.passes[i];const auto& y=canonical.passes[i];
      check(x.program.words==y.program.words && x.vertex_shader==y.vertex_shader &&
          std::memcmp(&x.constants,&y.constants,sizeof(x.constants))==0,"Vehicle alias changed compiled shader constants or program");
    }
  }
  std::cout<<"PASS 4 original vehicle damage-shader aliases, preserving datum, payload, program and constants\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

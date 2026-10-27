#include "halo2/mcc_sound_metadata_publication.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void need(bool yes,const char *why){if(!yes)throw FormatError(why);}
template<class F>void rejects(F &&call,const char *why){bool rejected{};try{call();}catch(const FormatError&){rejected=true;}need(rejected,why);}
}
int main(int argc,char **argv)try {
  if(argc!=2 || !*argv[1] || !std::filesystem::is_regular_file(argv[1]))return 77;
  auto cache=std::make_shared<const Cache>(argv[1]);
  const auto publication=MccSoundMetadataPublication::create(cache);
  publication.require_owner(cache);
  auto other=std::make_shared<const Cache>(argv[1]);
  rejects([&]{publication.require_owner(other);},"Immutable sound publication accepted another Cache");
  const std::shared_ptr<const Cache> alias(cache.get(),[](const Cache*){});
  rejects([&]{publication.require_owner(alias);},"Immutable sound publication accepted a foreign strong ownership group");
  nlohmann::json sounds=nlohmann::json::array();unsigned pitches{};
  for(const auto &tag:cache->tags())if(tag.group=="snd!" &&
      (tag.name.find("midship_grav_lift")!=std::string::npos || tag.name.find("midship_activate_lift")!=std::string::npos)) {
    const auto first=publication.load(*cache,tag.handle());
    for(unsigned pitch=0;pitch<first.pitch_ranges.size();++pitch) {
      const auto definition=publication.load(*cache,tag.handle(),pitch);
      need(definition.datum==tag.datum && !definition.permutations.empty(),"Immutable converted lift sound lost its full identity or permutations");
      sounds.push_back({{"sound",tag.datum},{"name",tag.name},{"pitch",pitch},
          {"native_flags",Bytes(definition.sound).u16(0)},{"permutations",definition.permutations.size()}});
      ++pitches;
    }
    rejects([&]{(void)publication.load(*cache,{tag.datum^0x10000u});},"Immutable sound publication accepted a wrong full salt");
    rejects([&]{(void)publication.load(*cache,tag.handle(),UINT32_MAX);},"Immutable sound publication accepted an out-of-range pitch");
  }
  need(pitches>=2,"Prepared native map has no complete base/activation lift metadata");
  std::cout<<nlohmann::json{{"passed",true},{"map",cache->name()},{"lift_pitch_definitions",pitches},
      {"sounds",sounds},{"foreign_cache_and_ownership_rejected",true},{"source_runtime_created",false},
      {"world_epoch_fabricated",false},{"runtime_emulation",false},
      {"scope","Immutable complete native metadata conversion and owner bounds; actual World provider equality and Audio delivery are separate checks"}}.dump(2)<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

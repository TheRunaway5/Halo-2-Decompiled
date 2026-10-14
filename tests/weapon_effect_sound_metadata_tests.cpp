#include "halo2/effect_sound.hpp"
#include "halo2/muzzle_effects.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <array>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=3)throw FormatError("Supply MAP SHARED");
  if(!std::filesystem::exists(argv[1]))return 77;
  Cache cache(argv[1],argv[2]);
  // Literal Lockout upper/lower and brute upper/lower firing references.
  // All four event0 sound parts point to the same actual upper-fire bank.
  const std::array effects{0xe4e0036bu,0xe4e4036fu,0xec380ab3u,0xec390ab4u};
  unsigned forwarded{};
  for(auto datum:effects) {
    const auto tag=cache.load_source_tag_metadata(datum);
    const Bytes root(tag.bytes_at(tag.source_address(),0x1c));
    const Bytes events(tag.bytes_at(root.u32(0x18),root.u32(0x14)*0x38));
    if(events.u32(0x1c)<tag.source_address())++forwarded;
    const auto cues=fixed_effect_sound_cues(cache,datum);
    if(cues.size()!=1 || cues[0].sound!=0xe4e1036cu || cues[0].elapsed!=0 ||
        cues[0].gain!=1 || cues[0].event!=0)
      throw FormatError("Actual shared plasma firing sound metadata differs");
    const auto muzzle=cache.load_muzzle_effect(datum);
    if(muzzle.sounds.size()!=1 || muzzle.sounds[0].sound!=cues[0].sound || muzzle.systems.empty())
      throw FormatError("Shared firing sound prevented actual authored muzzle admission");
  }
  if(forwarded!=3)throw FormatError("Actual cross-tag firing part controls missing");
  std::cout<<"PASS four literal plasma firing effects, three cross-tag part banks, exact sound/event/gain and authored FP systems\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

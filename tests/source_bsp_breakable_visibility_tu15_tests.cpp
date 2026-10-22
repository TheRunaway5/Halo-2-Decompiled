#include "halo2/source_bsp_breakable_visibility_tu15.hpp"
#include "halo2/source_bsp_part_commands.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
// Declared component input banks, not a constructed source world or an
// original-produced explosion. Actual weapon/bitmap/pixel acceptance is separate.
struct Memory final:SourcePhysicsContactMemory {
  std::uint32_t pointer=0x71004000u;
  SourceBspBreakableVisibilityTu15 bytes;
  std::size_t extent=bytes.size();
  Memory(){bytes.fill(std::byte{255});}
  void read(std::uint32_t address,std::span<std::byte> output)const override {
    if(address==0x4f9b50u && output.size()==4){
      for(unsigned i=0;i<4;++i)output[i]=std::byte(pointer>>(i*8));return;
    }
    if(address!=0x71004000u || output.size()>extent)throw std::runtime_error("Unowned breakable fixture read");
    std::copy_n(bytes.begin(),output.size(),output.begin());
  }
  void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("Readback wrote source state");}
};
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw,"Missing visibility backing was accepted");}
unsigned commands(std::span<const std::byte> visibility,bool instance=false) {
  SourceBspCommandGeometry geometry;geometry.resident=true;geometry.parts.resize(1);
  const std::uint16_t type=1;std::memcpy(geometry.parts[0].data(),&type,2);
  const std::array geometry_bank{geometry};const std::array<std::int16_t,1> instances{0};
  const std::array materials{SourceBspCommandMaterial{1,0}};
  SourceBspCommandShader shader;shader.lod.thresholds={0};shader.layers={1};
  SourceBspPartCommandWorld world;world.clusters=world.definitions=geometry_bank;
  world.instance_definitions=instances;world.materials=materials;world.material_visibility=visibility;
  world.shader=[&](auto)->const SourceBspCommandShader&{return shader;};
  SourceCullDrawUnit unit{};unit.words[0]=3u|(instance?0x1000u:0u);
  unit.words[1]=0xffffffffu;unit.words[3]=(31u<<4)|(instance?0u:2047u<<18);unit.words[6]=0x10000u;
  const std::array units{unit};const std::array<std::uint8_t,1> coverage{255},overrides{255};
  SourceBspPartCommandContext context;context.mode=2;context.draw_count=1;
  context.coverage=coverage;context.override_slots=overrides;context.mask_arena_address=0x71000000u;
  context.player_multiplier=1;context.lod_view.multipliers.fill(1);
  SourceBspPartCommandState state;for(auto &bank:state.banks){bank.header[0]=4;bank.storage.resize(4);}
  source_bsp_append_part_commands(state,units,SourceBspRangeArena{},world,context);
  return state.banks[1].header[1];
}
}
int main()try {
  Memory memory;const auto intact=source_bsp_breakable_visibility_tu15(memory);
  check(commands(intact)==1 && commands(intact,true)==1,"Intact terrain/instance lost its light command");
  // Exact184000 lookup: terrain BSP0 starts at byte1, instance0 at byte201.
  memory.bytes[1]&=std::byte{0xfe};memory.bytes[0x201]&=std::byte{0xfe};
  memory.bytes[0x4203]=std::byte{0x57};
  const auto broken=source_bsp_breakable_visibility_tu15(memory);
  check(commands(broken)==0 && commands(broken,true)==0,"Broken terrain/instance retained an effect-light command");
  check(commands(intact)==1 && broken[0x4203]==std::byte{0x57},"Readback aliases later state or loses the final source bytes");
  check(source_bsp_material_visible(broken,1,-1,0),"Breakage leaked to another BSP bank");
  memory.bytes[1]|=std::byte{1};memory.bytes[0x201]|=std::byte{1};
  const auto restored=source_bsp_breakable_visibility_tu15(memory);
  check(commands(restored)==1 && commands(restored,true)==1,"Source reset/restore retained stale hidden materials");
  memory.pointer=0;rejects([&]{source_bsp_breakable_visibility_tu15(memory);});
  memory.pointer=0xffffffffu;rejects([&]{source_bsp_breakable_visibility_tu15(memory);});
  memory.pointer=0x71008000u;rejects([&]{source_bsp_breakable_visibility_tu15(memory);});
  memory.pointer=0x71004000u;memory.extent=0x4203;rejects([&]{source_bsp_breakable_visibility_tu15(memory);});
  std::cout<<"PASS current source-bank readback drives intact/broken/restored terrain and instance light commands; other BSP, owned-copy/trailing-byte and missing-bank controls; component inputs only\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

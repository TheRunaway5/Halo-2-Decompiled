#include "halo2/native_campaign_script_world_tu15.hpp"
#include "halo2/native_tu15_object_motion.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
NativeCampaignScriptFloatingPoint native_campaign_script_floating_point(PhysicsLiveWorldTu15& world) {
  const auto abi=world.prepare_source_call();
  const auto& fp=abi.floating_point.value();
  return {fp.x87_control_word,fp.mxcsr};
}
NativeCampaignScriptBank native_campaign_script_bank(PhysicsLiveWorldTu15& world,bool floating_point) {
  return NativeCampaignScriptBank(world.memory(),
      [&world](std::uint32_t address,std::span<const std::byte> bytes){world.write_source_call_input(address,bytes);},
      floating_point?native_campaign_script_floating_point(world):NativeCampaignScriptFloatingPoint{});
}
namespace {
// Reads the world bank; every write is a journaled source-call input.
class JournaledWorldMemory final : public SourcePhysicsContactMemory {
public:
  explicit JournaledWorldMemory(PhysicsLiveWorldTu15& world):world_(world){}
  void read(std::uint32_t address,std::span<std::byte> output)const override{world_.memory().read(address,output);}
  void write(std::uint32_t address,std::span<const std::byte> input)override{world_.write_source_call_input(address,input);}
private:
  PhysicsLiveWorldTu15& world_;
};
}
void native_campaign_object_set_velocity(PhysicsLiveWorldTu15& world,std::uint32_t object,float forward,float left,float up) {
  const auto fp=native_campaign_script_floating_point(world);
  JournaledWorldMemory memory(world);
  native_tu15_object_set_velocity(memory,object,forward,left,up,{fp.x87_control_word,fp.mxcsr});
}
}

// Track "services": binds ObjectFactoryServices to the native owners of the
// other factory tracks (see docs/NATIVE_OBJECT_FACTORY.md). Nothing here
// executes original code; an owner no track has ported stays a refusal.
#include "halo2/native_object_factory_services_tu15.hpp"
#include "halo2/native_object_factory_havok_tu15.hpp"
#include "halo2/native_object_factory_modelstate_tu15.hpp"
#include "halo2/native_object_factory_nodebuild_tu15.hpp"
#include "halo2/native_object_factory_objtypes_tu15.hpp"
#include "halo2/native_object_factory_placement_tu15.hpp"
#include "halo2/native_object_factory_unitactions_tu15.hpp"
#include "halo2/native_object_factory_units_tu15.hpp"
#include "halo2/native_object_factory_weapon_tu15.hpp"
#include "halo2/native_tu15_object_scale.hpp"
#include "native_object_factory_tu15_internal.hpp"

namespace halo2::native_object_factory_tu15 {
ObjectFactoryServices native_object_factory_services(std::uint16_t x87,std::uint32_t mxcsr) {
  using detail::M;
  ObjectFactoryServices s;
  s.mxcsr=mxcsr;s.x87_control=x87;
  const auto frame=[x87,mxcsr]{return NativeTu15PhysicsFrameFp{x87,0,mxcsr};};
  // modelstate
  s.animation_setup=[x87,mxcsr](M& m,std::uint32_t record,std::uint32_t graph,std::uint32_t model){
    return animation_manager_setup(m,record,graph,model,true,x87,mxcsr);};
  s.regions=[x87,mxcsr](M& m,std::uint32_t o,std::uint32_t mask){object_regions_initialize(m,o,mask,x87,mxcsr);};
  s.change_colors=[x87,mxcsr](M& m,std::uint32_t o,std::uint32_t mask,std::uint32_t colors){
    object_change_colors_initialize(m,o,mask,colors,x87,mxcsr);};
  s.function_colors=[x87,mxcsr](M& m,std::uint32_t o){object_function_colors(m,o,x87,mxcsr);};
  s.type_finish=[x87,mxcsr](M& m,std::uint32_t o){object_type_finish(m,o,x87,mxcsr);};
  // nodebuild
  s.node_rebuild=[frame](M& m,std::uint32_t o){auto fp=frame();object_rebuild_nodes(m,o,fp);};
  // placement
  s.cluster_location=[frame](M& m,std::uint32_t o,std::uint32_t out){return object_get_location(m,o,out,frame());};
  s.connect=[frame](M& m,std::uint32_t o,std::uint32_t location){auto fp=frame();object_reconnect_to_map(m,o,location,fp);};
  s.activate=[frame](M& m,std::uint32_t o){auto fp=frame();object_defer_physics(m,o,fp);};
  // weapon: item FA820 (FD100 -> B96B0 scale with its BF050 refresh), weapon 102AD0
  const NativeObjectFactoryFp factory_fp{x87,mxcsr};
  s.construct[0xfa820u]=[factory_fp,frame](M& m,std::uint32_t o,std::uint32_t,std::uint32_t){
    const ItemScaleService scale=[&m,frame](std::uint32_t object,std::uint32_t scale_bits,std::uint32_t seconds_bits){
      NativeTu15ObjectScaleServices services;
      services.interpolate=[&m](std::uint32_t object,std::uint32_t seconds_bits,NativeTu15PhysicsFrameFp& fp){
        object_interpolation_start(m,object,seconds_bits,fp);};
      services.refresh=[&m](std::uint32_t object,NativeTu15PhysicsFrameFp& fp){object_rebuild_nodes(m,object,fp);};
      auto fp=frame();
      native_tu15_object_scale(m,object,scale_bits,seconds_bits,fp,services);
    };
    return item_new(m,o,factory_fp,scale);};
  s.construct[0x102ad0u]=[factory_fp](M& m,std::uint32_t o,std::uint32_t,std::uint32_t){return weapon_new(m,o,factory_fp);};
  // objtypes: node rebuild for scenery animation start; 1CE340 phantom wake
  // is the havok track's owner (hkWorld add/remove phantom, 318550/318680).
  ObjectTypeServices types;types.fp=frame();
  types.node_rebuild=[frame](M& m,std::uint32_t o){auto fp=frame();object_rebuild_nodes(m,o,fp);};
  types.phantom_wake=[frame](M& m,std::uint32_t o){auto fp=frame();havok_phantom_wake(m,o,fp);};
  for(auto& [address,callback]:objtypes_construct_callbacks(types))s.construct.emplace(address,callback);
  for(auto& [address,callback]:objtypes_construct_failed_callbacks(types))s.construct_failed.emplace(address,callback);
  // units: unit/biped/vehicle chain callbacks and the 1157D0 node notification.
  s.validate[0xde9e0u]=[mxcsr](M& m,std::uint32_t request){biped_validate(m,request,mxcsr);};
  s.construct[0xc6440u]=[frame](M& m,std::uint32_t o,std::uint32_t request,std::uint32_t failed){
    return unit_new(m,o,request,failed,0,frame());};
  s.construct[0xdea60u]=[frame](M& m,std::uint32_t o,std::uint32_t request,std::uint32_t failed){
    return biped_new(m,o,request,failed,frame());};
  s.construct[0xf10e0u]=[frame](M& m,std::uint32_t o,std::uint32_t request,std::uint32_t failed){
    return vehicle_new(m,o,request,failed,frame());};
  s.construct_failed[0xc68b0u]=[](M& m,std::uint32_t o){unit_new_failed(m,o);};
  s.construct_failed[0xdea70u]=[](M& m,std::uint32_t o){biped_new_failed(m,o);};
  s.place[0xc6850u]=[frame](M& m,std::uint32_t o){unit_place(m,o,frame());};
  s.unit_nodes=[frame](M& m,std::uint32_t o,std::uint32_t count,std::uint32_t matrices){
    unit_nodes_changed(m,o,count,matrices,frame());};
  return s;
}

namespace {
// World view over a script bank: reads from its memory, stores through it.
struct BankWorld final:SourcePhysicsContactMemory {
  NativeCampaignScriptBank& bank;
  explicit BankWorld(NativeCampaignScriptBank& b):bank(b){}
  void read(std::uint32_t address,std::span<std::byte> output)const override{bank.memory().read(address,output);}
  void write(std::uint32_t address,std::span<const std::byte> input)override{bank.store(address,input);}
};
NativeTu15PhysicsFrameFp bank_fp(const NativeCampaignScriptBank& bank) {
  return {bank.floating_point().x87_control_word,0,bank.floating_point().mxcsr};
}
}

// 2ADE92: EDX = argument 0 (vehicle), ECX = argument 1 (mapping); AX result.
std::uint16_t script_vehicle_unload(NativeCampaignScriptBank& bank,std::uint32_t vehicle,std::uint32_t mapping) {
  BankWorld world(bank);
  return unit_actions::vehicle_unload(world,mapping,vehicle,bank_fp(bank));
}
// 2ADC59: DX = argument 1 (mode word), EAX = argument 0 (unit).
void script_unit_exit_vehicle(NativeCampaignScriptBank& bank,std::uint32_t unit,std::uint16_t mode) {
  BankWorld world(bank);
  unit_actions::unit_exit_vehicle(world,unit,mode,bank_fp(bank));
}
// 2AFE29: ECX = argument 0 (reference), EBX = NONE; 27EAB0 walks the
// reference with 27CAA0/27CB30.
void script_ai_vehicle_exit(NativeCampaignScriptBank& bank,std::uint32_t reference) {
  BankWorld world(bank);
  const auto next_actor=native_campaign_script_tu15::ai_reference_actors(bank,reference);
  unit_actions::ai_vehicle_exit(world,0xffffffffu,next_actor,bank_fp(bank));
}
}

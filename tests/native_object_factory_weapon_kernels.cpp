// Oracle harness kernels for track "weapon" (see the registry header). Frame
// out-parameters are bound to their declared bank addresses so every store
// the original makes to its caller's frame is compared in order.
#include "halo2/native_object_factory_weapon_tu15.hpp"
#include "native_object_factory_test_registry.hpp"

namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
namespace a=native_object_factory_tu15::weapon_animation;
using native_object_factory_tests::Register;
using Args=std::vector<std::uint32_t>;
using World=SourcePhysicsContactMemory;
a::FrameWord bound(World& world,std::uint32_t address){a::FrameWord word;word.bind(world,address);return word;}
bool byte(std::uint32_t value){return (value&0xffu)!=0;}

const Register search("weapon_tag_block_search",[](World& w,auto&,const Args& v){
  return a::tag_block_search(w,v.at(0),v.at(1),v.at(2));});
const Register child("weapon_graph_child",[](World& w,auto&,const Args& v){return a::graph_child(w,v.at(0),v.at(1));});
const Register animation("weapon_graph_animation",[](World& w,auto&,const Args& v){
  return a::graph_animation(w,v.at(0),v.at(1));});
const Register resident("weapon_graph_require_resident",[](World& w,auto&,const Args& v){
  a::graph_require_resident(w,v.at(0),v.at(1));return 0u;});
const Register reset("weapon_channels_reset",[](World& w,auto&,const Args& v){a::manager_reset_channels(w,v.at(0));return 0u;});
const Register resolve("weapon_resolve_keys",[](World& w,auto&,const Args& v){
  a::FrameWord out[4]{bound(w,v.at(3)),bound(w,v.at(3)+4u),bound(w,v.at(3)+8u),bound(w,v.at(3)+12u)};
  a::resolve_keys(w,v.at(4),v.at(0),v.at(1),v.at(2),v.at(5),out);return 0u;});
const Register iterate("weapon_mode_iterate",[](World& w,auto&,const Args& v){
  const auto at=v.at(0);
  a::ModeIterator it{bound(w,at),bound(w,at+4u),bound(w,at+8u),bound(w,at+12u),bound(w,at+16u),bound(w,at+20u),bound(w,at+24u)};
  auto out0=bound(w,v.at(2)),out1=bound(w,v.at(3)),out2=bound(w,v.at(1));
  return a::mode_iterate(w,it,out0,out1,out2);});
const Register entry("weapon_mode_entry",[](World& w,auto&,const Args& v){
  return a::mode_entry(w,v.at(1),v.at(0),v.at(2),v.at(3));});
const Register prefetch("weapon_mode_prefetch",[](World& w,auto&,const Args& v){
  a::mode_prefetch(w,v.at(1),v.at(2),v.at(3),v.at(4),byte(v.at(0)),byte(v.at(5)));return 0u;});
const Register fallbacks("weapon_mode_prefetch_fallbacks",[](World& w,auto&,const Args& v){
  a::mode_prefetch_fallbacks(w,v.at(2),v.at(1),v.at(3),v.at(4),byte(v.at(0)),byte(v.at(5)));return 0u;});
const Register graph_prefetch("weapon_graph_prefetch",[](World& w,auto&,const Args& v){
  a::graph_prefetch(w,v.at(1),v.at(0),v.at(2),v.at(3),byte(v.at(4)),byte(v.at(5)));return 0u;});
const Register manager_prefetch("weapon_manager_prefetch",[](World& w,auto&,const Args& v){
  a::manager_prefetch(w,v.at(0),v.at(3),v.at(1),v.at(2),byte(v.at(4)),byte(v.at(5)));return 0u;});
// Words: key3, graph, result, r0, r1, r2, p, q; the out-pointers are p, q, p.
const Register lookup("weapon_animation_lookup",[](World& w,auto&,const Args& v){
  auto result=bound(w,v.at(2));
  const std::uint32_t addresses[3]{v.at(6),v.at(7),v.at(6)};
  a::FrameWord outs[3];a::FrameWord* pointers[3]{};
  for(int n=0;n<3;++n)if(addresses[n]){outs[n]=bound(w,addresses[n]);pointers[n]=&outs[n];}
  a::animation_lookup(w,v.at(1),v.at(0),v.at(3),v.at(4),v.at(5),pointers[0],pointers[1],pointers[2],result);
  return v.at(2);});
const Register choose("weapon_choose_animation",[](World& w,auto&,const Args& v){
  a::FrameWord selected[4],resolved[4];
  for(std::uint32_t n=0;n<4u;++n){selected[n]=bound(w,v.at(1)+4u*n);resolved[n]=bound(w,v.at(2)+4u*n);}
  auto result=bound(w,v.at(7));  // words: manager, selected, resolved, a0, a1 (= a2), a3, flags, result
  return std::uint32_t(a::choose_animation(w,v.at(0),selected,resolved,v.at(3),v.at(4),v.at(4),v.at(5),v.at(6),result));});
const Register query_find("weapon_physical_query_find",[](World& w,auto&,const Args& v){
  auto out=bound(w,v.at(1));
  // Words: row, out, key, b (both row bytes), k4, k5, c (+E and +F), c2.
  a::physical_query_find(w,v.at(0),out,v.at(2),std::uint8_t(v.at(3)),std::uint8_t(v.at(3)),v.at(4),v.at(5),
      std::uint8_t(v.at(6)),std::uint8_t(v.at(6)),std::uint8_t(v.at(7)));
  return v.at(1);});
const Register query_select("weapon_physical_query_select",[](World& w,auto&,const Args& v){
  auto out=bound(w,v.at(2));
  // Words: key1, graph, out, key0, key2, state, b (a5, a6, a9, a10), a11; a7 = key0, a8 = state.
  a::physical_query_select(w,v.at(1),v.at(0),out,v.at(3),v.at(4),v.at(5),v.at(6),v.at(6),v.at(3),v.at(5),v.at(6),
      v.at(6),v.at(7));
  return v.at(2);});
const Register query_state("weapon_physical_query_state",[](World& w,auto& bank,const Args& v){
  auto out=bound(w,v.at(1)),flag=bound(w,v.at(4));
  a::physical_query_state(w,v.at(0),out,v.at(2),v.at(3),flag,bank.floating_point());
  return v.at(1);});
const Register selector("weapon_physical_query_selector",[](World& w,auto&,const Args& v){
  auto out=bound(w,v.at(1)),mapped=bound(w,v.at(6));
  a::physical_query_selector(w,v.at(0),out,v.at(2),v.at(3),v.at(4),v.at(5),mapped);
  return v.at(1);});
const Register stopped("weapon_channel_stopped",[](World& w,auto&,const Args& v){
  return std::uint32_t(a::channel_stopped(w,v.at(0)));});
const Register channel_animation("weapon_channel_animation",[](World& w,auto&,const Args& v){
  return a::channel_animation(w,v.at(0));});
const Register valid("weapon_channel_valid",[](World& w,auto&,const Args& v){return a::channel_valid(w,v.at(0));});
const Register finished("weapon_channel_finished",[](World& w,auto&,const Args& v){
  return std::uint32_t(a::channel_finished(w,v.at(0)));});
const Register start("weapon_channel_start",[](World& w,auto&,const Args& v){
  return std::uint32_t(a::channel_start(w,v.at(0),v.at(1),v.at(2),v.at(3),v.at(4),std::uint8_t(v.at(5)),
      std::uint8_t(v.at(6)),std::uint8_t(v.at(7))));});
const Register events("weapon_channel_mark_events",[](World& w,auto& bank,const Args& v){
  a::channel_mark_events(w,v.at(0),bank.floating_point());return 0u;});
const Register frame("weapon_channel_set_frame",[](World& w,auto& bank,const Args& v){
  a::channel_set_frame(w,v.at(0),v.at(1),bank.floating_point());return 0u;});
const Register alias("weapon_variant_alias",[](World& w,auto&,const Args& v){
  auto out=bound(w,v.at(1));a::variant_alias(w,v.at(0),out,v.at(2));return v.at(1);});
const Register random_kernel("weapon_random_variant",[](World& w,auto& bank,const Args& v){
  auto out=bound(w,v.at(1));a::random_variant(w,v.at(0),out,v.at(2),byte(v.at(3)),bank.floating_point());return v.at(1);});
const Register variant("weapon_manager_variant",[](World& w,auto& bank,const Args& v){
  auto out=bound(w,v.at(1));a::manager_variant(w,v.at(0),out,v.at(2),bank.floating_point());return v.at(1);});
const Register start_channel("weapon_manager_start_channel",[](World& w,auto& bank,const Args& v){
  return std::uint32_t(a::manager_start_channel(w,v.at(0),v.at(1),v.at(2),v.at(3),v.at(4),std::uint8_t(v.at(5)),
      std::uint8_t(v.at(6)),std::uint8_t(v.at(7)),bank.floating_point()));});
const Register set_state("weapon_manager_set_state",[](World& w,auto& bank,const Args& v){
  return std::uint32_t(a::manager_set_state(w,v.at(1),v.at(0),v.at(2),v.at(3),v.at(4),v.at(5),v.at(6),bank.floating_point()));});
// 169863's 1D5320 call on an MCC cache (the oracle applies the translated
// route's joins) and on a TU1.5 cache (no joins).
std::uint32_t first_person_set_state(World& w,const Args& v,const a::Fp& fp,bool mcc) {
  const a::QueryNamespace names(mcc);
  return std::uint32_t(a::manager_set_state(w,v.at(1),v.at(0),v.at(2),v.at(3),v.at(4),v.at(5),v.at(6),fp,
      a::QueryOrigin::first_person_state));
}
const Register set_state_first_person("weapon_manager_set_state_first_person",[](World& w,auto& bank,const Args& v){
  return first_person_set_state(w,v,bank.floating_point(),true);});
const Register set_state_first_person_tu15("weapon_manager_set_state_first_person_tu15",[](World& w,auto& bank,const Args& v){
  return first_person_set_state(w,v,bank.floating_point(),false);});
const Register weapon_state("weapon_set_state",[](World& w,auto& bank,const Args& v){
  return std::uint32_t(f::weapon_set_state(w,v.at(0),v.at(1),byte(v.at(2)),bank.floating_point()));});
const Register weapon_new("weapon_new",[](World& w,auto& bank,const Args& v){
  return std::uint32_t(f::weapon_new(w,v.at(0),bank.floating_point()));});
const Register owner("weapon_owner",[](World& w,auto&,const Args& v){return f::weapon_owner(w,v.at(0));});
}

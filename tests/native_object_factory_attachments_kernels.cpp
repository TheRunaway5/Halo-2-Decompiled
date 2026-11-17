// Oracle harness kernels for track "attachments" (see the registry header).
// Argument words follow tools/native_object_factory_cases_attachments.py.
// Output pointers the cases pass are world addresses: every original store
// through them is compared.
#include "halo2/native_object_factory_attachments_tu15.hpp"
#include "native_object_factory_test_registry.hpp"
#include <array>

namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
using Output=f::OriginalOutput;
std::uint16_t x87(const NativeCampaignScriptBank& bank){return bank.floating_point().x87_control_word;}
std::uint32_t mxcsr(const NativeCampaignScriptBank& bank){return bank.floating_point().mxcsr;}

const Register looping_sound("object_looping_sound_attachment_create",[](auto& world,auto&,const auto& a){
  return f::object_looping_sound_attachment_create(world,a.at(0),a.at(1),a.at(2),a.at(3));});
const Register variable("object_function_variable",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)},slot{a.at(3)};
  return std::uint32_t(f::object_function_variable(world,a.at(0),a.at(1),value,slot,mxcsr(bank)));});
const Register base_value("object_base_function_value",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)},active{a.at(3)};
  return std::uint32_t(f::object_base_function_value(world,a.at(0),a.at(1),value,active,x87(bank),mxcsr(bank)));});
const Register type_value("object_type_function_value",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)},active{a.at(3)};
  return std::uint32_t(f::object_type_function_value(world,a.at(0),a.at(1),value,active,x87(bank),mxcsr(bank)));});
const Register reference_value("function_reference_value",[](auto& world,auto& bank,const auto& a){
  return f::function_reference_value(world,a.at(0),a.at(1),a.at(2),x87(bank),mxcsr(bank));});
const Register evaluate("object_function_evaluate",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)};
  return std::uint32_t(f::object_function_evaluate(world,a.at(0),a.at(1),value,x87(bank),mxcsr(bank)));});
const Register function_value("object_function_value",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)};
  return std::uint32_t(f::object_function_value(world,a.at(0),a.at(1),value,x87(bank),mxcsr(bank)));});
// The type-level callbacks (C9070, E0900, 1046C0) and their XMM0 helpers.
const Register unit_value("unit_function_value",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)},active{a.at(3)};
  return std::uint32_t(f::unit_function_value(world,a.at(0),a.at(1),value,active,x87(bank),mxcsr(bank)));});
const Register biped_value("biped_function_value",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)},active{a.at(3)};
  return std::uint32_t(f::biped_function_value(world,a.at(0),a.at(1),value,active,x87(bank),mxcsr(bank)));});
const Register weapon_value("weapon_function_value",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)},active{a.at(3)};
  return std::uint32_t(f::weapon_function_value(world,a.at(0),a.at(1),value,active,x87(bank),mxcsr(bank)));});
// 10DE40 again, over the unit, biped and weapon fixtures.
const Register unit_type_value("unit_type_function_value",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)},active{a.at(3)};
  return std::uint32_t(f::object_type_function_value(world,a.at(0),a.at(1),value,active,x87(bank),mxcsr(bank)));});
const Register vehicle_value("vehicle_function_value",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)},active{a.at(3)};
  return std::uint32_t(f::vehicle_function_value(world,a.at(0),a.at(1),value,active,x87(bank),mxcsr(bank)));});
// 100E70, 10BE20, 11E490 (+1DB800) and 24B430 (2026-11-15).
const Register projectile_value("projectile_function_value",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)},active{a.at(3)};
  return std::uint32_t(f::projectile_function_value(world,a.at(0),a.at(1),value,active,mxcsr(bank)));});
const Register device_value("device_function_value",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)},active{a.at(3)};
  return std::uint32_t(f::device_function_value(world,a.at(0),a.at(1),value,active,x87(bank),mxcsr(bank)));});
const Register machine_value("machine_function_value",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)},active{a.at(3)};
  return std::uint32_t(f::machine_function_value(world,a.at(0),a.at(1),value,active,mxcsr(bank)));});
const Register component_value("physics_component_function_value",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)},active{a.at(3)};
  return std::uint32_t(f::physics_component_function_value(world,a.at(0),a.at(1),value,active,mxcsr(bank)));});
const Register mode9_value("weapon_mode9_value",[](auto& world,auto& bank,const auto& a){
  return f::weapon_mode9_value(world,a.at(0),mxcsr(bank));});
// 10DE40 over projectiles, devices, machines and crates.
const Register device_type_value("device_type_function_value",[](auto& world,auto& bank,const auto& a){
  Output value{a.at(2)},active{a.at(3)};
  return std::uint32_t(f::object_type_function_value(world,a.at(0),a.at(1),value,active,x87(bank),mxcsr(bank)));});
const Register unit_ratio("unit_function_ratio",[](auto& world,auto& bank,const auto& a){
  return f::unit_function_ratio(world,a.at(0),mxcsr(bank));});
const Register unit_fraction("unit_animation_fraction",[](auto& world,auto& bank,const auto& a){
  return f::unit_animation_fraction(world,a.at(0),mxcsr(bank));});
const Register channel_fraction("animation_channel_fraction",[](auto& world,auto& bank,const auto& a){
  return f::animation_channel_fraction(world,a.at(0),mxcsr(bank));});
const Register trigger_fraction("weapon_trigger_fraction",[](auto& world,auto& bank,const auto& a){
  return f::weapon_trigger_fraction(world,a.at(0),a.at(1),mxcsr(bank));});
// The console clock: 13BB10's unpacked time (year, month, day, hour, minute,
// second) after the call that filled it.
f::LocalTime clock(const std::vector<std::uint32_t>& a,std::size_t first){
  return {a.at(first),a.at(first+1u),a.at(first+2u),a.at(first+3u),a.at(first+4u),a.at(first+5u)};
}
const Register clock_hour("clock_hour_value",[](auto&,auto& bank,const auto& a){
  return f::clock_hour_value(clock(a,0),mxcsr(bank));});
const Register clock_minute("clock_minute_value",[](auto&,auto& bank,const auto& a){
  return f::clock_minute_value(clock(a,0).minute,mxcsr(bank));});
const Register clock_second("clock_second_value",[](auto&,auto& bank,const auto& a){
  return f::clock_minute_value(clock(a,0).second,mxcsr(bank));});
const Register holiday("local_holiday",[](auto&,auto&,const auto& a){
  return f::local_holiday(clock(a,1));});
const Register holiday_value("holiday_function_value",[](auto&,auto&,const auto& a){
  return f::holiday_function_value(a.at(0));});
const Register profile("contrail_profile_advance",[](auto& world,auto&,const auto& a){
  f::contrail_profile_advance(world,a.at(0));return 0u;});
const Register contrail("object_contrail_attachment_create",[](auto& world,auto& bank,const auto& a){
  return f::object_contrail_attachment_create(world,a.at(0),a.at(1),a.at(2),x87(bank),mxcsr(bank));});

std::array<std::uint32_t,4> words4(SourcePhysicsContactMemory& world,std::uint32_t at){
  std::array<std::uint32_t,4> out{};
  for(std::uint32_t n=0;n<4u;++n){std::array<std::byte,4> b{};world.read(at+4u*n,b);
    out[n]=std::uint32_t(b[0])|std::uint32_t(b[1])<<8|std::uint32_t(b[2])<<16|std::uint32_t(b[3])<<24;}
  return out;
}
// 134870's two {weight, rgb} operands and change color live on the
// original's stack; the cases place them in world memory, read here first.
const Register blend("light_color_blend",[](auto& world,auto& bank,const auto& a){
  const auto first=words4(world,a.at(1)),second=words4(world,a.at(2));
  std::array<std::uint32_t,3> color{};
  if(a.at(3)){const auto c=words4(world,a.at(3));color={c[0],c[1],c[2]};}
  f::light_color_blend(world,a.at(0),first,second,a.at(3)?&color:nullptr,a.at(4),a.at(5),mxcsr(bank));return 0u;});
const Register interpolate("color_interpolate",[](auto& world,auto& bank,const auto& a){
  f::color_interpolate(world,a.at(0),a.at(1),a.at(2),a.at(3),a.at(4),mxcsr(bank));return 0u;});
const Register scales("light_function_scales",[](auto& world,auto& bank,const auto& a){
  f::light_function_scales(world,a.at(0),a.at(1),a.at(2),a.at(3),a.at(4),a.at(5),x87(bank),mxcsr(bank));return 0u;});
// 13E6E0: ECX colour function reference, stack input; EAX the colour.
const Register color_function("color_function_value",[](auto& world,auto& bank,const auto& a){
  return f::color_function_value(world,a.at(0),a.at(1),x87(bank),mxcsr(bank));});
const Register light_update("light_update",[](auto& world,auto& bank,const auto& a){
  f::light_update(world,a.at(0),x87(bank),mxcsr(bank));return 0u;});
const Register light("object_light_attachment_create",[](auto& world,auto& bank,const auto& a){
  return f::object_light_attachment_create(world,a.at(0),a.at(1),a.at(2),a.at(3),a.at(4),x87(bank),mxcsr(bank));});
}

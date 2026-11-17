// Oracle harness kernels for track "modelstate" (see the registry header).
// Argument words follow tools/native_object_factory_cases_modelstate.py.
#include "halo2/native_object_factory_modelstate_tu15.hpp"
#include "native_object_factory_test_registry.hpp"
#include <array>

namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
std::uint16_t x87(const NativeCampaignScriptBank& bank){return bank.floating_point().x87_control_word;}
std::uint32_t mxcsr(const NativeCampaignScriptBank& bank){return bank.floating_point().mxcsr;}

const Register rgb_to_hsv("color_rgb_to_hsv",[](auto& world,auto& bank,const auto& a){
  return f::color_rgb_to_hsv(world,a.at(0),a.at(1),mxcsr(bank));});
const Register hsv_to_rgb("color_hsv_to_rgb",[](auto& world,auto& bank,const auto& a){
  return f::color_hsv_to_rgb(world,a.at(0),a.at(1),mxcsr(bank));});
const Register pack("color_pack",[](auto& world,auto& bank,const auto& a){
  return f::color_pack(world,a.at(0),x87(bank));});
const Register channel("object_color_channel",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::object_color_channel(world,a.at(0),a.at(1),a.at(2)));});
const Register publish("object_color_publish",[](auto& world,auto& bank,const auto& a){
  f::object_color_publish(world,a.at(0),(a.at(1)&0xffu)!=0,x87(bank));return 0u;});
const Register change_colors("object_change_colors_initialize",[](auto& world,auto& bank,const auto& a){
  f::object_change_colors_initialize(world,a.at(0),a.at(1),a.at(2),x87(bank),mxcsr(bank));return 0u;});
const Register function_colors("object_function_colors",[](auto& world,auto& bank,const auto& a){
  return f::object_function_colors(world,a.at(0),x87(bank),mxcsr(bank));});
const Register region_bank("object_region_bank",[](auto& world,auto&,const auto& a){
  f::object_region_bank(world,a.at(0),a.at(1),a.at(2),a.at(3),a.at(4));return 0u;});
const Register permutations("model_region_permutations_select",[](auto& world,auto& bank,const auto& a){
  f::model_region_permutations_select(world,a.at(0),std::int32_t(a.at(1)),a.at(2),a.at(3),a.at(4),x87(bank),mxcsr(bank));
  return 0u;});
const Register effect_restart("object_region_effect_restart",[](auto& world,auto&,const auto& a){
  f::object_region_effect_restart(world,a.at(0));return 0u;});
// The original's EAX names the caller's prior-state bytes; the fixture keeps
// 32 of them readable.
const Register physics_check("object_region_physics_check",[](auto& world,auto& bank,const auto& a){
  std::array<std::uint8_t,32> previous{};
  for(std::uint32_t n=0;n<previous.size();++n){std::array<std::byte,1> b{};world.read(a.at(0)+n,b);previous[n]=std::uint8_t(b[0]);}
  f::object_region_physics_check(world,a.at(1),previous,x87(bank),mxcsr(bank));return 0u;});
const Register regions("object_regions_initialize",[](auto& world,auto& bank,const auto& a){
  f::object_regions_initialize(world,a.at(0),a.at(1),x87(bank),mxcsr(bank));return 0u;});
const Register manager_setup("animation_manager_setup",[](auto& world,auto& bank,const auto& a){
  return std::uint32_t(f::animation_manager_setup(world,a.at(0),a.at(1),a.at(2),(a.at(3)&0xffu)!=0,x87(bank),mxcsr(bank)));});
const Register widgets("object_widgets_create",[](auto& world,auto&,const auto& a){
  f::object_widgets_create(world,a.at(0));return 0u;});
const Register attachments("object_attachments_create",[](auto& world,auto& bank,const auto& a){
  f::object_attachments_create(world,a.at(0),x87(bank),mxcsr(bank));return 0u;});
const Register type_finish("object_type_finish",[](auto& world,auto& bank,const auto& a){
  f::object_type_finish(world,a.at(0),x87(bank),mxcsr(bank));return 0u;});
}

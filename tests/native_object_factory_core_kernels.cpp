// Oracle harness kernels for track "core" (see the registry header).
#include "halo2/native_object_factory_core_tu15.hpp"
#include "native_object_factory_test_registry.hpp"

namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
const Register placement("placement_data_new",[](auto& world,auto&,const auto& a){
  f::placement_data_new(world,a.at(3),a.at(0),a.at(1),a.at(2));return 0u;});
const Register position("object_world_position",[](auto& world,auto& bank,const auto& a){
  f::object_world_position(world,a.at(0),a.at(1),bank.floating_point().mxcsr);return a.at(1);});
const Register spatial("object_refresh_spatial",[](auto& world,auto&,const auto& a){
  f::object_refresh_spatial(world,a.at(0));return 0u;});
const Register nodes("object_notify_nodes",[](auto& world,auto&,const auto& a){
  f::object_notify_nodes(world,a.at(0));return 0u;});
const Register pressure("physics_memory_pressure",[](auto& world,auto& bank,const auto&){
  return f::physics_memory_pressure(world,bank.floating_point().mxcsr);});
const Register reserve("physics_memory_reserve",[](auto& world,auto& bank,const auto&){
  f::physics_memory_reserve(world,bank.floating_point().mxcsr);return 0u;});
}

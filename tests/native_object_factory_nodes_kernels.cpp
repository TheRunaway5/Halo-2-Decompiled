// Oracle harness kernels for track "nodes" (see the registry header).
#include "halo2/native_object_factory_nodes_tu15.hpp"
#include "native_object_factory_test_registry.hpp"

namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
const Register has_animation("object_has_animation",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::object_has_animation(world,a.at(0)));});
const Register node_matrix("object_node_matrix",[](auto& world,auto&,const auto& a){
  return f::object_node_matrix(world,a.at(1),a.at(0));});
}

// Native object node owners (track "nodes"): template for factory tracks.
// Each owner replicates its TU1.5 routine; the oracle cases live in
// tools/native_object_factory_cases_nodes.py and the harness kernels in
// tests/native_object_factory_nodes_kernels.cpp.
#include "halo2/native_object_factory_nodes_tu15.hpp"
#include "native_object_factory_tu15_internal.hpp"

namespace halo2::native_object_factory_tu15 {
using namespace detail;

bool object_has_animation(const M& m,std::uint32_t object) {
  return r16(m,object_body(m,object)+0x12au)!=0xffffu;
}

std::uint32_t object_node_matrix(const M& m,std::uint32_t object,std::uint32_t node) {
  const auto body=object_body(m,object);
  return sx16(std::uint16_t(node))*0x34u+sx16(r16(m,body+0x116u))+body;
}
}

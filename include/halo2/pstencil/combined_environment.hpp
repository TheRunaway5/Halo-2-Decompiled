#pragma once
#include "halo2/cache.hpp"
namespace halo2::pstencil {
// Exact current Chief visor consumers, including the texture-combined LODs.
// Unknown controls, implementations and vertex interfaces remain unmodified.
bool supports_combined_visor_environment(const Material&);
const Material* combined_visor_environment(const Material&);
void suppress_combined_visor_environment(PixelProgram&);
// Reuse a validated current independent environment interface with this
// visor's actual bump/cube, UV transforms and source owner. This own companion
// is scheduled only in pStencil; it uses the recovered response and current
// texture-accumulation alpha. It does not establish complete 2003 MRT alpha.
Material separate_visor_environment(const Material& visor,const Material& independent);
void trace_visor_environment(std::uint64_t frame,unsigned lod,unsigned vertices);
}

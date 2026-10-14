#include "halo2/source_particle_sampler.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
std::vector<SourceParticleSamplerWrite> source_constant_particle_sampler_writes(
    const ShaderAnimation &a,ShaderRange selected) {
  const auto range=[](const auto &items,ShaderRange r) {
    if(r.first()>items.size() || r.count()>items.size()-r.first())
      throw FormatError("particle sampler mapping outside original block");
    return std::span(items).subspan(r.first(),r.count());
  };
  std::vector<SourceParticleSamplerWrite> result;
  // TU1.5 property table47359C. These are SDK indices, not matrix lanes.
  constexpr std::array<unsigned,4> states{22,23,25,24};
  for(const auto &mapping:range(a.mappings,selected)) {
    const unsigned stage=mapping.destination&255u,property=mapping.destination>>8u;
    if(stage>=4 || property<4 || property>7 || mapping.component!=1 || mapping.source>=a.parameters.size())
      throw FormatError("particle sampler requires its original nonconstant property owner");
    const auto refs=range(a.references,a.parameters[mapping.source]);
    if(refs.size()!=1 || refs[0].transform!=0 || refs[0].function<0 || std::size_t(refs[0].function)>=a.overlays.size())
      throw FormatError("particle sampler requires its original transformed function owner");
    const auto &overlay=a.overlays[refs[0].function];const Bytes curve(overlay.curve);
    if(overlay.input || overlay.range || overlay.period!=1 || curve.size()!=28 ||
        curve.u32(0)!=1 || curve.u32(4)!=curve.u32(8) || !std::isfinite(curve.f32(4)) ||
        curve.u32(12) || curve.u32(16) || curve.u32(20)!=0x3f800000u || curve.u32(24)!=0x3f800000u)
      throw FormatError("particle sampler requires its completed original function value");
    result.push_back({stage,states[property-4],curve.u32(4)});
  }
  return result;
}
void source_apply_particle_sampler_writes(SourceParticleSamplerBank &bank,
    std::span<const SourceParticleSamplerWrite> writes) {
  for(const auto &write:writes)if(write.stage>=4 || write.state<22 || write.state>25)
    throw FormatError("particle sampler write outside original bump state");
  for(const auto &write:writes)bank[write.stage][write.state-22]=write.value;
}
ShaderVector source_particle_bump_matrix(const SourceParticleSamplerBank &bank,unsigned stage) {
  if(stage==0 || stage>=4)throw FormatError("particle bump matrix has no preceding texture stage");
  const auto &v=bank[stage];
  return {std::bit_cast<float>(v[0]),std::bit_cast<float>(v[1]),
          std::bit_cast<float>(v[3]),std::bit_cast<float>(v[2])};
}
} // namespace halo2

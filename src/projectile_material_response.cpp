#include "halo2/projectile_material_response.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cstring>
#include <cmath>
#include <limits>
#include <vector>

namespace halo2 {
namespace {
template<class T>T get(std::span<const std::byte> data,std::size_t offset){
  if(offset>data.size()||sizeof(T)>data.size()-offset)
    throw FormatError("Projectile material response backing is truncated");
  T value;std::memcpy(&value,data.data()+offset,sizeof value);return value;
}
}
std::optional<std::size_t> source_projectile_material_response(
    std::span<const std::byte> rows,std::span<const std::int16_t> parents,
    std::int16_t material){
  if(rows.size()%0x58||rows.size()/0x58>std::size_t(std::numeric_limits<std::int32_t>::max())||
      parents.size()>std::size_t(std::numeric_limits<std::int32_t>::max()))
    throw FormatError("Invalid projectile material response bank");
  // The executable does not guard malformed cycles. This owned-data adapter
  // rejects them without inventing a fallback/default response.
  std::vector<bool> visited(parents.size());
  while(material>=0&&std::size_t(material)<parents.size()){
    const auto index=std::size_t(material);
    if(visited[index])throw FormatError("Cyclic projectile material parent bank");
    visited[index]=true;
    for(std::size_t i=0;i<rows.size()/0x58;++i)
      if(get<std::int16_t>(rows,i*0x58+0x10)==material)return i;
    material=parents[index];
  }
  return std::nullopt;
}
std::int16_t choose_source_projectile_response(std::span<const std::byte> row,
    const SourceProjectileResponseInputs&in,const SourceProjectileResponseServices&s){
  if(row.size()!=0x58)throw FormatError("Projectile response row must retain88 source bytes");
  const auto chance=get<float>(row,0x18);
  bool eligible=chance>0.f;
  const auto angle_high=get<float>(row,0x20);
  // UCOMISS/LAHF/TEST/JNP skips signed zero only. COMISS/JB rejects
  // unordered lower/upper comparisons exactly like an out-of-range value.
  if(angle_high!=0.f){
    if(eligible){if(!in.angle)throw FormatError("Original projectile impact angle owner unavailable");
      eligible=*in.angle>=get<float>(row,0x1c)&&angle_high>=*in.angle;}
  }
  const auto speed_high=get<float>(row,0x28);
  if(speed_high!=0.f){
    if(eligible){if(!in.speed)throw FormatError("Original projectile impact speed owner unavailable");
      eligible=*in.speed>=get<float>(row,0x24)&&speed_high>=*in.speed;}
  }
  const auto flags=get<std::uint16_t>(row,0x16);
  auto unit_hit=[&]{if(in.collision_type!=4)return false;
    if(!s.object_type)throw FormatError("Original projectile target type owner unavailable");
    return ((1u<<(s.object_type()&31u))&3u)!=0;
  };
  if((flags&2)&&eligible)eligible=!unit_hit();
  if((flags&1)&&eligible)eligible=unit_hit();
  // COMISS one,chance/JBE skips the RNG branch for >=one and unordered.
  if(s.one>chance&&eligible){
    if(!s.rng)throw FormatError("Original projectile response RNG owner unavailable");
    auto&state=s.rng();state=state*0x19660du+0x3c6ef35fu;
    // FILD high16 / FMUL under the measured x87 PC53 context. Both
    // operands fit exactly in binary64; there is no binary32 store before
    // the original comparison. The high16 value never needs correction.
    volatile double random=double(state>>16)*double(s.rng_scale);
    eligible=double(chance)>random;
  }
  return get<std::int16_t>(row,eligible?0x14:2);
}
} // namespace halo2

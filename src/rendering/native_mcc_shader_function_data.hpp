#pragma once
#include "halo2/shader_animation.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace halo2::detail {
// The existing retained material library already owns the exact Xbox lookup
// samples. Gameplay17F790/17F8A0 consume the same tables as native materials.
  // Keep existing cold rows and their backing; fill only missing complete rows.
inline void mount_native_mcc_shader_functions(SourceRuntimeMemory& memory,
    const ShaderFunctionTables& tables) {
  constexpr std::uint32_t base=0x4752a8u;
  // The authenticated TU1.5 lookup extent is12 periodic plus8 transition
  // rows. The retained28KiB file also contains a tail used by the existing
  // material importer; that tail is not a gameplay table.47A2A8 already owns
  // writable placement state and must remain outside this readonly binding.
  std::array<std::array<std::byte,1024>,20> rows{};
  std::uint64_t fingerprint=14695981039346656037ull;
  for(unsigned row=0;row<28;++row)
    for(unsigned i=0;i<1024;++i) {
      const auto value=row<12?tables.periodic[row][i]:tables.transition[row-12][i];
      if(row<rows.size())rows[row][i]=std::byte(value);
      fingerprint=(fingerprint^value)*1099511628211ull;
    }
  if(fingerprint!=0x22708b4cc478ddb7ull)
    throw std::invalid_argument("Native gameplay requires authenticated retained function samples");
  std::array<bool,20> present{};
  // Validate every overlap before publishing anything. The original partial
  // owners are complete rows. The multiplayer provider also retains row8
  // with its original writable .data access. Authenticate that exact owner
  // and its initial bytes; never replace it or change its protection.
  for(const auto& region:memory.checkpoint().regions) {
    const auto end=std::uint64_t(region.address)+region.bytes.size();
    if(end<=base || region.address>=base+20u*1024u)continue;
    const bool authored_writable_row_eight=region.address==base+8u*1024u &&
        region.access==SourceRuntimeMemory::Access::ReadWrite &&
        region.owner=="native TU1.5 original round-reset response curve eight 1024 byte samples";
    if(region.address<base || (region.address-base)%1024u || region.bytes.size()!=1024u ||
        (region.access!=SourceRuntimeMemory::Access::ReadOnly && !authored_writable_row_eight) || region.page_bytes ||
        !region.committed.empty())
      throw std::invalid_argument("Native function samples overlap a different backing owner at"+
          std::to_string(region.address)+" bytes="+std::to_string(region.bytes.size())+" owner="+region.owner);
    const auto row=(region.address-base)/1024u;
    if(present[row] || !std::ranges::equal(region.bytes,rows[row]))
      throw std::invalid_argument("Native function samples differ from retained material data");
    present[row]=true;
  }
  for(unsigned row=0;row<rows.size();++row)if(!present[row])
    memory.mount(base+row*1024u,rows[row],SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 retained material function row "+std::to_string(row));
}
}

#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
struct SourceObjectModelResourceState {std::uint32_t object_pool,tag_table;};
struct SourceObjectModelResourceServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
};
struct SourceObjectModelResourceStore {
  std::uint32_t instruction,address,before,after;
  friend bool operator==(const SourceObjectModelResourceStore &,
                         const SourceObjectModelResourceStore &)=default;
};
using SourceObjectModelResourceObserver=std::function<void(const SourceObjectModelResourceStore &)>;
// CompleteC0890 (u5C19D0). Reads the actual low16 header/body and authored
// definition→hlmt→mode/collision references. No salt gate, ready flag or
// default output. Caller retains both four-byte output addresses, including
// their possible aliasing with source backing.
bool source_object_model_resources(const SourceObjectModelResourceState &,
    std::uint32_t object,std::uint32_t mode_output,std::uint32_t collision_output,
    const SourceObjectModelResourceServices &,const SourceObjectModelResourceObserver & = {});
} // namespace halo2

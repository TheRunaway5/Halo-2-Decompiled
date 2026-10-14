#pragma once
#include <compare>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace halo2 {
// Live engine address. Original image addresses and salted object identities
// are separate values; neither can implicitly become a native address.
struct NativeRuntimeAddress {
  std::uint64_t value{};
  constexpr explicit NativeRuntimeAddress(std::uint64_t address=0):value(address) {}
  constexpr bool is_null()const{return value==0;}
  constexpr NativeRuntimeAddress operator+(std::size_t offset)const {
    if(offset>std::numeric_limits<std::uint64_t>::max()-value)
      throw std::out_of_range("Native runtime address wraps64 bits");
    return NativeRuntimeAddress{value+offset};
  }
  auto operator<=>(const NativeRuntimeAddress&)const=default;
};
static_assert(sizeof(NativeRuntimeAddress)==8);
static_assert(!std::is_convertible_v<NativeRuntimeAddress,std::uint32_t>);
} // namespace halo2

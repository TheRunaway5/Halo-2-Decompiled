#include "halo2/native_tu15_function_power_special.hpp"
#include <stdexcept>
namespace halo2 {
SourcePowerSpecialTu15Result native_tu15_authored_function_infinite_power(
    std::uint64_t base,std::uint64_t exponent){
  constexpr std::uint64_t magnitude=0x7fffffffffffffffull,infinity=0x7ff0000000000000ull,
    one=0x3ff0000000000000ull;
  const auto absolute_base=base&magnitude,absolute_exponent=exponent&magnitude;
  if(absolute_base>infinity||absolute_exponent>infinity||
      (absolute_base!=infinity&&absolute_exponent!=infinity))
    throw std::invalid_argument("Native TU1.5 infinite power requires a genuine non-NaN infinite operand");
  if(!(exponent>>63u)&&absolute_exponent)
    return source_power_special_positive_tu15(base,exponent);
  if(!absolute_exponent)return{one,false};
  if(absolute_exponent==infinity){
    if(absolute_base==one)return source_power_special_positive_tu15(base,infinity);
    return{absolute_base<one?infinity:0u,false};
  }
  // Only the infinite-base case remains. The source positive owner already
  // authenticates finite integer parity and the sign; negative powers return
  // the corresponding signed zero.
  const auto positive=source_power_special_positive_tu15(base,absolute_exponent);
  return{positive.result_bits&0x8000000000000000ull,false};
}
} // namespace halo2

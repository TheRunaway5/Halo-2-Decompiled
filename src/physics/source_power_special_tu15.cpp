#include "halo2/source_power_special_tu15.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
constexpr std::uint64_t magnitude=0x7fffffffffffffffull;
constexpr std::uint64_t infinity=0x7ff0000000000000ull;
constexpr std::uint64_t one=0x3ff0000000000000ull;
// Semantic integer/parity classification corresponding to37D8E3. Its original
// rounding/helper status is inside the caller's discarded temporary FPU state.
bool odd_integer(std::uint64_t value){
  const int exponent=int((value>>52)&0x7ffu)-1023;
  if(exponent<0||exponent>52)return false;
  const unsigned shift=52-unsigned(exponent);
  const auto significand=(value&0x000fffffffffffffull)|(1ull<<52);
  return !(significand&((1ull<<shift)-1))&&((significand>>shift)&1u);
}
}
SourcePowerSpecialTu15Result source_power_special_positive_tu15(std::uint64_t base,std::uint64_t exponent){
  const auto absolute=base&magnitude;
  if((exponent>>63)||(exponent&magnitude)==0||(exponent&magnitude)>infinity||absolute>infinity)
    throw std::invalid_argument("TU1.5 special lift power requires non-NaN inputs and a positive exponent");
  if(exponent==infinity){
    if(absolute==one)return {0xfff8000000000000ull,true}; // Actual46E620 indefinite quiet NaN.
    return {absolute<one?0:infinity,false};
  }
  if(absolute==infinity){
    return {infinity|((base>>63)&&odd_integer(exponent)?1ull<<63:0),false};
  }
  throw std::invalid_argument("TU1.5 special lift power requires an actual infinite argument");
}
}

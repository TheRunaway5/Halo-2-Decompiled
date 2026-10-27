#pragma once
#include <cstdint>
#include <functional>
#include <stdexcept>
namespace halo2 {
struct SourceMathErrorServicesTu15 {
  // Native thread-local CRT storage supplied by the caller. Original32F1E5
  // obtains this cell from330179's thread record; no Xbox TLS is executed here.
  std::int32_t* error_number{};
  // Retained/native storage owners may provide a guarded semantic write. It
  // runs at the original errno phase and takes precedence over a raw cell.
  std::function<void(std::int32_t)> store_error_number;
};
class SourceMathErrorServiceRequiredTu15:public std::runtime_error {
public:
  static constexpr std::uint32_t owner=0x330179u;
  std::int32_t error_number;
  explicit SourceMathErrorServiceRequiredTu15(std::int32_t value);
};
// TU1.5 pow's admitted masked-exception records: domain1, overflow3 and
// underflow4. The source's installed33D1E8 matherr owner returns zero. Native
// x87 operations reproduce its actual exception side effects. The returned
// binary64 value includes original rounding/range adjustment.
double source_pow_math_error_tu15_current(unsigned kind,double value,
    std::uint16_t caller_control,const SourceMathErrorServicesTu15& = {});
}

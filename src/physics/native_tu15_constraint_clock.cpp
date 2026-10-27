#include "halo2/native_tu15_constraint_clock.hpp"
#include <stdexcept>
namespace halo2 {
std::uint32_t native_tu15_seconds_from_filetime(std::uint64_t ticks) noexcept {
    const auto unix_ticks=ticks+std::uint64_t{0xfe624e212ac18000u};
    return static_cast<std::uint32_t>(unix_ticks/std::uint64_t{10000000});
}
std::uint32_t native_tu15_constraint_clock_seconds(const NativeTu15SystemFiletimeSource&source){if(!source)throw std::runtime_error("Native TU1.5 constraint clock requires its actual recorded FILETIME source");return native_tu15_seconds_from_filetime(source());}
} // namespace halo2

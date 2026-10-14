#include "halo2/source_boot_smc_video_mode.hpp"
namespace halo2 {
std::uint32_t source_boot_smc_video_mode_after_read(std::uint32_t prior,
    std::uint32_t status,std::uint32_t response) noexcept {
    return (status&0x80000000u)?prior:(response&7u);
}
}

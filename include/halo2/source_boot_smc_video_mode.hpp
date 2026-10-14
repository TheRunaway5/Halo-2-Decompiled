#pragma once
#include <cstdint>
namespace halo2 {
// Publication at observed Complex4627 80054F49..80054F58, after the actual
// SMBus byte read (address20, command4). The prior word, status and response
// are explicit external inputs. This does not query hardware, choose a boot
// default, translate native errors, or establish physical retail equivalence.
std::uint32_t source_boot_smc_video_mode_after_read(std::uint32_t prior,
    std::uint32_t status,std::uint32_t response) noexcept;
}

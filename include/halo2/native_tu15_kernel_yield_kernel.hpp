#pragma once
#include <cstdint>
namespace halo2 {
// The platform owner returns its actual status. The source XAPI wrapper
// reports false only for STATUS_NO_YIELD_PERFORMED and leaves that comparison
// as its escaped arithmetic flags.
constexpr std::uint32_t native_tu15_kernel_yield_result(std::uint32_t status) {
    return status!=0x40000024u;
}
} // namespace halo2

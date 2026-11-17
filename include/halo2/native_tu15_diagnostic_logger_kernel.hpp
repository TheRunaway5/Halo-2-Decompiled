#pragma once
#include <cstdint>

namespace halo2::native_diagnostic_logger {
// The authored logger owns its first embedded stream at byte offset eight.
inline std::uint32_t stream(std::uint32_t logger){return logger+8u;}
}

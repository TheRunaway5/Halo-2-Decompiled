#pragma once
#include <cstdint>

namespace halo2 {
// Explicit reference-emulator profile, not physical Xbox RSQRTSS precision.
// xemu0.8.136/fc24584ce88f0915ad7f04775bb7712c2e3f49ee implements
// helper_rsqrtss as rounded float32 sqrt followed by rounded float32 divide,
// preserving the previous SSE exception flags. This adapter admits positive
// normal finite operands under observed nearest/masked/noDAZ/noFTZ SSE only.
// It calculates the answer from the actual operand; no captured lookup or
// fitted approximation is used. The weather caller must select it explicitly.
std::uint32_t source_weather_rsqrt_xemu_0_8_136(std::uint32_t squared_speed);
}

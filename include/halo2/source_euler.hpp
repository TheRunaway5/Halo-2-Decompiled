#pragma once
#include "halo2/math.hpp"

namespace halo2 {
// 141CE0 scenario Euler conversion. Trigonometric results are evaluated above
// binary32 precision then stored to binary32 before the source SSE products.
// Literal zero-minus sine/product operations preserve the original zero sign.
AuthoredBasis source_euler_basis(Vec3);
} // namespace halo2

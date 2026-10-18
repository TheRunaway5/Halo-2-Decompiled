#include "halo2/pstencil/projected_constants_2003.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cmath>

namespace halo2::pstencil {
std::array<ShaderVector, 6>
projected_constants_2003(const std::array<std::uint32_t, 31> &shape,
                         const std::array<float, 16> &frame, unsigned falloff,
                         float constant20_w,
                         const std::array<ShaderVector, 6> &previous) {
  if ((shape[0] != 2 && shape[0] != 3) || !(shape[6] & 255u) ||
      (falloff != 1 && falloff != 2))
    throw FormatError("2003 projected constants require a completed "
                      "projective/pyramid shape and explicit falloff");
  const auto s = [&](unsigned n) { return std::bit_cast<float>(shape[n]); };
  for (unsigned n :
       {7u, 8u, 9u, 10u, 11u, 12u, 13u, 15u, 16u, 17u, 19u, 20u, 24u, 25u})
    if (!std::isfinite(s(n)))
      throw FormatError("Nonfinite completed projected shape");
  for (unsigned n = 10; n != 16; ++n)
    if (!std::isfinite(frame[n]))
      throw FormatError("Nonfinite completed projected frame");
  if (s(19) == 0 || s(20) == 0 || s(25) <= 0 || s(25) <= s(24))
    throw FormatError("Degenerate completed projected dimensions");

  // Literal A3F4F..A3FBC: the reciprocal is stored to binary32 before
  // later x87 double-precision arithmetic. A41xx also has explicit stores
  // below; retaining those stores matters at a projected silhouette.
  const float scale =
      float(-1.0 / (falloff == 1 ? double(s(25)) - s(24) : double(s(25))));
  auto out = previous;
  out[0] = {s(11), s(12), s(13), 1};
  for (unsigned n = 0; n != 3; ++n)
    out[1][n] = s(11 + n);

  const double ux = double(frame[15]) * s(16) - double(frame[14]) * s(17);
  const double uy = double(s(17)) * frame[13] - double(frame[15]) * s(15);
  const float uz = float(double(frame[14]) * s(15) - double(s(16)) * frame[13]);
  const float vx = float(double(s(9)) * s(16) - double(s(17)) * s(8));
  const float vy = float(double(s(17)) * s(7) - double(s(15)) * s(9));
  const float vz = float(double(s(15)) * s(8) - double(s(7)) * s(16));
  const double width = -1.0 / s(19);
  const float stored_width = float(width);
  const float mapped_ux = float(ux * width + frame[10]);
  const double mapped_uy = uy * stored_width + frame[11];
  const float mapped_uz = float(double(uz) * stored_width + frame[12]);
  const double height = -1.0 / s(20);
  const float mapped_vx = float(double(vx) * height + frame[10]);
  const float mapped_vy = float(double(vy) * height + frame[11]);
  const double mapped_vz = double(vz) * height + frame[12];
  const double half_scale = double(scale) * .5;
  out[2] = {float(double(mapped_ux) * half_scale),
            float(mapped_uy * half_scale),
            float(double(mapped_uz) * half_scale), 0};
  out[3] = {float(double(mapped_vx) * half_scale),
            float(double(mapped_vy) * half_scale),
            float(mapped_vz * half_scale), 0};
  for (unsigned n = 0; n != 3; ++n)
    out[4][n] = float(double(scale) * frame[10 + n]);
  // A4340: the original 1/128 near-face offset is in C20.z, rather than
  // the different retail C36 mapping. Keep current assets' actual near/far.
  out[4][3] =
      falloff == 1 ? float((double(s(24)) + s(10)) * scale) : float(half_scale);
  out[5][2] = falloff == 1 ? float((double(s(10)) + .0078125) * scale) : 0;
  out[5][3] = constant20_w;
  return out;
}
} // namespace halo2::pstencil

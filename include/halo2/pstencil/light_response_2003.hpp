#pragma once
#include "halo2/render_program.hpp"

namespace halo2::pstencil {
// Original2F380 with x87 control word023F: double-precision RGB*255 and
// nearest-even FIST. Completed light colors must be finite and in [0,1].
std::uint32_t pack_light_rgb_2003(const std::array<float, 3> &);
// Original981C0/952C0 accepts geometry0..3. The program constructor separates
// sphere0 from the other three shapes; completed coordinates remain separate.
PixelProgram light_utility_2003(unsigned geometry, unsigned diffuse_contrast, unsigned specular_contrast,
                                int quality, bool dull);
// Original964B0 builtin7. Colors are the completed original2F380 RGB words;
// accepting packed values avoids inventing a different color quantizer here.
PixelProgram dependent_light_2003(unsigned geometry, unsigned specular_mask, unsigned color_shift,
                                  std::uint32_t packed_diffuse, std::uint32_t packed_specular);
} // namespace halo2::pstencil

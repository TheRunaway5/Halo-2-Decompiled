#pragma once
#include "halo2/mcc_native_shaders.hpp"
#include <string>
namespace halo2 {
struct MccHudShaderReferenceAccess {
  // Source views must belong to retained metadata. Native views are requested
  // only for complete, separately published typed template/pass allocations.
  MccShaderRead source_read, native_read;
  std::function<std::optional<Tag>(TagHandle)> resolve;
  std::function<std::string(std::uint32_t)> string_id;
  std::span<const Tag> directory;
};
struct MccHudNativeShaderClosure {
  Tag shader_template, shader_pass;
  std::span<const std::byte> template_payload, pass_payload;
};
// Copies the complete authenticated grey widget graph into an independent
// original-layout owner. Both retained native payloads and actual published
// bytes must match the finite closure before any result is built. Shader
// programs, source banks, and actual StringIDs are never replaced or changed.
std::optional<MccOwnedShaderProjection>
project_mcc_grey_widget_flash(const Tag &, const MccHudNativeShaderClosure &,
                              const MccHudShaderReferenceAccess &);
} // namespace halo2

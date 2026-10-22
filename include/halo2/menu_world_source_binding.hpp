#pragma once
#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/vertex_program.hpp"

namespace halo2 {
enum class MenuWorldSourceOwnerKind { bsp_cluster, bsp_instance, scenery_placement, dynamic_object };
struct MenuWorldSourceOwner {
  MenuWorldSourceOwnerKind kind{};
  // BSP datum for a cluster/instance. For scenery, scenario datum and ordinal
  // in its authored scenery block (+0x50), never another placement family.
  // Dynamic objects supply their actual mode datum, low16 object slot and
  // mandatory full salted object identity in generation.
  std::uint32_t scope_datum=0xffffffffu, ordinal=0xffffffffu;
  // Supplied live identity only. Absence does not manufacture a generation.
  std::optional<std::uint32_t> generation;
  bool operator==(const MenuWorldSourceOwner &) const = default;
};
using MenuWorldSourceConstants=std::array<ShaderVector,192>;
struct MenuWorldSourceInput {
  unsigned vertex_register{};
  MenuWorldSourceStream stream;
  bool consumed{};
};
// Raw class0..6 upload description. It owns the draw/palette, program,
// declaration and merged constant bank, sharing only immutable section bytes.
// No selector, CPU vertex decoding, graphics operation or clip policy runs.
class MenuWorldSourceBinding {
public:
  const MenuWorldSourceOwner &owner() const { return owner_; }
  const MenuWorldSourceDraw &draw() const { return draw_; }
  std::uint32_t vertex_shader() const { return vertex_shader_; }
  std::span<const std::byte> program_bytes() const { return program_bytes_; }
  const VertexProgram &program() const { return program_; }
  const SourceVertexDeclaration &declaration() const { return declaration_; }
  const std::vector<MenuWorldSourceInput> &inputs() const { return inputs_; }
  const std::vector<unsigned> &consumed_registers() const { return consumed_registers_; }
  const std::vector<unsigned> &consumed_constants() const { return consumed_constants_; }
  // Constant vertex attributes are separate from c[192]. Values are NEVER
  // synthesized here. A graphics consumer must supply these external reads.
  const std::vector<unsigned> &external_attribute_registers() const { return external_attributes_; }
  const MenuWorldSourceConstants &constants() const { return constants_; }
  std::span<const std::byte> resource_bytes(unsigned input) const;
private:
  MenuWorldSourceBinding() = default;
  friend MenuWorldSourceBinding menu_world_source_scaled_viewport(const MenuWorldSourceBinding &,unsigned);
  friend MenuWorldSourceBinding menu_world_source_resized_viewport(const MenuWorldSourceBinding &,Vec2);
  friend MenuWorldSourceBinding menu_world_source_binding(const Cache &,
      MenuWorldSourceOwner,MenuWorldSourceDraw,std::uint32_t,const MenuWorldSourceConstants &);
  friend MenuWorldSourceBinding menu_world_source_binding_class(const Cache &,
      MenuWorldSourceOwner,MenuWorldSourceDraw,std::uint32_t,unsigned,const MenuWorldSourceConstants &);
  MenuWorldSourceOwner owner_;
  MenuWorldSourceDraw draw_;
  std::uint32_t vertex_shader_{};
  std::vector<std::byte> program_bytes_;
  VertexProgram program_;
  SourceVertexDeclaration declaration_;
  std::vector<MenuWorldSourceInput> inputs_;
  std::vector<unsigned> consumed_registers_,consumed_constants_,external_attributes_;
  MenuWorldSourceConstants constants_;
};
// The caller supplies actual owner identity, selected vrtx datum and c[192].
// Only c50..(50+3*palette_count-1) and c170..172 are replaced from the draw.
// Other constants retain their supplied bits, including NaNs and signed zero.
// Model relative addressing retains the original c50..169 palette dependency.
// Unsupported raw declarations/resources and other relative reads are errors.
MenuWorldSourceBinding menu_world_source_binding(const Cache &,MenuWorldSourceOwner,
    MenuWorldSourceDraw,std::uint32_t actual_vertex_shader,const MenuWorldSourceConstants &);
// Explicit original1CB20 result. Class0 is admitted for BSP clusters with no
// object palette; c50 remains the caller's retained bank. Class1 preserves the
// existing actual c50-palette contract. Classes2..6 require their actual
// rigid-boned/skinned model owner and current palette. Unsupported reads fail.
MenuWorldSourceBinding menu_world_source_binding_class(const Cache &,MenuWorldSourceOwner,
    MenuWorldSourceDraw,std::uint32_t actual_vertex_shader,unsigned actual_vertex_class,
    const MenuWorldSourceConstants &);
// Native SSAA extension: re-evaluate literal12FA0 c8..11 for the larger
// render target, preserving the half-pixel bias instead of scaling it.
// Rejects nonstandard source viewport banks. Owner, pose, compression, program
// and every other constant retain their original bits; scale1 is unchanged.
MenuWorldSourceBinding menu_world_source_scaled_viewport(const MenuWorldSourceBinding &,unsigned scale);
// Native temporal input scaling re-evaluates the same12FA0 viewport constants
// at fractional X/Y sizes. The original half-pixel bias stays unscaled.
MenuWorldSourceBinding menu_world_source_resized_viewport(const MenuWorldSourceBinding &,Vec2 scale);
} // namespace halo2

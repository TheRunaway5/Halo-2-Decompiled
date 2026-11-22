#pragma once
#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/vertex_program.hpp"
#include <map>
#include <memory>
#include <tuple>

namespace halo2 {
struct MenuWorldVertexMetadataEntry;
class MenuWorldSourceBindingCache;
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
// Raw class0..6 upload description. Copies keep independent draw ranges and
// palettes while sharing the immutable program, fetch layout and constant bank.
// No selector, CPU vertex decoding, graphics operation or clip policy runs.
class MenuWorldSourceBinding {
public:
  const MenuWorldSourceOwner &owner() const { return owner_; }
  const MenuWorldSourceDraw &draw() const { return draw_; }
  std::uint32_t vertex_shader() const { return data_->vertex_shader; }
  std::span<const std::byte> program_bytes() const { return data_->program_bytes; }
  const VertexProgram &program() const { return data_->program; }
  const SourceVertexDeclaration &declaration() const { return data_->declaration; }
  const MenuWorldVertexMetadataEntry &vertex_metadata() const { return *data_->vertex_metadata; }
  const std::vector<MenuWorldSourceInput> &inputs() const { return data_->inputs; }
  const std::vector<unsigned> &consumed_registers() const { return data_->consumed_registers; }
  const std::vector<unsigned> &consumed_constants() const { return data_->consumed_constants; }
  // Constant vertex attributes are separate from c[192]. Values are NEVER
  // synthesized here. A graphics consumer must supply these external reads.
  const std::vector<unsigned> &external_attribute_registers() const { return data_->external_attributes; }
  const MenuWorldSourceConstants &constants() const { return *constants_; }
  std::span<const std::byte> resource_bytes(unsigned input) const;
  // Select a range within the already bound part, or restore its full range.
  // Range validation leaves the prepared shader, palette and constants intact.
  void set_index_range(std::optional<MenuWorldSourceIndexRange> range);
private:
  MenuWorldSourceBinding() = default;
  friend class MenuWorldSourceBindingCache;
  friend MenuWorldSourceBinding menu_world_source_scaled_viewport(const MenuWorldSourceBinding &,unsigned);
  friend MenuWorldSourceBinding menu_world_source_resized_viewport(const MenuWorldSourceBinding &,Vec2);
  friend MenuWorldSourceBinding menu_world_source_binding(const Cache &,
      MenuWorldSourceOwner,MenuWorldSourceDraw,std::uint32_t,const MenuWorldSourceConstants &);
  friend MenuWorldSourceBinding menu_world_source_binding_class(const Cache &,
      MenuWorldSourceOwner,MenuWorldSourceDraw,std::uint32_t,unsigned,const MenuWorldSourceConstants &);
  static MenuWorldSourceBinding bind(const Cache &,MenuWorldSourceOwner,MenuWorldSourceDraw,
      std::uint32_t,unsigned,const MenuWorldSourceConstants &,MenuWorldSourceBindingCache *);
  struct Data {
    std::uint32_t vertex_shader{};
    std::vector<std::byte> program_bytes;
    VertexProgram program;
    SourceVertexDeclaration declaration;
    const MenuWorldVertexMetadataEntry *vertex_metadata{};
    std::vector<MenuWorldSourceInput> inputs;
    std::vector<unsigned> consumed_registers,consumed_constants,external_attributes;
    unsigned required_palette_rows{};
  };
  static void load_inputs(Data &,const Cache &,const MenuWorldSourceSection &,
      std::uint32_t vertex_shader,unsigned vertex_class);
  MenuWorldSourceOwner owner_;
  MenuWorldSourceDraw draw_;
  std::shared_ptr<const Data> data_;
  std::shared_ptr<const MenuWorldSourceConstants> constants_;
};
// Scoped to a resident renderer and its Cache. A cached input layout retains
// the exact immutable geometry section, shader datum and vertex class.
// Each binding still admits its current owner, range, palette and constants.
class MenuWorldSourceBindingCache {
public:
  explicit MenuWorldSourceBindingCache(const Cache &cache):cache_(cache) {}
  MenuWorldSourceBinding bind(MenuWorldSourceOwner,MenuWorldSourceDraw,
      std::uint32_t vertex_shader,unsigned vertex_class,const MenuWorldSourceConstants &);
private:
  friend class MenuWorldSourceBinding;
  using Key=std::tuple<std::uintptr_t,std::uint32_t,unsigned>;
  struct Entry {
    std::shared_ptr<const MenuWorldSourceSection> section;
    std::shared_ptr<const MenuWorldSourceBinding::Data> data;
  };
  const Cache &cache_;
  std::map<Key,Entry> inputs_;
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

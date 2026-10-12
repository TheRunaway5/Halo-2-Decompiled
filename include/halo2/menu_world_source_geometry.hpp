#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_vertex_declaration.hpp"
#include <array>
#include <memory>

namespace halo2 {
// Local resource identity, independent of a material or a selected runtime view.
enum class MenuWorldSourceSectionKind { bsp_cluster, bsp_instance_definition, render_model };
enum class MenuWorldSourceFormat { float32x3, float32x2, snorm16x3, snorm16x2, packed_signed_11_11_10 };
enum class MenuWorldSourceTopology { triangle_list, triangle_strip };
struct MenuWorldSourceResource {
  std::uint16_t type{}, channel{};
  std::uint32_t raw_offset{}; // Offset in the owning raw resource allocation.
  std::vector<std::byte> bytes;
};
struct MenuWorldSourceStream {
  SourceVertexSemantic semantic{};
  MenuWorldSourceFormat format{};
  unsigned resource{}, byte_offset{}, byte_stride{}, descriptor{};
};
struct MenuWorldSourcePart {
  std::array<std::byte,72> bytes{};
  std::uint16_t type{}, flags{}, index_first{}, index_count{};
  std::int16_t material=-1;
  std::uint32_t shader=0xffffffffu;
};
class MenuWorldSourceSectionBuilder;
// Owns unchanged source bytes. No CPU decompression, skinning, basis rotation,
// triangle expansion, resource activation or shader selection occurs here.
class MenuWorldSourceSection {
public:
  MenuWorldSourceSectionKind kind() const { return kind_; }
  std::uint32_t owner_datum() const { return owner_datum_; }
  unsigned section() const { return section_; }
  unsigned vertex_count() const { return vertex_count_; }
  // Authored mode section class, absent for BSP allocations. This is distinct
  // from the compiled VP class selected by an original draw.
  std::optional<unsigned> model_geometry_class() const { return model_geometry_class_; }
  std::uint16_t compression_flags() const { return compression_flags_; }
  const std::array<float,12> &compression_rows() const { return compression_rows_; } // c170..172
  const std::vector<MenuWorldSourceResource> &resources() const { return resources_; }
  const std::vector<std::array<std::byte,32>> &descriptors() const { return descriptors_; }
  const std::vector<MenuWorldSourceStream> &streams() const { return streams_; }
  const std::vector<MenuWorldSourcePart> &parts() const { return parts_; }
  std::span<const std::uint16_t> indices() const { return indices_; }
  std::span<const std::uint8_t> node_map() const { return node_map_; }
  // Literal BSP geometry header, raw allocation[8,resource_base). Pointer
  // fields retain their file values: no relocation or count reconstruction.
  // Model header equivalence is unproved, so model sections return empty.
  std::span<const std::byte> raw_header() const { return raw_header_; }
  std::span<const std::uint16_t> part_indices(unsigned part) const;
private:
  friend class MenuWorldSourceSectionBuilder;
  MenuWorldSourceSectionKind kind_{};
  std::uint32_t owner_datum_{};
  unsigned section_{}, vertex_count_{};
  std::optional<unsigned> model_geometry_class_;
  std::uint16_t compression_flags_{};
  std::array<float,12> compression_rows_{};
  std::vector<MenuWorldSourceResource> resources_;
  std::vector<std::array<std::byte,32>> descriptors_;
  std::vector<MenuWorldSourceStream> streams_;
  std::vector<MenuWorldSourcePart> parts_;
  std::vector<std::uint16_t> indices_;
  std::vector<std::uint8_t> node_map_;
  std::vector<std::byte> raw_header_;
};
MenuWorldSourceSection load_menu_world_source_bsp_section(const Cache &, std::size_t bsp,
    MenuWorldSourceSectionKind, unsigned section);
// Actual mode datum and actual selected section; objects/variants/LOD are not chosen.
// Only the proved rigid class1, descriptor types1/2/24/25/27 path is admitted.
MenuWorldSourceSection load_menu_world_source_model_section(const Cache &,
    std::uint32_t render_model, unsigned section);
struct MenuWorldSourceBspOwner {
  std::uint32_t bsp_datum{};
  unsigned instance{}, section{};
  float separate_scale{}; // Literal retained field, deliberately absent from rows.
  std::array<float,12> rows{}; // 4D830 explicit matrix c50..52.
};
MenuWorldSourceBspOwner load_menu_world_source_bsp_owner(const Cache &, std::size_t bsp,
    unsigned instance);
struct MenuWorldSourceIndexRange {
  // Absolute ushort index ordinals in the immutable section, as supplied to
  // original4DAA0. This is a submitted range, never a visibility/mask result.
  unsigned first{}, count{};
  bool operator==(const MenuWorldSourceIndexRange &) const = default;
};
struct MenuWorldSourceDraw {
  std::shared_ptr<const MenuWorldSourceSection> geometry;
  unsigned part{};
  MenuWorldSourceTopology topology{};
  // Actual source palette range to upload at c50. No idle/controller is chosen.
  std::vector<std::array<float,12>> object_rows;
  // Absence preserves the whole authored part. Explicit ranges must remain
  // inside that part. Zero count is an empty description, not an admitted GPU
  // packet; original4DAA0 forwards unfiltered authored counts without a guard.
  std::optional<MenuWorldSourceIndexRange> range;
  MenuWorldSourceIndexRange index_range() const;
  std::span<const std::uint16_t> indices() const;
  std::size_t triangle_count() const;
  // Includes degenerates; strip odd order is (i,i+2,i+1), as in the original GS.
  // Each submitted range restarts strip parity at its own first index.
  std::array<std::uint16_t,3> triangle(std::size_t ordinal) const;
};
// Topology is a caller-proved original draw input (4D5E7/4D5F3 and4DAA0),
// never inferred from face/index counts. Current c50 palette rows are required.
MenuWorldSourceDraw menu_world_source_draw(std::shared_ptr<const MenuWorldSourceSection>,
    unsigned part, MenuWorldSourceTopology, std::span<const std::array<float,12>> object_rows,
    std::optional<MenuWorldSourceIndexRange> range = {});
// Original BSP cluster path does not upload a c50 object palette. This keeps
// an empty palette explicit, separate from the class1 factory above.
MenuWorldSourceDraw menu_world_source_cluster_draw(std::shared_ptr<const MenuWorldSourceSection>,
    unsigned part,MenuWorldSourceTopology,std::optional<MenuWorldSourceIndexRange> range = {});
} // namespace halo2

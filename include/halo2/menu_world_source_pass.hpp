#pragma once
#include "halo2/cache.hpp"
#include <array>

namespace halo2 {
// Original16B90 arguments, supplied after the caller's LOD/layer/lighting
// policy. No value is selected, defaulted, or recovered from a program name.
struct MenuWorldSourcePassChoice {
  unsigned shader_lod{},layer{},pass_in_layer{},implementation{};
  bool operator==(const MenuWorldSourcePassChoice &) const = default;
};
struct MenuWorldSourceMetadataRead {
  std::uint32_t backing_datum{},source_address{};
  std::size_t extent{};
  bool operator==(const MenuWorldSourceMetadataRead &) const = default;
};
// Owned resolution of an explicit caller choice. The original tag allocator
// deduplicates arrays across tag payloads; backing identities retain those
// aliases without mutable host pointer fixups or fabricated metadata.
class MenuWorldSourcePass {
public:
  std::uint32_t shader() const {return shader_;}
  std::uint32_t shader_template() const {return template_;}
  std::uint32_t shader_pass() const {return pass_;}
  std::uint32_t vertex_shader() const {return vertex_;}
  const MenuWorldSourcePassChoice &choice() const {return choice_;}
  unsigned template_pass() const {return template_pass_;}
  unsigned template_remap() const {return template_remap_;}
  ShaderRange lod_layers() const {return lod_layers_;}
  ShaderRange layer_passes() const {return layer_passes_;}
  ShaderRange implementations() const {return implementations_;}
  std::uint32_t implementation_address() const {return implementation_address_;}
  std::uint32_t pass_postprocess_address() const {return pass_postprocess_address_;}
  std::uint32_t lod_address() const {return lod_address_;}
  std::uint32_t layer_address() const {return layer_address_;}
  std::uint32_t template_pass_address() const {return template_pass_address_;}
  std::uint32_t template_remap_address() const {return template_remap_address_;}
  const std::array<std::byte,10> &lod_record() const {return lod_record_;}
  const std::array<std::byte,10> &pass_record() const {return pass_record_;}
  const std::array<std::byte,6> &remap_record() const {return remap_record_;}
  const std::array<std::byte,306> &implementation_record() const {return implementation_record_;}
  const std::vector<MenuWorldSourceMetadataRead> &metadata_reads() const {return reads_;}
private:
  MenuWorldSourcePass()=default;
  friend MenuWorldSourcePass load_menu_world_source_pass(const Cache &,
      std::uint32_t,const MenuWorldSourcePassChoice &);
  std::uint32_t shader_{},template_{},pass_{},vertex_{};
  MenuWorldSourcePassChoice choice_;
  unsigned template_pass_{},template_remap_{};
  ShaderRange lod_layers_,layer_passes_,implementations_;
  std::uint32_t implementation_address_{},pass_postprocess_address_{};
  std::uint32_t lod_address_{},layer_address_{},template_pass_address_{},template_remap_address_{};
  std::array<std::byte,10> lod_record_{},pass_record_{};
  std::array<std::byte,6> remap_record_{};
  std::array<std::byte,306> implementation_record_{};
  std::vector<MenuWorldSourceMetadataRead> reads_;
};
// Bounded ordinary shad→stem→spas resolution. Actual selected vrtx is returned
// to the explicit raw binding, which owns the class0/class1 import. This loader
// does not run source_shader_lod, lighting classification, fog or pass selection.
MenuWorldSourcePass load_menu_world_source_pass(const Cache &,std::uint32_t shader,
    const MenuWorldSourcePassChoice &);
} // namespace halo2

#include "halo2/residency.hpp"
#include "halo2/model_selection.hpp"
namespace halo2 {
std::shared_ptr<const PcmSound> NativeResidency::sound(std::uint32_t datum) {
  if (datum == null_datum)
    return {};
  if (!sounds_.contains(datum))
    sounds_.emplace(datum, std::make_shared<PcmSound>(
                               decode_sound(cache_.load_sound(datum))));
  return sounds_.at(datum);
}
void NativeResidency::geometry(std::uint32_t datum) {
  if (datum == null_datum)
    return;
  if (!models_.contains(datum)) {
    ModelAssets assets{cache_.load_model_resources(datum)};
    models_.emplace(datum, std::move(assets));
  }
}
void NativeResidency::model(std::uint32_t datum, int) {
  if (datum == null_datum) return;
  const auto *tag=cache_.find_tag({datum});
  if(!tag)throw FormatError("Model residency request has a stale tag handle");
  // Unit definitions retain an hlmt; section and bitmap resources belong to
  // its mode. Resolve through the shared importer before coalescing residency.
  if(!cache_.find_tag({datum},tag_groups::render_model))
    datum=cache_.load_initial_model_definition(datum).render_model;
  if(datum==null_datum)return;
  geometry(datum);
  auto &assets = models_.at(datum);
  if (!assets.materials_resident) {
    for (const auto &resource : cache_.model_bitmap_resources(datum))
      bitmap(resource.datum, resource.index);
    assets.materials_resident = true;
  }
}
void NativeResidency::bitmap(std::uint32_t datum, int index) {
  const auto key = std::pair(datum, index);
  if (!bitmaps_.contains(key))
    bitmaps_.emplace(key, cache_.load_bitmap_resource(datum, unsigned(index)));
}
void NativeResidency::object(std::uint32_t datum) {
  const auto resources = cache_.object_resources(datum);
  // The shipped campaign object_type_predict operands contain types 0 and 2.
  // Do not silently admit a resource whose native owner is still missing.
  for (const auto &resource : resources)
    if (resource.type != 0 && resource.type != 2)
      throw FormatError("object prediction native resource owner unavailable: " +
                        std::to_string(resource.type));
  for (const auto &resource : resources) {
    if (resource.type == 0) bitmap(resource.datum, resource.index);
    // Mode 2's 16E950 requests geometry without 16E330's material traversal.
    else geometry(resource.datum);
  }
}
void NativeResidency::structure(std::size_t bsp, int section, bool definition) {
  if (section == -1)
    return;
  const auto resources = cache_.structure_resources(bsp, section, definition);
  if (!structure_ || structure_->bsp != bsp)
    structure_ =
        BspAssets{bsp, cache_.load_bsp_resources(bsp)};
  for (const auto &resource : resources) {
    if (resource.type == 0) {
      bitmap(resource.datum, resource.index);
    } else if (resource.type == 2) {
      model(resource.datum, -1);
    }
    // Type1 touches existing audio entries without initiating loads. Other
    // native geometry/lightmap resources are admitted with the BSP blocks;
    // retail object lightmap buckets and driver residency remain separate.
  }
}
} // namespace halo2

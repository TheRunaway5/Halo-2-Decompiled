#pragma once
#include "halo2/audio.hpp"
#include "halo2/model.hpp"
#include <map>
namespace halo2 {
// Native decoded residency. Retail prediction is chunk/section granular;
// this backend admits whole native clips/models/BSP meshes synchronously.
class NativeResidency {
public:
  explicit NativeResidency(const Cache &cache) : cache_(cache) {}
  std::shared_ptr<const PcmSound> sound(std::uint32_t);
  void model(std::uint32_t, int section);
  void object(std::uint32_t);
  void structure(std::size_t, int section, bool definition);
  // All admitted reads finish in sound/model/structure before returning.
  void barrier() const {}
  std::size_t sound_count() const { return sounds_.size(); }
  std::size_t model_count() const { return models_.size(); }
  std::size_t bitmap_count() const { return bitmaps_.size(); }

private:
  const Cache &cache_;
  std::map<std::uint32_t, std::shared_ptr<const PcmSound>> sounds_;
  struct ModelAssets {
    RenderModel display;
    std::vector<std::vector<std::byte>> blocks;
    bool materials_resident{};
  };
  struct BspAssets {
    std::size_t bsp;
    Mesh display;
    std::vector<std::vector<std::byte>> blocks;
  };
  std::map<std::uint32_t, ModelAssets> models_;
  std::map<std::pair<std::uint32_t, int>, BitmapResource> bitmaps_;
  std::optional<BspAssets> structure_;
  void bitmap(std::uint32_t, int index);
  void geometry(std::uint32_t);
};
} // namespace halo2

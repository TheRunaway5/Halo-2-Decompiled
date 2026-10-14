#pragma once
#include "halo2/audio.hpp"
#include <map>
namespace halo2 {
// Native residency. Retain authored model blocks and bitmap resources;
// rendering decodes display geometry independently. Compressed sound
// predictions and playback PCM have separate owners; reads are synchronous.
class NativeResidency {
public:
  explicit NativeResidency(const Cache &cache) : cache_(cache) {}
  void predict_sound(std::uint32_t);
  std::shared_ptr<const PcmSound> sound(std::uint32_t);
  void model(std::uint32_t, int section);
  void object(std::uint32_t);
  void structure(std::size_t, int section, bool definition);
  // All admitted reads finish in predict_sound/sound/model/structure.
  void barrier() const {}
  std::size_t sound_count() const { return sounds_.size()+sound_predictions_.size(); }
  std::size_t decoded_sound_count() const { return sounds_.size(); }
  std::size_t model_count() const { return models_.size(); }
  std::size_t bitmap_count() const { return bitmaps_.size(); }

private:
  const Cache &cache_;
  std::map<std::uint32_t, std::shared_ptr<const PcmSound>> sounds_;
  std::map<std::uint32_t, std::vector<std::byte>> sound_predictions_;
  struct ModelAssets {
    std::vector<std::vector<std::byte>> blocks;
    bool materials_resident{};
  };
  struct BspAssets {
    std::size_t bsp;
    std::vector<std::vector<std::byte>> blocks;
  };
  std::map<std::uint32_t, ModelAssets> models_;
  std::map<std::pair<std::uint32_t, int>, BitmapResource> bitmaps_;
  std::optional<BspAssets> structure_;
  void bitmap(std::uint32_t, int index);
  void geometry(std::uint32_t);
};
} // namespace halo2

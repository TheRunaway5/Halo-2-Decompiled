#pragma once
#include "halo2/cache.hpp"
#include "halo2/prompt_device.hpp"
#include <memory>
namespace halo2 {
struct FontGlyph;
struct ControllerPromptSample {unsigned material{};std::array<float,4> uv;};
// Independent control-only textures. Ordinary font metrics, text, lookup and
// original/MCC atlases stay with their existing owners. Prepare before GPU
// upload; drawing only chooses immutable material/UV records per local view.
class ControllerPromptArtwork {
public:
  ControllerPromptArtwork(Mesh &,const std::filesystem::path &font_directory);
  ~ControllerPromptArtwork();
  void prepare(unsigned selector);
  std::optional<ControllerPromptSample> sample(unsigned selector,unsigned code,PromptDevice,
      const FontGlyph &,int x,int y,int width,int height)const;
private:
  struct Impl;std::unique_ptr<Impl> impl_;
};
}

#pragma once
#include "halo2/particle_draw.hpp"
#include "halo2/renderer_particle_program.hpp"
#include <vulkan/vulkan.h>

namespace halo2 {
// Borrowed current-surface copy. Its owner records ordered copies/barriers
// before draws that read external23 and retains the view through GPU completion.
struct ParticleSceneTexture {
  VkImageView view{};
};
// Internal Vulkan subrenderer. The owner waits for its previous frame before
// prepare(), and records the supplied ordered draws inside a compatible pass.
class ParticleRenderer {
public:
  ParticleRenderer(VkPhysicalDevice,VkDevice,VkQueue,unsigned queue_family,
    VkRenderPass,const std::filesystem::path &shader_directory);
  ~ParticleRenderer();
  ParticleRenderer(const ParticleRenderer&)=delete;
  ParticleRenderer &operator=(const ParticleRenderer&)=delete;
  void prepare(std::span<const ParticleDraw>,ParticleSceneTexture = {});
  RendererParticleProgramConfiguration program(std::shared_ptr<const ParticleMaterial>);
  bool reads_scene_texture(std::size_t index) const;
  void resolve_scene_lookup(std::size_t index,const ParticleBumpTextureState &);
  void record(VkCommandBuffer,VkViewport,VkRect2D,std::size_t first,std::size_t count) const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2

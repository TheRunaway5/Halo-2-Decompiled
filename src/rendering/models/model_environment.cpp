#include "halo2/appearance.hpp"
#include <algorithm>

namespace halo2 {
void prepare_model_environment(const Cache &cache, Material &material) {
  if(material.lods.empty()) return;
  const auto &lod=material.lods.front();
  if(!(lod.available_layers & (1u<<3)) || lod.layers.count()<=3) return;
  const auto layer=lod.layers.first()+3;
  if(layer>=material.layers.size()) throw FormatError("model environment layer outside template");
  const auto range=material.layers[layer];
  const auto selected=std::find_if(material.passes.begin(),material.passes.end(),[&](const auto &p) {
    return p.template_pass>=range.first() && p.template_pass<range.first()+range.count() &&
      p.implementation==1 && p.vertex_shader.ends_with("\\lightmap_pervertex_env");
  });
  if(selected==material.passes.end()) return;
  const auto &pass=*selected;
  // This admission uses the retained per-vertex-lighting implementation. The
  // dynamic object-light query is independent of the reflection/gel program.
  if(std::any_of(pass.textures.begin(),pass.textures.end(),[](const auto &b) {
    return b.stage>3 || (b.external && !((b.stage==0 && b.external==3) ||
      (b.stage==3 && b.external==1)));
  })) return;
  auto environment=std::make_shared<Material>();
  environment->name=material.name+" [environment]";
  environment->forward_pass=true;
  environment->forward_pass_index=0;
  environment->passes.push_back(pass);
  environment->shader_switches=material.shader_switches;
  environment->forward_vertex=4;
  environment->forward_program=pass.program;
  environment->vertex_program=pass.vertex_code;
  environment->vertex_declaration=pass.vertex_declaration;
  environment->program_constants=pass.constants;
  environment->accumulation_texture=true;
  std::copy(pass.texture_states.begin(),pass.texture_states.end(),environment->texture_states.begin());
  environment->forward_raster.depth_function=0x202;
  environment->forward_raster.depth_write=0;
  for(const auto &state:pass.states) {
    if(state.index==67) environment->forward_raster.color_write=state.value;
    if(state.index==57) environment->forward_raster.depth_function=state.value;
    if(state.index==64) environment->forward_raster.depth_write=state.value;
  }
  Texture *textures[]{&environment->diffuse,&environment->detail,&environment->bump,&environment->auxiliary};
  for(const auto &binding:pass.textures) {
    if(binding.external==3) continue;
    if(binding.external==1) {
      const auto tag=cache.query_tag({.group=tag_groups::bitmap, .name="rasterizer\\vector_normalization"});
      if(!tag) throw FormatError("model environment normalization cube missing");
      *textures[binding.stage]=cache.load_bitmap(tag->datum,0);
    } else {
      const auto remap=std::find_if(pass.bitmap_remaps.begin(),pass.bitmap_remaps.end(),[&](const auto &r) {
        return (r.destination&255)==binding.parameter;
      });
      if(remap==pass.bitmap_remaps.end() || remap->source>=material.bitmaps.size())
        throw FormatError("model environment bitmap remap missing");
      const auto &bitmap=material.bitmaps[remap->source];
      *textures[binding.stage]=cache.load_bitmap(bitmap.datum,bitmap.index);
    }
  }
  // A later LOD's static lightmap must not override a model's first LOD
  // texture-mapping pass. That pass produces the environment input.
  material.static_lightmap_pass=false;
  prepare_surface_material(cache,material);
  if(!material.forward_pass || material.forward_vertex!=3)
    throw FormatError("model environment requires an authored base texture pass");
  const auto &base=material.passes[material.forward_pass_index];
  for(const auto &state:base.states) {
    if(state.index==67) material.forward_raster.color_write=state.value;
    if(state.index==57) material.forward_raster.depth_function=state.value;
    if(state.index==64) material.forward_raster.depth_write=state.value;
  }
  std::erase_if(material.rendering_gaps,[](const auto &gap) {
    return gap.starts_with("surface program bound;");
  });
  material.rendering_gaps.push_back("authored environment/gel pass bound; dynamic object light-probe queries pending");
  material.environment_pass=std::move(environment);
}
} // namespace halo2

#include "halo2/mcc_loading_inputs.hpp"
#include "halo2/texture.hpp"
#include <utility>

namespace halo2 {
MccLoadingBitmapInputs load_mcc_loading_bitmap_inputs(const Cache& cache) {
  if(!cache.is_mcc())throw FormatError("MCC loading bitmap inputs require an MCC cache");
  struct Source { unsigned slot;const char* name;unsigned frame; };
  constexpr std::array<Source,5> sources{{
    {0,"globals\\loading_screen\\loading_screen",0},
    {1,"globals\\loading_screen\\loading_screen_logos",0},
    {3,"globals\\loading_screen\\loading_screen_spinners",0},
    {4,"globals\\loading_screen\\loading_screen_spinners",1},
    {5,"globals\\loading_screen\\loading_screen_sweep",0}}};
  MccLoadingBitmapInputs result;
  for(unsigned i=0;i<sources.size();++i) {
    const auto& source=sources[i];
    const Tag* tag{};
    for(const auto& owner:cache.select_tags({.group=TagGroup{"bitm"},.name=source.name,
                                           .require_address=true,.minimum_size=76})) {
      if(tag)throw FormatError("Ambiguous MCC loading bitmap owner: "+std::string(source.name));
      tag=&owner;
    }
    if(!tag)throw FormatError("Missing MCC loading bitmap owner: "+std::string(source.name));
    const auto resource=cache.load_bitmap_resource(tag->datum,source.frame);
    if(resource.type || resource.depth!=1 || resource.levels[0].empty() ||
       !resource.levels[1].empty() || !resource.levels[2].empty())
      throw FormatError("MCC loading bitmap is not a complete ordinary 2D input");
    auto image=cache.load_bitmap(tag->datum,source.frame);
    if(image.format>=14 && image.format<=16)image=prepare_xemu_bc_texture(image);
    if(image.format!=11 || image.layers!=1 || image.depth || image.pixels.empty())
      throw FormatError("MCC loading bitmap storage conversion is unsupported");
    result[i]={source.slot,tag->handle(),source.frame,
               {resource.width,resource.height,std::move(image)}};
  }
  return result;
}
}

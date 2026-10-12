#include "halo2/cache.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
namespace {
template<class T> std::span<const T> range(const std::vector<T> &v,ShaderRange r) {
  if(r.first()>v.size() || r.count()>v.size()-r.first())
    throw FormatError("animated bitmap range outside definition");
  return std::span<const T>(v).subspan(r.first(),r.count());
}
template<class F> void frames(Material &m,F consume) {
  if(!m.forward_pass || m.animation.overlays.empty()) return;
  const auto &a=m.animation;
  const auto &pass=m.passes.at(m.forward_pass_index);
  const auto implementations=range(a.implementations,a.passes.at(pass.template_pass));
  if(pass.implementation>=implementations.size())
    throw FormatError("animated bitmap implementation outside definition");
  for(const auto &mapping:range(a.mappings,implementations[pass.implementation].bitmap_transforms)) {
    const unsigned slot=mapping.destination&255;
    for(const auto &ref:range(a.references,a.parameters.at(mapping.source))) {
      if(ref.transform!=0) continue;
      for(const auto &binding:pass.textures) {
        if(binding.parameter==255 || binding.external) continue;
        const auto bitmap=std::find_if(pass.bitmap_remaps.begin(),pass.bitmap_remaps.end(),
            [&](const auto &r){return (r.destination&255)==binding.parameter;});
        if(bitmap==pass.bitmap_remaps.end()) throw FormatError("animated bitmap remap missing");
        if(bitmap->source!=slot) continue;
        if(binding.stage>=4) throw FormatError("animated bitmap stage outside NV2A slots");
        consume(binding.stage,m.bitmaps.at(slot),a.overlays.at(ref.function));
      }
    }
  }
}
float value(const ShaderOverlay &o,double seconds,const ShaderInput &input,
            const ShaderFunctionTables &tables) {
  float v=input?input(o.input):0, r=input?input(o.range):0;
  if(!o.input) {
    if(o.period==0 || !std::isfinite(seconds)) throw FormatError("invalid bitmap animation clock");
    v=float(seconds/o.period);
    if(o.curve.empty() || o.curve[0]!=std::byte{3}) v=float(std::fmod(double(v),1.0));
  }
  return shader_function_value(o.curve,v,r,tables);
}
}
void prepare_bitmap_animation(const Cache &cache,Material &m) {
  frames(m,[&](unsigned stage,const ShaderBitmap &bitmap,const ShaderOverlay &) {
    auto &images=m.animated_textures[stage];
    if(!images.empty()) return;
    const auto count=cache.bitmap_count(bitmap.datum);
    if(!count || count>256) throw FormatError("unsupported animated bitmap frame count");
    for(unsigned i=0;i<count;++i) images.push_back(cache.load_bitmap(bitmap.datum,i));
    if(bitmap.index>=count) throw FormatError("initial animated bitmap frame outside tag");
    m.texture_frames[stage]=bitmap.index;
  });
}
int bitmap_frame_index(float value,unsigned count) {
  if(!count || !std::isfinite(value))throw FormatError("invalid animated bitmap frame");
  if(std::abs(value)<.0001f)return 0;
  // __CIfmod computes with x87 precision, then 1B0A0 stores to float before
  // its SSE truncation/negative-fraction correction implements floor.
  return int(std::floor(float(std::fmod(double(value),double(count)))));
}
bool animate_material_bitmaps(Material &m,double seconds,const ShaderInput &input) {
  bool changed=false;
  frames(m,[&](unsigned stage,const ShaderBitmap &,const ShaderOverlay &o) {
    const auto count=m.animated_textures[stage].size();
    if(!count || !m.animation.tables) throw FormatError("animated bitmap images missing");
    const float frame=value(o,seconds,input,*m.animation.tables);
    const auto selected=bitmap_frame_index(frame,unsigned(count));
    if(selected<0 || unsigned(selected)>=count)throw FormatError("animated bitmap frame selects an invalid image");
    const auto index=std::uint32_t(selected);
    changed|=m.texture_frames[stage]!=index;
    m.texture_frames[stage]=index;
  });
  return changed;
}
std::optional<float> weapon_ammunition_input(std::uint32_t name,int loaded) {
  loaded=std::max(0,loaded);
  switch(name) {
  case 0x1700056b:return float(loaded%10)*.1f;
  case 0x1700056c:return float((loaded/10)%10)*.1f;
  default:return {};
  }
}
}

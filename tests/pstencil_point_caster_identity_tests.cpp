#include "halo2/pstencil/point_caster.hpp"
#include "halo2/source_draw_unit.hpp"
#include "halo2/effect_light_admission.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
}
int main(int argc,char **argv)try {
  check(argc==2,"Supply prepared MCC map");Cache cache(argv[1]);
  check(cache.is_mcc(),"Point caster identity acceptance requires MCC runtime data");
  const auto native=cache.query_tag({.group=tag_groups::vertex_shader,
      .name="rasterizer\\vertex_shaders_xbox\\lightaccum_sphere_diffuse",.require_address=true});
  const auto dx9=cache.query_tag({.group=tag_groups::vertex_shader,
      .name="rasterizer\\vertex_shaders_dx9\\lightaccum_sphere_diffuse",.require_address=true});
  check(native && dx9 && native->datum!=dx9->datum,"MCC fixture lost its distinct native and DX9 shader owners");
  const auto native_program=cache.load_vertex_shader_code(native->datum,0);
  check(!native_program.empty(),"Actual native point-caster class0 program is absent");
  bool rejected{};try{(void)cache.load_vertex_shader_code(dx9->datum,0);}
  catch(const FormatError &error){rejected=std::string_view(error.what())=="vertex shader is not original Xbox format";}
  check(rejected,"DX9 negative control entered the original vertex decoder");

  // Declared BSP draw inputs. Geometry and the complete shader
  // owner come from the real prepared cache; no physics or light result is supplied.
  const auto bsp=cache.load_source_bsp_admission(0);
  pstencil::PointCasterBank bank(cache,0);std::vector<pstencil::PointCaster> output;
  SourceEffectLightRuntime light{};light.identity=7;light.position={1,2,3};
  for(unsigned cluster=0;cluster<bsp.clusters.size() && output.empty();++cluster) {
    if(!bsp.clusters[cluster].geometry_count)continue;
    SourceDrawUnitState units;units.draw_count=1;
    units.units[0].words[1]=0xffffffffu;units.units[0].words[3]=cluster<<9;
    units.units[0].words[6]=255u<<16;
    bank.collect(units,nullptr,light,{},0,{},output);
  }
  check(!output.empty(),"Actual MCC BSP produced no point-caster raw geometry");
  for(const auto &caster:output) {
    check(caster.source.vertex_shader()==native->datum &&
        std::ranges::equal(caster.source.program_bytes(),native_program) &&
        caster.source.declaration().class_index==0,
        "Point caster selected a foreign program instead of the exact authored Xbox owner");
    check(caster.source.draw().triangle_count()>0,"Point caster lost its actual indexed BSP faces");
  }
  std::cout<<"PASS actual MCC point caster: exact native Xbox owner, distinct DX9 rejection, "
      <<output.size()<<" actual BSP raw bindings; no shader execution or framebuffer claim\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

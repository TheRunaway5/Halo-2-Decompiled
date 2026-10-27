#include "halo2/menu_world_vertex_catalog.hpp"
#include <algorithm>
#include <sstream>

// Build-time and offline-test source emission. The production cache never links this object.
namespace halo2 {
namespace {
bool packed(std::uint16_t semantic) {
  switch(SourceVertexSemantic(semantic)) {
  case SourceVertexSemantic::Position:case SourceVertexSemantic::UV:
  case SourceVertexSemantic::BlendIndices:case SourceVertexSemantic::BlendWeights:return false;
  case SourceVertexSemantic::Normal:case SourceVertexSemantic::Tangent:
  case SourceVertexSemantic::Binormal:return true;
  default:throw UnsupportedVertexProgram("menu raw vertex semantic has no proved fetch adapter");
  }
}
void declaration(const SourceVertexDeclaration &value) {
  if(value.class_index>6 || value.semantics.empty() || value.semantics.size()>16)
    throw UnsupportedVertexProgram("menu raw vertex catalog requires an actual class0..6 declaration");
  for(const auto semantic:value.semantics) {
    (void)packed(semantic);
    if(value.class_index<2 && (semantic==1 || semantic==2))
      throw UnsupportedVertexProgram("ordinary world class consumes model influence semantics");
  }
  if(value.class_index>=2 && (std::count(value.semantics.begin(),value.semantics.end(),1)!=1 ||
      std::count(value.semantics.begin(),value.semantics.end(),2)!=(value.class_index>=4?1:0)))
    throw UnsupportedVertexProgram("skinned class lacks its original influence declaration");
}
constexpr const char interface[]=R"GLSL(layout(set=1,binding=0,std140) uniform MenuWorldVertexUniforms {
    vec4 c[192];
    vec4 clipRange;
    vec2 fogParam;
    vec2 surfaceSize;
} source;
layout(location=0) out vec4 v_vtxD0;
layout(location=1) out vec4 v_vtxD1;
layout(location=2) out vec4 v_vtxB0;
layout(location=3) out vec4 v_vtxB1;
layout(location=4) out float v_vtxFog;
layout(location=5) out vec4 v_vtxT0;
layout(location=6) out vec4 v_vtxT1;
layout(location=7) out vec4 v_vtxT2;
layout(location=8) out vec4 v_vtxT3;
layout(location=9) flat out vec4 v_vtxPos0;
layout(location=10) flat out vec4 v_vtxPos1;
layout(location=11) flat out vec4 v_vtxPos2;
layout(location=12) flat out float v_triMZ;
vec4 vpInput[16],vpOut[16];
vec4 vpConstant(uint index) {return source.c[index];}
)GLSL";
// Independently retained MIT wrapper arithmetic from the measured original-
// input bridge. The compact descriptor layout is new and separately gated.
constexpr const char fetch_helpers[]=R"GLSL(float sourceClampW(float t) {
    return t>0.0 || floatBitsToUint(t)==0u
        ? clamp(t,uintBitsToFloat(528482304u),uintBitsToFloat(1602224128u))
        : clamp(t,uintBitsToFloat(3749707776u),uintBitsToFloat(2675965952u));
}
vec4 sourcePackedNormal(int p) {
    return vec4(float(bitfieldExtract(p,0,11))/1023.0,
                float(bitfieldExtract(p,11,11))/1023.0,
                float(bitfieldExtract(p,22,10))/511.0,1.0);
}
vec4 sourceColor(vec4 value) {
    return clamp(mix(value,vec4(1.0),isnan(value)),0.0,1.0);
}
)GLSL";
constexpr const char epilogue[]=R"GLSL(    vec4 screen=vpOut[0];
    screen.xy=trunc(screen.xy*16.0)/16.0;
    screen.w=sourceClampW(screen.w);
    vec4 clip=screen;
    clip.xy=(2.0*clip.xy-source.surfaceSize)/source.surfaceSize;
    clip.z=clip.z/source.clipRange.y;
    clip.xyz=clip.xyz*clip.w;
    gl_Position=clip;
    gl_PointSize=1.0;
    v_vtxD0=sourceColor(vpOut[3]);v_vtxD1=sourceColor(vpOut[4]);
    v_vtxB0=sourceColor(vpOut[7]);v_vtxB1=sourceColor(vpOut[8]);
    float distance=vpOut[5].x;
    float fog=source.fogParam.x+distance*source.fogParam.y;
    fog=fog-1.0;
    v_vtxFog=isinf(distance)?1.0:clamp(isnan(fog)?1.0:fog,-uintBitsToFloat(2139095039u),uintBitsToFloat(2139095039u));
    v_vtxT0=vpOut[9];v_vtxT1=vpOut[10];v_vtxT2=vpOut[11];v_vtxT3=vpOut[12];
    v_vtxPos0=screen;v_vtxPos1=screen;v_vtxPos2=screen;v_triMZ=0.0;
}
)GLSL";
}
namespace {
MenuWorldVertexShaderSource shader_source(
    std::span<const std::byte> bytes,const SourceVertexDeclaration &inputs,bool attributes,bool renderer_arithmetic=false) {
  declaration(inputs);
  const auto program=decode_vertex_program(bytes);
  const auto source=renderer_arithmetic?menu_world_renderer_vertex_source(program,inputs.class_index>=2):
      attributes?menu_world_material_vertex_source(program,inputs.class_index>=2):
      (inputs.class_index<2?menu_world_vertex_source(program):menu_world_skinned_vertex_source(program));
  for(const auto input:source.consumed_inputs)
    if(input>=inputs.semantics.size() && !(attributes &&
        ((renderer_arithmetic&&input<16) || input==10 || input==11)))
      throw UnsupportedVertexProgram("menu raw vertex program consumes an external constant attribute");
  std::ostringstream text;text<<"#version 450\n// Halo2 raw class0..6 vertex catalog ABI"<<(renderer_arithmetic?4:attributes?3:2)<<".\n";
  for(unsigned i=0;i<inputs.semantics.size();++i)
    text<<"layout(location="<<i<<") in "<<(packed(inputs.semantics[i])?"int":"vec4")
        <<" v"<<i<<(packed(inputs.semantics[i])?"_cmp":"")<<";\n";
  std::string uniform_interface=interface;
  if(attributes)uniform_interface.insert(uniform_interface.find("} source;"),"    vec4 attributes[16];\n");
  text<<uniform_interface<<source.helpers<<fetch_helpers<<"void main() {\n";
  for(const auto input:source.consumed_inputs) {
    if(input>=inputs.semantics.size()) {
      text<<"    vpInput["<<input<<"]=source.attributes["<<input<<"];\n";
      continue;
    }
    text<<"    vpInput["<<input<<"]="
        <<(packed(inputs.semantics[input])?"sourcePackedNormal(v":"v")<<input
        <<(packed(inputs.semantics[input])?"_cmp)":"")<<";\n";
  }
  text<<source.statements<<epilogue;
  return {text.str(),source.consumed_inputs,source.consumed_constants,
      renderer_arithmetic?menu_world_renderer_vertex_input_components(program,inputs.class_index>=2):
          menu_world_vertex_input_components(program,inputs.class_index>=2),menu_world_vertex_default_texture_w_mask(program)};
}
}
MenuWorldVertexShaderSource menu_world_vertex_shader_source(
    std::span<const std::byte> bytes,const SourceVertexDeclaration &inputs) {
  return shader_source(bytes,inputs,false);
}
MenuWorldVertexShaderSource menu_world_material_vertex_shader_source(
    std::span<const std::byte> bytes,const SourceVertexDeclaration &inputs) {
  return shader_source(bytes,inputs,true);
}
MenuWorldVertexShaderSource menu_world_renderer_vertex_shader_source(
    std::span<const std::byte> bytes,const SourceVertexDeclaration &inputs) {
  return shader_source(bytes,inputs,true,true);
}
} // namespace halo2

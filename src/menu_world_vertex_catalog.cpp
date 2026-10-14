#include "halo2/menu_world_vertex_catalog.hpp"
#include <algorithm>
#include <sstream>

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
bool equal(std::span<const std::byte> a,std::span<const std::byte> b) {
  return a.size()==b.size() && std::equal(a.begin(),a.end(),b.begin());
}
bool equal(std::span<const std::uint16_t> a,std::span<const std::uint16_t> b) {
  return a.size()==b.size() && std::equal(a.begin(),a.end(),b.begin());
}
}
MenuWorldVertexShaderSource menu_world_vertex_shader_source(
    std::span<const std::byte> bytes,const SourceVertexDeclaration &inputs) {
  declaration(inputs);
  const auto program=decode_vertex_program(bytes);
  const auto source=inputs.class_index<2?menu_world_vertex_source(program):menu_world_skinned_vertex_source(program);
  for(const auto input:source.consumed_inputs)
    if(input>=inputs.semantics.size())
      throw UnsupportedVertexProgram("menu raw vertex program consumes an external constant attribute");
  std::ostringstream text;text<<"#version 450\n// Halo2 raw class0..6 vertex catalog ABI2.\n";
  for(unsigned i=0;i<inputs.semantics.size();++i)
    text<<"layout(location="<<i<<") in "<<(packed(inputs.semantics[i])?"int":"vec4")
        <<" v"<<i<<(packed(inputs.semantics[i])?"_cmp":"")<<";\n";
  text<<interface<<source.helpers<<fetch_helpers<<"void main() {\n";
  for(const auto input:source.consumed_inputs)
    text<<"    vpInput["<<input<<"]="
        <<(packed(inputs.semantics[input])?"sourcePackedNormal(v":"v")<<input
        <<(packed(inputs.semantics[input])?"_cmp)":"")<<";\n";
  text<<source.statements<<epilogue;
  return {text.str(),source.consumed_inputs,source.consumed_constants};
}
const MenuWorldVertexCatalogEntry &menu_world_vertex_catalog_lookup(
    std::span<const MenuWorldVertexCatalogEntry> catalog,std::span<const std::byte> program,
    const SourceVertexDeclaration &inputs) {
  // Full module validation belongs to the generated-catalog build. This
  // trusted immutable storage needs only identity and compatibility admission.
  declaration(inputs);
  const MenuWorldVertexCatalogEntry *match{};
  for(const auto &entry:catalog) {
    if(entry.class_index!=inputs.class_index || !equal(entry.program,program) ||
       !equal(entry.semantics,inputs.semantics))continue;
    if(entry.abi!=menu_world_vertex_catalog_abi || entry.spirv.size()<5 ||
       entry.spirv[0]!=0x07230203u || entry.spirv[1]!=0x00010600u || entry.spirv[4]!=0)
      throw FormatError("menu raw vertex catalog entry has an incompatible ABI/SPIR-V header");
    if(match)throw FormatError("menu raw vertex catalog identity is ambiguous");
    match=&entry;
  }
  if(!match)throw UnsupportedVertexProgram("menu raw vertex program/declaration is absent from the build catalog");
  return *match;
}
} // namespace halo2

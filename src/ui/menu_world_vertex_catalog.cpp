#include "halo2/menu_world_vertex_catalog.hpp"
#include <algorithm>
#include <openssl/evp.h>

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
bool equal(std::span<const std::uint16_t> a,std::span<const std::uint16_t> b) {
  return a.size()==b.size() && std::equal(a.begin(),a.end(),b.begin());
}
}
const MenuWorldVertexCatalogEntry &menu_world_vertex_catalog_lookup(
    std::span<const MenuWorldVertexCatalogEntry> catalog,std::span<const std::byte> program,
    const SourceVertexDeclaration &inputs) {
  // Full module validation belongs to the generated-catalog build. This
  // trusted immutable storage needs only identity and compatibility admission.
  declaration(inputs);
  std::array<unsigned char,32> digest{};unsigned digest_bytes{};
  if(EVP_Digest(program.data(),program.size(),digest.data(),&digest_bytes,EVP_sha256(),nullptr)!=1 || digest_bytes!=digest.size())
    throw FormatError("menu vertex program SHA-256 failed");
  std::array<char,64> identity{};
  constexpr char hex[]="0123456789abcdef";
  for(std::size_t i=0;i<digest.size();++i){identity[2*i]=hex[digest[i]>>4];identity[2*i+1]=hex[digest[i]&15];}
  const MenuWorldVertexCatalogEntry *match{};
  for(const auto &entry:catalog) {
    if(entry.class_index!=inputs.class_index || entry.program_bytes!=program.size() ||
       entry.program_sha256!=std::string_view(identity.data(),identity.size()) ||
       !equal(entry.semantics,inputs.semantics))continue;
    if((entry.abi!=menu_world_vertex_catalog_abi && entry.abi!=menu_world_vertex_attribute_catalog_abi &&
        entry.abi!=menu_world_renderer_vertex_catalog_abi) || entry.spirv.size()<5 ||
       entry.spirv[0]!=0x07230203u || entry.spirv[1]!=0x00010600u || entry.spirv[4]!=0)
      throw FormatError("menu raw vertex catalog entry has an incompatible ABI/SPIR-V header");
    for(const auto reg:entry.consumed_inputs)if(reg>=inputs.semantics.size() &&
        (entry.abi!=menu_world_renderer_vertex_catalog_abi &&
         (entry.abi!=menu_world_vertex_attribute_catalog_abi || (reg!=10 && reg!=11))))
      throw FormatError("menu raw vertex catalog has an incompatible constant attribute read");
    if(match)throw FormatError("menu raw vertex catalog identity is ambiguous");
    match=&entry;
  }
  if(!match)throw UnsupportedVertexProgram("menu raw vertex program/declaration is absent from the build catalog");
  return *match;
}
} // namespace halo2

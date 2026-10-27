#include "halo2/menu_world_vertex_catalog.hpp"
#include "menu_world_vertex_metadata.generated.hpp"
#include <openssl/evp.h>
#include <algorithm>

namespace halo2 {
const MenuWorldVertexMetadataEntry &menu_world_vertex_metadata_lookup(
    std::span<const std::byte> bytes,const SourceVertexDeclaration& declaration) {
    if(bytes.empty() || declaration.class_index>6 || declaration.semantics.empty() || declaration.semantics.size()>16)
        throw UnsupportedVertexProgram("Raw vertex metadata requires its complete original program/declaration identity");
    std::array<unsigned char,32> digest{};unsigned size{};
    if(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&size,EVP_sha256(),nullptr)!=1 || size!=digest.size())
        throw FormatError("Cannot hash complete raw vertex metadata identity");
    constexpr std::string_view alphabet="0123456789abcdef";
    std::array<char,64> text{};
    for(unsigned index=0;index<digest.size();++index) {
        text[index*2]=alphabet[digest[index]>>4];text[index*2+1]=alphabet[digest[index]&15];
    }
    const std::string_view identity(text.data(),text.size());
    const MenuWorldVertexMetadataEntry* selected=nullptr;
    for(const auto& row:generated_menu_world_vertex_metadata::entries) {
        if(row.class_index!=declaration.class_index || row.program_bytes!=bytes.size() || row.program_sha256!=identity ||
           !std::ranges::equal(row.semantics,declaration.semantics))continue;
        if(selected)throw FormatError("Raw vertex metadata identity is ambiguous");
        selected=&row;
    }
    if(!selected)throw UnsupportedVertexProgram("Raw vertex program/declaration has no immutable native metadata");
    return *selected;
}
} // namespace halo2

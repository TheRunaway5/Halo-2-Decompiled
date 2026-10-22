#include "assets/tag_owner_index.hpp"
#include "halo2/source_shader_texture.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <map>

namespace halo2 {
namespace {
Bytes rasterizer(const SourceTagMetadataProviders &metadata,std::uint32_t datum) {
    const auto &globals=metadata.definition(datum);
    if(globals.group()!="matg")throw FormatError("Shader texture globals are not matg");
    const Bytes root(globals.bytes_at(globals.source_address(),globals.size()));
    if(root.u32(0x108)!=1)throw FormatError("Shader texture rasterizer row unavailable");
    return Bytes(metadata.read(root.u32(0x10c),0xd4));
}
std::uint32_t globals(const Cache &cache) {
    const auto tag=cache.query_tag({.group=tag_groups::globals});
    if(!tag)throw FormatError("Shader texture matg unavailable");
    return tag->datum;
}
// Serialized caches can share postprocess arrays across tag payloads. Resolve
// the actual containing allocation instead of treating every pointer as local
// to the tag whose root supplied it. Retain each backing for the whole call.
class Reader {
public:
    explicit Reader(const Cache &cache):cache_(cache){}
    const SourceTagMetadata &definition(std::uint32_t datum) {
        const auto found=owned_.find(datum);
        if(found!=owned_.end())return found->second;
        return owned_.emplace(datum,cache_.load_source_tag_metadata(datum)).first->second;
    }
    std::span<const std::byte> read(std::uint32_t address,std::size_t extent) {
        if(!extent || std::uint64_t(extent)>(std::uint64_t{1}<<32)-address)
            throw FormatError("Shader texture metadata has invalid extent");
        const Tag *owner=nullptr;
        for(const auto *candidate:indexed_tag_owners(cache_,{.contains=TagAddressRange{address,extent}, .require_address=true})) {
        const auto &tag=*candidate;
            if(tag.group=="sbsp" || tag.group=="ltmp")
                continue;
            if(owner)throw FormatError("Shader texture metadata has ambiguous resident backing");
            owner=&tag;
        }
        if(!owner)throw FormatError("Shader texture pointer escapes owned resident backing");
        return definition(owner->datum).bytes_at(address,extent);
    }
    SourceTagMetadataProviders providers() {
        return {[this](auto datum)->const SourceTagMetadata &{return definition(datum);},
            [this](auto datum){return definition(datum).source_address();},
            [this](auto address,auto extent){return read(address,extent);}};
    }
private:
    const Cache &cache_;
    std::map<std::uint32_t,SourceTagMetadata> owned_;
};
struct ShaderBitmapBank {
    std::uint32_t bitmap_address{},bitmap_count{};
    std::int16_t bump{};
};
ShaderBitmapBank bitmap_bank(const SourceTagMetadataProviders &metadata,std::uint32_t shader) {
    const auto &tag=metadata.definition(shader);
    if(tag.group()!="shad")throw FormatError("Shader texture owner is not shad");
    const Bytes root(tag.bytes_at(tag.source_address(),tag.size()));
    if(!root.u32(0x20))throw FormatError("Shader texture postprocess bank unavailable");
    const Bytes post(metadata.read(root.u32(0x24),0x64));
    // The source reads the fifth four-byte bitmap-property record's signed
    // short. It is not a byte-sized parameter or a bitmap-name heuristic.
    if(post.u32(0x5c)<5)throw FormatError("Shader bump property unavailable");
    const Bytes properties(metadata.read(post.u32(0x60),20));
    const auto bump=std::bit_cast<std::int16_t>(properties.u16(0x10));
    const auto address=post.u32(8),count=post.u32(4);
    return {address,count,bump};
}
}
std::uint32_t source_camouflage_shader(const SourceTagMetadataProviders &metadata,std::uint32_t globals) {
    const auto row=rasterizer(metadata,globals);
    if(row.u32(0xcc)!=0x73686164u)throw FormatError("Camouflage global reference is not shad");
    return row.u32(0xd0);
}
std::uint32_t source_camouflage_shader(const Cache &cache) {
    Reader reader(cache);return source_camouflage_shader(reader.providers(),globals(cache));
}
SourceShaderTextureReference source_shader_texture_reference(
    const SourceTagMetadataProviders &metadata,std::uint32_t globals,
    std::uint32_t shader,unsigned external) {
    if(external==1) {
        const auto row=rasterizer(metadata,globals);
        if(row.u32(8)!=0x6269746du)throw FormatError("Normalization global reference is not bitm");
        return {row.u32(0xc),0};
    }
    if(external!=26)throw FormatError("Shader texture extern requires its separate source owner");
    auto bank=bitmap_bank(metadata,shader);
    if(bank.bump==-1)bank=bitmap_bank(metadata,source_camouflage_shader(metadata,globals));
    if(bank.bump<0 || unsigned(bank.bump)>=bank.bitmap_count)
        throw FormatError("Shader bump index outside authored bitmap bank");
    const auto address=std::uint64_t(bank.bitmap_address)+unsigned(bank.bump)*12u;
    if(address>0xffffffffu)throw FormatError("Shader bump reference address overflow");
    const Bytes bitmap(metadata.read(std::uint32_t(address),6));
    return {bitmap.u32(0),bitmap.u16(4)};
}
SourceShaderTextureReference source_shader_texture_reference(
    const Cache &cache,std::uint32_t shader,unsigned external) {
    Reader reader(cache);return source_shader_texture_reference(reader.providers(),globals(cache),shader,external);
}
}

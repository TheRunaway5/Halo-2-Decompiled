#include "halo2/mcc_sound_class_reference.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <openssl/evp.h>
#include <set>

namespace halo2 {
namespace {
struct ClassContext {
    std::uint32_t globals_datum,globals_address,globals_size;
    std::uint32_t legacy_datum,legacy_address,legacy_size;
    std::string_view sound_block,sound_globals,class_root,source_sha256;
};
#include "audio/mcc_sound_class_reference.inc"
std::string hex(std::span<const std::byte> bytes) {
    std::string out;out.reserve(bytes.size()*2);
    for(auto b:bytes){const auto n=std::to_integer<unsigned>(b);out+="0123456789abcdef"[n>>4];out+="0123456789abcdef"[n&15];}
    return out;
}
std::string digest(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> result{};unsigned size{};
    if(EVP_Digest(bytes.data(),bytes.size(),result.data(),&size,EVP_sha256(),nullptr)!=1||size!=result.size())
        throw FormatError("Cannot authenticate named sound-class source");
    return hex({reinterpret_cast<const std::byte*>(result.data()),result.size()});
}
bool extent(std::uint32_t address,std::size_t bytes) {
    return address&&address!=0xffffffffu&&bytes&&bytes<=0x100000000ull-address;
}
bool contains(const Tag& owner,std::uint32_t address,std::size_t bytes) {
    return extent(owner.address,owner.size)&&extent(address,bytes)&&address>=owner.address&&
        std::uint64_t(address)+bytes<=std::uint64_t(owner.address)+owner.size;
}
std::vector<std::byte> read(const MccSoundRuntimeSource& source,std::uint32_t address,std::size_t bytes) {
    if(!extent(address,bytes))throw FormatError("Named sound-class source extent is unowned");
    const auto result=source.read(address,bytes);
    if(result.size()!=bytes)throw FormatError("Named sound-class source callback returned a partial view");
    return {result.begin(),result.end()};
}
void put(std::vector<std::byte>& bytes,std::size_t offset,std::uint32_t value) {
    Bytes(bytes).require(offset,4);for(unsigned n=0;n<4;++n)bytes[offset+n]=std::byte(value>>(n*8));
}
}
MccSoundClassReferenceProjection project_mcc_sound_class_reference(
    const MccSoundRuntimeSource& input,TagHandle legacy_owner,
    std::span<const std::byte> input_candidate) {
    MccSoundClassReferenceProjection out;
    if(!input.read||legacy_owner.is_null()||input.tags.size()>65536)return out;
    // A callback may reset the caller's wrapper or release its containers
    // while its immutable Cache views remain valid. Retain every callable,
    // typed string/row and complete candidate before invoking any callback.
    const std::vector<Tag> owned_tags(input.tags.begin(),input.tags.end());
    const std::vector<std::byte> owned_candidate=input_candidate.size()==54*92?
        std::vector<std::byte>(input_candidate.begin(),input_candidate.end()):std::vector<std::byte>{};
    // Copying a user callable can itself run its copy constructor. The caller's
    // tag/candidate containers must already be independent before that copy.
    auto source=input;
    source.tags=owned_tags;
    const std::span<const std::byte> candidate=owned_candidate;
    const Tag* globals{};const Tag* legacy{};std::set<std::uint32_t> datums,indices;
    for(const auto& tag:source.tags)if(!tag.handle().is_null()&&tag.address) {
        if(!datums.insert(tag.datum).second||!indices.insert(tag.handle().index()).second)return out;
        if(tag.group=="matg") {if(globals)return out;globals=&tag;}
        if(tag.handle()==legacy_owner)legacy=&tag;
    }
    if(!globals||!legacy||globals->name!="globals\\globals"||legacy->group!="sncl"||
        legacy->name!="sound\\sound_classes"||!contains(*globals,globals->address,0xc8)||
        !contains(*legacy,legacy->address,8))return out;
    // Every published sncl allocation is an actual exclusive directory owner.
    // A foreign root inside it cannot be silently corrected via this profile.
    for(const auto& tag:source.tags)if(tag.address&&tag.size&&tag.handle()!=legacy_owner&&
        std::uint64_t(tag.address)<std::uint64_t(legacy->address)+legacy->size&&
        std::uint64_t(tag.address)+tag.size>legacy->address)return out;
    const auto context=std::ranges::find_if(class_contexts,[&](const auto& c){return
        c.globals_datum==globals->datum&&c.globals_address==globals->address&&c.globals_size==globals->size&&
        c.legacy_datum==legacy->datum&&c.legacy_address==legacy->address&&c.legacy_size==legacy->size;});
    if(context==class_contexts.end())return out;
    // Multiple source contexts can share directory identities, but differ in
    // the exact pointer-bearing route and complete class extension bank.
    const auto block=read(source,globals->address+0xc0u,8);const Bytes b(block);
    if(b.u32(0)!=1||!contains(*globals,b.u32(4),44))return out;
    const auto sound=read(source,b.u32(4),44);const Bytes s(sound);
    if(s.u32(8)!=TagGroup{"sncl"}.value||s.u32(12)!=legacy_owner.value)return out;
    const auto root=read(source,legacy->address,8);const Bytes r(root);
    if(r.u32(0)!=54||!contains(*legacy,r.u32(4),54*120)||r.u32(4)<std::uint64_t(legacy->address)+8u)return out;
    const auto rows=read(source,r.u32(4),54*120);const auto sha=digest(rows);
    const auto block_hex=hex(block),sound_hex=hex(sound),root_hex=hex(root);
    if(!std::ranges::any_of(class_contexts,[&](const auto& c){return
        c.globals_datum==globals->datum&&c.globals_address==globals->address&&c.globals_size==globals->size&&
        c.legacy_datum==legacy->datum&&c.legacy_address==legacy->address&&c.legacy_size==legacy->size&&
        c.sound_block==block_hex&&c.sound_globals==sound_hex&&c.class_root==root_hex&&c.source_sha256==sha;}))return out;
    out.owner=legacy_owner;out.source_bank_sha256=sha;out.status=MccSoundClassReferenceStatus::CandidateMismatch;
    if(candidate.size()!=54*92)return out;
    const auto before=project_mcc_sound_class_records(rows);auto after=before;
    for(const auto& word:class_words) {
        const auto offset=std::size_t(word.class_index)*92+word.offset;
        if(Bytes(before).u32(offset)!=word.before_bits)
            throw FormatError("Authenticated named sound-class source differs from its typed word profile");
        put(after,offset,word.after_bits);
    }
    if(std::ranges::equal(candidate,after))out.status=MccSoundClassReferenceStatus::AlreadyEquivalent;
    else if(std::ranges::equal(candidate,before)) {
        out.status=MccSoundClassReferenceStatus::Converted;out.changed_words=48;
    } else return out;
    out.words=class_words;out.restored_named_rows=25;out.native_records=std::move(after);
    return out;
}
} // namespace halo2

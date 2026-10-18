#include "halo2/mcc_sound_vanilla_reference.hpp"
#include "mcc_sound_vanilla_reference_profile.hpp"
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/mcc_ordinary_pointer.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <map>
#include <memory>
#include <openssl/evp.h>
#include <set>

namespace halo2 {
namespace {
class Hash {
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> context{EVP_MD_CTX_new(),EVP_MD_CTX_free};
public:
    Hash(){if(!context||EVP_DigestInit_ex(context.get(),EVP_sha256(),nullptr)!=1)throw FormatError("Cannot initialize sound reference digest");}
    void bytes(std::span<const std::byte> data){if(!data.empty()&&EVP_DigestUpdate(context.get(),data.data(),data.size())!=1)throw FormatError("Cannot hash sound reference bytes");}
    void word(std::uint32_t value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(8*n));bytes(b);}
    void text(std::string_view value){if(value.size()>0x100000)throw FormatError("Excessive sound identity string");word(std::uint32_t(value.size()));bytes(std::as_bytes(std::span(value.data(),value.size())));}
    MccSoundVanillaDigest finish(){MccSoundVanillaDigest result;unsigned size=0;if(EVP_DigestFinal_ex(context.get(),reinterpret_cast<unsigned char*>(result.data()),&size)!=1||size!=result.size())throw FormatError("Cannot finish sound reference digest");return result;}
};
class Reader {
    const MccSoundVanillaSource& source;
    std::map<std::uint32_t,const Tag*> tags;
public:
    explicit Reader(const MccSoundVanillaSource& s):source(s){
        if(!s.metadata.read||!s.metadata.gestalt_owner||!s.string_id)throw FormatError("Sound reference needs typed metadata and StringID owners");
        for(const auto& t:s.metadata.tags)if(!t.handle().is_null()&&!tags.emplace(t.datum,&t).second)throw FormatError("Duplicate full-salt sound reference namespace");
    }
    const auto& all()const{return tags;}
    std::span<const std::byte> read(std::uint32_t address,std::size_t size)const{
        if(size>128u*1024u*1024u||!address||address==0xffffffffu||size>0x100000000ull-address)throw FormatError("Sound reference metadata extent is invalid");
        const auto bytes=source.metadata.read(address,size);if(bytes.size()!=size)throw FormatError("Sound reference metadata extent is partial");return bytes;
    }
    const Tag& tag(TagHandle h,std::string_view group)const{
        const auto i=tags.find(h.value);if(h.is_null()||i==tags.end()||!i->second->address||i->second->group!=group)throw FormatError("Sound reference lost full-salt typed owner");return *i->second;
    }
    void identity(Hash& hash,const Tag& t)const{hash.word(t.datum);hash.word(t.address);hash.word(t.size);hash.text(t.group);hash.text(t.name);}
    TagBlock block(std::span<const std::byte> root,unsigned offset,unsigned stride)const{
        const Bytes b(root);const auto count=b.u32(offset),pointer=b.u32(offset+4);if(count>0x100000||std::uint64_t(count)*stride>128u*1024u*1024u)throw FormatError("Sound reference block exceeds format bounds");if(count)(void)read(pointer,std::size_t(count)*stride);return {std::int32_t(count),pointer};
    }
    std::span<const std::byte> rows(const TagBlock& b,unsigned stride)const{return b.count?read(b.address,std::size_t(b.count)*stride):std::span<const std::byte>{};}
    void graph(Hash& hash,const Tag& t,unsigned root_size)const{
        identity(hash,t);hash.bytes(read(t.address,root_size));
        const auto fields=ordinary_mcc_pointer_fields(t.type(),t.address,[&](auto a,auto n){return read(a,n);});
        if(!fields)throw FormatError("Sound reference lacks a pinned complete metadata schema");hash.word(std::uint32_t(fields->size()));
        for(const auto& f:*fields){hash.word(f.field_address);hash.word(f.address);hash.word(f.bytes);if(f.bytes)hash.bytes(read(f.address,f.bytes));}
    }
    TagHandle owner(const Tag& t)const{const auto h=source.metadata.gestalt_owner(t.handle());(void)tag(h,"ugh!");return h;}
    std::pair<std::uint32_t,std::string> name(std::span<const std::byte> imports,std::uint16_t index)const{
        if(std::bit_cast<std::int16_t>(index)<0)throw FormatError("Sound reference import-name ordinal is negative");const auto sid=Bytes(imports).u32(std::size_t(index)*4);auto name=source.string_id(sid);if(name.empty())throw FormatError("Sound reference import name is empty");return {sid,std::move(name)};
    }
};
std::uint16_t normalized(std::uint16_t source){return std::uint16_t((source&0x03ffu)|((source&0xf000u)>>2));}
}
namespace mcc_sound_reference_detail {
std::vector<Tag> raw_globals_namespace(const MccSoundVanillaSource& source,std::span<const Tag> retained){
    Reader reader(source);std::vector<Tag> tags(source.metadata.tags.begin(),source.metadata.tags.end());
    Tag* active=nullptr;const Tag* raw=nullptr;
    for(auto& tag:tags)if(tag.address&&tag.group=="matg"){
        if(active)throw FormatError("Sound reference globals namespace is ambiguous");active=&tag;
    }
    if(!active||active->handle().is_null())throw FormatError("Sound reference globals owner is absent");
    for(const auto& tag:retained)if(tag.group=="matg"||tag.datum==active->datum){
        if(raw||tag.datum!=active->datum||tag.group!=active->group||tag.name!=active->name)
            throw FormatError("Retained sound globals full-salt identity is ambiguous or differs");
        raw=&tag;
    }
    if(!raw)throw FormatError("Retained raw sound globals owner is absent");
    const auto sound_row=[&](const Tag& tag){
        if(tag.size<0xc8||std::uint64_t(tag.address)+tag.size>0x100000000ull)
            throw FormatError("Sound globals declared owner extent is invalid");
        // Verify the entire declared allocation before accepting its identity.
        // Both authored and cloned rows are contained by their real matg owner.
        const auto allocation=reader.read(tag.address,tag.size);const Bytes bytes(allocation);
        const auto count=bytes.u32(0xc0),pointer=bytes.u32(0xc4);
        if(count!=1||pointer<tag.address||std::uint64_t(pointer)-tag.address+0x2c>tag.size)
            throw FormatError("Sound globals row escapes its exact declared owner");
        return bytes.slice(pointer-tag.address,0x2c);
    };
    const auto active_row=sound_row(*active),raw_row=sound_row(*raw);
    if(std::uint64_t(active->address)<std::uint64_t(raw->address)+raw->size&&
       std::uint64_t(raw->address)<std::uint64_t(active->address)+active->size)
        throw FormatError("Active and retained sound globals allocations overlap");
    if(!std::ranges::equal(active_row,raw_row))
        throw FormatError("Active sound globals differ from the retained raw owner");
    *active=*raw;return tags;
}
MccSoundVanillaDigest metadata_digest(const MccSoundVanillaSource& source,TagHandle owner){
    Reader r(source);const auto& gestalt=r.tag(owner,"ugh!");Hash hash;hash.text("H2_MCC_SOUND_VANILLA_OWNER_V1");
    // Directory identity and full headers are authenticated for the COMPLETE
    // selected owning gestalt, not only the paths with a proposed change.
    unsigned sounds=0;for(const auto&[_,t]:r.all())if(t->address&&t->group=="snd!"&&r.owner(*t)==owner)++sounds;hash.word(sounds);
    for(const auto&[_,t]:r.all())if(t->address&&t->group=="snd!"&&r.owner(*t)==owner){r.identity(hash,*t);hash.bytes(r.read(t->address,28));}
    r.graph(hash,gestalt,120);const auto root=r.read(gestalt.address,120);const auto imports=r.rows(r.block(root,0x18,4),4);hash.word(std::uint32_t(imports.size()/4));
    for(std::size_t n=0;n<imports.size();n+=4){const auto sid=Bytes(imports).u32(n);hash.word(sid);hash.text(source.string_id(sid));}
    unsigned classes=0,globals=0;for(const auto&[_,t]:r.all())if(t->address&&t->group=="sncl")++classes;hash.word(classes);
    for(const auto&[_,t]:r.all())if(t->address&&t->group=="sncl")r.graph(hash,*t,8);
    for(const auto&[_,t]:r.all())if(t->address&&t->group=="matg"){
        ++globals;r.identity(hash,*t);const auto descriptor=r.read(t->address+0xc0,8);hash.bytes(descriptor);const auto block=r.block(descriptor,0,0x2c);if(block.count!=1)throw FormatError("Sound reference globals do not have one owning record");hash.bytes(r.rows(block,0x2c));
    }
    if(!classes||globals!=1)throw FormatError("Sound reference class/global namespace is incomplete");hash.word(globals);return hash.finish();
}
MccSoundVanillaFlagPlan plan(const MccSoundVanillaSource& source,std::span<const Owner> profiles){
    Reader r(source);MccSoundVanillaFlagPlan out;std::set<std::uint32_t> profile_owners;
    for(const auto& p:profiles)if(!profile_owners.insert(p.handle.value).second)throw FormatError("Ambiguous canonical sound reference owner profile");
    for(const auto&[_,tag]:r.all())if(tag->address&&tag->group=="ugh!"){
        const auto found=std::ranges::find(profiles,tag->handle(),&Owner::handle);
        if(found==profiles.end()){out.owners.push_back({tag->handle(),MccSoundVanillaOwnerStatus::Unmatched,"No authenticated finite authored flag profile"});continue;}
        const auto& p=*found;std::vector<MccSoundVanillaFlagCorrection> staged;
        try{
            if(tag->address!=p.root||tag->name!=p.path||metadata_digest(source,tag->handle())!=p.metadata)throw FormatError("Complete sound owner metadata differs from the authenticated retail profile");
            if(!source.payload)throw FormatError("Sound reference has no actual encoded payload identity owner");
            const auto root=r.read(tag->address,120);const auto ranges=r.rows(r.block(root,0x28,12),12),permutations=r.rows(r.block(root,0x30,16),16),imports=r.rows(r.block(root,0x18,4),4);
            std::set<std::uint32_t> targets;
            for(const auto& s:p.sounds){
                const auto& target=r.tag(s.handle,"snd!");if(!targets.insert(target.datum).second||target.address!=s.root||target.name!=s.path||r.owner(target)!=p.handle)throw FormatError("Authored flag target namespace or ownership differs");
                if(std::ranges::count_if(r.all(),[&](const auto& v){return v.second->address&&v.second->group=="snd!"&&v.second->name==s.path;})!=1)throw FormatError("Authored flag target path is ambiguous");
                const auto raw=r.read(target.address,28);if(!std::ranges::equal(raw,s.header))throw FormatError("Authored sound header differs from its exact preimage");const Bytes b(raw);const auto before=normalized(b.u16(0));
                if(before!=s.before||!s.before||s.before==s.after||((s.before^s.after)!=4u&&(s.before^s.after)!=8u&&(s.before^s.after)!=32u)||((s.before^s.after)&s.after))throw FormatError("Authored flag delta exceeds the verified low-bit restoration domain");
                const auto first=b.u16(8);const auto count=std::to_integer<unsigned>(raw[10]);if(std::bit_cast<std::int16_t>(first)<0||!count||std::size_t(first)+count>ranges.size()/12)throw FormatError("Authored sound pitch interval escapes its owner");
                std::size_t expected=0;std::set<std::string> range_names;
                for(unsigned pitch=0;pitch<count;++pitch){
                    const auto global_range=first+pitch;const Bytes range(ranges.subspan(std::size_t(global_range)*12,12));const auto range_name=r.name(imports,range.u16(0));if(!range_names.insert(range_name.second).second)throw FormatError("Authored pitch names are ambiguous");
                    const auto perm_first=range.u16(8),perm_count=range.u16(10);if(std::bit_cast<std::int16_t>(perm_first)<0||std::bit_cast<std::int16_t>(perm_count)<=0||std::size_t(perm_first)+perm_count>permutations.size()/16)throw FormatError("Authored permutation interval escapes its owner");std::set<std::string> names;
                    for(unsigned n=0;n<perm_count;++n){
                        const auto global_perm=perm_first+n;const Bytes perm(permutations.subspan(std::size_t(global_perm)*16,16));const auto name=r.name(imports,perm.u16(0));if(!names.insert(name.second).second)throw FormatError("Authored permutation names are ambiguous");
                        if(expected>=s.permutations.size())throw FormatError("Authored payload profile omits an owned permutation");const auto& e=s.permutations[expected++];
                        if(e.range!=pitch||e.permutation!=n||e.global_range!=global_range||e.global_permutation!=global_perm||e.range_import!=range.u16(0)||e.permutation_import!=perm.u16(0)||e.range_sid!=range_name.first||e.permutation_sid!=name.first||e.range_name!=range_name.second||e.permutation_name!=name.second)throw FormatError("Authored permutation typed identity differs");
                        if(source.payload(target.handle(),pitch,n)!=e.payload)throw FormatError("Actual encoded sound payload/packet/format differs from retail");
                    }
                }
                if(expected!=s.permutations.size())throw FormatError("Authored payload profile adds a foreign permutation");staged.push_back({target.handle(),p.handle,target.address,b.u16(0),s.before,s.after});
            }
            if(staged.empty())throw FormatError("Empty authored flag profile cannot authorize correction");out.corrections.insert(out.corrections.end(),staged.begin(),staged.end());out.owners.push_back({p.handle,MccSoundVanillaOwnerStatus::Authenticated,"Complete metadata, typed names and actual encoded payloads match"});
        }catch(const FormatError& e){out.owners.push_back({p.handle,MccSoundVanillaOwnerStatus::Custom,e.what()});}
    }
    return out;
}
}
// Immutable generated catalog is populated only after payload facts pass the
// independent DAT-slice audit. No metadata-only profile is admitted.
#include "mcc_sound_vanilla_reference_profiles.inc"
MccSoundVanillaFlagPlan plan_mcc_sound_vanilla_flags(const MccSoundVanillaSource& source){return mcc_sound_reference_detail::plan(source,canonical_sound_flag_profiles());}
MccSoundVanillaFlagPlan plan_mcc_sound_vanilla_flags(const Cache& cache){
    if(!cache.is_mcc())throw FormatError("MCC sound vanilla reference requires an MCC Cache owner");
    const MccSoundVanillaSource source{{cache.tags(),[&](auto a,auto n){return cache.read_tag_bytes(a,n);},[&](TagHandle h){return cache.mcc_sound_gestalt_owner(h).handle();},{}},[&](auto sid){return cache.string_id(sid);},[&](TagHandle h,unsigned pitch,unsigned permutation){
        const auto sound=cache.load_sound(h.value,pitch,permutation);Hash hash;hash.bytes(sound.data);MccSoundVanillaPayload out{hash.finish(),sound.data.size(),sound.codec,sound.sample_rate,sound.channels,sound.samples,sound.sound_class,{}};for(const auto& p:sound.packet_chunks)out.packets.push_back({p.bytes,p.pcm_bytes,p.reset_after});return out;
    }};
    try{
        const auto tags=mcc_sound_reference_detail::raw_globals_namespace(source,cache.retained_tag_allocations());
        auto canonical=source;canonical.metadata.tags=tags;
        return mcc_sound_reference_detail::plan(canonical,raw_globals_sound_flag_profiles());
    }catch(const FormatError& error){
        MccSoundVanillaFlagPlan out;
        for(const auto& tag:cache.tags())if(tag.address&&tag.group=="ugh!"){
            const auto profiles=raw_globals_sound_flag_profiles();
            const bool known=std::ranges::find(profiles,tag.handle(),&mcc_sound_reference_detail::Owner::handle)!=profiles.end();
            out.owners.push_back({tag.handle(),known?MccSoundVanillaOwnerStatus::Custom:MccSoundVanillaOwnerStatus::Unmatched,error.what()});
        }
        return out;
    }
}
} // namespace halo2

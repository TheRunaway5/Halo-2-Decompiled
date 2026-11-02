#include "halo2/cache.hpp"
#include "halo2/mcc_gameplay_reference.hpp"
#include <functional>
#include "tags/directory.hpp"
#include "tags/native_directory.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <map>

namespace halo2 {
namespace {
// Assembly fd14ef3c46135e9422966c6a64cd926fb0bd23fb Halo2MCC snd!/ugh!.
// Keep the four real Classic/Remastered roots and only their referenced table
// rows. All resource offsets, sample extents, codec bytes and controls remain
// authored MCC data. This is a native namespace projection, not replacement PCM.
constexpr std::array<std::string_view,4> zoom_names{
    "sound\\weapons\\sniper_rifle\\zoom_in","sound\\weapons\\sniper_rifle\\zoom_out",
    "sound_remastered\\weapons\\sniper_rifle\\zoom_in\\zoom_in",
    "sound_remastered\\weapons\\sniper_rifle\\zoom_out\\zoom_out"};
void require(bool ok,const char *why){if(!ok)throw FormatError(why);}
void put(std::vector<std::byte>& bytes,std::size_t at,std::uint32_t value,unsigned width=4) {
    Bytes(bytes).require(at,width);for(unsigned n=0;n<width;++n)bytes[at+n]=std::byte(value>>(8*n));
}
struct Projection {
    std::vector<std::byte> gestalt;
    std::array<std::vector<std::byte>,4> sounds;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> pointers;
    std::array<std::uint32_t,2> remastered_fields{};
};
Projection project(const Cache& source,const std::array<Tag,4>& sounds,
    const std::function<std::uint32_t(std::uint32_t)>& string_id) {
    const auto &gestalt=source.mcc_sound_gestalt_owner(sounds[0].handle());
    struct Block {std::uint32_t count,address;unsigned stride;};
    const auto block=[&](std::uint32_t at,unsigned stride) {
        const Bytes b(source.read_tag_bytes(at,8));const auto count=b.u32(0),address=b.u32(4);
        require(count<=65536 && (!count || (address && address!=0xffffffffu)),"Invalid zoom dependency block");
        if(count)source.read_tag_bytes(address,std::size_t(count)*stride);
        return Block{count,address,stride};
    };
    const auto row=[&](Block b,unsigned index) {
        require(index<b.count,"Zoom dependency ordinal escapes its authored table");
        return source.read_tag_bytes(b.address+index*b.stride,b.stride);
    };
    constexpr std::array<unsigned,7> offsets{0,8,16,24,32,40,48};
    constexpr std::array<unsigned,7> strides{3,56,20,4,10,12,16};
    std::array<Block,7> input{};for(unsigned n=0;n<7;++n)input[n]=block(gestalt.address+offsets[n],strides[n]);
    const auto languages=block(gestalt.address+0x60,8),remastered=block(gestalt.address+0x50,8);
    require(languages.count>0 && languages.count<=16,"Zoom dependencies need their actual localized chunks");
    std::vector<Block> chunk_sources;for(unsigned n=0;n<languages.count;++n)chunk_sources.push_back(block(languages.address+n*8,16));
    std::array<std::vector<std::byte>,7> banks;
    std::array<std::map<unsigned,unsigned>,5> indices;
    const auto selected=[&](unsigned table,unsigned index) {
        auto &map=indices.at(table);if(const auto i=map.find(index);i!=map.end())return i->second;
        auto bytes=row(input[table],index);const auto next=unsigned(banks[table].size()/strides[table]);
        banks[table].insert(banks[table].end(),bytes.begin(),bytes.end());
        if(table==3)put(banks[table],next*4,string_id(Bytes(bytes).u32(0)));
        map.emplace(index,next);return next;
    };
    Projection out;
    std::array<std::vector<std::byte>,4> localized;
    std::vector<std::vector<std::byte>> chunks(languages.count);
    for(unsigned n=0;n<4;++n) {
        require(source.mcc_sound_gestalt_owner(sounds[n].handle()).handle()==gestalt.handle(),"Zoom roots have different source gestalts");
        const auto raw=source.read_tag_bytes(sounds[n].address,28);out.sounds[n]={raw.begin(),raw.end()};
        auto &sound=out.sounds[n];const Bytes s(raw);
        require(std::to_integer<unsigned>(raw[10])==1 && s.u16(14)==0xffff &&
            std::to_integer<unsigned>(raw[12])==255 && std::to_integer<unsigned>(raw[13])==255 &&
            s.u16(24)==0xffff && std::to_integer<unsigned>(raw[26])==0 && std::to_integer<unsigned>(raw[27])==255,
            "Zoom dependency requires an unsupported authored extra/reflection/promotion owner");
        require((s.u16(0)&0xc00u)==(n<2?0x400u:0x800u),"Zoom dependency lost its Classic/Remastered route");
        put(sound,3,selected(0,std::to_integer<unsigned>(raw[3])),1);
        if(s.u16(6)!=0xffff)put(sound,6,selected(1,s.u16(6)),2);
        if(std::to_integer<unsigned>(raw[11])!=255)put(sound,11,selected(2,std::to_integer<unsigned>(raw[11])),1);
        const auto range=row(input[5],s.u16(8));std::vector<std::byte> r(range.begin(),range.end());const Bytes rb(range);
        require(rb.u16(6)==0xffff && rb.u16(10)==1,"Zoom dependency needs a different permutation/flag owner");
        if(rb.u16(0)!=0xffff)put(r,0,selected(3,rb.u16(0)),2);
        if(rb.u16(2)!=0xffff)put(r,2,selected(4,rb.u16(2)),2);
        put(sound,8,n,2);put(r,8,n,2);banks[5].insert(banks[5].end(),r.begin(),r.end());
        const auto permutation=row(input[6],rb.u16(8));std::vector<std::byte> p(permutation.begin(),permutation.end());
        const Bytes pb(permutation);if(pb.u16(0)!=0xffff)put(p,0,selected(3,pb.u16(0)),2);
        const auto info=block(input[6].address+rb.u16(8)*16+8,8);
        require(info.count==languages.count,"Zoom permutation has a different localized owner count");
        for(unsigned language=0;language<info.count;++language) {
            const auto value=row(info,language);std::vector<std::byte> item(value.begin(),value.end());const Bytes ib(value);
            const auto first=ib.u16(4),count=ib.u16(6);
            require(std::uint64_t(first)+count<=chunk_sources[language].count,"Zoom chunks escape their language owner");
            const auto next=unsigned(chunks[language].size()/16);
            require(next+count<32768,"Zoom chunks exceed their signed index");put(item,4,next,2);
            for(unsigned j=0;j<count;++j) {
                const auto chunk=row(chunk_sources[language],first+j);
                require(Bytes(chunk).u32(12)==0xffffffffu,"Zoom dependency contains a foreign runtime cache handle");
                chunks[language].insert(chunks[language].end(),chunk.begin(),chunk.end());
            }
            localized[n].insert(localized[n].end(),item.begin(),item.end());
        }
        banks[6].insert(banks[6].end(),p.begin(),p.end());
        if(n<2) {
            const Bytes ref(row(remastered,s.u16(4)));
            require(ref.u32(0)==tag_groups::sound.value && ref.u32(4)==sounds[n+2].datum,
                "Zoom remastered reference has a different full-salt owner");
            put(sound,4,n,2);
        }else require(s.u16(4)==0xffff,"Remastered zoom has an unexpected secondary reference");
    }
    out.gestalt.resize(0x78);
    const auto append=[&](std::span<const std::byte> bytes) {
        const auto offset=(out.gestalt.size()+3)&~std::size_t(3);require(offset+bytes.size()<=65536,"Zoom dependency exceeds its owned metadata budget");
        out.gestalt.resize(offset);out.gestalt.insert(out.gestalt.end(),bytes.begin(),bytes.end());return std::uint32_t(offset);
    };
    const auto pointer=[&](unsigned field,unsigned target) {put(out.gestalt,field,target);out.pointers.emplace_back(field,target);};
    std::uint32_t permutation_bank{};
    for(unsigned n=0;n<7;++n)if(!banks[n].empty()) {
        const auto target=append(banks[n]);put(out.gestalt,offsets[n],banks[n].size()/strides[n]);pointer(offsets[n]+4,target);
        if(n==6)permutation_bank=target;
    }
    for(unsigned n=0;n<4;++n)pointer(permutation_bank+n*16+12,append(localized[n]));
    const auto language_bank=append(std::vector<std::byte>(languages.count*8));put(out.gestalt,0x60,languages.count);pointer(0x64,language_bank);
    for(unsigned n=0;n<languages.count;++n) {
        put(out.gestalt,language_bank+n*8,chunks[n].size()/16);
        if(!chunks[n].empty())pointer(language_bank+n*8+4,append(chunks[n]));
    }
    const auto references=append(std::vector<std::byte>(16));put(out.gestalt,0x50,2);pointer(0x54,references);
    for(unsigned n=0;n<2;++n){put(out.gestalt,references+n*8,tag_groups::sound.value);out.remastered_fields[n]=references+n*8+4;}
    return out;
}
}

void Cache::admit_mcc_weapon_zoom_sounds() {
    if(preserve_mcc_authoring_ || name_!="01b_spacestation")return;
    const auto *weapon=query_tag({.group=tag_groups::weapon,.name="objects\\weapons\\rifle\\battle_rifle\\battle_rifle",.require_address=true});
    if(!weapon)return;
    const auto weapon_root=weapon->address;
    const auto original=read_tag_bytes(weapon->address+0x26c,16);const Bytes refs(original);
    for(unsigned n=0;n<2;++n) {
        const auto *sound=query_tag({.handle=TagHandle{refs.u32(n*8+4)},.group=tag_groups::sound,.require_address=true});
        const auto expected=n?"sound\\weapons\\battle_rifle\\zoom_out":"sound\\weapons\\battle_rifle\\zoom_in";
        if(refs.u32(n*8)!=tag_groups::sound.value || !sound || sound->name!=expected)return;
    }
    for(auto name:zoom_names)if(query_tag({.group=tag_groups::sound,.name=name}))return;
    Cache donor(shared_directory_/"single_player_shared.map",shared_directory_);
    require(donor.is_mcc(),"Zoom dependencies require the prepared MCC resource owner");
    std::array<Tag,4> sounds;
    for(unsigned n=0;n<4;++n) {
        const auto *t=donor.query_tag({.group=tag_groups::sound,.name=zoom_names[n],.require_address=true});
        require(t,"Prepared MCC bank lacks the exact sniper zoom dependency");sounds[n]=*t;
    }
    std::map<std::string,std::uint32_t> local_strings;
    const Bytes header(storage_);const auto count=header.u32(48),text=header.u32(52),size=header.u32(56),table=header.u32(60);
    require(count<=65536,"Zoom dependency StringID bank is excessive");
    for(unsigned n=0;n<count;++n) {
        const auto offset=header.u32(table+n*4);if(offset==0xffffffffu)continue;
        require(offset<size,"Zoom dependency StringID escapes its bank");const auto value=header.string(text+offset,size-offset);
        local_strings.emplace(value,(std::uint32_t(value.size())<<24)|n);
    }
    auto projection=project(donor,sounds,[&](auto id) {
        const auto name=donor.string_id(id);const auto found=local_strings.find(name);
        require(found!=local_strings.end(),"Zoom dependency import name lacks a local StringID owner");return found->second;
    });
    // Reserve the complete new directory/resource cohort before publication.
    tags_.reserve(tags_.size()+5);mcc_resource_owners_.reserve(tags_.size()+5);
    const auto gestalt=detail::insert_native_tag(tags_,"ugh!","native\\weapon_zoom\\sniper_rifle");
    std::array<TagHandle,4> handles;
    for(unsigned n=0;n<4;++n)handles[n]=detail::insert_native_tag(tags_,"snd!",std::string(zoom_names[n]));
    for(unsigned n=0;n<2;++n)put(projection.gestalt,projection.remastered_fields[n],handles[n+2].value);
    const auto append=[&](TagHandle handle,std::vector<std::byte> bytes,bool relocate) {
        const auto current=std::uint64_t(std::int64_t(storage_.size())+metadata_magic_),next=(current+15)&~std::uint64_t(15);
        require(next+bytes.size()<0x100000000ull && next+bytes.size()-vista_metadata_regions_.front().first<=128ull*1024*1024,
            "Zoom dependency exceeds the native metadata address owner");
        if(relocate)for(const auto [field,target]:projection.pointers)put(bytes,field,std::uint32_t(next)+target);
        const auto offset=storage_.size()+std::size_t(next-current);storage_.resize(offset);storage_.insert(storage_.end(),bytes.begin(),bytes.end());
        auto &tag=tags_[handle.index()];tag.address=std::uint32_t(next);tag.size=std::uint32_t(bytes.size());
    };
    append(gestalt,std::move(projection.gestalt),true);
    for(unsigned n=0;n<4;++n)append(handles[n],std::move(projection.sounds[n]),false);
    mcc_resource_owners_.resize(tags_.size());
    for(auto handle:handles){auto &resource=mcc_resource_owners_[handle.index()];resource.sound_gestalt=gestalt.value;resource.inherited=true;}
    std::vector<std::pair<std::uint32_t,std::uint32_t>> retained;
    for(const auto &allocation:retained_tag_allocations_)retained.emplace_back(allocation.address,allocation.size);
    vista_metadata_regions_=tag_metadata_regions(tags_,retained);validate_tag_directory();
    const MccGameplayReferenceAccess access{
        .bytes=[&](auto address,auto size){return std::span(storage_).subspan(metadata_offset(address,size),size);},
        .resolve=[&](TagHandle handle)->std::optional<MccGameplayReferenceIdentity> {
            const auto *tag=find_tag(handle);
            if(!tag || !tag->address || !tag->size)return {};
            return MccGameplayReferenceIdentity{tag->handle(),tag->type(),tag->name};
        },
        .find=[&](TagGroup group,std::string_view name)->std::optional<MccGameplayReferenceIdentity> {
            const auto *tag=query_tag({.group=group,.name=name,.require_address=true,.minimum_size=1});
            if(!tag)return {};
            return MccGameplayReferenceIdentity{tag->handle(),tag->type(),tag->name};
        }
    };
    const auto converted=convert_mcc_gameplay_references(tag_groups::weapon,
        "objects\\weapons\\rifle\\battle_rifle\\battle_rifle",weapon_root,access);
    require(converted && converted->status==MccGameplayReferenceStatus::Converted,
        "Cairo zoom dependency did not complete its exact guarded native reference tuple");
}
}

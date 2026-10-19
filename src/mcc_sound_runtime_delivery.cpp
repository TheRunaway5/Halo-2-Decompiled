#include "halo2/mcc_sound_runtime_delivery.hpp"
#include "halo2/mcc_sound_vanilla_reference.hpp"
#include "halo2/mcc_sound_geometry.hpp"
#include "mcc_sound_stored_time_apply.hpp"
#include "mcc_sound_class_qualified.hpp"
#include "mcc_sound_permutation_time_reference_profile.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <map>
#include <set>

namespace halo2 {
namespace {
constexpr std::array<unsigned,11> offsets{8,0x10,0x18,0x20,0x28,0x30,0x38,0x58,0x60,0x68,0x70};
constexpr std::array<unsigned,11> strides{0x38,0x14,4,10,12,16,0x34,1,12,0x1c,0x2c};
constexpr std::uint64_t end32=0x100000000ull;
std::uint32_t checked(std::uint64_t x) {
    if(x>=end32)throw FormatError("Native sound metadata address/extent wraps");
    return std::uint32_t(x);
}
template<class T> void put(T& out,std::size_t at,std::uint32_t x,unsigned width=4) {
    Bytes(out).require(at,width);
    for(unsigned n=0;n<width;++n)out[at+n]=std::byte(x>>(8*n));
}
std::int32_t short_index(std::uint16_t x){return std::bit_cast<std::int16_t>(x);}
std::int32_t byte_index(std::byte x){return std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(x));}
struct Owner {
    const Tag* tag{};
    std::span<const std::byte> root,codecs;
    std::array<TagBlock,11> blocks;
    TagBlock language;
    MccSoundRuntimeGestalt layout;
};
class Builder {
    const MccSoundRuntimeSource& source;
    MccSoundRuntimePlan out;
    std::map<std::uint32_t,const Tag*> tags;
    std::map<std::uint32_t,Owner> owners;
    std::array<std::vector<std::byte>,11> banks;
    std::array<std::uint32_t,11> counts{};
    std::map<std::uint32_t,TagBlock> classes;
    std::optional<std::array<TagHandle,2>> class_tags;
    std::map<std::pair<std::uint32_t,std::uint32_t>,std::vector<MccSoundRuntimeFlagUser>> range_users;
    std::set<std::uint32_t> sound_source_roots,sound_native_roots;
    std::set<std::uint32_t> authored_applied;
    std::span<const std::byte> read(std::uint32_t at,std::size_t bytes)const {
        if(bytes && (!at||at==0xffffffffu||std::uint64_t(at)+bytes>end32))
            throw FormatError("MCC sound table pointer is null or wraps");
        const auto data=source.read(at,bytes);
        if(data.size()!=bytes)throw FormatError("MCC sound metadata reader returned a partial extent");
        return data;
    }
    const Tag& tag(TagHandle handle,std::string_view group={})const {
        const auto i=tags.find(handle.value);
        if(handle.is_null()||i==tags.end()||!i->second->address||(!group.empty()&&i->second->group!=group))
            throw FormatError("MCC sound binding has no exact full-salt typed namespace owner");
        return *i->second;
    }
    TagBlock block(std::span<const std::byte> data,unsigned at,unsigned stride)const {
        const Bytes b(data);const auto count=b.u32(at),pointer=b.u32(at+4);
        if(count>0x100000 || (count && (!pointer||pointer==0xffffffffu)) ||
           std::uint64_t(pointer)+std::uint64_t(count)*stride>end32)
            throw FormatError("MCC sound table descriptor exceeds its typed extent");
        if(count)(void)read(pointer,std::size_t(count)*stride);
        return {std::int32_t(count),pointer};
    }
    std::span<const std::byte> rows(const TagBlock& b,unsigned stride)const {
        return b.count?read(b.address,std::size_t(b.count)*stride):std::span<const std::byte>{};
    }
    std::uint32_t relocated(std::uint32_t at,std::size_t bytes)const {
        if(!bytes&&(!at||at==0xffffffffu))return at;
        if(bytes)(void)read(at,bytes);
        const auto target=source.relocate(at,bytes);
        if(!target||target==0xffffffffu||std::uint64_t(target)+bytes>end32)
            throw FormatError("MCC nested sound pointer has no complete canonical allocation");
        return target;
    }
    void pointer(std::vector<std::byte>& data,std::size_t at,unsigned stride)const {
        const auto b=block(std::span<const std::byte>(data).subspan(at),0,stride);
        put(data,at+4,relocated(b.address,std::size_t(b.count)*stride));
    }
    std::uint32_t allocate(std::span<const std::byte> bytes) {
        const auto offset=(std::uint64_t(out.bytes.size())+3)&~3ull;
        const auto address=checked(std::uint64_t(out.address)+offset);
        if(std::uint64_t(address)+bytes.size()>end32)
            throw FormatError("Merged native sound bank exceeds logical address space");
        out.bytes.resize(std::size_t(offset));
        out.bytes.insert(out.bytes.end(),bytes.begin(),bytes.end());return address;
    }
    void patch(const Tag& owner,std::size_t bytes,std::span<const std::byte> after) {
        if(after.size()!=bytes)throw FormatError("Native sound patch changes its declared extent");
        const auto before=read(owner.address,bytes);
        out.patches.push_back({owner.handle(),owner.address,{before.begin(),before.end()},{after.begin(),after.end()}});
    }
    static std::uint32_t rebase(std::int32_t index,std::uint32_t base,std::uint32_t count,unsigned bits) {
        if(index==-1)return bits==8?0xffu:0xffffu;
        if(index<0||std::uint32_t(index)>=count||std::uint64_t(base)+index>=(1u<<(bits-1)))
            throw FormatError("Merged sound ordinal exceeds its original signed field or owning table");
        return base+std::uint32_t(index);
    }
    void class_sources() {
        for(const auto& [datum,t]:tags)if(t->address&&t->group=="sncl")
            classes.emplace(datum,block(read(t->address,8),0,0x78));
        for(const auto& [datum,t]:tags)if(t->address&&t->group=="matg") {
            const auto b=block(read(t->address,0xc8),0xc0,0x2c);
            if(b.count!=1)throw FormatError("MCC SoundGlobals class source is not singular");
            const Bytes globals(read(b.address,0x2c));
            const std::array<TagHandle,2> selected{{{globals.u32(4)},{globals.u32(12)}}};
            for(const auto h:selected)(void)tag(h,"sncl");
            if(class_tags&&*class_tags!=selected)throw FormatError("Conflicting MCC SoundGlobals class namespaces");
            class_tags=selected;
        }
    }
    std::pair<TagHandle,std::uint16_t> class_flags(std::span<const std::byte> sound,bool native)const {
        if(!class_tags)throw FormatError("MCC sounds have no unique SoundGlobals class owner");
        const bool remastered=(Bytes(sound).u16(0)&0x800u)!=0;
        const auto handle=(*class_tags)[!native&&remastered?0:1];const auto cls=classes.find(handle.value);
        const auto index=byte_index(sound[2]);
        if(cls==classes.end()||index<0||index>=cls->second.count)
            throw FormatError("MCC sound class escapes the selected real class bank");
        return {handle,Bytes(read(checked(std::uint64_t(cls->second.address)+index*0x78),0x78)).u16(0xa)};
    }
    void users() {
        for(const auto& [datum,t]:tags)if(t->address&&t->group=="snd!") {
            const auto raw=read(t->address,0x1c);const auto h=source.gestalt_owner(t->handle());const auto found=owners.find(h.value);
            if(found==owners.end())throw FormatError("MCC sound has no fully merged owning gestalt");
            const auto& o=found->second;const auto first=short_index(Bytes(raw).u16(8)),count=byte_index(raw[10]);
            if(count<0||(count&&(first<0||std::uint64_t(first)+count>o.layout.count[4])))
                throw FormatError("MCC sound pitch interval escapes its owning gestalt");
            const auto donor=class_flags(raw,false),native=class_flags(raw,true);
            for(int n=0;n<count;++n)range_users[{h.value,std::uint32_t(first+n)}].push_back(
                {t->handle(),donor.first,native.first,std::to_integer<std::uint8_t>(raw[2]),donor.second,native.second});
        }
    }
    void owner(const Tag& t) {
        Owner o;o.tag=&t;o.root=read(t.address,0x78);o.layout.owner=t.handle();
        const auto codecs=block(o.root,0,3);o.codecs=rows(codecs,3);
        for(unsigned n=0;n<11;++n) {
            if(n==8)continue;
            o.blocks[n]=block(o.root,offsets[n],n==10?0x14c:strides[n]);
        }
        const auto languages=block(o.root,0x60,8);
        if(languages.count) {
            o.language=block(read(languages.address,8),0,16);
            o.blocks[8]={o.language.count,o.language.address};
        }else if(o.blocks[5].count)throw FormatError("MCC permutations have no selected English language owner");
        for(unsigned n=0;n<11;++n) {
            o.layout.first[n]=counts[n];o.layout.count[n]=std::uint32_t(o.blocks[n].count);
            counts[n]=checked(std::uint64_t(counts[n])+o.blocks[n].count);
        }
        owners.emplace(t.datum,o);
    }
    void merge(Owner& o) {
        for(unsigned n=0;n<11;++n) {
            auto& bank=banks[n];const auto first=bank.size();
            if(n==8) {
                const auto input=rows(o.language,16);
                for(unsigned c=0;c<std::uint32_t(o.language.count);++c) {
                    const auto projected=project_mcc_sound_chunk(input.subspan(c*16,16));
                    bank.insert(bank.end(),projected.chunk.begin(),projected.chunk.end());
                    const Bytes raw(input.subspan(c*16,16));
                    out.chunks.push_back({o.tag->handle(),checked(std::uint64_t(o.language.address)+c*16),0,c,
                        raw.u32(0),raw.u32(4)&0x1fffffffu,projected.decoded_bytes,projected.reset_after});
                }
            }else if(n==5) {
                const auto input=rows(o.blocks[n],16);
                const auto chunks=rows(o.language,16);
                for(unsigned p=0;p<std::uint32_t(o.blocks[n].count);++p) {
                    const auto raw=input.subspan(p*16,16);const auto localized=block(raw,8,8);
                    if(!localized.count)throw FormatError("MCC permutation lacks its selected localized metadata");
                    const auto info=read(localized.address,8);const Bytes b(info);
                    const auto start=short_index(b.u16(4)),count=short_index(b.u16(6));
                    if(count<0||(count&&(start<0||std::uint64_t(start)+count>std::uint32_t(o.language.count))))
                        throw FormatError("MCC localized permutation escapes its English chunk table");
                    std::uint64_t encoded=0;
                    for(int c=0;c<count;++c)encoded+=Bytes(chunks).u32(std::size_t(start+c)*16+4)&0x1fffffffu;
                    if(encoded>std::numeric_limits<std::uint32_t>::max())
                        throw FormatError("MCC permutation encoded byte extent overflows original SampleSize");
                    auto projected=project_mcc_sound_permutation(raw,info,std::uint32_t(encoded));
                    put(projected,0,rebase(short_index(Bytes(raw).u16(0)),o.layout.first[2],o.layout.count[2],16),2);
                    // +5 is a sound-local dialogue entry, proven21F8B2..8D7.
                    put(projected,12,rebase(start,o.layout.first[8],o.layout.count[8],16),2);
                    bank.insert(bank.end(),projected.begin(),projected.end());
                }
            }else if(n==10) {
                const auto input=rows(o.blocks[n],0x14c);
                for(unsigned e=0;e<std::uint32_t(o.blocks[n].count);++e) {
                    const auto raw=input.subspan(e*0x14c,0x2c);
                    std::vector<std::byte> projected(raw.begin(),raw.end());
                    pointer(projected,0,16);pointer(projected,0x18,16);
                    const TagHandle owner{Bytes(raw).u32(0x20)};
                    if(!owner.is_null())(void)tag(owner);
                    // +24 is signed section-local displacement. Actual22A683
                    // adds it to extra+8; FFF8 addresses this row's descriptor.
                    // Resource primary/secondary locators are section-local,
                    // not gestalt ordinals (22A6DF..702). Preserve all bits.
                    bank.insert(bank.end(),projected.begin(),projected.end());
                }
            }else {
                const auto input=rows(o.blocks[n],strides[n]);bank.insert(bank.end(),input.begin(),input.end());
                if(n==4)for(unsigned r=0;r<std::uint32_t(o.blocks[n].count);++r) {
                    const auto at=first+r*12;const Bytes raw(input.subspan(r*12,12));
                    put(bank,at,rebase(short_index(raw.u16(0)),o.layout.first[2],o.layout.count[2],16),2);
                    put(bank,at+2,rebase(short_index(raw.u16(2)),o.layout.first[3],o.layout.count[3],16),2);
                    const auto flag=raw.u16(6);const auto count=short_index(raw.u16(10));
                    if(count<0)throw FormatError("MCC pitch range has negative permutation count");
                    const auto p=short_index(raw.u16(8));
                    if(count&&(p<0||std::uint64_t(p)+count>o.layout.count[5]))
                        throw FormatError("MCC pitch range escapes its permutation table");
                    if(flag!=0xffffu) {
                        // Original uses whole-byte flag storage: U16>>3 at
                        // 21FFC4..FDA, with ceil(permutationCount/8) bytes.
                        const auto bytes=(std::uint32_t(count)+7)/8;
                        if(std::uint64_t(flag>>3)+bytes>o.layout.count[7]) {
                            const auto found=range_users.find({o.tag->datum,r});
                            if(found==range_users.end()||found->second.empty()||
                               !std::ranges::all_of(found->second,[](const auto& u){return (u.mcc_flags&0x20)&&(u.native_flags&0x20);}))
                                throw FormatError("Unowned MCC flag offset has a persistent or unclassified original consumer");
                            out.unused_flag_indices.push_back({o.tag->handle(),r,flag,found->second});
                            // The two actual consumers branch on class bit5
                            // before reading this field. Preserve its raw
                            // bits, including authored offsets with no bank.
                        }else {
                            if(std::uint64_t(o.layout.first[7])*8+flag>=0xffffu)
                                throw FormatError("Merged runtime permutation flags exceed original U16 storage");
                            put(bank,at+6,o.layout.first[7]*8+flag,2);
                        }
                    }
                    put(bank,at+8,rebase(p,o.layout.first[5],o.layout.count[5],16),2);
                    // +4/+5 are last/forced local permutations, not an index
                    // into any gestalt table (actual21B40F/21B46F).
                }
                if(n==6)for(unsigned c=0;c<std::uint32_t(o.blocks[n].count);++c)
                    for(const auto [at,stride]:{std::pair(0u,8u),{0x14u,0x48u},{0x1cu,0x30u},{0x24u,0x40u},{0x2cu,0x28u}})
                        pointer(bank,first+c*0x34+at,stride);
                if(n==9)for(unsigned p=0;p<std::uint32_t(o.blocks[n].count);++p) {
                    pointer(bank,first+p*0x1c,16);pointer(bank,first+p*0x1c+8,4);
                    // Rule+0 and active+10 are local pitch/rule selectors;
                    // 21F920..951 returns the former without a gestalt base.
                }
            }
        }
    }
    void sound(const Tag& t) {
        const auto raw=read(t.address,0x1c);const Bytes b(raw);
        const auto i=owners.find(source.gestalt_owner(t.handle()).value);
        if(i==owners.end())throw FormatError("MCC sound has no fully merged owning gestalt");
        const auto& o=i->second;auto projected=project_mcc_sound_definition(raw,o.codecs);
        if(out.authored_flags)for(const auto& c:out.authored_flags->corrections)if(c.sound==t.handle()) {
            if(c.gestalt!=o.tag->handle()||c.source_root!=t.address||c.source_flags!=b.u16(0)||
               c.native_before!=Bytes(projected.definition).u16(0)||!authored_applied.insert(t.datum).second)
                throw FormatError("Authenticated sound flag receipt no longer matches its exact source/native preimage");
            put(projected.definition,0,c.native_after,2);
        }
        const auto index16=[&](unsigned at,unsigned table) {
            put(projected.definition,at,rebase(short_index(b.u16(at)),o.layout.first[table],o.layout.count[table],16),2);
        };
        index16(6,0);index16(8,4);index16(14,10);
        const auto count=byte_index(raw[10]),range=short_index(b.u16(8));
        if(count<0||(count&&(range<0||std::uint64_t(range)+count>o.layout.count[4])))
            throw FormatError("MCC sound pitch interval escapes its owning gestalt");
        for(const auto [at,table]:{std::pair(11u,1u),{12u,9u},{13u,6u}})
            projected.definition[at]=std::byte(rebase(byte_index(raw[at]),o.layout.first[table],o.layout.count[table],8));
        const bool remastered=(b.u16(0)&0x800u)!=0;
        const auto cls=classes.at(class_flags(raw,false).first.value);
        const auto flags=Bytes(read(checked(std::uint64_t(cls.address)+byte_index(raw[2])*0x78),0x78)).u16(8);
        const auto file=(flags&0x80u)?MccSoundDataFile::English:
            remastered?MccSoundDataFile::Remastered:MccSoundDataFile::Neutral;
        const auto native_root=relocated(t.address,20);
        if(!sound_source_roots.insert(t.address).second||!sound_native_roots.insert(native_root).second)
            throw FormatError("Distinct full-salt sounds alias a native duration identity root");
        out.sounds.push_back({t.handle(),o.tag->handle(),projected.codec,file,projected.remastered_sound_index,
            projected.mcc_extension,t.address,native_root,b.u16(0),Bytes(projected.definition).u16(0)});
        if(projected.codec.sample_rate>2||projected.codec.encoding>2||projected.codec.compression>4)
            out.native_pcm_duration.push_back(t.handle());
        patch(t,20,projected.definition);
    }
public:
    Builder(const MccSoundRuntimeSource& s,std::uint32_t base,
        std::shared_ptr<const MccSoundVanillaFlagPlan> authored={}):source(s) {
        if(!source.read||!source.relocate||!source.gestalt_owner||!base||base==0xffffffffu||base%4)
            throw FormatError("Native sound delivery needs typed source/relocation owners and aligned address");
        out.address=base;
        out.authored_flags=std::move(authored);
        for(const auto& t:source.tags)if(!t.handle().is_null())
            if(!tags.emplace(t.datum,&t).second)throw FormatError("Duplicate full-salt sound source namespace identity");
    }
    MccSoundRuntimePlan build() {
        for(const auto& [datum,t]:tags)if(t->address&&t->group=="ugh!")owner(*t);
        class_sources();users();
        for(auto& [datum,o]:owners){merge(o);out.gestalts.push_back(o.layout);}
        for(unsigned n=0;n<11;++n) {
            if(banks[n].size()!=std::uint64_t(counts[n])*strides[n])
                throw FormatError("Merged sound bank count differs from its original record stride");
            out.blocks[n]={std::int32_t(counts[n]),allocate(banks[n])};
        }
        std::array<std::byte,0x58> root{};
        for(unsigned n=0;n<11;++n){put(root,n*8,counts[n]);put(root,n*8+4,out.blocks[n].address);}
        for(const auto& [datum,o]:owners)patch(*o.tag,root.size(),root);
        std::uint32_t c=0;
        for(const auto& [datum,o]:owners)for(unsigned n=0;n<o.layout.count[8];++n)
            out.chunks[c++].native_address=checked(std::uint64_t(out.blocks[8].address)+(o.layout.first[8]+n)*12);
        for(const auto& [datum,t]:tags)if(t->address&&t->group=="sncl") {
            const auto raw=read(t->address,8);const auto b=classes.at(datum);
            const auto data=project_mcc_sound_class_records(rows(b,0x78));
            patch(*t,8,project_mcc_sound_class_root(raw,allocate(data)));
        }
        for(const auto& [datum,t]:tags)if(t->address&&t->group=="snd!")sound(*t);
        if(out.authored_flags&&authored_applied.size()!=out.authored_flags->corrections.size())
            throw FormatError("Authenticated authored sound flag receipt lost a complete target owner");
        std::ranges::sort(out.patches,{},&MccSoundRuntimePatch::source_address);
        std::uint64_t previous=0;
        for(const auto& p:out.patches) {
            if(p.source_address<previous)throw FormatError("Native sound metadata patches overlap");
            previous=std::uint64_t(p.source_address)+p.after.size();
        }
        return std::move(out);
    }
};
}
MccSoundRuntimePlan plan_mcc_sound_runtime_delivery(const MccSoundRuntimeSource& source,std::uint32_t base) {
    return Builder(source,base).build();
}
MccSoundRuntimePlan plan_mcc_sound_runtime_delivery(const Cache& cache,std::uint32_t base,
    const std::function<std::uint32_t(std::uint32_t,std::size_t)>& relocate) {
    if(!cache.is_mcc())throw FormatError("Native MCC sound delivery requires an MCC cache owner");
    const MccSoundRuntimeSource source{cache.tags(),
        [&](std::uint32_t at,std::size_t n){return cache.read_tag_bytes(at,n);},
        [&](TagHandle sound){return cache.mcc_sound_gestalt_owner(sound).handle();},relocate};
    auto out=Builder(source,base,std::make_shared<const MccSoundVanillaFlagPlan>(
        plan_mcc_sound_vanilla_flags(cache))).build();
    out=mcc_sound_class_detail::stage_qualified(source,out);
    const auto permutation_times=plan_mcc_sound_permutation_times(cache,out,relocate);
    mcc_sound_permutation_time_detail::apply(out,source,permutation_times);
    const auto stored=plan_mcc_sound_stored_times(cache,out,relocate);
    mcc_sound_stored_time_detail::apply(out,source,stored);
    for(const auto& resource:plan_mcc_sound_geometry_resources(cache)) {
        const auto owner=std::ranges::find(out.gestalts,resource.gestalt,&MccSoundRuntimeGestalt::owner);
        if(owner==out.gestalts.end() || resource.index>=owner->count[10])
            throw FormatError("MCC facial resource lost its merged native gestalt interval");
        const auto row=std::uint64_t(out.blocks[10].address)+
            (std::uint64_t(owner->first[10])+resource.index)*0x2cu;
        if(row<out.address || row-out.address+0x2cu>out.bytes.size() ||
           Bytes(out.bytes).u32(std::size_t(row-out.address)+8u)!=resource.source_file_offset ||
           Bytes(out.bytes).u32(std::size_t(row-out.address)+12u)!=resource.declared_bytes)
            throw FormatError("MCC facial resource differs from its exact native metadata preimage");
        put(out.bytes,std::size_t(row-out.address)+8u,resource.encoded_offset);
    }
    return out;
}
} // namespace halo2

#include "audio/mcc_sound_variation_provider_internal.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <map>

namespace halo2 {
namespace {
constexpr TagGroup sound_group{"snd!"},gestalt_group{"ugh!"},class_group{"sncl"};
constexpr std::array<unsigned,11> strides{56,20,4,10,12,16,52,1,12,28,44};
void require(bool value,const char* message){if(!value)throw FormatError(message);}
void require_status(MccSoundTimingReadStatus status){
    switch(status){
    case MccSoundTimingReadStatus::Executed:return;
    case MccSoundTimingReadStatus::StaleOwner:throw FormatError("Native sound variation provider has a stale memory owner");
    case MccSoundTimingReadStatus::WrongThread:throw FormatError("Native sound variation provider used from a foreign thread");
    case MccSoundTimingReadStatus::ReentrantLeaseOperation:throw FormatError("Native sound variation provider cannot reenter a memory lease");
    }
    throw FormatError("Native sound variation provider has an unknown lease status");
}
std::int16_t signed_short(const Bytes& b,unsigned at){return std::bit_cast<std::int16_t>(b.u16(at));}
std::int8_t signed_byte(std::byte b){return std::bit_cast<std::int8_t>(b);}
struct NativeTag {
    TagGroup group;TagHandle handle;std::uint32_t address{},bytes{};
};
struct SoundRoot {NativeTag tag;TagHandle gestalt;std::array<std::byte,20> bytes;};
struct Catalog {
    const Cache* cache{};
    std::uint32_t base{},extent{},table{},count{},globals_child{};
    std::vector<std::byte> index_prefix;
    std::vector<NativeTag> tags;
    std::map<std::uint32_t,SoundRoot> sounds;
    NativeTag globals,primary,legacy;
    std::array<std::byte,36> globals_bytes;
    std::array<std::byte,88> gestalt_bytes;
    std::array<std::byte,8> class_bytes;
    std::array<TagBlock,11> blocks;
    // Immutable ordinal/PCM associations. Only range+4/+5 and the separate
    // used-byte bank are runtime selection state; changing a permutation
    // base in bounds must not pair another row's metadata with Cache PCM0.
    std::vector<std::array<std::byte,12>> ranges;
    std::vector<std::array<std::byte,16>> permutations;
};
class Reader {
    const Catalog& catalog;
    const MccSoundTimingOwnedReader& owner;
public:
    Reader(const Catalog& c,const MccSoundTimingOwnedReader& r):catalog(c),owner(r){}
    std::vector<std::byte> read(std::uint32_t at,std::size_t bytes)const{
        require(bytes&&at>=catalog.base&&std::uint64_t(at)-catalog.base<=catalog.extent&&
                bytes<=catalog.extent-(std::uint64_t(at)-catalog.base),
                "Native sound variation read escapes its retained canonical allocation");
        auto result=owner.read(at,bytes);
        require(result.size()==bytes,"Native sound variation reader returned a partial owner");return result;
    }
    std::uint32_t global_word(std::uint32_t at)const{
        const auto value=owner.read(at,4);require(value.size()==4,"Native sound global owner is truncated");return Bytes(value).u32(0);
    }
    void same(std::uint32_t at,std::span<const std::byte> expected)const{
        require(std::ranges::equal(read(at,expected.size()),expected),"Native sound variation canonical metadata changed");
    }
    const NativeTag& tag(TagHandle h,TagGroup group)const{
        require(!h.is_null()&&h.index()<catalog.tags.size(),"Native sound variation full datum is outside its catalog");
        const auto& t=catalog.tags[h.index()];
        require(t.handle==h&&t.group==group&&t.address&&t.bytes,
                "Native sound variation full salt, group or payload owner differs");
        const auto row=read(catalog.table+std::uint32_t(h.index())*16u,16);const Bytes b(row);
        require(b.u32(0)==t.group.value&&b.u32(4)==h.value&&b.u32(8)==t.address&&b.u32(12)==t.bytes,
                "Native sound variation published tag identity changed");return t;
    }
    std::vector<std::byte> row(TagBlock block,unsigned stride,std::uint32_t index)const{
        require(block.count>=0&&std::uint32_t(block.count)<=0x100000u&&index<std::uint32_t(block.count),
                "Native sound variation table index is outside its declared owner");
        const auto bytes=std::uint64_t(std::uint32_t(block.count))*stride;
        const auto at=std::uint64_t(block.address)+std::uint64_t(index)*stride;
        require(block.address&&bytes<=std::numeric_limits<std::uint32_t>::max()&&
                std::uint64_t(block.address)+bytes<=0x100000000ull&&at<=std::numeric_limits<std::uint32_t>::max(),
                "Native sound variation table pointer or extent wraps");
        // Validate the complete declared table as an interval before copying
        // the selected row; no unused flag-bank contents are read here.
        require(block.address>=catalog.base&&std::uint64_t(block.address)-catalog.base<=catalog.extent&&
                bytes<=catalog.extent-(std::uint64_t(block.address)-catalog.base),
                "Native sound variation table escapes its canonical owner");
        return read(std::uint32_t(at),stride);
    }
};
template<std::size_t N>std::array<std::byte,N> fixed(std::span<const std::byte> bytes){
    require(bytes.size()==N,"Native sound variation fixed metadata extent differs");
    std::array<std::byte,N> result;std::ranges::copy(bytes,result.begin());return result;
}
template<class Target>void copy(Target& target,std::span<const std::byte> bytes){
    require(target.size()==bytes.size(),"Native sound variation row stride differs");std::ranges::copy(bytes,target.begin());
}
void same_range(const Catalog& c,unsigned index,std::span<const std::byte> row){
    require(index<c.ranges.size()&&row.size()==12,"Native sound variation lacks its immutable range mapping");
    const auto& expected=c.ranges[index];
    for(unsigned n=0;n<12;++n)if(n!=4&&n!=5)
        require(row[n]==expected[n],"Native sound variation immutable range or PCM order changed");
}
void current_namespace(const Catalog& c,const Reader& r){
    require(r.global_word(0x556d90u)==c.base&&r.global_word(0x4f0414u)==c.table&&
            r.global_word(0x4ecc1cu)==c.globals.address&&r.global_word(0x52d664u)==c.primary.address,
            "Native sound variation globals no longer select its canonical namespace");
    r.same(c.base,c.index_prefix);(void)r.tag(c.globals.handle,tag_groups::globals);
    const auto global=r.read(c.globals.address+0xc0u,8);const Bytes b(global);
    require(b.u32(0)==1&&b.u32(4)==c.globals_child,"Native sound variation SoundGlobals owner changed");
    r.same(c.globals_child,c.globals_bytes);(void)r.tag(c.primary.handle,gestalt_group);
    r.same(c.primary.address,c.gestalt_bytes);(void)r.tag(c.legacy.handle,class_group);
    r.same(c.legacy.address,c.class_bytes);
}
SoundVariationDefinition parse(const Catalog& c,const MccSoundTimingOwnedReader& owner,TagHandle sound,unsigned pitch){
    const auto found=c.sounds.find(sound.value);
    require(found!=c.sounds.end(),"Native sound variation has no exact full-salt sound owner");
    const Reader r(c,owner);current_namespace(c,r);const auto& root=found->second;
    const auto& t=r.tag(sound,sound_group);r.same(t.address,root.bytes);
    const auto& gestalt=r.tag(root.gestalt,gestalt_group);r.same(gestalt.address,c.gestalt_bytes);
    SoundVariationDefinition out;out.datum=sound.value;copy(out.sound,r.read(t.address,20));
    const Bytes snd(out.sound);const auto cls=signed_byte(out.sound[2]);
    const auto ranges=std::to_integer<unsigned>(out.sound[10]);
    require(cls>=0&&pitch<ranges,"Native sound variation class or pitch range is invalid");
    out.sample_rate_index=std::to_integer<unsigned>(out.sound[3]);
    const Bytes classes(c.class_bytes);copy(out.sound_class,r.row({std::int32_t(classes.u32(0)),classes.u32(4)},92,unsigned(cls)));
    const auto playback=signed_short(snd,6),first_range=signed_short(snd,8);const auto scale=signed_byte(out.sound[11]);
    require(playback>=0&&first_range>=0&&scale>=0,"Native sound variation has a negative authored index");
    copy(out.playback,r.row(c.blocks[0],56,unsigned(playback)));copy(out.scale,r.row(c.blocks[1],20,unsigned(scale)));
    require(std::uint64_t(unsigned(first_range))+ranges<=std::uint32_t(c.blocks[4].count),
            "Native sound variation pitch ranges escape the merged table");
    for(unsigned n=0;n<ranges;++n){
        const auto row=r.row(c.blocks[4],12,unsigned(first_range)+n);same_range(c,unsigned(first_range)+n,row);
        const Bytes range(row);const auto interval=signed_short(range,2);
        require(interval>=0,"Native sound variation interval index is negative");
        auto& target=out.pitch_ranges.emplace_back();copy(target.interval,r.row(c.blocks[3],10,unsigned(interval)));
        target.permutations=signed_short(range,10);
    }
    copy(out.range,r.row(c.blocks[4],12,unsigned(first_range)+pitch));same_range(c,unsigned(first_range)+pitch,out.range);
    const Bytes range(out.range);
    const auto first=signed_short(range,8),count=signed_short(range,10);
    require(first>=0&&count>=1&&count<=32,"Unrecovered native sound permutation count");
    require(std::uint64_t(unsigned(first))+unsigned(count)<=std::uint32_t(c.blocks[5].count),
            "Native sound variation permutations escape the merged table");
    out.permutations.resize(unsigned(count));
    for(unsigned n=0;n<unsigned(count);++n){
        const auto ordinal=unsigned(first)+n;copy(out.permutations[n],r.row(c.blocks[5],16,ordinal));
        require(ordinal<c.permutations.size()&&out.permutations[n]==c.permutations[ordinal],
                "Native sound variation immutable permutation or PCM association changed");
    }
    if(count>1&&!(Bytes(out.sound_class).u16(10)&0x20u)){
        const auto bytes=unsigned(count+7)/8,offset=unsigned(range.u16(6))>>3;
        require(c.blocks[7].count>=0&&std::uint64_t(offset)+bytes<=std::uint32_t(c.blocks[7].count),
                "Native sound variation used flags escape the actual byte bank");
        out.used=r.read(c.blocks[7].address+offset,bytes);
    }
    return out;
}
Catalog catalog(const Cache& cache,const MccRuntimeCacheDelivery& delivery,const MccSoundRuntimePlan& plan,
                const MccSoundTimingOwnedReader& owner){
    require(cache.is_mcc(),"Native sound variation provider requires converted MCC ownership");
    const auto& index=delivery.index();const Bytes raw(index.region.bytes);
    require(index.table>=index.region.address&&index.count<=0x10000u&&
            std::uint64_t(index.table-index.region.address)+std::uint64_t(index.count)*16u==index.region.bytes.size(),
            "Native sound variation canonical directory extent differs");
    Catalog c;c.cache=&cache;c.base=index.region.address;c.extent=delivery.reservation_bytes();c.table=index.table;c.count=index.count;
    require(c.extent>=index.region.bytes.size()&&std::uint64_t(c.base)+c.extent<=0x100000000ull,
            "Native sound variation canonical allocation wraps");
    const auto prefix=std::size_t(index.table-index.region.address);c.index_prefix.assign(index.region.bytes.begin(),index.region.bytes.begin()+prefix);
    for(unsigned n=0;n<index.count;++n){
        const auto at=prefix+n*16u;c.tags.push_back({TagGroup{raw.u32(at)},TagHandle{raw.u32(at+4)},raw.u32(at+8),raw.u32(at+12)});
    }
    const auto tag=[&](TagHandle h,TagGroup group)->const NativeTag&{
        require(!h.is_null()&&h.index()<c.tags.size(),"Native sound catalog tag is outside its complete directory");const auto& t=c.tags[h.index()];
        require(t.handle==h&&t.group==group&&t.address&&t.bytes,"Native sound catalog full-salt typed owner differs");return t;
    };
    c.globals=tag({index.globals},tag_groups::globals);
    const auto* globals=cache.find_tag({index.globals},tag_groups::globals);require(bool(globals),"Native sound catalog lost its raw globals owner");
    const auto globals_root=cache.read_tag_bytes(globals->address,0xc8);const Bytes g(globals_root);
    require(g.u32(0xc0)==1,"Native sound catalog SoundGlobals is not singular");
    c.globals_child=delivery.runtime_address(g.u32(0xc4),0x2c);
    c.globals_bytes=project_mcc_sound_globals(cache.read_tag_bytes(g.u32(0xc4),0x2c));const Bytes sg(c.globals_bytes);
    require(sg.u32(0)==class_group.value,"Native sound catalog Legacy class reference is untyped");
    c.legacy=tag({sg.u32(4)},class_group);c.primary=tag({sg.u32(32)},gestalt_group);
    c.blocks=plan.blocks;
    for(unsigned n=0;n<11;++n){
        require(c.blocks[n].count>=0&&std::uint32_t(c.blocks[n].count)<=0x100000u,"Native sound catalog merged table count differs");
        for(unsigned byte=0;byte<4;++byte){c.gestalt_bytes[n*8+byte]=std::byte(std::uint32_t(c.blocks[n].count)>>(byte*8));c.gestalt_bytes[n*8+4+byte]=std::byte(c.blocks[n].address>>(byte*8));}
        const auto bytes=std::uint64_t(std::uint32_t(c.blocks[n].count))*strides[n];
        require(c.blocks[n].address>=c.base&&std::uint64_t(c.blocks[n].address)-c.base<=c.extent&&
                bytes<=c.extent-(std::uint64_t(c.blocks[n].address)-c.base),"Native sound catalog merged table is outside its allocation");
    }
    const auto projected=[&](std::uint32_t at,std::size_t bytes){
        require(at>=plan.address&&std::uint64_t(at)-plan.address<=plan.bytes.size()&&
                bytes<=plan.bytes.size()-(std::uint64_t(at)-plan.address),
                "Native sound catalog range mapping has no projected metadata owner");
        return std::span<const std::byte>(plan.bytes).subspan(at-plan.address,bytes);
    };
    for(unsigned n=0;n<unsigned(c.blocks[4].count);++n)
        c.ranges.push_back(fixed<12>(projected(c.blocks[4].address+n*12u,12)));
    for(unsigned n=0;n<unsigned(c.blocks[5].count);++n)
        c.permutations.push_back(fixed<16>(projected(c.blocks[5].address+n*16u,16)));
    const auto patch=[&](TagHandle h,std::size_t bytes)->const MccSoundRuntimePatch&{
        const auto found=std::ranges::find_if(plan.patches,[&](const auto& p){return p.owner==h&&p.after.size()==bytes&&p.before.size()==bytes;});
        require(found!=plan.patches.end(),"Native sound catalog lacks its complete projected root");return *found;
    };
    c.class_bytes=fixed<8>(patch(c.legacy.handle,8).after);
    require(std::ranges::equal(patch(c.primary.handle,88).after,c.gestalt_bytes),"Native sound catalog primary gestalt differs from the merged owner");
    for(const auto& s:plan.sounds){
        const auto& t=tag(s.sound,sound_group);const auto& p=patch(s.sound,20);const auto* raw_sound=cache.find_tag(s.sound,sound_group);
        require(raw_sound&&raw_sound->address==s.source_root&&p.source_address==s.source_root&&
                s.native_root==t.address&&t.bytes>=20&&Bytes(p.after).u16(0)==s.native_flags,
                "Native sound catalog projected sound has a foreign root or flags");
        (void)tag(s.gestalt,gestalt_group);
        require(std::ranges::equal(patch(s.gestalt,88).after,c.gestalt_bytes),"Native sound catalog sound gestalt is not the merged owner");
        require(c.sounds.emplace(s.sound.value,SoundRoot{t,s.gestalt,fixed<20>(p.after)}).second,
                "Native sound catalog has duplicate salted sound identities");
    }
    require(!c.sounds.empty(),"Native sound variation provider has no converted sounds");
    const Reader r(c,owner);current_namespace(c,r);
    for(const auto& [id,s]:c.sounds){(void)id;(void)r.tag(s.tag.handle,sound_group);r.same(s.tag.address,s.bytes);}
    return c;
}
} // namespace
struct MccSoundVariationProvider::Impl {
    MccSoundTimingReadLease lease;Catalog catalog;
    std::shared_ptr<const MccSoundTimingMemoryOwner> memory;
    Impl(MccSoundTimingReadLease l,Catalog c,std::shared_ptr<const MccSoundTimingMemoryOwner> m)
        :lease(std::move(l)),catalog(std::move(c)),memory(std::move(m)){}
};
struct MccSoundVariationProviderAccess {
    static MccSoundVariationProvider make(MccSoundTimingReadLease l,Catalog c,
        std::shared_ptr<const MccSoundTimingMemoryOwner> memory){
        return MccSoundVariationProvider(std::make_shared<const MccSoundVariationProvider::Impl>(
            std::move(l),std::move(c),std::move(memory)));
    }
    static void require_owner(const MccSoundVariationProvider& provider,const Cache& expected,
        const std::shared_ptr<const MccSoundTimingMemoryOwner>& memory){
        const auto owner=provider.impl_;
        require(bool(owner),"Native sound variation provider is absent");
        require_status(check_mcc_sound_timing_read_lease(owner->lease));
        require(&expected==owner->catalog.cache,"Native sound variation provider belongs to another Cache");
        require(memory&&owner->memory==memory && !owner->memory.owner_before(memory) && !memory.owner_before(owner->memory),
                "Native script Access and sound metadata belong to different World owners");
    }
};
MccSoundVariationProvider::MccSoundVariationProvider(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
void MccSoundVariationProvider::require_current(const Cache& expected)const{
    const auto owner=impl_;require(bool(owner),"Native sound variation provider is absent");
    require_status(check_mcc_sound_timing_read_lease(owner->lease));
    require(&expected==owner->catalog.cache,"Native sound variation provider belongs to another Cache");
}
SoundVariationDefinition MccSoundVariationProvider::load(const Cache& expected,TagHandle sound,unsigned pitch)const{
    const auto owner=impl_;require(bool(owner),"Native sound variation provider is absent");std::optional<SoundVariationDefinition> result;
    require_status(with_mcc_sound_timing_owned_read(owner->lease,[&](const auto& reader){
        require(&expected==owner->catalog.cache,"Native sound variation provider belongs to another Cache");
        result.emplace(parse(owner->catalog,reader,sound,pitch));
    }));require(result.has_value(),"Native sound variation provider did not execute its owned parser");return std::move(*result);
}
MccSoundTimingReadLease MccSoundVariationProvider::timing_lease()const {
    require(bool(impl_),"Native sound variation provider is absent");
    return impl_->lease;
}
namespace detail {
void require_mcc_sound_variation_owner(const MccSoundVariationProvider& provider,const Cache& expected,
    const std::shared_ptr<const MccSoundTimingMemoryOwner>& memory){
    MccSoundVariationProviderAccess::require_owner(provider,expected,memory);
}
MccSoundVariationProvider make_mcc_sound_variation_provider(const Cache& actual,const Cache& expected,
    const MccRuntimeCacheDelivery& delivery,const MccSoundRuntimePlan& plan,
    const std::shared_ptr<const MccSoundTimingMemoryOwner>& owner){
    auto lease=capture_mcc_sound_timing_read_lease(owner);std::optional<Catalog> c;
    require_status(with_mcc_sound_timing_owned_read(lease,[&](const auto& reader){
        require(&actual==&expected,"Native sound variation provider requires this World's Cache");
        c.emplace(catalog(actual,delivery,plan,reader));
    }));require(c.has_value(),"Native sound variation provider failed its catalog transaction");
    return MccSoundVariationProviderAccess::make(std::move(lease),std::move(*c),owner);
}
} // namespace detail
} // namespace halo2

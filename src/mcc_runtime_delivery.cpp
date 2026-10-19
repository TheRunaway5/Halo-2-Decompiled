#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/mcc_animation.hpp"
#include "halo2/mcc_physics.hpp"
#include "halo2/mcc_ordinary_pointer.hpp"
#include "halo2/mcc_sound_tag_layout.hpp"
#include "halo2/mcc_sound_runtime_delivery.hpp"
#include <algorithm>
#include <limits>
#include <set>

namespace halo2 {
namespace {
constexpr std::uint64_t address_end=0x100000000ull;
void word(std::vector<std::byte>& data,std::size_t at,std::uint32_t value) {
    Bytes(data).require(at,4);
    for(unsigned n=0;n<4;++n)data[at+n]=std::byte(value>>(n*8));
}
std::uint32_t address(std::uint64_t value) {
    if(value>=address_end)throw FormatError("Canonical runtime cache address wraps");
    return std::uint32_t(value);
}
std::uint64_t aligned(std::uint64_t value,std::uint64_t alignment) {
    if(value>address_end-alignment)throw FormatError("Canonical runtime cache alignment wraps");
    return (value+alignment-1)&~(alignment-1);
}
void validate_ranges(std::span<const MccRuntimeRelocation> ranges) {
    std::vector<MccRuntimeRelocation> source(ranges.begin(),ranges.end()),target=source;
    for(const auto& r:source)if(!r.source || !r.target ||
        std::uint64_t(r.source)+r.bytes>address_end || std::uint64_t(r.target)+r.bytes>address_end)
        throw FormatError("Invalid canonical cache relocation allocation");
    std::ranges::sort(source,{},&MccRuntimeRelocation::source);
    std::ranges::sort(target,{},&MccRuntimeRelocation::target);
    const auto nonempty_disjoint=[](const auto& allocations,bool reverse) {
        std::uint64_t end=0;
        for(const auto& r:allocations)if(r.bytes) {
            const auto start=reverse?r.target:r.source;
            if(start<end)throw FormatError("Canonical cache relocation allocations overlap");
            end=std::uint64_t(start)+r.bytes;
        }
    };
    nonempty_disjoint(source,false);nonempty_disjoint(target,true);
    // An empty location may coincide with a known endpoint only when its
    // mapping agrees. Distinct source locations must retain distinct logical
    // locations so reverse resource translation stays unambiguous.
    for(const auto& empty:source)if(!empty.bytes)for(const auto& other:source) {
        if(&empty==&other)continue;
        if(empty.source>=other.source && std::uint64_t(empty.source)-other.source<=other.bytes &&
           empty.target!=std::uint64_t(other.target)+empty.source-other.source)
            throw FormatError("Canonical empty source location has conflicting ownership");
        if(empty.target>=other.target && std::uint64_t(empty.target)-other.target<=other.bytes &&
           empty.source!=std::uint64_t(other.source)+empty.target-other.target)
            throw FormatError("Canonical empty target location has conflicting ownership");
    }
}
std::uint32_t translate(std::span<const MccRuntimeRelocation> ranges,std::uint32_t source,std::size_t bytes,bool reverse) {
    if(!source || source==0xffffffffu || bytes>address_end-source)
        throw FormatError("Canonical cache pointer is null or wraps");
    const MccRuntimeRelocation* selected=nullptr;
    for(const auto& r:ranges) {
        const auto base=reverse?r.target:r.source;
        if(source>=base && std::uint64_t(source)-base<=r.bytes && bytes<=r.bytes-(std::uint64_t(source)-base))
            if(!selected || base>(reverse?selected->target:selected->source))selected=&r;
    }
    if(!selected)throw FormatError("Canonical cache pointer has no declared allocation owner");
    return address(std::uint64_t(reverse?selected->source:selected->target)+
                   source-(reverse?selected->target:selected->source));
}
// Finalized plans retain source-sorted, validated disjoint allocations. Empty
// aliases may sit inside a payload or at its endpoints, so skip them when they
// cannot satisfy a nonempty extent. The nearest nonempty predecessor is the
// only remaining possible owner; earlier disjoint allocations cannot cover it.
std::uint32_t translate_sorted_source(std::span<const MccRuntimeRelocation> ranges,
                                    std::uint32_t source,std::size_t bytes) {
    if(!source || source==0xffffffffu || bytes>address_end-source)
        throw FormatError("Canonical cache pointer is null or wraps");
    auto at=std::ranges::upper_bound(ranges,source,{},&MccRuntimeRelocation::source);
    while(at!=ranges.begin()) {
        const auto& r=*--at;const auto offset=std::uint64_t(source)-r.source;
        if(offset<=r.bytes && bytes<=r.bytes-offset)
            return address(std::uint64_t(r.target)+offset);
        if(r.bytes)break;
    }
    throw FormatError("Canonical cache pointer has no declared allocation owner");
}
}
std::uint32_t relocate_mcc_runtime_address(std::span<const MccRuntimeRelocation> ranges,
                                         std::uint32_t source,std::size_t bytes) {
    validate_ranges(ranges);
    return translate(ranges,source,bytes,false);
}
CanonicalRuntimeCacheIndex build_canonical_runtime_cache_index(
    std::span<const TagGroupDefinition> groups,std::span<const Tag> tags,
    TagHandle scenario,TagHandle globals,std::uint32_t base) {
    if(!base || base%16 || tags.empty() || tags.size()>65536 || groups.size()>65536)
        throw FormatError("Invalid canonical runtime cache index shape");
    const auto table=std::uint64_t(base)+32+groups.size()*12;
    const auto size=32+groups.size()*12+tags.size()*16;
    if(std::uint64_t(base)+size>address_end)throw FormatError("Canonical runtime index exceeds logical address space");
    for(std::size_t n=0;n<groups.size();++n)
        if(groups[n].group==tag_groups::none || (n && groups[n-1].group.value>=groups[n].group.value))
            throw FormatError("Canonical runtime group directory is not unique and sorted");
    const auto group_present=[&](TagGroup group){return std::ranges::any_of(groups,[&](const auto& g){return g.group==group;});};
    for(const auto& g:groups)if((g.parent!=tag_groups::none&&!group_present(g.parent)) ||
        (g.grandparent!=tag_groups::none&&!group_present(g.grandparent)))
        throw FormatError("Canonical runtime group hierarchy is incomplete");
    CanonicalRuntimeCacheIndex out{{base,std::vector<std::byte>(size)},address(table),
        std::uint32_t(tags.size()),scenario.value,globals.value};
    auto& data=out.region.bytes;
    word(data,0,address(std::uint64_t(base)+32));word(data,4,std::uint32_t(groups.size()));
    word(data,8,out.table);word(data,12,out.scenario);word(data,16,out.globals);
    word(data,24,out.count);word(data,28,0x74616773u);
    for(std::size_t n=0;n<groups.size();++n) {
        word(data,32+n*12,groups[n].group.value);word(data,36+n*12,groups[n].parent.value);
        word(data,40+n*12,groups[n].grandparent.value);
    }
    for(std::size_t n=0;n<tags.size();++n) {
        const auto& tag=tags[n];const auto at=std::size_t(table-base)+n*16;
        if(tag.handle().is_null()) {
            if(tag.address || tag.size)throw FormatError("Null canonical tag owns metadata");
            word(data,at,0xffffffffu);word(data,at+4,0xffffffffu);continue;
        }
        if((tag.datum&65535u)!=n || !group_present(tag.type()) ||
           (!tag.address&&tag.size) || std::uint64_t(tag.address)+tag.size>address_end)
            throw FormatError("Canonical tag salt, group or allocation differs");
        word(data,at,tag.type().value);word(data,at+4,tag.datum);
        word(data,at+8,tag.address);word(data,at+12,tag.size);
    }
    const auto selected=[&](TagHandle handle,TagGroup group) {
        if(handle.is_null())return;
        const auto slot=handle.value&65535u;
        if(slot>=tags.size() || tags[slot].handle()!=handle || tags[slot].type()!=group || !tags[slot].address)
            throw FormatError("Canonical scenario/globals owner is unavailable");
    };
    selected(scenario,tag_groups::scenario);selected(globals,tag_groups::globals);
    return out;
}

MccRuntimeCacheDelivery MccRuntimeCacheDelivery::plan(const Cache& cache,std::uint32_t base,std::uint32_t available) {
    return plan(cache,base,available,{});
}
MccRuntimeCacheDelivery MccRuntimeCacheDelivery::plan(const Cache& cache,std::uint32_t base,std::uint32_t available,
    const std::function<void(MccSoundRuntimePlan&&)>& capture_sound) {
    if(!cache.is_mcc() || base<0x80061000u || base%4096 || !available || std::uint64_t(base)+available>address_end)
        throw FormatError("Canonical MCC delivery requires an explicit aligned runtime reservation");
    MccRuntimeCacheDelivery out;
    auto tags=cache.tags();
    // Index size is independent of relocation. Reserve it before BSP resources
    // and main metadata, preserving the existing original-ABI order.
    const auto index_size=32+cache.tag_groups().size()*12+tags.size()*16;
    std::uint64_t cursor=aligned(std::uint64_t(base)+index_size,16);
    const auto capacity=std::uint64_t(base)+available;
    const auto allocate=[&](std::size_t bytes) {
        const auto result=address(cursor);
        if(cursor>capacity || bytes>capacity-cursor)throw FormatError("Canonical MCC delivery exceeds its explicit reservation");
        cursor=aligned(cursor+bytes,16);return result;
    };
    if(cursor>capacity)throw FormatError("Canonical MCC index exceeds its explicit reservation");
    std::set<std::uint32_t> bsp_tags;
    const auto issue=[&](const Tag& tag,std::optional<std::uint32_t> field,std::string name,std::string reason) {
        out.issues_.push_back({tag.handle(),tag.type(),field,std::move(name),std::move(reason)});
    };
    if(cache.scenario_tag().is_null() || cache.globals_tag().is_null())
        out.issues_.push_back({{},tag_groups::none,{},"scenario/globals","A gameplay delivery requires both resident owners"});
    const Tag* scenario=cache.find_tag(cache.scenario_tag(),tag_groups::scenario);
    std::uint32_t mappings{};std::size_t mapping_count{};
    if(scenario) {
        const Bytes root(cache.read_tag_bytes(scenario->address,0x218));
        mapping_count=root.u32(0x210);mappings=root.u32(0x214);
        if(mapping_count!=cache.bsps().size() || mapping_count>256)
            throw FormatError("Canonical MCC BSP count differs from its normalized Cache owner");
    }
    const Bytes cache_bytes(cache.source_runtime_cache_bytes());
    for(std::size_t n=0;n<mapping_count;++n) {
        const auto& source=cache.bsps()[n];
        const auto mapping=address(std::uint64_t(mappings)+n*0x44);
        const auto row=cache.read_tag_bytes(mapping,0x44);const Bytes fields(row);
        const auto* bsp=cache.find_tag({fields.u32(0x14)},tag_groups::scenario_structure_bsp);
        const auto lightmap_handle=TagHandle{fields.u32(0x1c)};
        const auto* lightmap=lightmap_handle.is_null()?nullptr:cache.find_tag(lightmap_handle,tag_groups::scenario_structure_lightmap);
        if(!bsp || bsp->datum!=source.datum || (!lightmap_handle.is_null()&&!lightmap))
            throw FormatError("Canonical MCC BSP identities differ from the scenario");
        const auto source_bytes=cache_bytes.slice(source.file_offset,source.size);const Bytes header(source_bytes);
        if(source.size<16 || header.u32(0)!=source.size || header.u32(4)!=bsp->address ||
           header.u32(8)!=(lightmap?lightmap->address:0xffffffffu) || (header.u32(12)&&header.u32(12)!=0x73627370u))
            throw FormatError("Canonical MCC BSP view differs from its normalized owner");
        SourceRuntimeCacheBspImage result;
        result.mapping=mapping;result.index=std::uint32_t(n);result.file_offset=source.file_offset;
        result.bsp=bsp->datum;result.lightmap=lightmap_handle.value;
        std::copy(row.begin(),row.end(),result.mapping_bytes.begin());
        result.region={allocate(source_bytes.size()),{source_bytes.begin(),source_bytes.end()}};
        for(const auto* tag:{bsp,lightmap})if(tag) {
            if(!bsp_tags.insert(tag->datum).second)throw FormatError("Canonical MCC BSP allocation has multiple resource owners");
            if(!tag->address || !tag->size || tag->address<source.memory_address ||
               std::uint64_t(tag->address)-source.memory_address+tag->size>source.size)
                throw FormatError("Canonical MCC BSP payload escapes its owned resource");
            out.relocations_.push_back({tag->address,address(std::uint64_t(result.region.address)+tag->address-source.memory_address),tag->size});
        }
        word(result.region.bytes,12,0x73627370u);
        out.bsps_.push_back(std::move(result));
    }
    out.metadata_.address=address(cursor);
    struct Copy {const Tag* tag;std::size_t offset;bool retained=false;};std::vector<Copy> copies;
    for(const auto& tag:cache.tags())if(!tag.handle().is_null() && tag.address && tag.size && !bsp_tags.contains(tag.datum)) {
        const auto target=allocate(tag.size);
        out.relocations_.push_back({tag.address,target,tag.size});
        copies.push_back({&tag,std::size_t(target-out.metadata_.address)});
    }
    for(const auto& tag:cache.retained_tag_allocations()) {
        if(tag.handle().is_null() || !tag.address || !tag.size || !cache.find_tag(tag.handle(),tag.type()))
            throw FormatError("Canonical MCC retained allocation has no typed tag identity");
        const auto target=allocate(tag.size);
        out.relocations_.push_back({tag.address,target,tag.size});
        copies.push_back({&tag,std::size_t(target-out.metadata_.address),true});
    }
    for(const auto& tag:cache.tags())if(!tag.handle().is_null() && tag.address && !tag.size) {
        try{(void)translate(out.relocations_,tag.address,0,false);}
        catch(const FormatError&) {
            // The directory declares a location, not a payload extent. Give
            // it a deterministic distinct logical location in alignment
            // padding without changing its zero size or admitting reads.
            const auto location=allocate(0);
            if(cursor>capacity || 16>capacity-cursor)
                throw FormatError("Canonical MCC empty location exceeds its reservation");
            cursor+=16;
            out.relocations_.push_back({tag.address,location,0});
        }
    }
    validate_ranges(out.relocations_);
    out.metadata_.bytes.resize(std::size_t(cursor-out.metadata_.address));
    for(const auto& copy:copies) {
        const auto bytes=cache.read_tag_bytes(copy.tag->address,copy.tag->size);
        std::copy(bytes.begin(),bytes.end(),out.metadata_.bytes.begin()+std::ptrdiff_t(copy.offset));
    }
    for(auto& tag:tags)if(!tag.handle().is_null() && tag.address) {
        if(bsp_tags.contains(tag.datum)){tag.address=0;tag.size=0;continue;}
        try{tag.address=translate(out.relocations_,tag.address,tag.size,false);}
        catch(const FormatError&){issue(tag,{},"resident allocation","Forwarding/empty tag location has no declared typed relocation owner");tag.address=0;tag.size=0;}
    }
    out.index_=build_canonical_runtime_cache_index(cache.tag_groups(),tags,cache.scenario_tag(),cache.globals_tag(),base);
    const auto patch_pointer=[&](std::uint32_t field,std::uint32_t pointer,std::size_t bytes) {
        const auto location=translate(out.relocations_,field,4,false);
        const auto value=translate(out.relocations_,pointer,bytes,false);
        if(location>=out.metadata_.address &&
           std::uint64_t(location)-out.metadata_.address+4<=out.metadata_.bytes.size()) {
            word(out.metadata_.bytes,location-out.metadata_.address,value);return;
        }
        for(auto& bsp:out.bsps_)if(location>=bsp.region.address &&
            std::uint64_t(location)-bsp.region.address+4<=bsp.region.bytes.size()) {
            word(bsp.region.bytes,location-bsp.region.address,value);return;
        }
        throw FormatError("Canonical MCC pointer word has no owned destination backing");
    };
    // These fields are the exact fixups already applied by prepare() in the
    // preserved native library importer, including its compiled-layout shader
    // exceptions. Generic donor XML must never reinterpret those payloads.
    for(const auto& map:cache.preserved_native_pointer_maps()) {
        const auto* owner=cache.find_tag(map.owner);
        if(!owner || !map.bytes || map.address<owner->address ||
           std::uint64_t(map.address)-owner->address+map.bytes>owner->size)
            throw FormatError("Preserved native pointer map has no typed allocation owner");
        (void)translate(out.relocations_,map.address,map.bytes,false);
        for(const auto& [field,target]:map.pointers) {
            if(field>map.bytes || 4>map.bytes-field || target>=map.bytes)
                throw FormatError("Preserved native pointer field escapes its allocation");
            const auto at=address(std::uint64_t(map.address)+field);
            const auto pointer=address(std::uint64_t(map.address)+target);
            if(Bytes(cache.read_tag_bytes(at,4)).u32(0)!=pointer)
                throw FormatError("Preserved native pointer map differs from its verified payload");
            // The native map declares a location within one exact allocation.
            // Its complete remaining owned extent is known without guessing
            // an XML element stride or interpreting resource words.
            patch_pointer(at,pointer,map.bytes-target);
        }
    }
    // Explicit closed leaf maps: Assembly fd14ef3c46135e9422966c6a64cd926fb0bd23fb
    // Halo2 itmc/vehc (one16-byte permutation block) and colo (one48-byte
    // color block). We never discover pointers by inspecting word values.
    std::set<TagGroup> missing;
    for(const auto& copy:copies) {
        const auto& tag=*copy.tag;const auto group=tag.type();
        // Retained banks are allocation owners, not live directory roots.
        // Any reachable child descriptors are visited through the live root
        // which references them, using that field's verified record schema.
        if(copy.retained)continue;
        if(cache.is_preserved_native_shader(tag))continue;
        if(tag.group=="phmo" || tag.group=="coll" || tag.group=="jmad") {
            const auto payload=cache.read_tag_bytes(tag.address,tag.size);
            const auto fields=tag.group=="phmo"?normalized_mcc_physics_pointer_fields(payload,tag.address):
                tag.group=="coll"?normalized_mcc_collision_pointer_fields(payload,tag.address):
                normalized_mcc_animation_pointer_fields(payload,tag.address);
            for(const auto& field:fields)
                word(out.metadata_.bytes,copy.offset+field.offset,
                    translate(out.relocations_,field.address,field.bytes,false));
            continue;
        }
        try {
            const auto fields=ordinary_mcc_pointer_fields(group,tag.address,
                [&](std::uint32_t at,std::size_t bytes){return cache.read_tag_bytes(at,bytes);},
                MccOrdinaryPointerFlavor::Mcc,
                [&](std::uint32_t root,std::size_t bytes) {
                    return std::ranges::any_of(cache.preserved_native_pointer_maps(),[&](const auto& native) {
                        const auto* owner=cache.find_tag(native.owner,tag_groups::globals);
                        return owner && native.address==root && bytes<=native.bytes;
                    });
                });
            if(fields) {
                for(const auto& field:*fields)patch_pointer(field.field_address,field.address,field.bytes);
                continue;
            }
        }catch(const FormatError& error) {
            issue(tag,{},"typed metadata pointer closure",error.what());continue;
        }
        const unsigned stride=tag.group=="itmc"||tag.group=="vehc"?16:tag.group=="colo"?48:0;
        if(stride) {
            const Bytes root(cache.read_tag_bytes(tag.address,tag.size));root.require(0,stride==48?8:12);
            const auto count=root.u32(0),pointer=root.u32(4);
            if(count>65536)throw FormatError("Canonical MCC leaf block count is excessive");
            if(count) {
                cache.read_tag_bytes(pointer,std::size_t(count)*stride);
                word(out.metadata_.bytes,copy.offset+4,translate(out.relocations_,pointer,std::size_t(count)*stride,false));
            }
        }else if(missing.insert(group).second) {
            const auto reason=tag.group=="scnr"?"Scenario descriptors, dynamic syntax String Address fields, and BSP mappings require a closed relocation map":
                tag.group=="matg"?"Globals descriptors and locale pointer fields require a closed relocation map":
                tag.group=="jmad"?"Normalized clip and added resource-group pointer fields require their canonical relocation map":
                tag.group=="phmo"||tag.group=="coll"?"Use the normalized physics record graph; donor XML strides are not valid after normalization":
                "This normalized tag group has no complete source-backed relocation field map";
            issue(tag,{},"metadata pointer closure",reason);
        }
    }
    for(auto& bsp:out.bsps_) {
        bsp.mapping=translate(out.relocations_,bsp.mapping,0x44,false);
        word(bsp.region.bytes,4,translate(out.relocations_,Bytes(bsp.region.bytes).u32(4),0x23c,false));
        if(bsp.lightmap!=0xffffffffu)word(bsp.region.bytes,8,translate(out.relocations_,Bytes(bsp.region.bytes).u32(8),1,false));
        const auto at=std::size_t(bsp.mapping-out.metadata_.address);
        word(out.metadata_.bytes,at,bsp.file_offset);word(out.metadata_.bytes,at+4,std::uint32_t(bsp.region.bytes.size()));
        word(out.metadata_.bytes,at+8,bsp.region.address);
        const auto row=Bytes(out.metadata_.bytes).slice(at,0x44);std::copy(row.begin(),row.end(),bsp.mapping_bytes.begin());
        for(const auto identity:{bsp.bsp,bsp.lightmap})if(identity!=0xffffffffu) {
            const auto* tag=cache.find_tag({identity});
            if(!tag)throw FormatError("Canonical MCC BSP metadata identity is unavailable");
            try {
                const auto fields=ordinary_mcc_pointer_fields(tag->type(),tag->address,
                    [&](std::uint32_t address,std::size_t bytes){return cache.read_tag_bytes(address,bytes);});
                if(fields) {
                    for(const auto& field:*fields)patch_pointer(field.field_address,field.address,field.bytes);
                }else issue(*tag,{},"BSP/lightmap nested metadata",
                    "This normalized BSP resource has no complete source-backed relocation field map");
            }catch(const FormatError& error) {
                issue(*tag,{},"BSP/lightmap typed pointer closure",error.what());
            }
        }
    }
    // The source binder follows matg+C0/C4 and reads the original Sound
    // Globals' gestalt datum at +20. MCC prepends a remastered-class ref,
    // shifting every original field by eight bytes. Project this declared
    // record only in the runtime copy; native PCM retains the complete MCC
    // donor row and its exact sound-resource ownership.
    const auto* globals=cache.find_tag(cache.globals_tag(),tag_groups::globals);
    if(globals) {
        const Bytes root(cache.read_tag_bytes(globals->address,0xc8));
        if(root.u32(0xc0)!=1)throw FormatError("Canonical MCC Sound Globals requires one declared record");
        const auto source=root.u32(0xc4);
        const auto projected=project_mcc_sound_globals(cache.read_tag_bytes(source,0x2c));
        const auto target=translate(out.relocations_,source,0x2c,false);
        if(target<out.metadata_.address ||
           std::uint64_t(target)-out.metadata_.address+projected.size()>out.metadata_.bytes.size())
            throw FormatError("Canonical MCC Sound Globals has no owned runtime backing");
        std::copy(projected.begin(),projected.end(),
            out.metadata_.bytes.begin()+std::ptrdiff_t(target-out.metadata_.address));
    }
    // Source sound consumers use the original eleven-block gestalt, not the
    // MCC codec/reflection/language descriptors. Keep the raw Cache and PCM
    // DAT ownership intact; only this vanilla runtime copy receives the
    // complete merged tables and projected definition/class roots.
    auto sound=plan_mcc_sound_runtime_delivery(cache,address(cursor),
        [&](std::uint32_t source,std::size_t bytes){return translate(out.relocations_,source,bytes,false);});
    struct SoundCommit {std::size_t offset;const MccSoundRuntimePatch* patch;};
    std::vector<SoundCommit> sound_commits;
    std::vector<std::pair<std::uint64_t,std::uint64_t>> sound_ranges;
    for(const auto& patch:sound.patches) {
        const auto* owner=cache.find_tag(patch.owner);
        if(!owner || owner->address!=patch.source_address || patch.before.empty() ||
           patch.before.size()!=patch.after.size() || patch.before.size()>owner->size ||
           (owner->group!="ugh!" && owner->group!="snd!" && owner->group!="sncl"))
            throw FormatError("Native sound patch has no complete typed root owner");
        if(!std::ranges::equal(cache.read_tag_bytes(patch.source_address,patch.before.size()),patch.before))
            throw FormatError("Native sound source changed after planning");
        const auto target=translate(out.relocations_,patch.source_address,patch.before.size(),false);
        if(target<out.metadata_.address || std::uint64_t(target)-out.metadata_.address>out.metadata_.bytes.size() ||
           patch.before.size()>out.metadata_.bytes.size()-(std::uint64_t(target)-out.metadata_.address))
            throw FormatError("Native sound patch escapes the canonical metadata owner");
        auto expected=patch.before;
        const auto fields=ordinary_mcc_pointer_fields(owner->type(),owner->address,
            [&](std::uint32_t at,std::size_t bytes){return cache.read_tag_bytes(at,bytes);});
        if(!fields)throw FormatError("Native sound root has no declared donor pointer map");
        for(const auto& field:*fields)if(field.field_address>=patch.source_address &&
            std::uint64_t(field.field_address)-patch.source_address+4<=expected.size())
            word(expected,field.field_address-patch.source_address,
                translate(out.relocations_,field.address,field.bytes,false));
        const auto offset=std::size_t(target-out.metadata_.address);
        if(!std::ranges::equal(std::span(out.metadata_.bytes).subspan(offset,expected.size()),expected))
            throw FormatError("Native sound runtime root differs from its complete relocated preimage");
        sound_ranges.emplace_back(target,std::uint64_t(target)+patch.after.size());
        sound_commits.push_back({offset,&patch});
    }
    std::ranges::sort(sound_ranges);
    for(std::size_t n=1;n<sound_ranges.size();++n)if(sound_ranges[n].first<sound_ranges[n-1].second)
        throw FormatError("Native sound runtime root patches overlap");
    if(allocate(sound.bytes.size())!=sound.address)
        throw FormatError("Native sound append differs from its declared allocation");
    out.metadata_.bytes.resize(std::size_t(cursor-out.metadata_.address));
    std::copy(sound.bytes.begin(),sound.bytes.end(),
        out.metadata_.bytes.begin()+std::ptrdiff_t(sound.address-out.metadata_.address));
    for(const auto& commit:sound_commits)
        std::copy(commit.patch->after.begin(),commit.patch->after.end(),
            out.metadata_.bytes.begin()+std::ptrdiff_t(commit.offset));
    const auto reserved=aligned(cursor-base,4096);
    if(reserved>available || reserved>0xffffffffu)throw FormatError("Canonical MCC reservation exceeds available pages");
    out.reservation_bytes_=std::uint32_t(reserved);
    // No later allocation is appended. Keep serialized addresses, bytes and
    // validation unchanged; only the host lookup order is indexed here.
    std::ranges::sort(out.relocations_,{},&MccRuntimeRelocation::source);
    if(capture_sound) {
        out.require_complete();
        capture_sound(std::move(sound));
    }
    return out;
}
void MccRuntimeCacheDelivery::require_complete() const {
    if(!index_.region.address || !index_.count || !reservation_bytes_ || metadata_.bytes.empty())
        throw FormatError("Canonical MCC runtime delivery has no owned plan");
    if(!issues_.empty()) {
        const auto& first=issues_.front();
        throw FormatError("Canonical MCC runtime delivery rejected: "+first.group.string()+" "+first.field+": "+first.reason);
    }
}
SourceRuntimeCacheTagImage MccRuntimeCacheDelivery::tag_image() const {
    require_complete();SourceRuntimeCacheTagImage out;
    out.regions={index_.region,metadata_};out.index=index_.region.address;out.table=index_.table;
    out.count=index_.count;out.scenario=index_.scenario;out.globals=index_.globals;out.reservation_bytes=reservation_bytes_;return out;
}
SourceRuntimeCacheBspImage MccRuntimeCacheDelivery::bsp_image(std::size_t index) const {
    require_complete();if(index>=bsps_.size())throw FormatError("Canonical MCC BSP selection escapes delivery");return bsps_[index];
}
std::uint32_t MccRuntimeCacheDelivery::source_address(std::uint32_t logical,std::size_t bytes) const {
    return translate(relocations_,logical,bytes,true);
}
std::uint32_t MccRuntimeCacheDelivery::runtime_address(std::uint32_t source,std::size_t bytes) const {
    return translate_sorted_source(relocations_,source,bytes);
}
} // namespace halo2

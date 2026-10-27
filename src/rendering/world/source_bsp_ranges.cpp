#include "halo2/source_bsp_ranges.hpp"
#include "halo2/native_parallel.hpp"
#include "halo2/source_bsp_accelerator.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <cstdlib>
#include <bit>
#include <cmath>

namespace halo2 {
void source_bsp_range_arena_begin(SourceBspRangeArena& arena) {
    if(arena.used>arena.words.size())
        throw FormatError("source query mask cursor escapes its allocation");
    std::fill_n(arena.words.begin(),arena.used,0u);
    arena.used=0;
}
void source_bsp_query_begin(SourceBspQueryResetState& state) {
    // Validate before exposing a partially reset native context.
    if(state.arena.used>state.arena.words.size())
        throw FormatError("source query mask cursor escapes its allocation");
    state.queue_counts={};state.query_mask={};
    source_bsp_range_arena_begin(state.arena);
    state.association_count=0;state.cluster_entries.fill(255);
}
namespace {
std::int32_t signed32(std::uint32_t value) {return std::bit_cast<std::int32_t>(value);}
std::int16_t signed16(std::uint16_t value) {return std::bit_cast<std::int16_t>(value);}
void finite(const ShaderVector &p) {
    for(const auto v:p)if(!std::isfinite(v))throw FormatError("source BSP plane is nonfinite");
}
void finite(const Transform &t) {
    if(!std::isfinite(t.scale))throw FormatError("source BSP plane transform scale is nonfinite");
    for(const auto p:t.basis)for(const auto v:{p.x,p.y,p.z})
        if(!std::isfinite(v))throw FormatError("source BSP plane transform basis is nonfinite");
    for(const auto v:{t.position.x,t.position.y,t.position.z})
        if(!std::isfinite(v))throw FormatError("source BSP plane transform position is nonfinite");
}
std::size_t mask_words(const SourceBspRangeGeometry &g) {
    if(g.subpart_count<0 || std::size_t(g.subpart_count)!=g.subparts.size())
        throw FormatError("source BSP signed subpart count differs from its resource");
    return (std::size_t(g.subpart_count)+31)/32;
}
void valid_parts(const SourceBspRangeGeometry &g) {
    if(g.part_count<0 || std::size_t(g.part_count)!=g.parts.size())
        throw FormatError("source BSP signed part count differs from its resource");
}
SourceBspRangeGeometry decode_geometry(std::span<const std::byte> raw_header,
    std::span<const MenuWorldSourceResource> resources) {
    const Bytes header(raw_header);SourceBspRangeGeometry g;
    g.part_count=signed32(header.u32(0));g.subpart_count=signed32(header.u32(8));
    g.accelerator_count=signed32(header.u32(0x30));g.index_count=header.u32(0x20);
    const MenuWorldSourceResource *parts=nullptr,*subparts=nullptr,*accelerator=nullptr,*map=nullptr,*indices=nullptr,*bounds=nullptr;
    for(const auto &resource:resources) {
        const auto set=[&](const MenuWorldSourceResource *&target) {
            if(target)throw FormatError("source BSP range resource identity is ambiguous");
            target=&resource;
        };
        if(resource.type==0 && resource.channel==72)set(parts);
        if(resource.type==8 && resource.channel==8)set(subparts);
        if(resource.type==16 && resource.channel==20)set(bounds);
        if(resource.type==40 && resource.channel==65535)set(accelerator);
        if(resource.type==48 && resource.channel==2)set(map);
        if(resource.type==32 && resource.channel==2)set(indices);
    }
    if(!parts || parts->bytes.size()%72 || (g.subpart_count>0 && !subparts) || (subparts && subparts->bytes.size()%8) ||
        !indices || g.index_count>65535 || indices->bytes.size()!=std::size_t(g.index_count)*2)
        throw FormatError("source BSP part/subpart/index resource is unavailable");
    for(std::size_t i=0;i<parts->bytes.size();i+=72) {
        std::array<std::byte,72> value;std::copy_n(parts->bytes.begin()+i,72,value.begin());g.parts.push_back(value);
    }
    for(std::size_t i=0;subparts && i<subparts->bytes.size();i+=8) {
        std::array<std::byte,8> value;std::copy_n(subparts->bytes.begin()+i,8,value.begin());g.subparts.push_back(value);
    }
    valid_parts(g);(void)mask_words(g);
    const auto bound_count=signed32(header.u32(0x10));
    if(bound_count<0 || (bounds && bounds->bytes.size()!=std::size_t(bound_count)*20) ||
        (bound_count && !bounds))throw FormatError("source BSP linear bound resource is unavailable");
    if(bounds) {
        const Bytes b(bounds->bytes);
        for(unsigned i=0;i<unsigned(bound_count);++i) {
            const auto at=std::size_t(i)*20;
            g.bounds.push_back({{b.f32(at),b.f32(at+4),b.f32(at+8)},b.f32(at+12),
                std::uint8_t(std::to_integer<unsigned>(b.slice(at+16,1)[0]))});
        }
    }
    if(g.accelerator_count>0) {
        if(!accelerator || !map || map->bytes.size()!=std::size_t(g.accelerator_count)*2)
            throw FormatError("source BSP accelerator/mapping counts are unavailable");
        g.accelerator=accelerator->bytes;const Bytes m(map->bytes);
        for(unsigned i=0;i<unsigned(g.accelerator_count);++i)g.mapping.push_back(m.u16(i*2));
    }
    return g;
}
std::pair<int,int> part_subparts(const SourceBspRangeGeometry &g,unsigned part) {
    valid_parts(g);
    if(part>=g.parts.size())throw FormatError("source BSP part index escapes its resource");
    const Bytes b(g.parts[part]);const int first=signed16(b.u16(0xa)),count=signed16(b.u16(0xc));
    if(first<0 || count<0 || std::size_t(first)>g.subparts.size() ||
        std::size_t(count)>g.subparts.size()-first)
        throw FormatError("source BSP signed part subpart interval escapes its resource");
    return {first,count};
}
std::uint16_t mapping(const SourceBspRangeGeometry &g,std::int64_t at) {
    if(at<0 || std::uint64_t(at)>=g.mapping.size())
        throw FormatError("source BSP accelerator mapping escapes its resource");
    return g.mapping[std::size_t(at)];
}
void mark(const SourceBspRangeGeometry &g,std::span<std::uint32_t> mask,unsigned subpart) {
    if(subpart>=g.subparts.size() || subpart/32>=mask.size())
        throw FormatError("source BSP accelerator subpart escapes its mask");
    mask[subpart/32]|=1u<<(subpart&31);
}
void mark_key(const SourceBspRangeGeometry &g,bool parts_tree,
    std::span<std::uint32_t> mask,std::uint32_t key) {
    if(!parts_tree) {mark(g,mask,std::uint16_t(key));return;}
    const auto [first,count]=part_subparts(g,key);
    for(int i=first;i<first+count;++i)mark(g,mask,unsigned(i));
}
void expand(const SourceBspRangeGeometry &g,bool parts_tree,
    std::span<const std::uint32_t> direct,std::span<const std::uint32_t> ranges,
    std::span<std::uint32_t> mask,std::uint32_t &flags) {
    if(mask.size()<mask_words(g) || ranges.size()%2)
        throw FormatError("source BSP accelerator list/mask extent is incomplete");
    valid_parts(g);
    if(!direct.empty() || !ranges.empty())flags|=1;
    // Range list is consumed before the direct list. Each endpoint uses MOVSX
    // low16; mapped first/last endpoints use MOVZX, including inclusive last.
    const std::int64_t base=parts_tree?std::int64_t(g.part_count)+2ll*g.subpart_count:g.subpart_count;
    const std::int64_t key_base=parts_tree?2ll*g.subpart_count:0;
    for(std::size_t r=0;r<ranges.size();r+=2) {
        const auto first=mapping(g,base+signed16(std::uint16_t(ranges[r])));
        const auto last=mapping(g,base+signed16(std::uint16_t(ranges[r+1])));
        for(unsigned i=first;i<=unsigned(last);++i)
            mark_key(g,parts_tree,mask,mapping(g,key_base+signed16(std::uint16_t(i))));
    }
    for(const auto key:direct)mark_key(g,parts_tree,mask,key);
}
std::span<std::uint32_t> mask_for(SourceBspRangeArena &arena,
    std::uint32_t base,std::uint32_t address,const SourceBspRangeGeometry &g) {
    if(!base || address<base || (address-base)%4)
        throw FormatError("source BSP actual mask backing is unavailable");
    const auto first=std::size_t(address-base)/4,count=mask_words(g);
    if(first>arena.words.size() || count>arena.words.size()-first)
        throw FormatError("source BSP mask escapes shared arena backing");
    return std::span<std::uint32_t>(arena.words).subspan(first,count);
}
void cull_entry(SourceCullDrawUnit &unit,const SourceBspFrustumDrawBank &bank,
    SourceBspRangeArena &arena,const SourceBspFrustumCullContext &context,
    const SourcePortalVisibilitySet &visibility,unsigned entry) {
    if(entry>=visibility.clusters.size())throw FormatError("source BSP constructed cluster entry is unavailable");
    if(!bank.geometry_resolved)throw FormatError("source BSP frustum geometry resolver is unavailable");
    if(!bank.geometry)throw FormatError("source BSP frustum geometry is not resident");
    if(unit.words[1]!=0xffffffffu)
        throw FormatError("source BSP frustum posed/model resolver is not supplied");
    const auto &g=*bank.geometry;
    if(g.accelerator_count<=0)throw FormatError("source BSP frustum requires accelerated geometry");
    const bool parts_tree=context.mode==0 && (unit.words[0]&0x1000) &&
        std::int64_t(g.accelerator_count)>2ll*g.subpart_count;
    std::span<const std::byte> tree(g.accelerator);
    if(parts_tree) {
        const Bytes header(tree);const auto offset=(std::uint64_t(header.u32(0x20))+15)&~15ull;
        if(offset>tree.size())throw FormatError("source BSP second accelerator escapes its resource");
        tree=tree.subspan(std::size_t(offset));
    }
    if(visibility.groups.size()>6)throw FormatError("source BSP frustum group bank exceeds six");
    std::vector<std::uint32_t> direct,ranges;
    const auto &cluster=visibility.clusters[entry];
    for(unsigned group=0;group<visibility.groups.size();++group) {
        const int first=cluster.first_indices[group],count=cluster.counts[group];
        if(count>0 && (first<0 || std::size_t(first)>visibility.frustums.size() ||
            std::size_t(count)>visibility.frustums.size()-first))
            throw FormatError("source BSP frustum group range is unavailable");
        for(int i=0;i<count;++i) {
            if(unit.words[7] && !bank.transform)
                throw FormatError("source BSP actual owner transform is unavailable");
            const auto transform=unit.words[7]?bank.transform:std::optional<Transform>{};
            const auto planes=source_bsp_accelerator_planes(visibility.frustums[first+i].planes,transform);
            const auto result=source_bsp_accelerator_query0(tree,planes,direct,ranges);
            if(!result.complete)throw FormatError("source BSP accelerator query0: "+result.diagnostic);
        }
        // Original458D0/45A80 return before consuming the mask pointer when
        // both lists are empty. Empty groups need no owner transform either.
        if(!direct.empty() || !ranges.empty())
            expand(g,parts_tree,direct,ranges,
                mask_for(arena,context.mask_arena_address,unit.words[5],g),unit.words[0]);
    }
}
}
SourceBspRangeGeometry source_bsp_range_geometry(const MenuWorldSourceSection &section) {
    if(section.kind()==MenuWorldSourceSectionKind::render_model)
        throw FormatError("source BSP range geometry is not a BSP allocation");
    auto g=decode_geometry(section.raw_header(),section.resources());
    if(g.index_count!=section.indices().size() || g.parts.size()!=section.parts().size())
        throw FormatError("source BSP range geometry differs from owned vertex section");
    return g;
}
SourceBspRangeGeometry source_model_range_geometry(const MenuWorldSourceSection &section) {
    if(section.kind()!=MenuWorldSourceSectionKind::render_model)
        throw FormatError("source model range geometry requires a model allocation");
    auto g=decode_geometry(section.raw_model_header(),section.resources());
    if(g.index_count!=section.indices().size() || g.parts.size()!=section.parts().size())
        throw FormatError("source model range geometry differs from owned vertex section");
    return g;
}
SourceBspRangeGeometry load_source_model_range_geometry(const Cache& cache,std::uint32_t model,unsigned section) {
    const auto owned=cache.load_source_tag_metadata(model);
    if(owned.group()!="mode")throw FormatError("source model range loader requires mode metadata");
    const Bytes root(owned.bytes_at(owned.source_address(),0x2c));
    if(section>=root.u32(0x24))throw FormatError("source model range section escapes metadata");
    const Bytes metadata(owned.bytes_at(root.u32(0x28)+section*92,92));
    const auto allocations=cache.load_model_resources(model);
    if(section>=allocations.size())throw FormatError("source model range resource allocation missing");
    const Bytes raw(allocations[section]);const auto tail=metadata.u32(0x44);
    if(std::uint64_t(tail)+12>raw.size())throw FormatError("source model range header is truncated");
    const auto base=raw.size()-tail-4;
    const auto count=metadata.u32(0x48),start=metadata.u32(0x4c);
    if(count>128)throw FormatError("source model range resource bank escapes metadata");
    std::vector<MenuWorldSourceResource> resources;
    for(unsigned i=0;i<count;++i){
        const Bytes record(owned.bytes_at(start+i*16,16));const auto offset=std::uint64_t(base)+record.u32(12);
        if(offset>raw.size())throw FormatError("source model range resource offset escapes allocation");
        const auto bytes=raw.slice(std::size_t(offset),record.u32(8));
        resources.push_back({record.u16(4),record.u16(6),std::uint32_t(offset),{bytes.begin(),bytes.end()}});
    }
    return decode_geometry(raw.slice(8,base-8),resources);
}
SourceCullLinearGeometry source_bsp_sphere_cull_geometry(const SourceBspRangeGeometry& geometry) {
    SourceCullLinearGeometry result;
    result.accelerator_count=geometry.accelerator_count;
    result.subpart_count=geometry.subpart_count;
    if(geometry.accelerator_count>0)result.accelerator=&geometry;
    else {
        if(geometry.subpart_count<0 || std::size_t(geometry.subpart_count)!=geometry.subparts.size())
            throw FormatError("source BSP sphere subpart resource is incomplete");
        for(const auto& subpart:geometry.subparts)result.subpart_bounds.push_back(Bytes(subpart).u16(4));
        result.bounds=geometry.bounds;
    }
    return result;
}
SourceBspRangeGeometry load_source_bsp_range_geometry(const Cache &cache,std::size_t bsp,
    MenuWorldSourceSectionKind kind,unsigned section) {
    if(kind!=MenuWorldSourceSectionKind::bsp_cluster && kind!=MenuWorldSourceSectionKind::bsp_instance_definition)
        throw FormatError("source BSP range loader requires a BSP allocation");
    const auto owned=cache.load_source_bsp_metadata(bsp);
    const Bytes m(owned.bytes_at(owned.source_address(),owned.size()));
    
    const auto [nc,clusters]=TagAddressSpace(m, owned.source_address()).block(16+0x9c, 0xb0, 2048);
    const auto [nd,definitions]=TagAddressSpace(m, owned.source_address()).block(16+0x138, 0xc8, 2048);
    const bool instance=kind==MenuWorldSourceSectionKind::bsp_instance_definition;
    if(section>=(instance?nd:nc))throw FormatError("source BSP range section escapes metadata");
    const auto at=(instance?definitions:clusters)+std::size_t(section)*(instance?0xc8:0xb0);
    const auto allocations=cache.load_bsp_resources(bsp);const auto ordinal=instance?nc+section:section;
    if(allocations.size()!=std::size_t(nc)+nd || ordinal>=allocations.size())
        throw FormatError("source BSP range raw allocations differ from metadata");
    const Bytes raw(allocations[ordinal]);const std::uint64_t base=std::uint64_t(m.u32(at+48))+8;
    if(base>raw.size())throw FormatError("source BSP range raw header escapes allocation");
    const auto [nr,resource_at]=TagAddressSpace(m, owned.source_address()).block(at+56, 16, 128);
    std::vector<MenuWorldSourceResource> resources;
    for(unsigned i=0;i<nr;++i) {
        const auto r=resource_at+i*16;const auto offset=base+m.u32(r+12);
        if(offset>raw.size())throw FormatError("source BSP range resource offset escapes allocation");
        const auto bytes=raw.slice(std::size_t(offset),m.u32(r+8));
        resources.push_back({m.u16(r+4),m.u16(r+6),std::uint32_t(offset),{bytes.begin(),bytes.end()}});
    }
    return decode_geometry(raw.slice(8,std::size_t(base)-8),resources);
}
std::optional<unsigned> SourceBspRangeArena::allocate(std::int32_t count) {
    if(count<=0)return {};
    if(used>512 || count>16384)throw FormatError("source BSP mask request exceeds actual arena backing");
    const unsigned required=(unsigned(count)+31)/32,next=used+required;
    if(next>=512)return 0;
    const unsigned offset=used;used=std::uint16_t(next);return offset;
}
std::array<ShaderVector,6> source_bsp_accelerator_planes(
    const std::array<ShaderVector,6> &source,const std::optional<Transform> &transform) {
    if(transform)finite(*transform);
    std::array<ShaderVector,6> result;
    for(unsigned i=0;i<source.size();++i) {
        const auto &p=source[i];finite(p);float d=p[3],x=p[0],y=p[1],z=p[2];
        if(transform) {
            const auto &t=*transform;
            d=t.scale==0.f?0.f:d-((t.position.y*y+t.position.z*z)+t.position.x*x);
            if(t.scale!=0.f && t.scale!=1.f)d=d/t.scale;
            if(t.scale!=1.f) {const float reciprocal=1.f/t.scale;x=reciprocal*x;y=reciprocal*y;z=reciprocal*z;}
            result[i][0]=(t.basis[0].z*z+t.basis[0].y*y)+t.basis[0].x*x;
            result[i][1]=(t.basis[1].z*z+t.basis[1].y*y)+t.basis[1].x*x;
            result[i][2]=(t.basis[2].z*z+t.basis[2].y*y)+t.basis[2].x*x;
        } else {result[i][0]=x;result[i][1]=y;result[i][2]=z;}
        result[i][3]=0.f-d;
    }
    return result;
}
void source_bsp_expand_accelerator_lists(const SourceBspRangeGeometry &g,bool parts_tree,
    std::span<const std::uint32_t> direct,std::span<const std::uint32_t> ranges,
    std::span<std::uint32_t> mask,std::uint32_t &flags) {
    auto copy=std::vector<std::uint32_t>(mask.begin(),mask.end());auto new_flags=flags;
    expand(g,parts_tree,direct,ranges,copy,new_flags);
    std::copy(copy.begin(),copy.end(),mask.begin());flags=new_flags;
}
std::vector<MenuWorldSourceIndexRange> source_bsp_part_ranges(const SourceBspRangeGeometry &g,
    unsigned part,MenuWorldSourceTopology topology,bool filtered,std::span<const std::uint32_t> mask) {
    valid_parts(g);if(part>=g.parts.size())throw FormatError("source BSP index range part is unavailable");
    if(topology!=MenuWorldSourceTopology::triangle_list && topology!=MenuWorldSourceTopology::triangle_strip)
        throw FormatError("source BSP index range topology is unsupported");
    const auto checked=[&](unsigned first,unsigned count) {
        if(first>g.index_count || count>g.index_count-first)
            throw FormatError("source BSP emitted range escapes the original index bank");
        return MenuWorldSourceIndexRange{first,count};
    };
    const Bytes p(g.parts[part]);
    if(!filtered)return {checked(p.u16(6),p.u16(8))};
    if(mask.size()<mask_words(g))throw FormatError("source BSP filtered index mask is incomplete");
    const auto [first,count]=part_subparts(g,part);
    std::vector<MenuWorldSourceIndexRange> out;
    for(int i=first;i<first+count;++i)if(mask[unsigned(i)/32]&(1u<<(i&31))) {
        const Bytes subpart(g.subparts[i]);const auto range=checked(subpart.u16(0),subpart.u16(2));
        if(topology==MenuWorldSourceTopology::triangle_list && !out.empty() &&
            out.back().first+out.back().count==range.first)out.back().count+=range.count;
        else out.push_back(range);
    }
    return out;
}
void source_bsp_frustum_cull(std::span<SourceCullDrawUnit> units,
    std::span<const SourceBspFrustumDrawBank> banks,SourceBspRangeArena &arena,
    const SourceBspFrustumCullContext &c,const SourcePortalVisibilitySet &visibility,
    std::span<const std::array<std::uint32_t,4>> associations) {
    source_bsp_frustum_cull(units,banks,arena,c,visibility,associations,SourceBspFrustumCullExecution::automatic);
}
void source_bsp_frustum_cull(std::span<SourceCullDrawUnit> units,
    std::span<const SourceBspFrustumDrawBank> banks,SourceBspRangeArena &arena,
    const SourceBspFrustumCullContext &c,const SourcePortalVisibilitySet &visibility,
    std::span<const std::array<std::uint32_t,4>> associations,SourceBspFrustumCullExecution execution) {
    auto next_units=std::vector<SourceCullDrawUnit>(units.begin(),units.end());
    auto next_arena=arena;
    const unsigned first=c.mode?c.main_prefix&65535u:0u;
    const unsigned end=(c.mode?c.draw_count:c.main_prefix)&65535u;
    const auto cull_unit=[&](unsigned i) {
        if(i>=32768 || i>=next_units.size())throw FormatError("source BSP frustum unit prefix is unavailable");
        auto &unit=next_units[i];
        if((unit.words[0]&1) || !(unit.words[0]&0x40))return;
        if(c.query_mode!=0)throw FormatError("source BSP frustum query mode is unsupported");
        if(i>=banks.size())throw FormatError("source BSP frustum draw bank is unavailable");
        if(unit.words[0]&8) {
            const int association=signed16(std::uint16_t(unit.words[6]));
            if(association<0 || std::size_t(association)>=associations.size())
                throw FormatError("source BSP frustum association bank is unavailable");
            const auto &bits=associations[association];
            for(unsigned entry=0;entry<128;++entry)if(bits[entry/32]&(1u<<(entry&31)))
                cull_entry(unit,banks[i],next_arena,c,visibility,entry);
        } else cull_entry(unit,banks[i],next_arena,c,visibility,std::uint16_t(unit.words[6]));
    };
    // Each unit's cull writes only its own record and its own pre-allocated
    // mask words (mask_for maps unit+5; nothing is allocated here), so units
    // cull in index bands on the worker pool when no two culled units share an
    // arena word. Every precondition the serial walk would reject keeps the
    // serial walk, which then rejects it in the same order.
    // Banded execution stays opt-in (HALO2_NATIVE_BANDED_CULL=1) until a
    // byte-identical frame capture has been recorded for it.
    static const bool banded_cull=[]{const auto *p=std::getenv("HALO2_NATIVE_BANDED_CULL");return p && *p;}();
    const auto workers=native_parallel_workers();
    const bool requested=execution==SourceBspFrustumCullExecution::banded ||
        (execution==SourceBspFrustumCullExecution::automatic && banded_cull);
    bool banded=requested && workers>0 && end>first+1;
    if(banded) {
        std::vector<std::pair<std::uint32_t,std::uint32_t>> spans;
        for(unsigned i=first;i<end && banded;++i) {
            if(i>=next_units.size()){banded=false;break;}
            const auto &unit=next_units[i];
            if((unit.words[0]&1) || !(unit.words[0]&0x40))continue;
            if(i>=banks.size() || !banks[i].geometry || c.query_mode!=0){banded=false;break;}
            const auto base=c.mask_arena_address,address=unit.words[5];
            if(!base || address<base || (address-base)%4){banded=false;break;}
            const auto word_first=std::uint32_t((address-base)/4),count=std::uint32_t(mask_words(*banks[i].geometry));
            if(word_first>next_arena.words.size() || count>next_arena.words.size()-word_first){banded=false;break;}
            spans.emplace_back(word_first,word_first+count);
        }
        if(banded) {
            std::sort(spans.begin(),spans.end());
            std::uint32_t reach=0;
            for(const auto &[word_first,word_end]:spans){if(word_first<reach){banded=false;break;}reach=std::max(reach,word_end);}
        }
    }
    if(banded) {
        const unsigned count=end-first,bands=std::min(count,workers+1u);
        native_parallel_for(bands,[&](std::size_t band) {
            const unsigned band_first=first+unsigned(std::uint64_t(count)*band/bands);
            const unsigned band_end=first+unsigned(std::uint64_t(count)*(band+1)/bands);
            for(unsigned i=band_first;i<band_end;++i)cull_unit(i);
        });
    } else for(unsigned i=first;i<end;++i)cull_unit(i);
    std::copy(next_units.begin(),next_units.end(),units.begin());
    arena=next_arena;
}
} // namespace halo2

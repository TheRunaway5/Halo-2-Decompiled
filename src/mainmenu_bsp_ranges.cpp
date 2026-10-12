#include "halo2/mainmenu_bsp_ranges.hpp"
#include "halo2/source_shader_lod.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <map>

namespace halo2 {
namespace {
//1332F0, normal context+2A60=0. The fog fields are the actual aggregate
//4B9EF0/+10C/+114 and selected planar definition flags, not geometry guesses.
std::uint32_t collector_flags(std::uint16_t queue,const BspFogSample &fog) {
    std::uint32_t flags=((queue>>2&0x2c00u)|(queue&0x4000u))>>9;
    flags|=(queue&0x200u)<<5;
    if(fog.classification==3) {
        if(fog.planar_datum!=0xffffffffu && fog.eye_fraction==0.f &&
           !(queue&0x400u) && !(fog.planar_flags&2u))flags|=0x100;
        if(!fog.planar_only || (queue&0x400u))flags|=0x80;
    }
    return flags;
}
}
struct MainmenuBspRangeProducer::Impl {
    const Cache &cache;
    std::size_t bsp;
    SourceBspAdmissionBsp authored;
    std::vector<Transform> transforms;
    std::map<std::pair<bool,unsigned>,SourceBspRangeGeometry> geometries;
    SourceBspRangeArena retained_arena;
    Impl(const Cache &cache,std::size_t bsp):cache(cache),bsp(bsp),authored(cache.load_source_bsp_admission(bsp)) {
        if(cache.load_source_geometry_scenario_type()!=2)
            throw FormatError("mainmenu BSP ranges require source scenario type2");
        const auto owned=cache.load_source_bsp_metadata(bsp);
        const Bytes m(owned.bytes_at(owned.source_address(),owned.size()));
        const auto count=m.u32(16+0x140),pointer=m.u32(16+0x144);
        if(count!=authored.instances.size() || (count && pointer<owned.source_address()))
            throw FormatError("mainmenu BSP range instance transform bank differs from admission metadata");
        const auto at=count?std::size_t(pointer-owned.source_address()):0;
        m.require(at,std::size_t(count)*0x58);
        for(unsigned i=0;i<count;++i) {
            std::array<std::uint32_t,13> words;
            for(unsigned j=0;j<13;++j)words[j]=m.u32(at+i*0x58+j*4);
            transforms.push_back(std::bit_cast<Transform>(words));
        }
    }
    const SourceBspRangeGeometry &geometry(bool instance,unsigned section) {
        const auto key=std::pair{instance,section};const auto found=geometries.find(key);
        if(found!=geometries.end())return found->second;
        return geometries.emplace(key,load_source_bsp_range_geometry(cache,bsp,
            instance?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster,section)).first->second;
    }
    MainmenuBspRangeFrame prepare(const MainmenuBspAdmissionFrame &frame) {
        if(!frame.location_query_succeeded || frame.current_location.cluster<0)
            throw FormatError("mainmenu BSP ranges require a current inside-BSP admission frame");
        MainmenuBspRangeFrame result;
        //131FF0 initializes storage.1348E0 clears only the previously used
        //prefix, then resets used. Retain the tail and any overflow alias
        //effects rather than replacing the original allocator with a clear.
        result.arena=retained_arena;
        std::fill_n(result.arena.words.begin(),result.arena.used,0u);
        result.arena.used=0;
        constexpr std::uint32_t arena_identity=4;
        std::vector<SourceCullDrawUnit> units;
        std::vector<SourceBspFrustumDrawBank> banks;
        const auto camera=source_shader_coverage_camera(frame.render_camera);
        const auto schedule=[&](MainmenuBspRangeUnit unit,bool instance,bool metadata_geometry) {
            if(units.size()>=850)throw FormatError("mainmenu BSP prefix exceeds original2CBF0 unit capacity");
            const auto &g=geometry(instance,unit.section);
            //2D000 returns3 with no active lightmap and the BSP collector's
            //model datumNONE. No retained/unit pointer word is invented here.
            unit.flags=(unit.flags&0x1fffffu)|(3u<<29);
            SourceCullDrawUnit raw;raw.words[0]=unit.flags;raw.words[1]=0xffffffffu;
            raw.words[2]=0xffffffffu;raw.words[6]=0xffffu;
            if(unit.flags&0x40) {
                if(metadata_geometry)unit.mask_word_offset=result.arena.allocate(g.subpart_count);
                if(unit.mask_word_offset)raw.words[5]=arena_identity+4 * *unit.mask_word_offset;
                else raw.words[0]=(raw.words[0]&~0x40u)|1u;
            }
            raw.words[7]=unit.transform?1:0;
            unit.flags=raw.words[0];units.push_back(raw);
            banks.push_back({&g,true,unit.transform});result.units.push_back(std::move(unit));
        };
        //133390 preserves the cluster queue order and obtains its constructed
        //entry from context+1C, rather than using the authored cluster ID.
        for(const auto &record:frame.admission.clusters) {
            if(record.id>=authored.clusters.size() || record.id>=512)
                throw FormatError("mainmenu BSP range cluster escapes authored bank");
            const auto entry=frame.admission.cluster_entries[record.id];
            if(entry==0xff || entry>=frame.visibility.clusters.size() ||
               frame.visibility.clusters[entry].source_cluster!=int(record.id))
                throw FormatError("mainmenu BSP range cluster lacks its actual constructed entry");
            MainmenuBspRangeUnit unit;unit.geometry=int(record.id);unit.section=record.id;
            unit.flags=collector_flags(record.flags,frame.fog)|0x40;
            unit.coverage=source_shader_coverage_byte(source_shader_cluster_coverage(authored.clusters[record.id].bounds,camera));unit.alpha=255;
            schedule(std::move(unit),false,authored.clusters[record.id].geometry_count!=0);
            units.back().words[6]=std::uint32_t(entry)|(255u<<16);
        }
        //133520 computes alpha before2CBF0. Zero-alpha commands never allocate
        //a draw unit/mask; the source queue's skipped bit is reported separately.
        for(const auto &record:frame.admission.instances) {
            if(record.id>=authored.instances.size() || record.id>=2048)
                throw FormatError("mainmenu BSP range instance escapes original11-bit owner bank");
            const auto &instance=authored.instances[record.id];
            if(instance.definition<0 || unsigned(instance.definition)>=authored.definitions.size())
                throw FormatError("mainmenu BSP range instance definition is unavailable");
            const float coverage=source_shader_instance_coverage(instance.center,instance.radius,camera);
            const auto alpha=source_shader_instance_alpha(coverage);
            if(!alpha){result.zero_alpha_instances.push_back(record.id);continue;}
            MainmenuBspRangeUnit unit;unit.geometry=-2-int(record.id);unit.section=unsigned(instance.definition);
            unit.flags=collector_flags(record.flags,frame.fog)|0x1008u|((record.flags&0x800u)?1u:0x40u);
            unit.coverage=source_shader_coverage_byte(coverage);unit.alpha=alpha;unit.transform=transforms[record.id];
            schedule(std::move(unit),true,authored.definitions[unsigned(instance.definition)].geometry_count!=0);
            units.back().words[6]=std::uint32_t(record.association)|(std::uint32_t(alpha)<<16);
        }
        const SourceBspFrustumCullContext context{unsigned(units.size()),unsigned(units.size()),0,0,arena_identity};
        source_bsp_frustum_cull(units,banks,result.arena,context,frame.visibility,frame.admission.associations);
        for(unsigned i=0;i<units.size();++i) {
            auto &unit=result.units[i];unit.flags=units[i].words[0];const auto &g=*banks[i].geometry;
            std::span<const std::uint32_t> mask;
            if(unit.mask_word_offset)mask=std::span<const std::uint32_t>(result.arena.words).subspan(
                *unit.mask_word_offset,(unsigned(g.subpart_count)+31)/32);
            const bool filtered=(unit.flags&0x40u) && units[i].words[5] && g.subpart_count!=0;
            for(unsigned part=0;part<g.parts.size();++part) {
                const Bytes p(g.parts[part]);
                //4D5E7/4D5F3: these ordinary BSP collectors never set0200.
                //Retain the literal predicate rather than infer from counts.
                const auto topology=(unit.flags&0x200u) && !(p.u16(2)&8u)?
                    MenuWorldSourceTopology::triangle_strip:MenuWorldSourceTopology::triangle_list;
                unit.parts.push_back({part,topology,source_bsp_part_ranges(g,part,topology,filtered,mask)});
            }
        }
        result.complete=true;retained_arena=result.arena;return result;
    }
};
MainmenuBspRangeProducer::MainmenuBspRangeProducer(const Cache &cache,std::size_t bsp)
    :impl_(std::make_unique<Impl>(cache,bsp)) {}
MainmenuBspRangeProducer::~MainmenuBspRangeProducer()=default;
MainmenuBspRangeFrame MainmenuBspRangeProducer::prepare(const MainmenuBspAdmissionFrame &frame) {
    try{return impl_->prepare(frame);}
    catch(const FormatError &error){MainmenuBspRangeFrame failed;failed.diagnostic=error.what();return failed;}
}
const MainmenuBspRangeUnit *MainmenuBspRangeFrame::unit_for_geometry(int geometry) const {
    const auto found=std::find_if(units.begin(),units.end(),[&](const auto &unit){return unit.geometry==geometry;});
    return found==units.end()?nullptr:&*found;
}
} // namespace halo2

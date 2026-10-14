#include "halo2/source_map_effects.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/model_selection.hpp"
#include <map>
#include <set>

namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
Bytes block(const Cache &cache,const SourceTagMetadata &,const Bytes &root,std::size_t offset,std::size_t stride) {
    const auto descriptor=read_tag_block_descriptor(root,offset);
    const auto extent=descriptor.extent(stride,65536);
    if(!descriptor.count)return Bytes(std::span<const std::byte>{});
    //Retail alias rows can point into another retained tag packet. Validate
    //the actual containing owner rather than requiring the requesting alias
    //to own storage preceding its root.
    for(const auto &owner:cache.select_tags({.contains=TagAddressRange{descriptor.address,extent},.require_address=true})) {
        (void)cache.tag_payload(owner.handle());
        return Bytes(cache.read_tag_bytes(descriptor.address,extent));
    }
    throw FormatError("Map effect block has no retained metadata owner");
}
}
struct SourceMapEffects::Impl {
    struct Attachment {std::uint32_t effect{};std::string marker;bool prepared{};};
    struct Placement {
        std::vector<Attachment> attachments;
        std::map<std::string,std::vector<ObjectAttachmentMarker>> markers;
    };
    const Cache &cache;NativeWorldEffects &effects;
    std::map<std::size_t,Placement> placements;
    struct Live {std::size_t placement{};std::uint64_t birth{};};
    std::map<std::uint32_t,Live> live;
    SourceMapEffectStats counts;std::set<std::string> gaps;
    Impl(const Cache &c,std::span<const ScenarioObject> objects,NativeWorldEffects &e):cache(c),effects(e) {
        for(std::size_t index=0;index<objects.size();++index) {
            const auto &object=objects[index];
            if(object.definition==none || (object.type!=6 && object.type!=7 && object.type!=11))continue;
            const auto metadata=c.load_source_tag_metadata(object.definition);
            const Bytes root(metadata.bytes_at(metadata.source_address(),0x9c));
            const auto attachments=block(c,metadata,root,0x94,0x18);
            if(!root.u32(0x94))continue;
            std::optional<SourceTagMetadata> model;
            auto &placement=placements[index];
            std::set<std::uint32_t> names;
            for(unsigned n=0;n<root.u32(0x94);++n) {
                const auto at=n*0x18;
                if(attachments.u32(at)!=0x65666665u || attachments.u32(at+4)==none)continue;
                ++counts.attachments;
                Attachment attachment;attachment.effect=attachments.u32(at+4);
                const auto marker=attachments.u32(at+8);attachment.marker=marker?c.string_id(marker):std::string{};
                const auto effect=c.load_source_tag_metadata(attachment.effect);
                const auto info=c.query_tag({.handle=TagHandle{attachment.effect}});
                const std::string name=info?info->name:std::to_string(attachment.effect);
                try {
                    const auto initial=c.load_initial_model_definition(object.definition);
                    if(initial.render_model==none)throw FormatError("Map attachment requires current object WORLD nodes");
                    if(!model)model=c.load_source_tag_metadata(initial.render_model);
                    const Bytes definition(effect.bytes_at(effect.source_address(),0x1c));
                    const auto locations=block(c,effect,definition,0xc,4);
                    for(unsigned j=0;j<definition.u32(0xc);++j) {
                        auto location=locations.u32(j*4);
                        if(location==0x0700054eu)location=marker; //178360's literal marker alias.
                        if(location)names.insert(location);
                    }
                    if(attachments.u32(at+0x10) || attachments.u32(at+0x14))
                        throw FormatError("Map attachment requires current original object-function values");
                    //Visual playback cannot implement a damage/projectile part by
                    //treating it as a particle. Preserve that gameplay boundary.
                    const auto events=block(c,effect,definition,0x14,0x38);
                    for(unsigned j=0;j<definition.u32(0x14);++j) {
                        const auto parts=block(c,effect,events,j*0x38+0x18,0x38);
                        for(unsigned k=0;k<events.u32(j*0x38+0x18);++k) {
                            const auto group=parts.u32(k*0x38+8);
                            if(parts.u32(k*0x38+0x10)!=none && group!=0x736e6421u && group!=0x6c696768u && group!=0x64656361u)
                                throw FormatError("Map attachment requires its original nonvisual part owner");
                        }
                    }
                    e.preload_attached(attachment.effect);attachment.prepared=true;++counts.prepared;
                }catch(const FormatError &error){gaps.insert(name+": "+error.what());}
                placement.attachments.push_back(std::move(attachment));
            }
            if(model)for(const auto id:names)placement.markers.emplace(c.string_id(id),source_model_markers(*model,id));
            if(placement.attachments.empty())placements.erase(index);
        }
    }
};
SourceMapEffects::SourceMapEffects(const Cache &c,std::span<const ScenarioObject> objects,NativeWorldEffects &e)
    :impl_(std::make_unique<Impl>(c,objects,e)){}
SourceMapEffects::~SourceMapEffects()=default;
void SourceMapEffects::synchronize(std::span<const SourceSceneEffectOwner> owners,std::uint64_t tick) {
    auto &i=*impl_;std::set<std::uint32_t> present;
    for(const auto &owner:owners) {
        const auto at=i.placements.find(owner.placement);if(at==i.placements.end())continue;
        if(owner.object==none || owner.created_tick>tick || owner.nodes.empty())
            throw FormatError("Map effect owner lacks its current full lifetime/pose");
        if(!present.insert(owner.object).second)throw FormatError("Duplicate current map effect owner");
        NativeWorldEffects::ObjectMarkers markers;
        for(const auto &[name,bank]:at->second.markers) {
            auto posed=pose_model_markers(bank,owner.nodes,owner.permutations);
            //178360 grants only the literal root a missing-marker fallback.
            if(posed.empty() && name=="root")posed.push_back({0,{},owner.nodes.front(),1});
            for(const auto &p:posed)markers[name].push_back({p.world.position,p.world.basis[2],p.world.basis[0],p.world.basis[0],p.world.basis[2],false,0,1});
        }
        i.effects.object_markers(owner.object,std::move(markers));
        const auto existing=i.live.find(owner.object);
        if(existing!=i.live.end()) {
            if(existing->second.placement!=owner.placement || existing->second.birth!=owner.created_tick)
                throw FormatError("Map effect full generation changed its placement/birth");
            continue;
        }
        i.live.emplace(owner.object,Impl::Live{owner.placement,owner.created_tick});
        const auto scales=source_map_effect_unnamed_functions();
        for(const auto &attachment:at->second.attachments)if(attachment.prepared) {
            i.effects.spawn_looping_attached(attachment.effect,owner.object,owner.created_tick,
                attachment.marker,scales[0],scales[1]);++i.counts.spawned;
        }
    }
    for(auto at=i.live.begin();at!=i.live.end();) {
        if(!present.contains(at->first)){i.effects.remove_object(at->first);at=i.live.erase(at);}
        else ++at;
    }
}
SourceMapEffectStats SourceMapEffects::stats()const {
    const auto &i=*impl_;auto out=i.counts;out.live_objects=i.live.size();
    out.unavailable.assign(i.gaps.begin(),i.gaps.end());return out;
}
} // namespace halo2

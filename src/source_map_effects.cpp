#include "halo2/source_map_effects.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_effect_seed_location.hpp"
#include <algorithm>
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
    struct Attachment {
        std::uint32_t effect{},function_a{},function_b{};
        std::uint16_t slot{};std::string marker;bool prepared{},partial{},source_damage_only{};
    };
    struct Placement {
        std::uint32_t definition{};
        std::vector<Attachment> attachments;
        std::map<std::string,std::vector<ObjectAttachmentMarker>> markers;
    };
    const Cache &cache;NativeWorldEffects &effects;
    std::map<std::size_t,Placement> placements;
    struct Live {
        std::size_t placement{};std::uint64_t birth{};
        std::array<Transform,2> seed_locations;
        std::map<std::uint16_t,std::uint32_t> instances;
    };
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
            placement.definition=object.definition;
            std::set<std::uint32_t> names;
            for(unsigned n=0;n<root.u32(0x94);++n) {
                const auto at=n*0x18;
                if(attachments.u32(at)!=0x65666665u || attachments.u32(at+4)==none)continue;
                ++counts.attachments;
                Attachment attachment;attachment.effect=attachments.u32(at+4);
                attachment.slot=std::uint16_t(n);
                attachment.function_a=attachments.u32(at+0x10);attachment.function_b=attachments.u32(at+0x14);
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
                        if(location==0x0700054eu)location=marker; //17B2EC's literal replace alias.
                        if(location)names.insert(location);
                    }
                    // Pure jpt! graphs already execute through the actual
                    //17D730 ->D91D0 world owner. Bind that completed allocation
                    //without a second native event clock, RNG or damage owner.
                    const auto events=block(c,effect,definition,0x14,0x38);
                    bool damage_parts{},visual_parts{},particle_systems{},partial_lens{};
                    for(unsigned j=0;j<definition.u32(0x14);++j) {
                        particle_systems=particle_systems || events.u32(j*0x38+0x30)!=0;
                        const auto parts=block(c,effect,events,j*0x38+0x18,0x38);
                        for(unsigned k=0;k<events.u32(j*0x38+0x18);++k) {
                            const auto group=parts.u32(k*0x38+8);
                            if(parts.u32(k*0x38+0x10)==none)continue;
                            if(group==0x6a707421u)damage_parts=true;
                            else if(group==0x736e6421u || group==0x6c696768u || group==0x64656361u)visual_parts=true;
                            else if(group==0x6c656e73u)partial_lens=true;
                            else
                                throw FormatError("Map attachment requires its original nonvisual part owner");
                        }
                    }
                    if(damage_parts && (visual_parts || particle_systems || partial_lens))
                        throw FormatError("Mixed map damage effect requires its completed original part/presentation ownership");
                    if(partial_lens && !visual_parts && !particle_systems)
                        throw FormatError("Map attachment lens part has no supported visual components");
                    attachment.source_damage_only=damage_parts;
                    if(!damage_parts)e.preload_attached(attachment.effect);
                    attachment.prepared=!partial_lens;attachment.partial=partial_lens;
                    if(attachment.prepared)++counts.prepared;
                    else gaps.insert(name+": Lens part requires its original render owner; supported authored components are prepared separately");
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
void SourceMapEffects::synchronize(std::span<const SourceSceneEffectOwner> owners,std::uint64_t tick,
    std::span<const SourceSceneRoot> source_roots) {
    auto &i=*impl_;std::set<std::uint32_t> present;
    std::map<std::uint32_t,const SourceSceneRoot*> source;
    for(const auto &root:source_roots) {
        validate_source_attached_effects(root.attached_effects);
        if(!source.emplace(root.object,&root).second)throw FormatError("Duplicate source attachment owner");
    }
    for(const auto &owner:owners) {
        const auto at=i.placements.find(owner.placement);if(at==i.placements.end())continue;
        if(owner.object==none || owner.created_tick>tick || owner.nodes.empty())
            throw FormatError("Map effect owner lacks its current full lifetime/pose");
        if(!present.insert(owner.object).second)throw FormatError("Duplicate current map effect owner");
        auto existing=i.live.find(owner.object);
        const bool created=existing==i.live.end();
        if(created) {
            if(!owner.origin)throw FormatError("Map attachment seed requires its actual object world origin");
            // Actual C0E2E ->179540 ->1791C0 constructs gravity/up seed
            // markers.17B580 retains their node-local transforms at birth.
            existing=i.live.emplace(owner.object,Impl::Live{owner.placement,owner.created_tick,{
                source_effect_seed_location(&owner.nodes.front(),*owner.origin,{0,0,-1}),
                source_effect_seed_location(&owner.nodes.front(),*owner.origin,{0,0,1})}}).first;
        }else if(existing->second.placement!=owner.placement || existing->second.birth!=owner.created_tick)
            throw FormatError("Map effect full generation changed its placement/birth");
        NativeWorldEffects::ObjectMarkers markers;
        for(const auto &[name,bank]:at->second.markers) {
            auto posed=pose_model_markers(bank,owner.nodes,owner.permutations);
            //178360 grants only the literal root a missing-marker fallback.
            if(posed.empty() && name=="root")posed.push_back({0,{},owner.nodes.front(),1});
            for(const auto &p:posed)markers[name].push_back({p.world.position,p.world.basis[2],p.world.basis[0],p.world.basis[0],p.world.basis[2],false,0,1});
        }
        for(unsigned n=0;n<2;++n) {
            const auto world=compose_transform(owner.nodes.front(),existing->second.seed_locations[n]);
            //178360 gives the explicit constructor packet priority over the
            //render-model marker bank. Arbitrary missing names stay missing.
            markers[i.cache.string_id(n?0x020000cau:0x070000c0u)]={
                {world.position,world.basis[2],world.basis[0],world.basis[0],world.basis[2],false,0,1}};
        }
        i.effects.object_markers(owner.object,std::move(markers));
        const auto scales=source_map_effect_unnamed_functions();
        for(const auto &attachment:at->second.attachments)if(attachment.prepared || attachment.partial) {
            if(attachment.function_a || attachment.function_b || attachment.source_damage_only) {
                const auto current=source.find(owner.object);
                if(current==source.end()) {
                    i.gaps.insert("Source-owned map attachment requires its completed source instance/controls");
                    continue;
                }
                const auto &root=*current->second;
                if(root.placement!=owner.placement || root.definition!=at->second.definition || root.created_tick!=owner.created_tick)
                    throw FormatError("Source attachment owner differs from its current scene generation");
                const auto row=std::find_if(root.attached_effects.begin(),root.attached_effects.end(),
                    [&](const auto &e){return e.slot==attachment.slot;});
                if(row==root.attached_effects.end())throw FormatError("Source attachment snapshot omitted an authored effect slot");
                if(row->instance!=none && row->definition!=attachment.effect)
                    throw FormatError("Source attachment allocation differs from its authored effect slot");
                NativeWorldEffects::ObjectMarkers locations;
                for(const auto &location:row->locations) {
                    Transform world=location.local;
                    if(location.node!=-1) {
                        if(location.node<0 || unsigned(location.node)>=owner.nodes.size())
                            throw FormatError("Source map attachment location lacks its current WORLD node");
                        world=compose_transform(owner.nodes[unsigned(location.node)],location.local);
                    }
                    locations[i.cache.string_id(location.name)].push_back(
                        {world.position,world.basis[2],world.basis[0],world.basis[0],world.basis[2],false,row->scale_a,row->scale_b});
                }
                auto [prior,inserted]=existing->second.instances.try_emplace(attachment.slot,none);
                if(prior->second!=row->instance) {
                    if(prior->second!=none && !attachment.source_damage_only)
                        i.effects.update_looping_attached(owner.object,prior->second,SourceAttachedEffect{});
                    prior->second=row->instance;
                    if(row->instance!=none) {
                        if(!attachment.source_damage_only)
                            i.effects.spawn_looping_attached(attachment.effect,owner.object,tick,
                                attachment.marker,row->scale_a,row->scale_b,&*row,&locations);
                        ++i.counts.spawned;
                    }
                }else if(row->instance!=none && !attachment.source_damage_only &&
                    !i.effects.update_looping_attached(owner.object,row->instance,*row,locations) && !(row->flags&0x40u))
                    throw FormatError("Current source attachment lost its native presentation lifetime");
            }else if(created) {
                i.effects.spawn_looping_attached(attachment.effect,owner.object,owner.created_tick,
                    attachment.marker,scales[0],scales[1]);++i.counts.spawned;
            }
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

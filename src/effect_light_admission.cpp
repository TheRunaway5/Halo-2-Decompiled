#include "halo2/effect_light_admission.hpp"
#include <bit>
#include <algorithm>

namespace halo2 {
namespace {
bool local_allowed(const EffectLightDefinition& definition,
    const SourceEffectLightRuntime& record,const SourceEffectLightRenderContext& context) {
    std::uint8_t local=0;
    if(record.attached_node!=-1) {
        if(!context.local_priority)throw FormatError("Effect light requires original local-owner priority");
        local=context.local_priority(record.owner);
    }
    return !(((definition.flags&0x20) && !local) || ((definition.flags&0x40) && local));
}
}
bool source_effect_light_live(const SourceEffectLightRuntime& record) {
    constexpr float epsilon=std::bit_cast<float>(0x38d1b717u);
    constexpr float minimum_energy=std::bit_cast<float>(0x3d4ccccdu);
    if(record.identity==0xffffffffu || !(record.flags&2))return false;
    // C3140 uses ordered SSE comparisons; NaNs fail these positive tests.
    if(!(record.spatial_radius>epsilon) || !(record.overall_fade>epsilon))return false;
    const auto energy=[](Vec3 v){return (v.x*v.x+v.y*v.y)+v.z*v.z;};
    if(!(energy(record.state.diffuse)>minimum_energy) &&
       !(energy(record.state.specular)>minimum_energy))return false;
    return true;
}
SourceEffectLightAdmission source_effect_light_render_admission(
    const EffectLightDefinition& definition,const SourceEffectLightRuntime& record,
    const SourceEffectLightRenderContext& context) {
    constexpr float epsilon=std::bit_cast<float>(0x38d1b717u);
    SourceEffectLightAdmission result;
    if(!source_effect_light_live(record))return result;

    const auto flags=definition.flags;
    if(!local_allowed(definition,record,context))return result;
    const bool debug_allowed=(flags&0x100) || !context.debug_gate || !(context.debug_flags&2);
    if(context.renderer_enabled && debug_allowed) {
        if(record.animation_index!=-1)
            throw FormatError("Effect light requires original runtime animation shape owner");
        result.sphere=source_effect_light_sphere(definition,record.position,record.forward,
            record.up,record.state.size);
        result.eligible=result.sphere->radius>epsilon;
    }
    if((flags&0x400) && context.view_type!=2)result.eligible=false;
    if(record.cluster==-1 && (!(flags&0x20) || !(flags&0x40000)))result.eligible=false;
    return result;
}
std::vector<SourceShadowCandidate> source_effect_light_candidates(
    std::span<const SourceEffectLightCandidateSource> sources,
    std::span<const std::uint32_t> visible_queue,
    const SourceEffectLightCandidateContext& context) {
    if(visible_queue.size()>32767)throw FormatError("Effect light visible queue exceeds signed source count");
    std::vector<SourceShadowCandidate> result;
    for(const auto identity:visible_queue) {
        if(identity==0xffffffffu)continue;
        const SourceEffectLightCandidateSource* source=nullptr;
        for(const auto& entry:sources)if(entry.record.identity==identity) {
            if(source)throw FormatError("Effect light candidate has duplicate runtime identity");
            source=&entry;
        }
        if(!source || !source->definition)throw FormatError("Effect light candidate requires original runtime/tag bank");
        if(!source_effect_light_render_admission(*source->definition,source->record,context.admission).eligible ||
            result.size()>=128)continue;
        const auto& record=source->record;
        const auto flags=source->definition->flags;
        SourceShadowCandidate candidate;
        candidate.identity=identity;candidate.owner=-1;
        if(record.owner!=0xffffffffu) {
            if(!context.root_owner || !context.owner_kind || !context.admission.local_priority)
                throw FormatError("Effect light candidate requires original owner services");
            candidate.owner=std::bit_cast<std::int32_t>(context.root_owner(record.owner));
        }
        candidate.requested_fade=record.overall_fade;
        candidate.coverage=source_shadow_coverage(record.spatial_center,record.spatial_radius,
            context.depth_plane,context.pixel_scale);
        candidate.flags=((flags>>5)&1u)<<8 | ((flags>>20)&1u)<<24;
        if(record.owner!=0xffffffffu) {
            candidate.flags|=std::uint32_t(context.owner_kind(record.owner)==2)<<16;
            candidate.flags|=context.admission.local_priority(std::bit_cast<std::uint32_t>(candidate.owner));
        }
        result.push_back(candidate);
    }
    return result;
}
bool source_effect_light_visibility_visit(const SourceEffectLightRuntime& record,
    std::uint32_t& last_seen_epoch,SourceEffectLightVisibleQueue& queue,
    const SourceEffectLightVisibilityContext& context,std::uint32_t cluster_entry,
    std::uint8_t force) {
    if(!source_effect_light_live(record) || last_seen_epoch==context.epoch)return false;
    if(!force) {
        bool visible=true;
        if(context.query_mode==0) {
            if(!context.frustums)throw FormatError("Effect light visibility requires original frustum bank");
            visible=source_cluster_frustum_visibility(*context.frustums,cluster_entry,
                record.spatial_center,record.spatial_radius).visible;
        } else if(context.query_mode==1) {
            const float x=record.spatial_center.x-context.sphere_center.x,
                z=record.spatial_center.z-context.sphere_center.z,
                y=record.spatial_center.y-context.sphere_center.y;
            const float radius=context.sphere_radius+record.spatial_radius;
            visible=radius*radius>=(x*x+z*z)+y*y;
        }
        if(!visible && context.rejection_override>=0)return false;
    }
    if(queue.capacity<1 || queue.capacity>32768 || queue.count>queue.storage.size() ||
        queue.storage.size()<queue.capacity || queue.count>=queue.capacity)
        throw FormatError("Effect light visible queue requires original storage/count");
    if(queue.count>=queue.capacity-1)return false;
    queue.storage[queue.count++]={record.identity,0,0xffff,0};
    last_seen_epoch=context.epoch;
    return true;
}
void source_effect_light_collect_visible(
    std::span<SourceEffectLightVisibilitySource> sources,
    std::span<const std::vector<std::uint32_t>> cluster_lists,
    SourceEffectLightVisibleQueue& queue,const SourceEffectLightCollectionContext& context) {
    if(context.pass_mode || (context.flags&0x10))return;
    const auto* frustums=context.visibility.frustums;
    if(!frustums)throw FormatError("Effect light collection requires constructed cluster entries");
    const auto find=[&](std::uint32_t identity)->SourceEffectLightVisibilitySource& {
        const auto it=std::find_if(sources.begin(),sources.end(),[&](const auto& s){return s.record.identity==identity;});
        if(it==sources.end())throw FormatError("Effect light spatial list has no live datum owner");
        return *it;
    };
    auto visibility=context.visibility;
    visibility.rejection_override=std::int8_t(context.flags);
    for(std::size_t ordinal=0;ordinal<frustums->clusters.size();++ordinal) {
        const auto cluster=frustums->clusters[ordinal].source_cluster;
        if(cluster<0 || std::size_t(cluster)>=cluster_lists.size())
            throw FormatError("Effect light collection cluster exceeds actual spatial heads");
        for(const auto identity:cluster_lists[cluster]) {
            if(identity==0xffffffffu)break; // Source owner NONE terminates the walk.
            auto& source=find(identity);
            source_effect_light_visibility_visit(source.record,source.last_seen_epoch,
                queue,visibility,std::uint32_t(ordinal),0);
        }
    }
    if(context.containing_cluster==-1 || context.local_player==0xffffffffu)return;
    std::array<SourceEffectLightVisibilitySource*,4> fallback{};unsigned count=0;
    std::uint32_t previous_index=0;bool first=true;
    for(auto& source:sources) {
        const auto index=source.record.identity&0xffffu;
        if(source.record.identity==0xffffffffu || (!first && index<=previous_index))
            throw FormatError("Effect light fallback requires actual occupied pool iteration order");
        first=false;previous_index=index;
        if(!source.definition)continue;
        // C3950 still consumes31520 for every occupied tagged record after
        // reaching four. Cluster/liveness checks happen only after this scan.
        if(local_allowed(*source.definition,source.record,context.owners) &&
            (source.definition->flags&0x20) && (source.definition->flags&0x40000) && count<4)
            fallback[count++]=&source;
    }
    for(unsigned i=0;i<count;++i) {
        auto& source=*fallback[i];
        if(source.record.cluster==-1)
            source_effect_light_visibility_visit(source.record,source.last_seen_epoch,
                queue,visibility,0xffffffffu,1);
    }
}
}

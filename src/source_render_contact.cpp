#include "halo2/source_render_contact.hpp"
#include "halo2/math.hpp"
#include <bit>

namespace halo2 {
namespace {
float scalar(std::uint32_t word){return std::bit_cast<float>(word);}
std::uint32_t bits(float value){return std::bit_cast<std::uint32_t>(value);}
PosedCollisionHit object_hit(const SourceVectorContactPacket& p){
    return {scalar(p[1]),{{scalar(p[10]),scalar(p[11]),scalar(p[12])},scalar(p[13])},p[18],
        std::int32_t(p[19]),std::int32_t(p[20]),std::int32_t(p[21]),
        std::bit_cast<std::int16_t>(std::uint16_t(p[17])),std::bit_cast<std::int16_t>(std::uint16_t(p[17]>>16)),
        std::bit_cast<std::int16_t>(std::uint16_t(p[22]>>16)),std::bit_cast<std::int16_t>(std::uint16_t(p[9])),
        std::uint8_t(p[22]),std::uint8_t(p[22]>>8)};
}
bool map_attribute(const BspSurfaceLighting& bank,const SourceVectorContactPacket& p,
    std::span<const std::uint8_t> clusters,std::span<const std::uint8_t> definitions,
    SourceRenderContactPacket& out) {
    const auto instance=std::int32_t(p[15]);std::int32_t record=0;
    if(p[0]==1) {
        if(bank.collision.empty() || p[19]>=bank.collision[0].leaves.size())
            throw FormatError("source map attribution leaf bank missing");
        const auto cluster=bank.collision[0].leaves[p[19]].cluster;
        if(cluster<0 || std::size_t(cluster)>=clusters.size())
            throw FormatError("source map attribution cluster residency missing");
        out[6]=0;
        if(clusters[cluster]>1)throw FormatError("source map attribution residency scheduling unresolved");
        if(!clusters[cluster])return false;
    }else if(p[0]==3) {
        if(instance<0 || std::size_t(instance)>=bank.instances.size())
            throw FormatError("source map attribution instance missing");
        record=bank.instances[instance].definition;
        if(std::size_t(record)>=bank.definition_collision.size())
            throw FormatError("source map attribution definition leaf bank missing");
        if(bank.definition_collision[record].leaves.empty())return false; //14AC60+B8 guard.
        if(std::size_t(record)>=definitions.size())throw FormatError("source map attribution definition residency missing");
        out[6]=0;
        if(definitions[record]>1)throw FormatError("source instance attribution residency scheduling unresolved");
        if(!definitions[record])return false;
    }else throw FormatError("source14AC60 unsupported contact type");
    const CollisionSurfaceIdentity identity{bank.bsp,record,std::int32_t(p[20]),instance,std::int32_t(p[21])};
    const auto hit=bank.attribute(identity,{scalar(p[2]),scalar(p[3]),scalar(p[4])},p[19]);
    if(!hit)return false;
    out[0]=std::uint32_t(hit->cluster);out[1]=std::uint32_t(instance);out[6]=0;
    out[8]=std::uint32_t(hit->part);out[9]=hit->strip;out[10]=hit->lightmap_triangle;
    out[12]=bits(hit->weight_b);out[13]=bits(hit->weight_c);return true;
}
bool object_attribute(const SourceRenderContactOwner& owner,const SourceVectorContactPacket& p,
    SourceRenderContactPacket& out) {
    if(!owner.render)throw FormatError("source object attribution resident model bank missing");
    const auto& model=*owner.render;const auto hit=object_hit(p);
    for(auto field:{2u,3u,4u,5u,6u,8u,9u,10u})out[field]=0xffffffffu;
    if(hit.region<0 || std::size_t(hit.region)>=owner.active_permutations.size() ||
        std::size_t(hit.region)>=model.highest_sections.size())
        throw FormatError("source object attribution actual active region missing");
    const auto permutation=owner.active_permutations[hit.region];
    if(permutation==255)out[5]=255;
    else {
        if(permutation>=128 || permutation>=model.highest_sections[hit.region].size())
            throw FormatError("source object attribution actual active permutation missing");
        out[5]=std::uint32_t(std::int32_t(model.highest_sections[hit.region][permutation]));
    }
    const SourceObjectLightAttributionInput input{p[16],owner.active_permutations,owner.world_nodes,
        owner.collision,hit,{scalar(p[2]),scalar(p[3]),scalar(p[4])}};
    const auto result=source_object_light_attribution(model,input);
    if(!result)return false;
    out[2]=result->owner;out[3]=result->render_model;out[5]=result->section;
    out[7]=std::uint32_t(result->region);out[8]=result->part;out[9]=result->strip;
    out[10]=result->lightmap_triangle;out[11]=result->node_hint;
    out[12]=bits(result->weight_b);out[13]=bits(result->weight_c);return true;
}
}
SourceRenderContactResult source_render_contact(const SourceVectorContactBank& bank,
    const SourceObjectSpatialPool& pool,std::span<const SourceObjectVectorOwner> vector_owners,
    std::span<const SourceRenderContactOwner> render_owners,
    std::span<const std::uint8_t> clusters,std::span<const std::uint8_t> definitions,
    const SourceObjectVectorTarget& target,const SourceRenderContactInput& input) {
    SourceRenderContactResult out{false,input.origin,input.caller_packet,{}};
    if(!input.bank_valid)return out;
    out.packet[0]=out.packet[1]=out.packet[2]=0xffffffffu;
    auto collision=input.collision_packet;collision[9]=(collision[9]&0xffff0000u)|65535u;
    std::uint32_t flags=input.initial_query?0x4480000du:0x04800c0du;
    const auto query=[&](std::uint32_t query_flags,std::uint32_t excluded) {
        const SourceObjectVectorInput q{query_flags,0,excluded,0xffffffffu,out.origin,input.delta};
        auto result=source_vector_contact(bank,pool,vector_owners,q,target,collision);
        collision=result.packet;const auto hit=result.hit;
        out.attempts.push_back({query_flags,excluded,out.origin,std::move(result)});return hit;
    };
    for(unsigned attempt=0;attempt<3;++attempt) {
        if(!query(flags,input.excluded))return out;
        if(collision[16]==0xffffffffu)out.hit=map_attribute(bank.bsp,collision,clusters,definitions,out.packet);
        else {
            const SourceRenderContactOwner* owner{};
            for(const auto& candidate:render_owners)if(std::uint16_t(candidate.full_handle)==std::uint16_t(collision[16])) {
                if(owner)throw FormatError("duplicate source render-contact owner slot");owner=&candidate;
            }
            if(!owner)throw FormatError("source render-contact owner metadata missing");
            const auto& def=owner->definition;
            const auto action=source_object_light_contact_action(owner->current_type,def.scenery_policy,def.model_attribution_available());
            if(action==ObjectLightContactAction::attribute_model) {
                out.packet[3]=def.render_model;out.packet[4]=def.collision;
                out.hit=object_attribute(*owner,collision,out.packet);
            }else if(action==ObjectLightContactAction::fallback_map) {
                //BF9A0 writes these even when its PRT/leaf count admission fails.
                if(def.model!=0xffffffffu && def.render_model!=0xffffffffu && def.collision!=0xffffffffu) {
                    out.packet[3]=def.render_model;out.packet[4]=def.collision;
                }
                flags=0x04800005u;
                if(query(flags,0xffffffffu))out.hit=map_attribute(bank.bsp,collision,clusters,definitions,out.packet);
            }
        }
        if(out.hit)return out; // Before the authored double-sided test.
        if(!(collision[22]&1) || attempt==2)return out;
        out.origin=out.origin+input.delta*std::bit_cast<float>(0x3ca3d70au); //45DF0C=.02.
    }
    return out;
}
}

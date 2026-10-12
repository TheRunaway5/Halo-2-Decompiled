#include "halo2/source_object_vector_contact.hpp"
#include <array>

namespace halo2 {
bool source_object_vector_child_admitted(std::uint32_t requested,
    const SourceObjectVectorOwner& owner,const SourceObjectVectorInput& in) {
    if(requested==in.excluded_a || requested==in.excluded_b)return false;
    if((owner.header_flags&0x10) || (owner.flags&1))return false;
    if(!(in.flags&(1u<<((owner.header_type+4u)&31u))))return false;
    if((in.flags&0x80000) && (owner.flags&0x100000))return false;
    if((in.flags&0x40000) && !(owner.flags&0x400000))return false;
    if((in.flags&0x40000000) && !(owner.flags&0x80000000))return false;
    if(owner.header_type==0) {
        if((in.flags&0x100000) && (owner.unit_flags_10a&4))return false;
        if((in.flags&0x400000) && (owner.biped_flags_348&8))return false;
    } else if(owner.header_type==11 && (in.flags&0x200000) && (owner.prop_flags_12c&2))return false;
    return true;
}
SourceObjectVectorResult source_object_vector_contact(
    std::span<const SourceObjectVectorOwner> owners,std::uint32_t first,
    bool first_admitted,const SourceObjectVectorInput& in,float maximum,
    const SourceObjectVectorTarget& target) {
    if(!target)throw FormatError("source posed object target service missing");
    std::array<const SourceObjectVectorOwner*,source_object_vector_slot_capacity> slots{};
    for(const auto& owner:owners) {
        if(owner.full_handle==0xffffffffu)throw FormatError("NONE source object owner");
        const auto index=owner.full_handle&65535u;
        if(index>=slots.size())throw FormatError("source object slot outside original capacity");
        auto& slot=slots[index];
        if(slot)throw FormatError("duplicate source object slot");
        slot=&owner;
    }
    SourceObjectVectorResult result;std::array<bool,source_object_vector_slot_capacity> visiting{};
    const bool first_hit=(in.flags&0x10000000u)!=0;
    const auto walk=[&](auto&& self,std::uint32_t requested,bool admitted,unsigned depth)->bool {
        if(depth>owners.size())throw FormatError("cyclic source object child chain");
        std::vector<std::uint16_t> chain;
        bool changed=false;
        while(requested!=0xffffffffu) {
            const auto index=std::uint16_t(requested);
            if(index>=slots.size())throw FormatError("source requested object slot outside original capacity");
            const auto owner=slots[index];
            if(!owner)throw FormatError("source contact owner slot missing");
            if(visiting[index])throw FormatError("cyclic source object sibling/child chain");
            visiting[index]=true;chain.push_back(index);
            if(admitted || (source_object_vector_child_admitted(requested,*owner,in) &&
                source_object_light_segment_intersects(in.origin,in.delta,owner->primitive_sphere.center,owner->primitive_sphere.radius))) {
                result.targets.push_back(requested);
                if(auto hit=target(requested,*owner,in);hit && hit->fraction<maximum) {
                    maximum=hit->fraction;result.contact=SourceObjectVectorContact{requested,*hit};
                    changed=true;
                    if(first_hit){for(auto i:chain)visiting[i]=false;return true;}
                }
                if(!(in.flags&0x20000u) && owner->child_head!=0xffffffffu &&
                    self(self,owner->child_head,false,depth+1)) {
                    changed=true;
                    if(first_hit){for(auto i:chain)visiting[i]=false;return true;}
                }
            }
            requested=owner->sibling;
        }
        for(auto i:chain)visiting[i]=false;
        return changed;
    };
    walk(walk,first,first_admitted,0);return result;
}
}

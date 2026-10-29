#include "halo2/native_campaign_ai_clump_retirement_tu15.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include <array>
#include <bit>
#include <sstream>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t p,unsigned n=4) {
    std::array<std::byte,4> b{};m.read(p,std::span(b).first(n));std::uint32_t v{};
    for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8u);
    return v;
}
void write(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t v,unsigned n=4) {
    std::array<std::byte,4> b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(i*8u));
    m.write(p,std::span(b).first(n));
}
std::uint32_t row(std::uint32_t rows,std::uint32_t datum,std::uint32_t stride) {
    return rows+(datum&0xffffu)*stride;
}
std::int16_t signed16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}

// Actual299AA0 attachment traversal; clear only references matching the
// removed perception handle in the unit's authored AI attachment extension.
void invalidate_attachments(SourcePhysicsContactMemory &m,std::uint32_t attachment,
        std::uint32_t reference) {
    const auto pool=read(m,0x510fa8u),rows=read(m,pool+0x44u);
    auto unit=read(m,row(rows,attachment,0x34u)+0x18u);
    if(unit==none)return;
    const auto objects=read(m,0x4ecbd0u);
    do {
        const auto current_rows=read(m,objects+0x44u);
        const auto body=read(m,row(current_rows,unit,12u)+8u);
        if(read(m,body+0x134u)!=0)return;
        const auto extension=body+std::uint32_t(std::int32_t(signed16(read(m,body+0x13au,2))));
        if(extension==0)return;
        if(read(m,extension+0x10u)==reference)write(m,extension+0x10u,none);
        if(read(m,extension+0x50u)==reference)write(m,extension+0x50u,none);
        unit=read(m,extension+0xcu);
    } while(unit!=none);
}

// Actual1EA400 reference invalidation before its real descriptor+28 callback.
void invalidate_actor(SourcePhysicsContactMemory &m,std::uint32_t actor,
        std::uint32_t reference,const NativeCampaignAiReferenceInvalidationServiceTu15 &service) {
    const auto pool=read(m,0x5020c8u),rows=read(m,pool+0x44u),record=row(rows,actor,0x888u);
    for(const auto offset:{0x338u,0x32cu,0x344u})
        if(read(m,record+offset)==reference)write(m,record+offset,none);
    if(read(m,record+0x368u)==reference){write(m,record+0x368u,none);write(m,record+0x358u,0,2);}
    if(read(m,record+0x722u,2)==1u&&read(m,record+0x724u)==reference) {
        write(m,record+0x724u,none);write(m,record+0x722u,0,2);
    }
    for(const auto offset:{0x7e0u,0x3b4u})
        if(read(m,record+offset)==reference)write(m,record+offset,none);
    if(read(m,record+0x4acu,2)==7u&&read(m,record+0x4b8u)==reference) {
        write(m,record+0x4acu,0,2);write(m,record+0x4e4u,none);
    }
    for(const auto offset:{0x688u,0x6a0u,0x6b0u})
        if(read(m,record+offset,2)==1u&&read(m,record+offset+4u)==reference)
            write(m,record+offset+4u,none);
    if(read(m,record+7u,1)!=0) {
        const auto attachment=read(m,record+0x1cu);
        if(attachment!=none)invalidate_attachments(m,attachment,reference);
    }
    std::uint16_t slot{};
    if(signed16(read(m,record+0x190u,2))<0)return;
    do {
        const auto behavior=record+0x90u+std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(slot)))*0x40u;
        const auto kind=signed16(read(m,behavior,2));
        const auto descriptor=read(m,0x47b478u+std::uint32_t(std::int32_t(kind))*4u);
        const auto callback=read(m,descriptor+0x28u);
        if(callback) {
            NativeCampaignAiReferenceInvalidationTu15 request{callback,actor,signed16(slot)<4?behavior:0u,reference};
            if(!service){std::ostringstream why;why<<"Native AI clump retirement missing behavior invalidation at 1ea562 -> "<<std::hex<<callback;throw std::runtime_error(why.str());}
            service(request);
        }
        ++slot;
    } while(signed16(slot)<=signed16(read(m,record+0x190u,2)));
}

//274970 detaches one perception from its clump-group intrusive list.
void detach_group_property(SourcePhysicsContactMemory &m,std::uint32_t property) {
    const auto previous=read(m,property+0x38u),pool=read(m,0x50eef0u),next=read(m,property+0x34u);
    if(previous==none) {
        const auto group=read(m,property+8u),groups=read(m,0x50eef4u),rows=read(m,groups+0x44u);
        write(m,row(rows,group,0xc4u)+0x18u,next);
    } else {
        const auto rows=read(m,pool+0x44u);write(m,row(rows,previous,0x3cu)+0x34u,next);
    }
    const auto current_next=read(m,property+0x34u);
    if(current_next!=none) {
        const auto rows=read(m,pool+0x44u),current_previous=read(m,property+0x38u);
        write(m,row(rows,current_next,0x3cu)+0x38u,current_previous);
    }
    write(m,property+8u,none);write(m,property+0x38u,none);write(m,property+0x34u,none);
}

//2748E0 detaches from the actor list, invalidates actual actor references,
// and finally retires this record's actor/previous/next association.
void detach_actor_property(SourcePhysicsContactMemory &m,std::uint32_t property,
        const NativeCampaignAiReferenceInvalidationServiceTu15 &service) {
    const auto actor=read(m,property+4u),actors=read(m,0x5020c8u),actor_rows=read(m,actors+0x44u);
    const auto previous=read(m,property+0x30u),pool=read(m,0x50eef0u);
    std::uint32_t reference;
    if(previous==none) {
        const auto next=read(m,property+0x2cu),head=row(actor_rows,actor,0x888u)+0x58u;
        reference=read(m,head);write(m,head,next);
    } else {
        const auto rows=read(m,pool+0x44u),link=row(rows,previous,0x3cu)+0x2cu;
        reference=read(m,link);const auto next=read(m,property+0x2cu);write(m,link,next);
    }
    const auto next=read(m,property+0x2cu);
    if(next!=none) {
        const auto rows=read(m,pool+0x44u),current_previous=read(m,property+0x30u);
        write(m,row(rows,next,0x3cu)+0x30u,current_previous);
    }
    const auto current_actor=read(m,property+4u);invalidate_actor(m,current_actor,reference,service);
    write(m,property+4u,none);write(m,property+0x30u,none);write(m,property+0x2cu,none);
}

void retire_group(SourcePhysicsContactMemory &m,std::uint32_t clump,std::uint32_t group,
        const NativeCampaignAiReferenceInvalidationServiceTu15 &service) {
    auto groups=read(m,0x50eef4u);const auto group_rows=read(m,groups+0x44u);
    auto property=read(m,row(group_rows,group,0xc4u)+0x18u);
    if(property!=none) {
        auto pool=read(m,0x50eef0u);
        do {
            const auto rows=read(m,pool+0x44u),record=row(rows,property,0x3cu),next=read(m,record+0x34u);
            detach_actor_property(m,record,service);detach_group_property(m,record);
            const auto query=read(m,record+0x14u);
            if(query!=none){const auto queries=read(m,0x50eeecu);native_tu15_data_array_delete(m,queries,query);}
            pool=read(m,0x50eef0u);native_tu15_data_array_delete(m,pool,property);property=next;
        } while(property!=none);
        groups=read(m,0x50eef4u);
    }
    auto link=clump+0x14u;
    if(read(m,link)!=none) {
        const auto rows=read(m,groups+0x44u);
        for(;;) {
            const auto member=read(m,link)&0xffffu,record=rows+member*0xc4u;
            if(member==(group&0xffffu)){const auto next=read(m,record+0x14u);write(m,link,next);break;}
            link=record+0x14u;if(read(m,link)==none)break;
        }
    }
    native_tu15_data_array_delete(m,groups,group);
}

void detach_actor(SourcePhysicsContactMemory &m,std::uint32_t clump,std::uint32_t actor) {
    const auto clumps=read(m,0x50eef8u),clump_rows=read(m,clumps+0x44u),record=row(clump_rows,clump,0x50u);
    auto link=record+0x18u;if(read(m,link)==none)return;
    const auto actors=read(m,0x5020c8u),rows=read(m,actors+0x44u);
    for(;;) {
        const auto member=read(m,link),body=row(rows,member,0x888u);
        if(member==actor) {
            const auto next=read(m,body+0x80u),properties=read(m,0x50eef0u);
            write(m,link,next);write(m,body+0x7cu,none);
            const auto current_rows=read(m,actors+0x44u);
            auto property=read(m,row(current_rows,actor,0x888u)+0x58u);
            while(property!=none) {
                const auto property_rows=read(m,properties+0x44u),perception=row(property_rows,property,0x3cu);
                property=read(m,perception+0x2cu);detach_group_property(m,perception);
            }
            write(m,record+0x10u,read(m,record+0x10u,2)-1u,2);return;
        }
        link=body+0x80u;if(read(m,link)==none)return;
    }
}
}
void native_campaign_ai_perception_group_retirement_tu15(SourcePhysicsContactMemory &m,
        std::uint32_t clump,std::uint32_t group,const NativeCampaignAiReferenceInvalidationServiceTu15 &service) {
    retire_group(m,clump,group,service);
}
void native_campaign_ai_clump_retirement_tu15(SourcePhysicsContactMemory &m,
        std::uint32_t clump,const NativeCampaignAiReferenceInvalidationServiceTu15 &service) {
    const auto index=clump&0xffffu,clumps=read(m,0x50eef8u),rows=read(m,clumps+0x44u),record=rows+index*0x50u;
    auto group=read(m,record+0x14u);
    while(group!=none) {
        const auto groups=read(m,0x50eef4u),group_rows=read(m,groups+0x44u);
        const auto next=read(m,row(group_rows,group,0xc4u)+0x14u);
        retire_group(m,record,group,service);group=next;
    }
    const auto current_clumps=read(m,0x50eef8u),current_rows=read(m,current_clumps+0x44u);
    auto actor=read(m,current_rows+index*0x50u+0x18u);
    const auto actors=read(m,0x5020c8u);
    while(actor!=none) {
        const auto actor_rows=read(m,actors+0x44u),next=read(m,row(actor_rows,actor,0x888u)+0x80u);
        detach_actor(m,clump,actor);actor=next;
    }
    const auto final_clumps=read(m,0x50eef8u);native_tu15_data_array_delete(m,final_clumps,clump);
}
}

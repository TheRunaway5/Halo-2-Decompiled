#include "halo2/native_campaign_ai_awareness_tu15.hpp"
#include "halo2/native_campaign_ai_query_tu15.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <optional>
#include <xmmintrin.h>
#include <emmintrin.h>

namespace halo2 {
NativeCampaignAiAwarenessPrivateStateRequiredTu15::NativeCampaignAiAwarenessPrivateStateRequiredTu15(
    std::uint32_t instruction):std::runtime_error("Native AI awareness requires defined original private iterator/mask state"),instruction_(instruction){}
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
std::int32_t short_value(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
std::uint32_t record(std::uint32_t rows,std::uint32_t datum,std::uint32_t stride){return rows+(datum&0xffffu)*stride;}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t instruction,std::uint32_t at,std::uint16_t value) {
    const std::array<std::byte,2> bytes{std::byte(value),std::byte(value>>8u)};
    if(!observe){memory.write(at,bytes);return;}
    SourcePhysicsContactRecordStore event{instruction,at,std::vector<std::byte>(2),{bytes.begin(),bytes.end()}};
    memory.read(at,event.before);memory.write(at,bytes);observe(event);
}
class Environment {
    alignas(16) std::array<std::byte,108> host_{};
    std::array<std::byte,28> native_{};
    std::uint32_t host_mxcsr_;
    NativeTu15PhysicsFrameFp &fp_;
public:
    Environment(NativeTu15PhysicsFrameFp &fp,unsigned occupied):fp_(fp) {
        if(occupied>7 || (fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) ||
            (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native AI awareness requires masked retained FP and one free x87 value slot");
        host_mxcsr_=_mm_getcsr();
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),native_.size());constexpr std::uint16_t empty=0xffffu;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);
        std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){
        asm volatile("fnstcw %0;fnstsw %1":"=m"(fp_.control_word),"=m"(fp_.status));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);
    }
};
std::uint16_t duration(std::int32_t tick_rate,std::uint32_t seconds_bits) {
    auto value=_mm_setzero_ps();const auto seconds=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(seconds_bits)));
    asm volatile("cvtsi2ssl %1,%0;mulss %2,%0":"+&x"(value):"r"(tick_rate),"x"(seconds):"memory");
    const auto single=std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));std::int32_t ticks;
    asm volatile("flds %1;fistpl %0":"=m"(ticks):"m"(single):"memory","st");
    return std::uint16_t(ticks);
}
// Exact20CFF0/20D030 squad-linked actor iteration. A disabled initializer
// deliberately leaves existing private state intact, as the original does.
struct Members {
    std::optional<std::uint32_t> current,next;
    void begin(const SourcePhysicsContactMemory &memory,std::uint32_t squad) {
        const auto globals=read(memory,0x5020a8u);if(!read(memory,globals+1u,1))return;
        current=none;
        if(squad==none)next=read(memory,globals+0x14u);
        else {const auto pool=read(memory,0x52be7cu),rows=read(memory,pool+0x44u);next=read(memory,record(rows,squad,0x98u)+0x68u);}
    }
    std::uint32_t advance(const SourcePhysicsContactMemory &memory) {
        const auto globals=read(memory,0x5020a8u);if(!read(memory,globals+1u,1))return 0;
        if(!next)throw NativeCampaignAiAwarenessPrivateStateRequiredTu15(0x20d040u);
        if(*next==none)return 0;
        const auto pool=read(memory,0x5020c8u),rows=read(memory,pool+0x44u),body=record(rows,*next,0x888u);
        current=*next;next=read(memory,body+0x20u);return body;
    }
};
// Exact20D070/20D0D0 depth-first group/squad traversal; parent and sibling
// references are authored links, with no artificial traversal-count cutoff.
struct Squads {
    std::uint32_t current{},root{},next=none,squad=none;
    void descend(const SourcePhysicsContactMemory &memory,std::uint32_t pool) {
        for(auto child=read(memory,current+4u);child!=none;child=read(memory,current+4u))
            current=record(read(memory,pool+0x44u),child,0x38u);
    }
    void begin(const SourcePhysicsContactMemory &memory,std::uint32_t group) {
        const auto pool=read(memory,0x52be80u);current=root=record(read(memory,pool+0x44u),group,0x38u);
        descend(memory,pool);next=read(memory,current+8u);
    }
    std::uint32_t advance(const SourcePhysicsContactMemory &memory) {
        if(!current)return 0;
        const auto groups=read(memory,0x52be80u);
        while(next==none) {
            if(current==root){current=0;return 0;}
            const auto sibling=read(memory,current+0xcu);
            if(sibling==none){const auto parent=read(memory,current+0x10u),rows=read(memory,groups+0x44u);current=record(rows,parent,0x38u);}
            else {current=record(read(memory,groups+0x44u),sibling,0x38u);descend(memory,groups);}
            next=read(memory,current+8u);
            if(!current)return 0;
        }
        const auto index=next,pool=read(memory,0x52be7cu),rows=read(memory,pool+0x44u),body=record(rows,index,0x98u);
        squad=index;next=std::uint32_t(short_value(read(memory,body+0x74u,2)));return body;
    }
};
// Actual27CAA0/27CB30 compose the iterators above and the independently
// qualified27C880 actor/starting-location resolver.
class Actors {
    const SourcePhysicsContactMemory &memory_;
    Members members_;Squads squads_;
    std::uint32_t squad_=none,actor_=none;
    bool group_{},single_{};
public:
    Actors(const SourcePhysicsContactMemory &memory,std::uint32_t reference):memory_(memory) {
        const auto kind=reference>>30;
        if(kind==1u){group_=true;squads_.begin(memory,reference&0xffffu);squads_.advance(memory);squad_=squads_.squad;members_.begin(memory,squad_);}
        else if(!kind){squad_=reference&0xffffu;members_.begin(memory,squad_);}
        else {actor_=native_campaign_ai_actor_tu15(memory,reference);single_=true;}
    }
    std::uint32_t advance() {
        while(squad_!=none) {
            const auto body=members_.advance(memory_);
            if(body){actor_=*members_.current;return body;}
            actor_=none;
            if(!group_)return 0;
            const auto squad_body=squads_.advance(memory_);squad_=squads_.squad;
            if(!squad_body)return 0;
            members_.begin(memory_,squad_);
        }
        if(!single_)return 0;
        std::uint32_t body{};
        if(actor_!=none){const auto pool=read(memory_,0x5020c8u);body=record(read(memory_,pool+0x44u),actor_,0x888u);}
        single_=false;return body;
    }
};
bool belongs(const SourcePhysicsContactMemory &memory,std::uint32_t squad,std::uint32_t group) {
    const auto pool=read(memory,0x52be7cu),rows=read(memory,pool+0x44u);
    auto parent=read(memory,record(rows,squad,0x98u)+0x6cu);if(parent==none)return false;
    const auto groups=read(memory,0x52be80u);
    do {
        if(parent==group)return true;
        const auto group_rows=read(memory,groups+0x44u);parent=read(memory,record(group_rows,parent,0x38u)+0x10u);
    }while(parent!=none);
    return false;
}
bool squad_matches(const SourcePhysicsContactMemory &memory,std::uint32_t squad,std::uint32_t reference) {
    if(!(reference>>30))return squad==(reference&0xffffu);
    return (reference>>30)==1u && belongs(memory,squad,reference&0xffffu);
}
bool first_clump(std::uint32_t &visited,std::uint32_t datum,std::uint32_t instruction) {
    const auto index=datum&0xffffu;
    if(index>=32u)throw NativeCampaignAiAwarenessPrivateStateRequiredTu15(instruction);
    const auto bit=std::uint32_t{1}<<index;if(visited&bit)return false;visited|=bit;return true;
}
}
void native_campaign_ai_magically_see_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t observers,std::uint32_t targets,NativeTu15PhysicsFrameFp &fp,
    unsigned occupied,const SourcePhysicsContactRecordObserver &observe) {
    Environment environment(fp,occupied);Actors actors(memory,observers);auto actor=actors.advance();if(!actor)return;
    const auto seconds=read(memory,0x46a100u);std::uint32_t visited{};
    do {
        const auto clump=read(memory,actor+0x7cu);
        if(clump!=none && first_clump(visited,clump,0x27d823u)) {
            const auto clumps=read(memory,0x50eef8u),rows=read(memory,clumps+0x44u);auto group=read(memory,record(rows,clump,0x50u)+0x14u);
            while(group!=none) {
                const auto groups=read(memory,0x50eef4u),group_rows=read(memory,groups+0x44u),body=record(group_rows,group,0xc4u);
                const auto target=read(memory,body+0x1cu);bool selected{};
                if(target!=none) {
                    if((targets>>30)<2u){const auto pool=read(memory,0x5020c8u),records=read(memory,pool+0x44u),squad=read(memory,record(records,target,0x888u)+0x30u);selected=squad!=none && squad_matches(memory,squad,targets);}
                    else selected=target==native_campaign_ai_actor_tu15(memory,targets);
                }
                if(selected) {
                    auto property=read(memory,body+0x18u);
                    while(property!=none) {
                        const auto properties=read(memory,0x50eef0u),property_rows=read(memory,properties+0x44u),link=record(property_rows,property,0x3cu);
                        const auto observer=read(memory,link+4u);bool matches{};
                        if((observers>>30)<2u){const auto pool=read(memory,0x5020c8u),records=read(memory,pool+0x44u),squad=read(memory,record(records,observer,0x888u)+0x30u);matches=squad!=none && squad_matches(memory,squad,observers);}
                        else matches=observer==native_campaign_ai_actor_tu15(memory,observers);
                        if(matches) {
                            const auto pool=read(memory,0x50eef0u),records=read(memory,pool+0x44u),output=record(records,property,0x3cu),clock=read(memory,0x51df54u);
                            publish(memory,observe,0x27d9a6u,output+0x1cu,3);
                            const auto ticks=duration(short_value(read(memory,clock+2u,2)),seconds);
                            publish(memory,observe,0x27d9cbu,output+0x1eu,ticks);
                        }
                        property=read(memory,link+0x34u);
                    }
                }
                group=read(memory,body+0x14u);
            }
        }
        actor=actors.advance();
    }while(actor);
}
void native_campaign_ai_magically_see_object_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t observers,std::uint32_t object,NativeTu15PhysicsFrameFp &fp,
    unsigned occupied,const SourcePhysicsContactRecordObserver &observe) {
    Environment environment(fp,occupied);if(observers&0xc0000000u)return;
    const auto squad=observers&0xffffu;Members members;members.begin(memory,squad);
    const auto properties=read(memory,0x50eef0u),actors=read(memory,0x5020c8u),seconds=read(memory,0x46a100u);std::uint32_t visited{};
    while(true) {
        const auto globals=read(memory,0x5020a8u);if(!read(memory,globals+1u,1))break;
        if(!members.next)throw NativeCampaignAiAwarenessPrivateStateRequiredTu15(0x27da90u);
        if(*members.next==none)break;
        const auto actor_rows=read(memory,actors+0x44u),actor=record(actor_rows,*members.next,0x888u);
        const auto next=read(memory,actor+0x20u),clump=read(memory,actor+0x7cu);members.next=next;
        if(clump==none || !first_clump(visited,clump,0x27dad3u))continue;
        const auto clumps=read(memory,0x50eef8u),clump_rows=read(memory,clumps+0x44u);auto group=read(memory,record(clump_rows,clump,0x50u)+0x14u);
        while(group!=none) {
            const auto groups=read(memory,0x50eef4u),group_rows=read(memory,groups+0x44u),body=record(group_rows,group,0xc4u);
            if(read(memory,body+8u)==object) {
                auto property=read(memory,body+0x18u);
                while(property!=none) {
                    const auto property_rows=read(memory,properties+0x44u),member_rows=read(memory,actors+0x44u),link=record(property_rows,property,0x3cu);
                    const auto member=read(memory,link+4u);
                    if(read(memory,record(member_rows,member,0x888u)+0x30u)==squad) {
                        const auto clock=read(memory,0x51df54u);publish(memory,observe,0x27db6bu,link+0x1cu,3);
                        const auto ticks=duration(short_value(read(memory,clock+2u,2)),seconds);
                        publish(memory,observe,0x27db90u,link+0x1eu,ticks);
                    }
                    property=read(memory,link+0x34u);
                }
            }
            group=read(memory,body+0x14u);
        }
    }
}
}

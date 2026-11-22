#include "halo2/native_campaign_ai_candidate_collection_tu15.hpp"
#include "halo2/native_campaign_ai_action_identity_tu15.hpp"
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include "halo2/detail/physics_scalar_memory.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
constexpr std::uint32_t actor_pool=0x5020c8u,squad_pool=0x52be7cu,scenario_cell=0x4ecc20u,
    dynamic_pool=0x52d87cu,owner_table=0x56aeb0u,structure_bsp=0x4ecc18u;
std::int32_t word(std::uint32_t value){return std::int16_t(std::uint16_t(value));}
std::int32_t sbyte(std::uint32_t value){return std::int8_t(std::uint8_t(value));}
[[noreturn]] void refuse(const char *what,std::uint32_t value) {
    std::ostringstream text;text<<"Native 26BA30 refuses "<<what<<" ("<<std::hex<<value<<")";
    throw std::runtime_error(text.str());
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// 3EA20 as inlined at 26C16A/26C189: |first-second| from three x87 single
// differences, in the original operand order, ending with ST(0) = the root.
#define HALO2_3EA20 \
    "flds %[a0]\n\tfsubs %[b0]\n\tflds %[a1]\n\tfsubs %[b1]\n\tflds %[a2]\n\tfsubs %[b2]\n\t" \
    ".byte 0xd9,0xc2\n\t.byte 0xde,0xcb\n\t.byte 0xd9,0xc1\n\t.byte 0xd8,0xca\n\t.byte 0xde,0xc3\n\t" \
    ".byte 0xd9,0xc0\n\t.byte 0xd8,0xc9\n\t.byte 0xde,0xc3\n\t.byte 0xd9,0xca\n\t.byte 0xd9,0xfa\n\t" \
    ".byte 0xdd,0xda\n\t.byte 0xdd,0xd8\n\t"
// 26C16A..26C17D: d = 3EA20(point, owner); FST d; FLD one; FCOMIP; FSTP; JA.
bool owner_too_close(NativeTu15PhysicsFrameFp &fp,const std::array<std::uint32_t,3> &point,
    const std::array<std::uint32_t,3> &owner,std::uint32_t one,std::uint32_t &distance) {
    unsigned char above{};
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    asm volatile(HALO2_3EA20 "fsts %[d]\n\tflds %[one]\n\t.byte 0xdf,0xf1\n\t.byte 0xdd,0xd8\n\tseta %[above]"
        :[d]"=m"(distance),[above]"=q"(above)
        :[a0]"m"(point[0]),[a1]"m"(point[1]),[a2]"m"(point[2]),
         [b0]"m"(owner[0]),[b1]"m"(owner[1]),[b2]"m"(owner[2]),[one]"m"(one)
        :"cc","memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    return above;
}
// 26C189..26C19A: s = 3EA20(point, self); FADD ST,ST; FLD d; FXCH; FCOMIP;
// FSTP; JBE (unordered compares accept, as in the original).
bool self_within(NativeTu15PhysicsFrameFp &fp,const std::array<std::uint32_t,3> &point,
    const std::array<std::uint32_t,3> &self,std::uint32_t distance) {
    unsigned char within{};
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    asm volatile(HALO2_3EA20 ".byte 0xdc,0xc0\n\tflds %[d]\n\t.byte 0xd9,0xc9\n\t.byte 0xdf,0xf1\n\t"
                 ".byte 0xdd,0xd8\n\tsetbe %[within]"
        :[within]"=q"(within)
        :[a0]"m"(point[0]),[a1]"m"(point[1]),[a2]"m"(point[2]),
         [b0]"m"(self[0]),[b1]"m"(self[1]),[b2]"m"(self[2]),[d]"m"(distance)
        :"cc","memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    return within;
}
#undef HALO2_3EA20
#else
bool owner_too_close(NativeTu15PhysicsFrameFp &,const std::array<std::uint32_t,3> &,
    const std::array<std::uint32_t,3> &,std::uint32_t,std::uint32_t &) {
    throw std::runtime_error("Exact native 26C0B0 x87 backend unavailable");
}
bool self_within(NativeTu15PhysicsFrameFp &,const std::array<std::uint32_t,3> &,
    const std::array<std::uint32_t,3> &,std::uint32_t) {
    throw std::runtime_error("Exact native 26C0B0 x87 backend unavailable");
}
#endif

class Collection {
public:
    Collection(SourcePhysicsContactMemory &memory,NativeTu15PhysicsFrameFp &fp,
        const SourcePhysicsContactRecordObserver &observe,std::uint32_t actor,std::uint32_t candidates,
        std::uint32_t capacity,std::uint32_t request)
        :memory_(memory),fp_(fp),observe_(observe),actor_(actor),candidates_(candidates),
         capacity_(std::bit_cast<std::int32_t>(capacity)),request_(request) {}
    std::uint32_t run();
private:
    SourcePhysicsContactMemory &memory_;
    NativeTu15PhysicsFrameFp &fp_;
    const SourcePhysicsContactRecordObserver &observe_;
    std::uint32_t actor_,candidates_;
    std::int32_t capacity_;
    std::uint32_t request_;
    // 26BA30 [esp+10]: 26BE90 increments its low word, 26BA30 the dword.
    std::uint32_t count_{};
    unsigned depth_{};
    std::uint32_t read(std::uint32_t address,unsigned width=4)const {
        return physics_step_detail::r(memory_,address,width);
    }
    void publish(std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width) {
        physics_step_detail::w(memory_,observe_,site,address,width,value);
    }
    std::uint32_t candidate()const{return std::uint32_t(word(count_))*0x78u+candidates_;}
    bool squad(std::uint32_t squad);
    void area(std::uint32_t zone,std::uint32_t point,std::uint32_t kind);
    bool admit(std::uint32_t row,std::uint32_t goal);
    std::uint32_t owner(std::uint32_t goal);
    bool initialize(std::uint32_t candidate,std::uint32_t goal);
    void mark_clusters(std::uint32_t point,std::uint32_t zone,std::uint32_t caller_record);
};

// 26C740 -> 140DB0: the "actor firing-position owners" table lookup. The
// table's callbacks are data; only the original 267A30 hash and 267A40
// equality have native owners.
std::uint32_t Collection::owner(std::uint32_t goal) {
    const auto table=read(owner_table);
    const auto hash_owner=read(table+0x2cu);
    if(hash_owner!=0x267a30u)throw NativeCampaignAiPerformRequiredOwnerTu15(0x140db8u,hash_owner);
    const auto hash=(goal&0xffu)<<2;
    const auto buckets=read(table+0x20u);
    if(!buckets)refuse("the 140DBF divide by a zero owner-table bucket count",table);
    auto node=read(table+(hash%buckets)*4u+0x3cu);
    for(;node;node=read(node+8u)) {
        if(hash!=read(node+4u))continue;
        const auto key=read(node);
        const auto equal_owner=read(table+0x30u);
        if(equal_owner!=0x267a40u)throw NativeCampaignAiPerformRequiredOwnerTu15(0x140dd9u,equal_owner);
        if(key==goal)break;
    }
    if(!node)return none;
    // REP MOVSD/MOVSB into the dword local initialized to NONE.
    const auto size=read(table+0x28u);
    if(size>4u)refuse("a 26C740 owner value that overwrites its own frame",size);
    std::array<std::byte,4> value{std::byte{0xff},std::byte{0xff},std::byte{0xff},std::byte{0xff}};
    std::uint32_t at=0;
    for(std::uint32_t n=0;n<size/4u;++n,at+=4u)memory_.read(node+0xcu+at,std::span(value).subspan(at,4));
    for(std::uint32_t n=0;n<size%4u;++n,++at)memory_.read(node+0xcu+at,std::span(value).subspan(at,1));
    std::uint32_t result{};
    for(unsigned n=0;n<4;++n)result|=std::to_integer<unsigned>(value[n])<<(n*8u);
    return result;
}

// 26C0B0 (EAX request): firing-position admission for this actor.
bool Collection::admit(std::uint32_t row,std::uint32_t goal) {
    if(!request_)return true;
    const auto definition=native_campaign_ai_goal_definition_tu15(memory_,goal);
    if(!definition)return false;
    if(!read(request_+0x56u,1) && !read(request_+0x58u,1)) {
        if(read(definition+0x14u)==none)return false;
        if(!(read(definition+0xeu,1)&0x60u))return false;
    }
    const auto require_facing=read(request_+0x57u,1);
    const bool facing=(read(definition+0xeu,1)>>5)&1u;
    bool accepted;
    if(require_facing)accepted=facing;
    else if(read(request_+0x59u,1))accepted=true;
    else accepted=!facing;
    if(!accepted)return false;
    const auto holder=owner(goal);
    if(holder==actor_ || holder==none)return accepted;
    const auto actors=read(actor_pool),rows=read(actors+0x44u);
    const auto holder_row=(holder&0xffffu)*0x888u+rows;
    if(read(holder_row+0x24u,2)!=read(row+0x24u,2))return false;
    // 218B20 into a private local: the projected point is a host value.
    const auto point=native_campaign_ai_project_point_value_tu15(memory_,definition,fp_);
    const std::array<std::uint32_t,3> held{read(holder_row+0x238u),read(holder_row+0x23cu),read(holder_row+0x240u)};
    const auto one=read(0x46a0d0u);
    std::uint32_t distance{};
    if(owner_too_close(fp_,point,held,one,distance))return false;
    const std::array<std::uint32_t,3> self{read(row+0x238u),read(row+0x23cu),read(row+0x240u)};
    return self_within(fp_,point,self,distance)?accepted:false;
}

// 26C1E0 (EDI candidate): initialize one 0x78-byte candidate.
bool Collection::initialize(std::uint32_t at,std::uint32_t goal) {
    const auto definition=native_campaign_ai_goal_definition_tu15(memory_,goal);
    if(!definition)return false;
    const auto origin=read(0x474d04u);
    const auto limit=read(0x45695cu);
    publish(0x26c206u,at+0x18u,limit,4);
    publish(0x26c20bu,at,definition,4);
    publish(0x26c20du,at+4u,goal,4);
    publish(0x26c210u,at+8u,0,2);
    publish(0x26c21bu,at+0x1cu,read(origin),4);
    publish(0x26c220u,at+0x20u,read(origin+4u),4);
    publish(0x26c226u,at+0x24u,read(origin+8u),4);
    publish(0x26c229u,at+0x28u,limit,4);
    publish(0x26c235u,at+0x40u,read(origin),4);
    publish(0x26c23au,at+0x44u,read(origin+4u),4);
    publish(0x26c240u,at+0x48u,read(origin+8u),4);
    publish(0x26c243u,at+0x2cu,limit,4);
    publish(0x26c250u,at+0x34u,read(origin),4);
    publish(0x26c255u,at+0x38u,read(origin+4u),4);
    publish(0x26c25bu,at+0x3cu,read(origin+8u),4);
    publish(0x26c261u,at+0x30u,0,4);
    publish(0x26c266u,at+0x50u,0,4);
    publish(0x26c26bu,at+0x54u,0,4);
    publish(0x26c270u,at+0x4cu,1,1);
    publish(0x26c274u,at+0x4du,0,1);
    publish(0x26c277u,at+0x5bu,0,1);
    publish(0x26c27au,at+0x5au,0,1);
    publish(0x26c27du,at+0x58u,0,1);
    publish(0x26c280u,at+0x59u,0,1);
    publish(0x26c283u,at+0x5cu,0,2);
    native_campaign_ai_project_point_tu15(memory_,definition,at+0xcu,fp_,observe_);
    return true;
}

// 2A78A0 (AX area, DX zone, ESI request+6A0): append the area's BSP cluster
// references to the request's 30-entry list. The original keeps a 64-bit
// "seen" mask in its frame and indexes it with the signed cluster >> 5
// without a bound. The frame model covers the words that index can reach
// with a known meaning: its locals (counter, geometry, area record, mask)
// and the saved EBX/EDI (the request and 26BE90's area record, both dead
// after the return). Saved EBP (26BE90's count pointer, read again after
// the return) and the return address are refused.
void Collection::mark_clusters(std::uint32_t point,std::uint32_t zone,std::uint32_t caller_record) {
    const auto list=request_+0x6a0u;
    const auto area16=word(point),zone16=word(zone);
    if(word(read(list+0xb4u,2))>=0x1e)return;
    const auto scenario=read(scenario_cell);
    if(zone16<0)return;
    if(zone16>=std::bit_cast<std::int32_t>(read(scenario+0x168u)))return;
    const auto zone_record=std::uint32_t(zone16)*0x38u+read(scenario+0x16cu);
    if(area16<0)return;
    if(area16>=std::bit_cast<std::int32_t>(read(zone_record+0x30u)))return;
    // 202AB0: the current structure BSP's AI resource.
    const auto bsp=read(structure_bsp);
    std::uint32_t resource=0;
    if(std::bit_cast<std::int32_t>(read(bsp+0xc4u))>0)resource=read(bsp+0xc8u);
    if(!resource)return;
    if(std::bit_cast<std::int32_t>(read(resource+0x6cu))<=0)return;
    const auto geometry=read(resource+0x70u);
    const auto record=std::uint32_t(area16)*0x88u+read(zone_record+0x34u);
    // [esp0-8] saved EBX, [-4] saved EDI, [+0] counter, [+4] geometry,
    // [+8] area record, [+C..+13] mask: frame[(cluster >> 5) + 5].
    std::array<std::uint32_t,7> frame{request_,caller_record,0,geometry,record,0,0};
    auto area_register=record;
    if(std::bit_cast<std::int32_t>(read(area_register+0x80u))<=0)return;
    std::uint32_t index=0;
    for(;;) {
        const auto references=read(area_register+0x84u);
        const auto reference=references+index*4u;
        const auto cluster=read(reference,2);
        const auto signed_cluster=word(cluster);
        const auto slot=(signed_cluster>>5)+5;
        if(slot<0 || slot>6)refuse("a 2A78A0 cluster index that reaches its saved EBP or return address",cluster);
        const auto bit=1u<<(std::uint32_t(signed_cluster)&31u);
        // A cluster already seen goes straight to the next reference with
        // the area register unchanged (2A7975 -> 2A7A38).
        if(!(frame[std::size_t(slot)]&bit)) {
            frame[std::size_t(slot)]|=bit;
            if(word(read(list+0xb4u,2))>0) {
                for(std::uint32_t k=0;;) {
                    if(read(list+std::uint32_t(word(k))*6u,2)==cluster)return;
                    ++k;
                    if(!(word(k)<word(read(list+0xb4u,2))))break;
                }
            }
            bool append=signed_cluster>=0;
            std::uint32_t entry=0;
            if(append) {
                const auto table=frame[3];
                if(signed_cluster>=std::bit_cast<std::int32_t>(read(table+0x40u)))append=false;
                else entry=read(table+0x44u)+std::uint32_t(signed_cluster)*8u;
            }
            if(append) {
                if(std::bit_cast<std::int32_t>(read(entry))>0) {
                    for(std::uint32_t sub=0;;) {
                        const auto at=list+std::uint32_t(word(read(list+0xb4u,2)))*6u;
                        const auto value=read(reference,2);
                        publish(0x2a79edu,at,value,2);
                        publish(0x2a79fbu,list+std::uint32_t(word(read(list+0xb4u,2)))*6u+2u,sub,2);
                        publish(0x2a7a0au,list+std::uint32_t(word(read(list+0xb4u,2)))*6u+4u,0,2);
                        publish(0x2a7a0fu,list+0xb4u,read(list+0xb4u,2)+1u,2);
                        if(word(read(list+0xb4u,2))>=0x1e)return;
                        const auto limit=std::bit_cast<std::int32_t>(read(entry));
                        ++sub;
                        if(!(word(sub)<limit))break;
                    }
                }
                if(word(read(list+0xb4u,2))>=0x1e)return;
            }
            area_register=frame[4];
        }
        const auto counter=frame[2]+1u;
        const auto limit=std::bit_cast<std::int32_t>(read(area_register+0x80u));
        frame[2]=counter;
        if(!(word(counter)<limit))return;
        index=std::uint32_t(word(counter));
    }
}

// 26BE90: collect one area's firing positions (zone, area: sign-extended
// words; kind: the order entry's type word).
void Collection::area(std::uint32_t zone,std::uint32_t point,std::uint32_t kind) {
    const auto actors=read(actor_pool),rows=read(actors+0x44u);
    const auto row=(actor_&0xffffu)*0x888u+rows;
    const bool targeted=read(row+0x26cu)!=none;
    if(zone==none || point==none)return;
    if(request_ && read(request_+0x68cu,1)) {
        if(zone!=std::uint32_t(word(read(request_+0x68eu,2))))return;
        if(point!=std::uint32_t(word(read(request_+0x690u,2))))return;
    }
    const auto scenario=read(scenario_cell);
    const auto zone_record=(zone&0xffffu)*0x38u+read(scenario+0x16cu);
    if(read(zone_record+0x24u,2)!=read(0x474c24u,2))return;
    const auto areas=read(zone_record+0x34u);
    const auto record=point*0x88u+areas;
    const auto flags=read(record+0x20u);
    if(targeted!=bool(flags&1u))return;
    bool open=(flags>>1)&1u;
    if(request_) {
        bool test=true;
        if(targeted)open=!open;
        else if(read(request_+0x57u,1)) {}
        else if(read(request_+0x59u,1))test=false;
        else open=!open;
        if(test && !open)return;
    }
    std::uint32_t cursor=read(record+0x38u,2);
    if(word(count_)<capacity_) {
        for(;;) {
            const auto span=word(read(record+0x3au,2));
            const auto first=word(read(record+0x38u,2));
            if(!(word(cursor)<first+span))break;
            const auto goal=((zone&0xffffu)<<16)|(cursor&0xffffu);
            if(admit(row,goal)) {
                const auto at=candidate();
                if(initialize(at,goal)) {
                    const auto selector=word(kind);
                    if(selector==1)publish(0x26c014u,candidate()+0x5bu,1,1);
                    else if(selector==2)publish(0x26c002u,candidate()+0x5au,1,1);
                    const auto first_definition=read(candidate());
                    publish(0x26c033u,candidate()+0x58u,(std::uint32_t(sbyte(read(first_definition+0xeu,1)))>>5)&1u,1);
                    const auto second_definition=read(candidate());
                    publish(0x26c04eu,candidate()+0x59u,(~(read(second_definition+0xeu,1)>>6))&1u,1);
                    count_=(count_&0xffff0000u)|((count_+1u)&0xffffu);
                }
            }
            ++cursor;
            if(!(word(count_)<capacity_))break;
        }
    }
    if(request_ && read(request_+0x56u,1))mark_clusters(point,zone,record);
}

// 26BBD0: a squad's firing areas, from its explicit pairs, its order's
// entries (following order redirects) or its definition's zone.
bool Collection::squad(std::uint32_t squad) {
    if(++depth_>4096u)refuse("an order redirect chain deeper than the original stack",squad);
    struct Depth{unsigned &value;~Depth(){--value;}} depth{depth_};
    const auto squads=read(squad_pool),squad_rows=read(squads+0x44u);
    const auto index=squad&0xffffu;
    const auto offset=index*0x98u;
    const auto indicator=read(offset+squad_rows+0x80u);
    const auto record=offset+squad_rows;
    if(indicator!=none) {
        const auto pairs=sbyte(read(record+0x30u,1));
        if(pairs<0)return false;
        if(pairs==0)return true;
        for(std::uint32_t n=0;;) {
            const auto at=record+std::uint32_t(word(n))*4u;
            const auto point=word(read(at+0x34u,2));
            const auto zone=word(read(at+0x32u,2));
            area(std::uint32_t(zone),std::uint32_t(point),0);
            const auto limit=sbyte(read(record+0x30u,1));
            ++n;
            if(!(word(n)<limit))return true;
        }
    }
    const auto order=read(record+0x2au,2);
    if(order==0xffffu) {
        const auto scenario=read(scenario_cell);
        const auto definitions=read(scenario+0x164u);
        const auto zone=read(index*0x74u+definitions+0x38u,2);
        if(zone==0xffffu)return false;
        const auto zones=read(scenario+0x16cu);
        const auto zone_record=zone*0x38u+zones;
        if(std::bit_cast<std::int32_t>(read(zone_record+0x30u))<=0)return false;
        for(std::uint32_t n=0;;) {
            area(std::uint32_t(word(zone)),std::uint32_t(word(n)),0);
            const auto limit=std::bit_cast<std::int32_t>(read(zone_record+0x30u));
            ++n;
            if(!(word(n)<limit))return false;
        }
    }
    const auto scenario=read(scenario_cell);
    const auto order_offset=std::uint32_t(word(order))*0x7cu;
    const auto orders=read(scenario+0x244u);
    const auto order_flags=read(order_offset+orders+0x24u,1);
    const auto order_row=order_offset+orders;
    if(order_flags&0x20u) {
        const auto parent=read(order_row+0x4eu,2);
        if(parent!=0xffffu)return this->squad(std::uint32_t(word(parent)));
    }
    const auto actors=read(actor_pool),rows=read(actors+0x44u);
    const auto secondary=read(record+0x60u,1);
    const auto self=(actor_&0xffffu)*0x888u+rows;
    const auto list=order_row+(secondary?0x5cu:0x54u);
    std::uint32_t mask=read(self+0x3f2u,1)?6u:4u;
    if(!request_ || !read(request_+0x69cu,1))mask|=1u;
    if((mask&4u) && !(mask&1u)) {
        const auto entries=std::bit_cast<std::int32_t>(read(list));
        bool found=false;
        if(entries>0) {
            for(std::uint32_t n=0;;) {
                const auto base=read(order_row+(read(record+0x60u,1)?0x60u:0x58u));
                if(read(base+std::uint32_t(word(n))*8u,2)==2u){found=true;break;}
                ++n;
                if(!(word(n)<entries))break;
            }
        }
        if(!found)mask|=1u;
    }
    if(request_)publish(0x26bd6fu,request_+0x69eu,mask,2);
    if(std::bit_cast<std::int32_t>(read(list))<=0)return true;
    const auto selection=std::uint32_t(word(mask));
    for(std::uint32_t n=0;;) {
        const auto base=read(order_row+(read(record+0x60u,1)?0x60u:0x58u));
        const auto entry=base+std::uint32_t(word(n))*8u;
        const auto kind=read(entry,2);
        if(selection&(1u<<(kind&31u))) {
            const auto point=word(read(entry+6u,2));
            const auto zone=word(read(entry+4u,2));
            area(std::uint32_t(zone),std::uint32_t(point),kind);
        }
        const auto limit=std::bit_cast<std::int32_t>(read(list));
        ++n;
        if(!(word(n)<limit))return true;
    }
}

// 26BA30 body.
std::uint32_t Collection::run() {
    const auto actors=read(actor_pool),rows=read(actors+0x44u);
    const auto row=(actor_&0xffffu)*0x888u+rows;
    count_=0;
    const auto squad_index=read(row+0x30u);
    if(squad_index!=none)squad(squad_index);
    const auto dynamic=read(row+0x3f4u);
    if(dynamic==none)return count_;
    if(request_ && read(request_,2)==4u) {
        const auto current=read(row+0x30u);
        if(current!=none && word(count_)>0) {
            const auto squads=read(squad_pool),squad_rows=read(squads+0x44u);
            const auto squad_offset=(current&0xffffu)*0x98u;
            const auto order=read(squad_offset+squad_rows+0x2au,2);
            const auto squad_row=squad_offset+squad_rows;
            if(order!=0xffffu && !read(row+0x3cu,1)) {
                const auto scenario=read(scenario_cell),orders=read(scenario+0x244u);
                const auto order_offset=std::uint32_t(word(order))*0x7cu;
                const auto flags=read(order_offset+orders+0x24u);
                const auto order_row=order_offset+orders;
                bool held=flags&0x10u;
                if(!held && (flags&0x20u))held=read(order_row+0x4eu,2)!=0xffffu;
                // A squad held by its order keeps only its own positions.
                if(held && read(squad_row+0x80u)!=none)return count_;
            }
        }
    }
    const auto dynamics=read(dynamic_pool),dynamic_rows=read(dynamics+0x44u);
    const auto record=(dynamic&0xffffu)*0x484u+dynamic_rows;
    if(word(read(record+8u,2))<=0)return count_;
    for(std::uint32_t index=0;;) {
        if(word(count_)>=capacity_)return count_;
        const auto goal=(((dynamic&0xffffu)|0x8000u)<<16)|(index&0xffffu);
        if(admit(row,goal) && initialize(candidate(),goal)) {
            if(read(record+2u,2)==1u) {
                const auto slot=std::uint32_t(word(count_))*0x78u;
                const auto value=read(record+std::uint32_t(word(index))*2u+0x444u,2);
                publish(0x26bba6u,slot+candidates_+0x5cu,value,2);
            }
            ++count_;
        }
        ++index;
        if(!(word(index)<word(read(record+8u,2))))return count_;
    }
}
}

std::uint32_t native_campaign_ai_collect_candidates_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t actor,std::uint32_t candidates,std::uint32_t capacity,std::uint32_t request,
    NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    return Collection(memory,fp,observe,actor,candidates,capacity,request).run();
}

void native_campaign_ai_collect_candidates_call_tu15(SourcePhysicsContactMemory &memory,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth>4u)
        throw std::invalid_argument("Native 26BA30 requires retained FP controls and four free x87 temporaries");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    struct Publication {
        SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;
        ~Publication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}
    } publication{abi,fp};
    const auto sp=abi.gp[4];
    const auto word_at=[&](std::uint32_t address) {
        std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
        return value;
    };
    const auto actor=word_at(sp+4u),candidates=word_at(sp+8u),capacity=word_at(sp+12u),request=word_at(sp+16u);
    abi.gp[0]=native_campaign_ai_collect_candidates_tu15(memory,actor,candidates,capacity,request,fp,observe);
    abi.gp[4]=sp+0x14u;
}
}

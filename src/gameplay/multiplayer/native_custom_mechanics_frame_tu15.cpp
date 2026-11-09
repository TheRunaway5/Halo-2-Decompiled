#include "halo2/native_custom_mechanics_frame_tu15.hpp"
#include "physics/native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::int32_t signed32(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
std::int32_t signed16(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t out{};
        for(unsigned n=0;n<width;++n)out|=std::to_integer<unsigned>(b[n])<<(n*8u);return out;
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width=4){
        if(const auto*domain=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&memory_))domain->require_retained_extent(p,width);
        const auto&observer=native_tu15_effective_store_observer(memory_,observe_);
        const bool visible=native_tu15_observes_retained_store(memory_,observer,pc,p,width);
        std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(visible){event.before.resize(width);memory_.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
        memory_.write(p,std::span(b).first(width));if(visible)observer(event);
    }
    void mask(const NativeCustomRosterMaskRequest&r,const NativeCustomMechanicsFrameServices&s){
        if(!s.roster_mask)throw std::runtime_error("Native custom frame requires its checked roster mask storage");
        if(const auto*domain=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&memory_))domain->require_retained_extent(r.address,r.width);
        const auto result=s.roster_mask(r);const auto&observer=native_tu15_effective_store_observer(memory_,observe_);
        if(native_tu15_observes_retained_store(memory_,observer,r.instruction,r.address,r.width))
            observer({r.instruction,r.address,{result.before.begin(),result.before.begin()+r.width},
                      {result.written.begin(),result.written.begin()+r.width}});
    }
};
std::uint32_t selected(Memory&m,std::uint32_t engine){return m.word(0x56dfa0u+m.word(engine+0xc14u)*4u);}
std::uint32_t player_row(Memory&m,std::uint32_t player){const auto pool=m.word(0x4f54f4u),rows=m.word(pool+0x44u);return rows+(player&0xffffu)*0x21cu;}
bool exhausted(Memory&m,std::uint32_t player){
    const auto game=m.word(0x4f3218u);if(signed32(m.word(game+0x1b8u))<=0)return false;
    const auto row=player_row(m,player);if(m.word(row+0x2cu)!=none)return false;
    return m.word(row+0x1acu,2)==0;
}
void assign_iterator(Memory&m,NativeTu15DataArrayIterator&state,std::uint32_t index){
    const auto stride=m.word(state.pool+0x24u),rows=m.word(state.pool+0x44u),row=rows+index*stride;
    state.cursor=index;const auto salt=m.word(row,2);state.datum=(salt<<16u)|index;state.current=row;
}
void exhaust_iterator(Memory&m,NativeTu15DataArrayIterator&state){
    state.cursor=m.word(state.pool+0x20u);state.datum=none;state.current=0;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using joint_constraint_numeric::Environment;
__m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
std::uint32_t bits(__m128 value){return joint_constraint_numeric::scalar_bits(value);}
std::uint32_t integer_float(NativeTu15PhysicsFrameFp&fp,unsigned depth,std::int32_t value){Environment scope(fp,depth);return bits(_mm_cvtsi32_ss(_mm_setzero_ps(),value));}
std::uint32_t divide(NativeTu15PhysicsFrameFp&fp,unsigned depth,std::uint32_t a,std::uint32_t b){Environment scope(fp,depth);auto x=scalar(a),y=scalar(b);asm volatile("divss %1,%0":"+x"(x):"x"(y):"memory");return bits(x);}
std::uint32_t add(NativeTu15PhysicsFrameFp&fp,unsigned depth,std::uint32_t a,std::uint32_t b){Environment scope(fp,depth);auto x=scalar(a),y=scalar(b);asm volatile("addss %1,%0":"+x"(x):"x"(y):"memory");return bits(x);}
std::uint32_t multiply(NativeTu15PhysicsFrameFp&fp,unsigned depth,std::uint32_t a,std::uint32_t b){Environment scope(fp,depth);auto x=scalar(a),y=scalar(b);asm volatile("mulss %1,%0":"+x"(x):"x"(y):"memory");return bits(x);}
std::uint32_t subtract(NativeTu15PhysicsFrameFp&fp,unsigned depth,std::uint32_t a,std::uint32_t b){Environment scope(fp,depth);auto x=scalar(a),y=scalar(b);asm volatile("subss %1,%0":"+x"(x):"x"(y):"memory");return bits(x);}
bool above(NativeTu15PhysicsFrameFp&fp,unsigned depth,std::uint32_t a,std::uint32_t b){Environment scope(fp,depth);unsigned char yes;const auto x=scalar(a),y=scalar(b);asm volatile("comiss %2,%1; seta %0":"=qm"(yes):"x"(x),"x"(y):"cc","memory");return yes;}
bool below_equal(NativeTu15PhysicsFrameFp&fp,unsigned depth,std::uint32_t a,std::uint32_t b){Environment scope(fp,depth);unsigned char yes;const auto x=scalar(a),y=scalar(b);asm volatile("comiss %2,%1; setbe %0":"=qm"(yes):"x"(x),"x"(y):"cc","memory");return yes;}
std::int32_t rounded_integer(NativeTu15PhysicsFrameFp&fp,unsigned depth,std::uint32_t input){Environment scope(fp,depth);std::int32_t out;asm volatile("flds %1; fistpl %0":"=m"(out):"m"(input):"memory","st");return out;}
#endif
}

bool native_custom_player_iterator_advance_tu15(SourcePhysicsContactMemory&memory,NativeTu15DataArrayIterator&state){
    const SourcePhysicsContactRecordObserver no_observer;Memory m(memory,no_observer);
    const auto index=native_tu15_data_array_next(memory,state.pool,signed32(state.cursor+1u));
    if(index==none)exhaust_iterator(m,state);else assign_iterator(m,state,index);
    while(state.current){
        if(!(m.word(state.current+2u,1)&2u))return true;
        auto next=state.cursor+1u;bool found=false;
        if(signed32(next)>=0){
            const auto limit=m.word(state.pool+0x38u);
            if(signed32(next)<signed32(limit)){
                const auto bitmap=m.word(state.pool+0x48u);
                do{if(m.word(bitmap+std::uint32_t(signed32(next)>>5)*4u)&(1u<<(next&31u))){found=true;break;}++next;}while(signed32(next)<signed32(limit));
            }
        }
        if(!found||next==none)exhaust_iterator(m,state);else assign_iterator(m,state,next);
    }
    return false;
}
bool native_custom_player_shield_allowed_tu15(SourcePhysicsContactMemory&memory,std::uint32_t player){
    const SourcePhysicsContactRecordObserver no_observer;Memory m(memory,no_observer);
    if(!selected(m,m.word(0x4f63b8u))||player==none)return true;
    const auto game=m.word(0x4f3218u);
    if(m.word(game+0x180u)==7u){const auto mask=m.word(0x51df9cu);if((m.word(mask,2)&(1u<<(player&31u)))&&(m.word(game+0x22cu,1)&2u))return true;}
    return m.word(game+0x1c4u)!=1u;
}
bool native_custom_team_active_tu15(SourcePhysicsContactMemory&memory,std::uint32_t team){
    const SourcePhysicsContactRecordObserver no_observer;Memory m(memory,no_observer);const auto engine=m.word(0x4f63b8u);
    if(!selected(m,engine))return false;const auto game=m.word(0x4f3218u);if(!(m.word(game+0x184u,1)&1u))return false;
    if(signed32(team)<0||signed32(team)>=8)return false;return (m.word(engine+0xcu,2)&(1u<<(team&31u)))!=0;
}
bool native_custom_player_respawn_allowed_tu15(SourcePhysicsContactMemory&memory,std::uint32_t player){
    const SourcePhysicsContactRecordObserver no_observer;Memory m(memory,no_observer);const auto row=player_row(m,player),flags=m.word(row+2u,2);
    if(!(flags&1u)||(flags&2u)||m.word(row+0xc0u,1)==0xffu)return false;
    if(m.word(row+0x2cu)!=none)return true;if(flags&0x4000u)return false;return !exhausted(m,player);
}
std::uint32_t native_custom_publish_roster_count_tu15(SourcePhysicsContactMemory&memory,std::uint32_t count,
 const NativeCustomMechanicsFrameServices&s,const SourcePhysicsContactRecordObserver&observe,NativeCustomMechanicsFrameStats*stats){
    Memory m(memory,observe);const auto engine=m.word(0x4f63b8u);const bool large=signed32(count)>8;
    m.word(0x15ecc8u,engine+0xc0cu,large);
    m.mask(large?NativeCustomRosterMaskRequest{0x15ecd0u,engine+0xc10u,2,0xffffu,1u}:
                 NativeCustomRosterMaskRequest{0x15ecd9u,engine+0xc10u,1,0xfeu,0u},s);
    if(stats)++stats->roster_mask_writes;return engine;
}

void native_custom_mechanics_frame_tu15(SourcePhysicsContactMemory&memory,NativeTu15PhysicsFrameFp&fp,
 unsigned depth,const NativeCustomMechanicsFrameServices&s,const SourcePhysicsContactRecordObserver&observe,
 NativeCustomMechanicsFrameStats*stats){
    Memory m(memory,observe);if(stats)++stats->calls;
    const auto refuse=[&](std::uint32_t owner){if(stats)++stats->refusals;throw std::runtime_error("Native custom frame requires qualified child owner="+std::to_string(owner));};
    const auto shared=[&](NativeCustomMechanicsSharedOwner owner,std::uint32_t argument=0){if(!s.shared)refuse(std::uint32_t(owner));s.shared(owner,argument,fp);if(stats)++stats->completed_children[std::uint32_t(owner)];};
    const auto player=[&](NativeCustomMechanicsPlayerOwner owner,std::uint32_t datum){if(!s.player)refuse(std::uint32_t(owner));s.player(owner,datum,fp);if(stats)++stats->completed_children[std::uint32_t(owner)];};
    if(!selected(m,m.word(0x4f63b8u)))return;if(stats)++stats->active_calls;
    //15E870 is custom round policy, not a dropped shared-mechanics owner.
    shared(NativeCustomMechanicsSharedOwner::shared_2482f0);
    shared(NativeCustomMechanicsSharedOwner::loose_weapons_15c4e0);
    shared(NativeCustomMechanicsSharedOwner::netgame_rows_15d890,m.word(0x4f63b8u)+0x7dcu);
    shared(NativeCustomMechanicsSharedOwner::local_roster_165860);
    if(m.word(m.word(0x4f3218u)+0xcu,1)!=4u)shared(NativeCustomMechanicsSharedOwner::teleporters_1a6830);
    NativeTu15DataArrayIterator iterator{0,m.word(0x4f54f4u),none,none};std::uint32_t count{};
    while(native_custom_player_iterator_advance_tu15(memory,iterator)){
        const auto datum=iterator.datum;++count;if(stats)++stats->eligible_players;
        if(!native_custom_player_shield_allowed_tu15(memory,datum)){
            const auto row=player_row(m,datum),unit=m.word(row+0x2cu);
            if(unit!=none){const auto objects=m.word(0x4ecbd0u),headers=m.word(objects+0x44u),body=m.word(headers+(unit&0xffffu)*12u+8u);m.word(0x15ec67u,body+0xf0u,0);}
        }
        player(NativeCustomMechanicsPlayerOwner::camouflage_15dc90,datum);
        player(NativeCustomMechanicsPlayerOwner::lifecycle_15e540,datum);
        player(NativeCustomMechanicsPlayerOwner::maintenance_1649b0,datum);
        const auto receiver=selected(m,m.word(0x4f63b8u)),table=m.word(receiver),owner=m.word(table+0x3cu);
        if(owner!=0x2e4d60u)refuse(owner);if(stats)++stats->native_virtual_noops;
        player(NativeCustomMechanicsPlayerOwner::engine_state_1654b0,datum);
    }
    auto engine=native_custom_publish_roster_count_tu15(memory,count,s,observe,stats);
    if(selected(m,engine)){
        const auto game=m.word(0x4f3218u);if(m.word(game+0x184u,1)&1u)
            for(std::uint32_t team=0;team<8;++team)if(native_custom_team_active_tu15(memory,team))shared(NativeCustomMechanicsSharedOwner::team_lifetime_15e4b0,team);
    }
    const auto receiver=selected(m,engine),table=m.word(receiver),tail_owner=m.word(table+0x48u);
    if(tail_owner!=0x31d400u)refuse(tail_owner);if(stats)++stats->native_virtual_noops;
    if(m.word(m.word(0x4f3218u)+0xcu,1)!=4u){
        shared(NativeCustomMechanicsSharedOwner::player_fanout_15ea50);
        //15E440 and2489F0 are custom winner/ranking policy.
    }
    shared(NativeCustomMechanicsSharedOwner::player_status_161610);
    if(s.phase)s.phase(NativeCustomMechanicsPhase::player_status_completed,fp);
    shared(NativeCustomMechanicsSharedOwner::countdown_161720);
    shared(NativeCustomMechanicsSharedOwner::player_flags_1617a0);
    if(s.phase)s.phase(NativeCustomMechanicsPhase::before_fade_tail,fp);
    native_custom_mechanics_frame_tail_tu15(memory,fp,depth,observe,stats);
}
void native_custom_mechanics_frame_tail_tu15(SourcePhysicsContactMemory&memory,NativeTu15PhysicsFrameFp&fp,
 unsigned depth,const SourcePhysicsContactRecordObserver&observe,NativeCustomMechanicsFrameStats*stats){
    Memory m(memory,observe);const auto engine=m.word(0x4f63b8u);
    bool reset=false;
    if(selected(m,engine)&&m.word(engine+0x6cu,2)==1u){const auto game=m.word(0x4f3218u);reset=m.word(game+0xcu,1)==4u||m.word(engine+0xc04u)==1u;}
    if(reset)m.word(0x15ed89u,engine+0xfau,0,2);
    else native_tu15_adjust_retained_counter(memory,0x15ed94u,engine+0xfau,2,1,observe);
    if(stats)++stats->frame_counter_writes;
    auto local=m.word(0x4f54f0u);std::uint32_t view{};
    while(view<4&&m.word(local+0xcu+view*4u)==none)++view;
    if(view==4)return;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto one=m.word(0x46a0d0u),one_half=m.word(0x449fd4u),three=m.word(0x46a0e4u);
    do{
        local=m.word(0x4f54f0u);const auto datum=m.word(local+0xcu+view*4u);
        if(datum!=none){
            const auto row=player_row(m,datum);const auto counter=signed16(m.word(engine+0xfau,2));
            const auto time=m.word(0x51df54u);const auto rate=signed16(m.word(time+2u,2));
            bool increase=counter>rate*4;
            if(!increase&&m.word(row+0x2cu)==none&&signed32(m.word(row+0x170u))<=0)
                increase=native_custom_player_respawn_allowed_tu15(memory,datum);
            if(increase){
                // Source reloads rate only after the15B470 child branch.
                const auto actual_rate=counter>rate*4?rate:signed16(m.word(m.word(0x51df54u)+2u,2));
                const auto converted=integer_float(fp,depth,actual_rate),delta=divide(fp,depth,one,converted);
                const auto value=add(fp,depth,delta,m.word(engine+0xe4u+view*4u));
                m.word(0x15ef49u,engine+0xe4u+view*4u,above(fp,depth,one,value)?value:one);if(stats)++stats->fade_stores;
            }else{
                bool decrease=false;
                if(!(m.word(engine+0xf4u,1)&(1u<<view))&&m.word(row+0x2cu)!=none)decrease=true;
                else if(m.word(row+0x2cu)==none&&!native_custom_player_respawn_allowed_tu15(memory,datum)){
                    const auto current_time=m.word(0x51df54u),start=m.word(engine+0x70u),tick=m.word(current_time+8u);const auto current_rate=signed16(m.word(current_time+2u,2));
                    const auto elapsed=tick-start;decrease=signed32(signed32(elapsed)<0?0u:elapsed)>current_rate;
                }
                if(decrease){
                    const auto current_time=m.word(0x51df54u),tick=m.word(current_time+8u);const auto current_rate=signed16(m.word(current_time+2u,2));
                    const auto converted=integer_float(fp,depth,current_rate),threshold=multiply(fp,depth,converted,one_half);
                    if(signed32(tick)>rounded_integer(fp,depth,threshold)){
                        const auto final_time=m.word(0x51df54u);const auto final_rate=signed16(m.word(final_time+2u,2));
                        const auto final_converted=integer_float(fp,depth,final_rate),step=divide(fp,depth,three,final_converted);
                        const auto value=subtract(fp,depth,m.word(engine+0xe4u+view*4u),step);
                        m.word(0x15ef49u,engine+0xe4u+view*4u,below_equal(fp,depth,0,value)?value:0);if(stats)++stats->fade_stores;
                    }
                }
            }
        }
        ++view;
        if(view<4){local=m.word(0x4f54f0u);while(view<4&&m.word(local+0xcu+view*4u)==none)++view;}
    }while(view<4);
#else
    (void)fp;(void)depth;throw std::runtime_error("Native custom frame requires its exact SSE/x87 fade backend");
#endif
}
}

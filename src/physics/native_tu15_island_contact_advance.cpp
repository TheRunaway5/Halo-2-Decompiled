#include "halo2/native_tu15_island_contact_advance.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;}
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));SourcePhysicsContactRecordStore store{site,p,{},{}};if(observer_){store.before.resize(4);memory_.read(p,store.before);store.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(store);}
};
bool positive(std::uint32_t value){return std::bit_cast<std::int32_t>(value)>0;}
template<class Function>void require(const Function&function,const char*why){if(!function)throw std::runtime_error(why);}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};unsigned host_mxcsr_{};NativeTu15PhysicsFrameFp&fp_;bool active_{};
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
        if(depth>6||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 island contact advance requires its unsupported FP owner");
        host_mxcsr_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 island contact advance requires its unsupported dependency FP owner");const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class Function>void dependency(Function&&function){publish();try{function();}catch(...){resume();throw;}resume();}
};
void push(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
void add(std::uint32_t bits){asm volatile("fadds %0"::"m"(bits):"memory","st");}
void subtract(std::uint32_t bits){asm volatile("fsubs %0"::"m"(bits):"memory","st");}
void multiply(std::uint32_t bits){asm volatile("fmuls %0"::"m"(bits):"memory","st");}
void absolute(){asm volatile("fabs":::"memory","st");}
void add_second(){asm volatile("fadd %%st(1),%%st(0)":::"memory","st","st(1)");}
std::uint32_t store(bool pop){std::uint32_t bits;if(pop)asm volatile("fstps %0":"=m"(bits)::"memory","st");else asm volatile("fsts %0":"=m"(bits)::"memory","st");return bits;}
std::uint16_t compare_pop(std::uint32_t bits){std::uint16_t status;asm volatile("fcomps %1\n\tfnstsw %0":"=a"(status):"m"(bits):"memory","st");return status;}
std::uint16_t compare_pop_pair(){std::uint16_t status;asm volatile("fucompp\n\tfnstsw %0":"=a"(status)::"memory","st","st(1)");return status;}
bool strict_less(std::uint16_t status){return((status>>8u)^(status>>10u))&1u;}
bool strict_greater(std::uint16_t status){return!(status&0x4100u);}
bool even_41(std::uint16_t status){return std::popcount(unsigned((status>>8u)&0x41u))%2u==0;}
// Three actual outcomes: controller admission/any distance below time (0),
// positive contacts all outside (1), or an empty signed contact span (2).
unsigned classify(Memory&m,NativeTu15PhysicsIsland island,NativeTu15IslandContactOutput output,std::uint32_t time){
    const auto world=m.word(island.identity+0x20u),controller=m.word(world+0x254u);if(!m.word(controller+0xcu))return 0;
    const auto header=m.word(output.identity),count=m.word(header+4u);if(!positive(count))return 2;
    auto cursor=m.word(header)+12u;for(std::uint32_t n=0;n<count;++n,cursor+=48u){push(m.word(cursor));if(strict_less(compare_pop(time)))return 0;}return 1;
}
bool any_below(Memory&m,NativeTu15IslandContactOutput output,std::uint32_t time){
    const auto header=m.word(output.identity),count=m.word(header+4u);if(!positive(count))return false;
    auto cursor=m.word(header)+12u;for(std::uint32_t n=0;n<count;++n,cursor+=48u){push(m.word(cursor));if(strict_less(compare_pop(time)))return true;}return false;
}
#endif
}
void native_tu15_advance_island_contact_time(SourcePhysicsContactMemory&memory,NativeTu15PhysicsIsland island,NativeTu15PhysicsStepConfiguration configuration,NativeTu15IslandAgentEntry entry,NativeTu15IslandContactOutput output,std::uint32_t time,NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15IslandContactAdvanceServices&s,const SourcePhysicsContactRecordObserver&observer,NativeTu15IslandContactAdvanceStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Memory m(memory,observer);if(stats)++stats->calls;Environment environment(fp,depth);
    const auto first_collidable=m.word(entry.identity),second_collidable=m.word(entry.identity+4u);
    const auto first_body=m.word(first_collidable+0x10u),second_body=m.word(second_collidable+0x10u);
    const NativeTu15PhysicsMotion second{m.word(second_body+0x3cu)},first{m.word(first_body+0x3cu)};
    const auto first_time=m.word(first.identity+0x7cu),second_time=m.word(second.identity+0x7cu);
    push(first_time);const auto initial_compare=compare_pop(second_time);const auto initial_minimum=strict_less(initial_compare)?first_time:second_time;
    push(second_time);push(first_time);const bool equal=(compare_pop_pair()&0x4400u)==0x4000u;
    const auto motion=[&](NativeTu15PhysicsMotion receiver,std::uint32_t value){const NativeTu15MotionAdvanceConfigurationWord input{m.word(configuration.identity+0x10u)};const auto table=m.word(receiver.identity);const NativeTu15PhysicsVirtualOwner owner{m.word(table+0x74u)};require(s.advance_motion,"Native TU1.5 contact advance requires its actual motion vtable+74 owner");if(stats)++stats->motion_calls;environment.dependency([&]{s.advance_motion(receiver,owner,value,input,fp);});};
    const auto process=[&]{const auto second_body_now=m.word(entry.identity+4u)-16u;const NativeTu15IslandCollisionAgent agent{m.word(entry.identity+8u)};const auto table=m.word(agent.identity);const auto first_body_now=m.word(entry.identity)-16u;const NativeTu15PhysicsVirtualOwner owner{m.word(table+0x10u)};require(s.process_agent,"Native TU1.5 contact advance requires its actual agent vtable+10 owner");if(stats)++stats->process_calls;environment.dependency([&]{s.process_agent(agent,owner,{first_body_now},{second_body_now},configuration,output,fp);});};
    const auto clear=[&](std::uint32_t site){const auto header=m.word(output.identity);m.word(site,header+4u,0);};
    const auto controller=[&](std::uint32_t mode_site,std::uint32_t threshold_site,std::uint32_t threshold){const auto world=m.word(island.identity+0x20u),policy=m.word(world+0x254u);m.word(mode_site,policy+0xcu,2);const auto current_world=m.word(island.identity+0x20u),current_policy=m.word(current_world+0x254u);m.word(threshold_site,current_policy+0x10u,threshold);};
    if(!equal){
        push(first_time);const auto ordered=compare_pop(second_time);const auto selected=strict_greater(ordered)?first:second;
        motion(selected,initial_minimum);clear(0x320647u);controller(0x320657u,0x32066bu,time);process();
        const auto initial=classify(m,island,output,time);if(initial==1)return;
        if(initial==2){
            push(first_time);const auto world=m.word(island.identity+0x20u);subtract(second_time);absolute();multiply(m.word(0x46a0ccu));auto gap=store(false);push(initial_minimum);add_second();auto candidate=store(true);bool iterate=strict_greater(compare_pop(m.word(world+0x24cu)));
            while(iterate){
                if(stats)++stats->single_motion_iterations;motion(selected,candidate);clear(0x320744u);controller(0x320754u,0x320768u,time);process();const auto outcome=classify(m,island,output,time);if(outcome==1)return;
                push(gap);multiply(m.word(0x46a0ccu));gap=store(true);push(candidate);if(outcome==0)subtract(gap);else add(gap);candidate=store(true);
                const auto current_world=m.word(island.identity+0x20u);push(gap);iterate=strict_greater(compare_pop(m.word(current_world+0x24cu)));
            }
            const auto current_world=m.word(island.identity+0x20u),policy=m.word(current_world+0x254u);const bool negative=!m.word(policy+0xcu)||any_below(m,output,time);
            if(negative){if(stats)++stats->negative_single_restores;motion(selected,initial_minimum);m.word(0x3208a5u,entry.identity+0xcu,0xfffffffdu);clear(0x3208aeu);controller(0x3208beu,0x3208ceu,0xc47a0000u);process();return;}
        }
    }
    // Source rereads both current motion times after any single-motion work.
    push(m.word(first.identity+0x7cu));const auto current_compare=compare_pop(m.word(second.identity+0x7cu));push(m.word((strict_less(current_compare)?first:second).identity+0x7cu));multiply(m.word(0x46a0ccu));const auto world=m.word(island.identity+0x20u);auto candidate=store(false);auto gap=store(false);bool iterate=strict_greater(compare_pop(m.word(world+0x24cu)));
    while(iterate){
        if(stats)++stats->paired_motion_iterations;motion(first,candidate);motion(second,candidate);controller(0x320949u,0x32095du,time);clear(0x320962u);process();const auto outcome=classify(m,island,output,time);if(outcome==1)return;
        push(gap);multiply(m.word(0x46a0ccu));if(outcome==0){gap=store(true);push(candidate);subtract(gap);}else{gap=store(false);add(candidate);}candidate=store(true);
        const auto current_world=m.word(island.identity+0x20u);push(gap);iterate=strict_greater(compare_pop(m.word(current_world+0x24cu)));
    }
    const auto current_world=m.word(island.identity+0x20u);push(gap);if(even_41(compare_pop(m.word(current_world+0x24cu))))return;
    const auto policy=m.word(current_world+0x254u);if(m.word(policy+0xcu)&&!any_below(m,output,time))return;
    if(stats)++stats->negative_paired_restores;m.word(0x320aa8u,entry.identity+0xcu,0xfffffffdu);motion(first,first_time);motion(second,second_time);
    const auto final_world=m.word(island.identity+0x20u),final_policy=m.word(final_world+0x254u);m.word(0x320ad8u,final_policy+0x10u,0xc47a0000u);clear(0x320ae1u);process();
#else
    (void)memory;(void)island;(void)configuration;(void)entry;(void)output;(void)time;(void)fp;(void)depth;(void)s;(void)observer;(void)stats;throw std::runtime_error("Exact native TU1.5 island contact advance backend unavailable");
#endif
}
} // namespace halo2

#include "halo2/native_tu15_island_agent_phase.hpp"
#include "halo2/native_tu15_constraint_step.hpp"
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
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    void float_word(std::uint32_t site,std::uint32_t p,bool pop){
        SourcePhysicsContactRecordStore event{site,p,{},{}};if(observer_){event.before.resize(4);memory_.read(p,event.before);}
        alignas(16)std::array<std::byte,108>prior{};std::uint32_t value;
        asm volatile("fnsave %0\n\tfrstor %0":"=m"(prior)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        if(pop)asm volatile("fstps %0":"=m"(value)::"memory","st");else asm volatile("fsts %0":"=m"(value)::"memory","st");
        std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));try{memory_.write(p,b);}catch(...){asm volatile("frstor %0"::"m"(prior):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}
        if(observer_){event.written.assign(b.begin(),b.end());observer_(event);}
    }
#endif
    NativeTu15PhysicsVirtualOwner owner(std::uint32_t object,std::uint32_t slot)const{return{word(word(object)+slot)};}
};
bool signed_less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
std::uint32_t storage_bytes(std::uint32_t capacity){return(capacity*48u+15u)&0xfffffff0u;}
template<class Function>void require(const Function&fn,const char*why){if(!fn)throw std::runtime_error(why);}
void profile(Memory&m,bool ending,const NativeTu15IslandAgentPhaseServices&s,bool flag_set=false){
    std::uint32_t cursor,limit;if(ending){cursor=m.word(0x48c974u);limit=m.word(0x48c978u);}else{limit=m.word(0x48c978u);cursor=m.word(0x48c974u);}if(cursor>=limit)return;
    m.word(flag_set?(ending?0x321ceau:0x321aa1u):(ending?0x321e1fu:0x321d58u),cursor,ending?0x41c394u:0x41f0d8u);
    require(s.timestamp_counter,"Native TU1.5 island agent phase requires its actual timestamp input");const auto ticks=s.timestamp_counter();
    m.word(flag_set?(ending?0x321cfau:0x321ab1u):(ending?0x321e2fu:0x321d68u),cursor+4u,std::uint32_t(ticks));m.word(flag_set?(ending?0x321d00u:0x321ab7u):(ending?0x321e35u:0x321d6eu),0x48c974u,cursor+12u);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};unsigned host_mxcsr_{};NativeTu15PhysicsFrameFp&fp_;bool active_{};
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
        if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 island agent phase requires its unsupported FP owner");
        host_mxcsr_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 island agent phase requires its unsupported FP dependency owner");const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class Function>void dependency(Function&&function){publish();try{function();}catch(...){resume();throw;}resume();}
};
void push_integer(std::uint32_t bits){const auto value=std::bit_cast<std::int32_t>(bits);asm volatile("fildl %0"::"m"(value):"memory","st");}
void push_float(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
void pop_float(){asm volatile("fstp %%st(0)":::"memory","st");}
void multiply(std::uint32_t bits){asm volatile("fmuls %0"::"m"(bits):"memory","st");}
void add(std::uint32_t bits){asm volatile("fadds %0"::"m"(bits):"memory","st");}
std::uint16_t compare(std::uint32_t bits,bool pop){std::uint16_t status;if(pop)asm volatile("fcomps %1\n\tfnstsw %0":"=a"(status):"m"(bits):"memory","st");else asm volatile("fcoms %1\n\tfnstsw %0":"=a"(status):"m"(bits):"memory","st");return status;}
bool less_than(std::uint16_t status){return((status>>8u)^(status>>10u))&1u;}
void threshold(Memory&m,std::uint32_t world,std::uint32_t count_address){push_integer(m.word(count_address));multiply(m.word(world+0x248u));add(m.word(world+0x244u));}
// Actual320510 remains in the native x87 expression: its true scalar return
// stays live for the caller's FCOMP. There is no lifted-return-state adapter.
void minimum_distance(Memory&m,NativeTu15IslandContactOutput output){
    push_float(m.word(0x41f064u));const auto header=m.word(output.identity);const auto count=m.word(header+4u);std::uint32_t processed{};
    const auto candidate=[&](std::uint32_t address){if(!(compare(m.word(address),false)&0x4100u)){pop_float();push_float(m.word(address));}};
    if(!signed_less(count,4u)){auto cursor=m.word(header)+0x3cu;auto groups=((count-4u)>>2u)+1u;processed=groups*4u;do{candidate(cursor-0x30u);candidate(cursor);candidate(cursor+0x30u);candidate(cursor+0x60u);cursor+=0xc0u;--groups;}while(groups);}
    if(signed_less(processed,count)){auto cursor=m.word(header)+processed*48u+12u;auto remaining=count-processed;do{candidate(cursor);cursor+=48u;--remaining;}while(remaining);}
}
#endif
}
NativeTu15IslandContactMinimum native_tu15_minimum_island_contact_distance(SourcePhysicsContactMemory&memory,NativeTu15IslandContactOutput output,NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const SourcePhysicsContactRecordObserver none;Memory m(memory,none);Environment environment(fp,depth);minimum_distance(m,output);environment.publish();NativeTu15IslandContactMinimum result{};asm volatile("fstpt %0":"=m"(result.extended_value)::"memory","st");return result;
#else
    (void)memory;(void)output;(void)fp;(void)depth;throw std::runtime_error("Exact native TU1.5 island contact minimum backend unavailable");
#endif
}
void native_tu15_construct_island_contact_buffer(SourcePhysicsContactMemory&memory,NativeTu15IslandContactBuffer buffer,std::uint32_t capacity,NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandAgentPhaseServices&s,const SourcePhysicsContactRecordObserver&observer,NativeTu15IslandAgentPhaseStats*stats){
    Memory m(memory,observer);if(stats)++stats->constructed_buffers;const auto p=buffer.identity;
    m.word(0x3211a0u,p,0);m.word(0x3211a2u,p+4u,0);m.word(0x3211a5u,p+8u,0x80000000u);
    const NativeTu15ConstraintStackAllocator allocator{m.word(0x48c938u)};const auto remaining=m.word(allocator.identity+0xcu);const auto bytes=storage_bytes(capacity);std::uint32_t data;
    if(signed_less(remaining,bytes)){const auto owner=m.owner(allocator.identity,0x24u);require(s.allocate_stack_block,"Native TU1.5 contact buffer requires its actual stack allocation owner");if(stats)++stats->allocated_stack_blocks;data=s.allocate_stack_block(allocator,owner,bytes,fp);}
    else{data=m.word(allocator.identity+8u);m.word(0x3211ddu,allocator.identity+8u,data+bytes);m.word(0x3211e5u,allocator.identity+0xcu,remaining-bytes);}
    m.word(0x3211f1u,p+8u,capacity|0x80000000u);m.word(0x3211f8u,p+0x10u,capacity);m.word(0x3211fbu,p,data);m.word(0x3211fdu,p+0xcu,data);
}
void native_tu15_destroy_island_contact_buffer(SourcePhysicsContactMemory&memory,NativeTu15IslandContactBuffer buffer,NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandAgentPhaseServices&s,const SourcePhysicsContactRecordObserver&observer,NativeTu15IslandAgentPhaseStats*stats){
    Memory m(memory,observer);if(stats)++stats->destroyed_buffers;const auto p=buffer.identity;const auto bytes=storage_bytes(m.word(p+0x10u));const auto original=m.word(p+0xcu);const NativeTu15ConstraintStackAllocator allocator{m.word(0x48c938u)};
    bool release=false;if(bytes){const auto marker=m.word(allocator.identity+0x14u);release=marker==original||marker==0;}
    if(release){const auto owner=m.owner(allocator.identity,0x28u);require(s.release_stack_block,"Native TU1.5 contact buffer requires its actual stack release owner");if(stats)++stats->released_stack_blocks;s.release_stack_block(allocator,owner,original,bytes,fp);}
    else{const auto data=m.word(allocator.identity+8u);const auto remaining=m.word(allocator.identity+0xcu);m.word(0x32127eu,allocator.identity+8u,data-bytes);m.word(0x321281u,allocator.identity+0xcu,remaining+bytes);}
    const auto capacity=m.word(p+8u);if(std::bit_cast<std::int32_t>(capacity)<0)return;
    const NativeTu15ConstraintStackAllocator grown_allocator{m.word(0x48c938u)};const auto table=m.word(grown_allocator.identity);const auto grown_bytes=(capacity&0x7fffffffu)*48u;const auto data=m.word(p);const NativeTu15PhysicsVirtualOwner owner{m.word(table+0x14u)};
    require(s.release_owned_contacts,"Native TU1.5 contact buffer requires its actual grown storage release owner");if(stats)++stats->released_owned_buffers;s.release_owned_contacts(grown_allocator,owner,data,grown_bytes,0x12u,fp);
}
void native_tu15_collide_island_flag_clear(SourcePhysicsContactMemory&memory,NativeTu15PhysicsIsland island,NativeTu15PhysicsStepConfiguration configuration,NativeTu15IslandAgentPhaseScratch scratch,NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandAgentPhaseServices&s,const SourcePhysicsContactRecordObserver&observer,NativeTu15IslandAgentPhaseStats*stats){
    Memory m(memory,observer);if(stats)++stats->flag_clear_calls;profile(m,false,s);
    native_tu15_construct_island_contact_buffer(memory,scratch.buffer,0x40u,fp,s,observer,stats);
    auto entry=m.word(island.identity+0x68u);const auto count=m.word(island.identity+0x6cu);m.word(0x321d96u,scratch.output.identity,scratch.buffer.identity);
    if(!signed_less(count-1u,0u))for(auto remaining=count;remaining;--remaining,entry+=16u){
        if(!m.word(entry+8u))continue;const auto header=m.word(scratch.output.identity);m.word(0x321dacu,header+4u,0);
        const auto second=m.word(entry+4u)-16u;const auto first=m.word(entry)-16u;const NativeTu15IslandCollisionAgent agent{m.word(entry+8u)};const auto owner=m.owner(agent.identity,0x10u);
        require(s.process_agent,"Native TU1.5 island agent phase requires its actual collision agent owner");if(stats)++stats->process_calls;s.process_agent(agent,owner,{first},{second},configuration,scratch.output,fp);
        const auto actual_header=m.word(scratch.output.identity);if(!m.word(actual_header+4u))continue;
        const auto actual_agent=m.word(entry+8u);const NativeTu15IslandCollisionAgent finisher{m.word(actual_agent+8u)};const auto finish_owner=m.owner(finisher.identity,0xcu);
        require(s.finish_agent,"Native TU1.5 island agent phase requires its actual contact manager owner");if(stats)++stats->finish_calls;s.finish_agent(finisher,finish_owner,{first},{second},configuration,scratch.output,fp);
    }
    native_tu15_destroy_island_contact_buffer(memory,scratch.buffer,fp,s,observer,stats);profile(m,true,s);
}
void native_tu15_collide_island_flag_set(SourcePhysicsContactMemory&memory,NativeTu15PhysicsIsland island,NativeTu15PhysicsStepConfiguration configuration,NativeTu15IslandAgentPhaseScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15IslandAgentPhaseServices&s,const SourcePhysicsContactRecordObserver&observer,NativeTu15IslandAgentPhaseStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Memory m(memory,observer);if(stats)++stats->flag_set_calls;profile(m,false,s,true);native_tu15_construct_island_contact_buffer(memory,scratch.buffer,64u,fp,s,observer,stats);
    const auto count=m.word(island.identity+0x6cu);auto entry=m.word(island.identity+0x68u);m.word(0x321adbu,scratch.output.identity,scratch.buffer.identity);
    if(!signed_less(count-1u,0u))for(auto remaining=count;remaining;--remaining,entry+=16u){
        if(!m.word(entry+8u))continue;const auto header=m.word(scratch.output.identity);m.word(0x321b04u,header+4u,0);const auto rank=m.word(entry+0xcu);m.word(0x321b15u,scratch.output.identity+4u,rank);Environment environment(fp,depth);
        if(!signed_less(rank,0u)){
            auto world=m.word(island.identity+0x20u);threshold(m,world,scratch.output.identity+4u);auto controller=m.word(world+0x254u);m.word(0x321b39u,controller+0xcu,2);
            world=m.word(island.identity+0x20u);controller=m.word(world+0x254u);m.float_word(0x321b45u,scratch.output.identity+4u,false);m.float_word(0x321b49u,controller+0x10u,true);
        }else{
            auto world=m.word(island.identity+0x20u);auto controller=m.word(world+0x254u);m.word(0x321c1au,controller+0xcu,2);world=m.word(island.identity+0x20u);controller=m.word(world+0x254u);m.word(0x321c26u,controller+0x10u,0xc47a0000u);
        }
        auto second=m.word(entry+4u)-16u;auto first=m.word(entry)-16u;const NativeTu15IslandCollisionAgent agent{m.word(entry+8u)};const auto owner=m.owner(agent.identity,0x10u);require(s.process_agent,"Native TU1.5 island agent phase requires its actual collision agent owner");if(stats)++stats->process_calls;
        environment.dependency([&]{s.process_agent(agent,owner,{first},{second},configuration,scratch.output,fp);});
        if(!signed_less(rank,0u)){
            const auto world=m.word(island.identity+0x20u);const auto controller=m.word(world+0x254u);bool advance=m.word(controller+0xcu)==0;
            if(!advance){const auto actual_header=m.word(scratch.output.identity);const auto contacts=m.word(actual_header+4u);if(signed_less(0u,contacts)){auto cursor=m.word(actual_header)+12u;for(std::uint32_t n=0;signed_less(n,contacts);++n,cursor+=48u){push_float(m.word(cursor));if(less_than(compare(m.word(scratch.output.identity+4u),true))){advance=true;break;}}}}
            if(advance){const auto actual_rank=m.word(entry+0xcu);push_float(m.word(scratch.output.identity+4u));m.word(0x321be9u,entry+0xcu,actual_rank+1u);const auto actual_world=m.word(island.identity+0x20u);add(m.word(actual_world+0x248u));m.float_word(0x321bf9u,scratch.output.identity+4u,true);const auto time=m.word(scratch.output.identity+4u);require(s.advance_contact_time,"Native TU1.5 island agent phase requires actual3205B0 owner");if(stats)++stats->advance_time_calls;environment.dependency([&]{s.advance_contact_time(island,configuration,{entry},scratch.output,time,fp);});}
            else{const auto actual_rank=m.word(entry+0xcu);if(signed_less(0u,actual_rank)){const auto lower=actual_rank-1u;const auto actual_world=m.word(island.identity+0x20u);m.word(0x321ba9u,scratch.output.identity+4u,lower);threshold(m,actual_world,scratch.output.identity+4u);m.float_word(0x321bc1u,scratch.output.identity+4u,true);if(stats)++stats->native_minimum_calls;minimum_distance(m,scratch.output);if(!(compare(m.word(scratch.output.identity+4u),true)&0x4100u))m.word(0x321bd9u,entry+0xcu,lower);}}
        }else if(m.word(entry+0xcu)==0xfffffffeu){
            const auto world=m.word(island.identity+0x20u);const auto controller=m.word(world+0x254u);if(m.word(controller+0xcu)){
                const auto actual_header=m.word(scratch.output.identity);const auto contacts=m.word(actual_header+4u);bool keep=false;
                if(signed_less(0u,contacts)){auto cursor=m.word(actual_header)+12u;for(std::uint32_t n=0;signed_less(n,contacts);++n,cursor+=48u){push_float(m.word(cursor));if(less_than(compare(m.word(0x46a0e8u),true))){keep=true;break;}}}
                if(!keep)m.word(0x321c87u,entry+0xcu,0);
            }
        }
        const auto actual_agent=m.word(entry+8u);const NativeTu15IslandCollisionAgent finisher{m.word(actual_agent+8u)};second=m.word(entry+4u)-16u;first=m.word(entry)-16u;const auto finish_owner=m.owner(finisher.identity,0xcu);require(s.finish_agent,"Native TU1.5 island agent phase requires its actual contact manager owner");if(stats)++stats->finish_calls;environment.dependency([&]{s.finish_agent(finisher,finish_owner,{first},{second},configuration,scratch.output,fp);});
    }
    native_tu15_destroy_island_contact_buffer(memory,scratch.buffer,fp,s,observer,stats);profile(m,true,s,true);
#else
    (void)memory;(void)island;(void)configuration;(void)scratch;(void)fp;(void)depth;(void)s;(void)observer;(void)stats;throw std::runtime_error("Exact native TU1.5 island agent phase backend unavailable");
#endif
}
} // namespace halo2

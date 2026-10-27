#include "halo2/native_tu15_island_deactivation.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
using Period=NativeTu15DeactivationPeriod;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16) std::array<std::byte,108> host_{};
    std::array<std::byte,28> native_{};
    unsigned host_mxcsr_{};
    NativeTu15PhysicsFrameFp&fp_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
        if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
           (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 deactivation requires unsupported floating-point fault ownership");
        host_mxcsr_=_mm_getcsr();
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),native_.size());const std::uint16_t empty=0xffff;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){
        asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);
    }
};
void load(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
void add(std::uint32_t bits){asm volatile("fadds %0"::"m"(bits):"memory","st");}
void subtract(std::uint32_t bits){asm volatile("fsubs %0"::"m"(bits):"memory","st");}
std::uint32_t pop(){std::uint32_t bits;asm volatile("fstps %0":"=m"(bits)::"memory","st");return bits;}
bool greater(std::uint32_t bits){std::uint16_t status;asm volatile("fcomps %1\n\tfnstsw %0":"=a"(status):"m"(bits):"memory","st");return (status&0x4100u)==0;}
#endif
}
bool native_tu15_default_deactivation_policy()noexcept{return false;}
bool native_tu15_check_island_deactivation(NativeTu15IslandDeactivationStorage&s,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15DeactivationQuery&query){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    // Keep platform FP ownership local to arithmetic phases. A policy callback
    // receives the actual updated context, and may update it before resumption.
    const auto short_period=Period::short_period,long_period=Period::long_period;
    NativeTu15PhysicsWorld world;std::uint32_t next_long{};bool short_due{},long_due{};
    {
        Environment environment(fp,depth);
        load(s.seconds());add(s.elapsed(short_period));world=s.world();s.elapsed(short_period,pop(),false);
        load(s.elapsed(long_period));add(s.seconds());next_long=pop();
        load(s.elapsed(short_period));s.elapsed(long_period,next_long,false);short_due=greater(s.interval(world,short_period));
        load(next_long);long_due=greater(s.interval(world,long_period));
    }
    auto check_bodies=[&](Period period,std::uint32_t count){
        bool all=true;
        if(std::bit_cast<std::int32_t>(count)<=0)return true;
        std::uint32_t index{};
        do{
            const auto body=s.body(index);const auto policy=s.policy(body);
            if(!policy)all=false;
            else{
                // The original re-reads body+54 after its null decision.
                const auto actual=s.policy(body);const auto owner=s.policy_owner(actual,period);
                if(owner.identity==0x3233c0u){if(!native_tu15_default_deactivation_policy())all=false;}
                else{
                    if(!query)throw std::runtime_error("Native TU1.5 deactivation requires its body policy owner");
                    if(!query(actual,owner,body,period,fp))all=false;
                }
            }
            count=s.body_count();++index;
        }while(std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(count));
        return all;
    };
    auto finish=[&](bool value,std::uint32_t site){s.result(value,site);return value;};
    if(short_due){
        std::uint32_t count{};
        {Environment environment(fp,depth);load(s.elapsed(short_period));count=s.body_count();subtract(s.interval(world,short_period));s.elapsed(short_period,pop(),true);}
        if(!check_bodies(short_period,count))return finish(false,0x320d99u);
        const auto checks=s.short_check_count();
        if(std::bit_cast<std::int8_t>(checks)<5){s.short_check_count(std::uint8_t(checks+1u),false);return finish(false,0x320d28u);}
        s.short_check_count(0,true);return finish(true,0x320d39u);
    }
    if(long_due){
        std::uint32_t count{};
        {Environment environment(fp,depth);load(next_long);count=s.body_count();subtract(s.interval(world,long_period));s.elapsed(long_period,pop(),true);}
        if(check_bodies(long_period,count))return finish(true,0x320d39u);
    }
    return finish(false,0x320d99u);
#else
    (void)s;(void)fp;(void)depth;(void)query;throw std::runtime_error("Exact native TU1.5 deactivation backend unavailable");
#endif
}
NativeTu15IslandDeactivationMemory::NativeTu15IslandDeactivationMemory(SourcePhysicsContactMemory&m,NativeTu15PhysicsIsland island,NativeTu15PhysicsStepPacket step,std::uint32_t output,const SourcePhysicsContactRecordObserver&o):memory_(m),island_(island.identity),step_(step.identity),output_(output),observer_(o){}
std::uint32_t NativeTu15IslandDeactivationMemory::read(std::uint32_t p,unsigned n)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(b[i])<<(i*8u);return v;}
void NativeTu15IslandDeactivationMemory::write(std::uint32_t site,std::uint32_t p,std::uint32_t v,unsigned n){std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(i*8u));SourcePhysicsContactRecordStore store{site,p,{},{}};if(observer_){store.before.resize(n);memory_.read(p,store.before);store.written.assign(b.begin(),b.begin()+n);}memory_.write(p,std::span(b).first(n));if(observer_)observer_(store);}
std::uint32_t NativeTu15IslandDeactivationMemory::seconds()const{return read(step_);}
std::uint32_t NativeTu15IslandDeactivationMemory::elapsed(Period p)const{return read(island_+(p==Period::short_period?0x34u:0x38u));}
NativeTu15PhysicsWorld NativeTu15IslandDeactivationMemory::world()const{return{read(island_+0x20)};}
std::uint32_t NativeTu15IslandDeactivationMemory::interval(NativeTu15PhysicsWorld w,Period p)const{return read(w.identity+(p==Period::short_period?0x30u:0x34u));}
void NativeTu15IslandDeactivationMemory::elapsed(Period p,std::uint32_t value,bool wrapping){const bool short_period=p==Period::short_period;write(wrapping?(short_period?0x320cdeu:0x320d54u):(short_period?0x320c91u:0x320ca4u),island_+(short_period?0x34u:0x38u),value);}
std::uint32_t NativeTu15IslandDeactivationMemory::body_count()const{return read(island_+0x40);}
NativeTu15PhysicsBody NativeTu15IslandDeactivationMemory::body(std::uint32_t index)const{return{read(read(island_+0x3c)+index*4u)};}
std::uint32_t NativeTu15IslandDeactivationMemory::policy(NativeTu15PhysicsBody b)const{return read(b.identity+0x54);}
NativeTu15PhysicsVirtualOwner NativeTu15IslandDeactivationMemory::policy_owner(std::uint32_t p,Period period)const{return{read(read(p)+(period==Period::short_period?0x14u:0x18u))};}
std::uint8_t NativeTu15IslandDeactivationMemory::short_check_count()const{return std::uint8_t(read(island_+0x2c,1));}
void NativeTu15IslandDeactivationMemory::short_check_count(std::uint8_t v,bool reset){write(reset?0x320d2fu:0x320d1fu,island_+0x2c,v,1);}
void NativeTu15IslandDeactivationMemory::result(bool value,std::uint32_t site){write(site,output_,value,1);}
} // namespace halo2

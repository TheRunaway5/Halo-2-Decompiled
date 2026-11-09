#include "halo2/native_tu15_mopp_ray_traversal.hpp"
#include "halo2/native_tu15_mopp_ray_traversal_kernel.hpp"
#include <cstring>
#include <stdexcept>
#include <type_traits>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// This owns the host processor's numerical environment. Arithmetic below
// executes directly on native x87/SSE; there is no emulated register file.
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned host_mxcsr_;bool active_{};
    void validate(){
        if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||
            (fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))
            throw std::runtime_error("Native MOPP ray requires its FP exception owner");
    }
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),host_mxcsr_(_mm_getcsr()){
        if(depth)throw std::runtime_error("Native MOPP ray requires its balanced caller FP domain");validate();
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){validate();const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    // A DATA provider is ordinary host code. Keep live ray intermediates and
    // their complete FP environment across its normal return or exception.
    // Child services use dependency() instead, because they explicitly own fp.
    template<class F>void io(F&&f){
        struct Scope {
            alignas(16)std::array<std::byte,108>ray;
            unsigned mxcsr;
            explicit Scope(const Environment&e):mxcsr(_mm_getcsr()){
                asm volatile("fnsave %0;frstor %1":"=m"(ray):"m"(e.host_):"memory",
                    "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
                _mm_setcsr(e.host_mxcsr_);
            }
            ~Scope(){
                asm volatile("frstor %0"::"m"(ray):"memory",
                    "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
                _mm_setcsr(mxcsr);
            }
        }restore(*this);
        f();
    }
    template<class F>decltype(auto)dependency(F&&f){publish();try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){f();resume();return;}else{auto value=f();resume();return value;}}catch(...){resume();throw;}}
};

class Memory {
    Environment&environment_;SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(Environment&e,SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):environment_(e),memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};environment_.io([&]{memory_.read(p,std::span(b).first(width));});std::uint32_t n{};
        for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width=4){
        std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(value>>(k*8u));
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(width);environment_.io([&]{memory_.read(p,event.before);});event.written.assign(b.begin(),b.begin()+width);}
        environment_.io([&]{memory_.write(p,std::span(b).first(width));});if(observe_)environment_.io([&]{observe_(event);});
    }
    std::array<std::uint32_t,4> vector(std::uint32_t p)const{
        std::array<std::byte,16>b{};environment_.io([&]{memory_.read_vector(p,b);});std::array<std::uint32_t,4>v{};std::memcpy(v.data(),b.data(),16);return v;
    }
};
// The public value contract keeps all authored property writes locally. Only
// property zero is part of its returned traversal-variable DATA; the remaining
// words have a shorter lifetime and are overwritten by a later ray-body packet.
struct Variables:NativeTu15MoppTraversalVariables {
    std::array<std::uint32_t,3>local_properties{};
    explicit Variables(const NativeTu15MoppTraversalVariables&value):NativeTu15MoppTraversalVariables(value){}
    void property(unsigned slot,std::uint32_t value){
        if(slot==0u)fields10_1c[3]=value;
        else local_properties[slot-1u]=value;
    }
};

#endif
}
void native_tu15_traverse_mopp_ray(SourcePhysicsContactMemory&memory,
    std::uint32_t cache,std::uint32_t code,const NativeTu15MoppTraversalVariables&variables,
    NativeTu15MoppTraversalBounds&bounds,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15MoppRayTraversalServices&services,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15MoppRayTraversalStats*stats){
    if(stats)++stats->calls;try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth);Memory m(environment,memory,observe);native_mopp_ray_detail::Traversal walk(m,environment,fp,cache,services,stats);walk.run(code,Variables(variables),bounds);if(stats)++stats->completed;
#else
    (void)memory;(void)cache;(void)code;(void)variables;(void)bounds;(void)fp;(void)depth;(void)services;(void)observe;
    throw std::runtime_error("Native MOPP ray FP backend is unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2

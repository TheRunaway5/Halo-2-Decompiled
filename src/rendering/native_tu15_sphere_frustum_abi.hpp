#pragma once
#include "halo2/native_tu15_sphere_frustum_kernel.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "physics/runtime/native_tu15_sse_mmx_scope.hpp"
#include <cstdlib>
#include <optional>
#include <thread>

namespace halo2::native_sphere_frustum_abi_detail {
using namespace native_sphere_frustum_detail;
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_SPHERE_FRUSTUM");return p&&*p;}();return enabled;}
template<class State>bool policy_admitted(const State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    // A cleared callable revokes the cached fast-route witness. The exact
    // retained body skips cursor publication when its callback is empty.
    if(!s.services.instruction&&(s.instruction_cursor||s.literal_service_sites))return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*p=s.instruction_cursor;
        // State caches the initial fast-route witness. Recheck its live
        // callable, executor and protocol before retiring this body's hooks.
        if(p!=&s.services.instruction_cursor||!p->callback_type||
           s.services.instruction.target_type()!=*p->callback_type||
           p->executor!=std::this_thread::get_id()||!p->literal_service_sites||
           p->service_sites.size()!=3||p->service_sites[0]!=0x38846eu||
           p->service_sites[1]!=0x21f590u||p->service_sites[2]!=0x33b595u)return false;
        if((p->first_trace&&*p->first_trace)||(p->second_trace&&*p->second_trace))return false;
    }
    // The authenticated leaf has no stores or callees. Its observer is never
    // invoked, so no observer-purity witness is consumed by this classifier.
    return true;
#endif
}
template<class State>bool admitted(const State&s){return policy_admitted(s)&&(native_raw_x87_detail::supported(s)||native_sse_mmx_detail::supported(s));}
template<class State,class Before,class Scope>class Frame {
    State&s_;Before before_;Scope&scope_;
    struct Pending {Read kind;unsigned axis,plane;};std::optional<Pending>last_;
    static unsigned accumulator_register(unsigned plane){return std::array{2u,6u,5u,4u,0u,3u}[plane];}
    static unsigned scratch_register(unsigned plane){return plane==4?3u:0u;}
    static __m128 scalar(std::uint32_t word){const std::array<std::uint32_t,4>bits{word,0,0,0};return std::bit_cast<__m128>(bits);}
    void complete(const Work<__m128>&d){
        if(!last_)return;const auto pending=*last_;last_.reset();
        switch(pending.kind){
        case Read::lower_center:case Read::upper_center:s_.xmm[0]=d.scratch;break;
        case Read::lower_bound:s_.xmm[0]=d.scratch;s_.xmm[2]=d.bound;break;
        case Read::normal_z:case Read::center_z:s_.xmm[accumulator_register(pending.plane)]=d.accumulator;break;
        case Read::normal_y:case Read::center_y:case Read::normal_x:case Read::center_x:case Read::plane_distance:
            s_.xmm[scratch_register(pending.plane)]=d.scratch;s_.xmm[accumulator_register(pending.plane)]=d.accumulator;break;
        case Read::upper_bound:break;
        }
    }
    void at(std::uint32_t pc){
        if(s_.instruction_cursor&&s_.instruction_cursor->position)*s_.instruction_cursor->position=pc;
        scope_.io([&]{before_(s_,pc);});
    }
public:
    using Scalar=__m128;
    Frame(State&s,Before before,Scope&scope):s_(s),before_(std::move(before)),scope_(scope){}
    Scalar radius()const{return s_.xmm[1];}
    Scalar zero()const{return _mm_setzero_ps();}
    Scalar add(Scalar a,Scalar b)const{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;
#else
        return _mm_add_ss(a,b);
#endif
    }
    Scalar subtract(Scalar a,Scalar b)const{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;
#else
        return _mm_sub_ss(a,b);
#endif
    }
    Scalar multiply(Scalar a,Scalar b)const{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;
#else
        return _mm_mul_ss(a,b);
#endif
    }
    Scalar read(Read kind,const Work<Scalar>&d){
        complete(d);const auto axis=d.axis,plane=d.plane;std::uint32_t pc{},address{};std::optional<unsigned>destination;
        switch(kind){
        case Read::lower_center:pc=std::array{0x184690u,0x1846a2u,0x1846b5u}[axis];address=s_.gp[1]+axis*4u;destination=0;break;
        case Read::upper_bound:pc=std::array{0x184698u,0x1846abu,0x1846beu}[axis];address=s_.gp[0]+0x64u+axis*8u;break;
        case Read::upper_center:pc=std::array{0x1846c8u,0x1846e6u,0x1846fcu}[axis];address=s_.gp[1]+axis*4u;if(!axis)destination=0;break;
        case Read::lower_bound:pc=std::array{0x1846ccu,0x1846deu,0x1846f4u}[axis];address=s_.gp[0]+0x60u+axis*8u;destination=2;break;
        case Read::normal_z:pc=std::array{0x18470au,0x184743u,0x184782u,0x1847c1u,0x1847fcu,0x184837u}[plane];address=s_.gp[0]+0x80u+plane*16u;destination=accumulator_register(plane);break;
        case Read::center_z:pc=std::array{0x184712u,0x18474bu,0x18478au,0x1847c9u,0x184804u,0x18483fu}[plane];address=s_.gp[1]+8u;break;
        case Read::normal_y:pc=std::array{0x184717u,0x184750u,0x18478fu,0x1847ceu,0x184809u,0x184844u}[plane];address=s_.gp[0]+0x7cu+plane*16u;destination=scratch_register(plane);break;
        case Read::center_y:pc=std::array{0x18471cu,0x184758u,0x184797u,0x1847d6u,0x184811u,0x18484cu}[plane];address=s_.gp[1]+4u;break;
        case Read::normal_x:pc=std::array{0x184725u,0x184761u,0x1847a0u,0x1847dfu,0x18481au,0x184859u}[plane];address=s_.gp[0]+0x78u+plane*16u;if(plane!=5)destination=scratch_register(plane);break;
        case Read::center_x:pc=std::array{0x18472au,0x184769u,0x1847a8u,0x1847e7u,0x184822u,0x184855u}[plane];address=s_.gp[1];if(plane==5)destination=0;break;
        case Read::plane_distance:pc=std::array{0x184732u,0x184771u,0x1847b0u,0x1847efu,0x18482au,0x184865u}[plane];address=s_.gp[0]+0x84u+plane*16u;break;
        }
        at(pc);std::array<std::byte,4>bytes{};scope_.io([&]{s_.memory.read(address,bytes);});std::uint32_t word{};std::memcpy(&word,bytes.data(),4);const auto value=scalar(word);
        if(destination)s_.xmm[*destination]=value;last_=Pending{kind,axis,plane};return value;
    }
    bool above(Scalar left,Scalar right,Comparison kind,const Work<Scalar>&d){
        complete(d);if(kind==Comparison::contained)s_.xmm[0]=d.scratch;
        s_.flags.scalar_compare(left,right,false);return !s_.flags.cf&&!s_.flags.zf;
    }
    Visibility finish(Visibility value,const Work<Scalar>&d){
        complete(d);if(value==Visibility::outside)s_.gp[0]=s_.flags.logic(0,4);else s_.gp[0]=std::uint32_t(value);
        at(value==Visibility::outside?0x184874u:0x18489fu);std::array<std::byte,4>ignored{};
        scope_.io([&]{s_.memory.read(s_.gp[4],ignored);});s_.gp[4]+=4u;return value;
    }
};
template<class State,class Before=Noop>bool bridge(State&s,Before before={}){
    if(!admitted(s))return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx){
        native_sse_mmx_detail::Scope scope{s};Frame frame{s,std::move(before),scope};classify(frame);return true;
    }
    native_raw_x87_detail::Scope scope{s};Frame frame{s,std::move(before),scope};classify(frame);return true;
}
}

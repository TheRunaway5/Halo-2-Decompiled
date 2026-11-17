#pragma once
#include "halo2/native_tu15_packed_rotation_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_integer_fp_scope.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include <bit>

namespace halo2::native_packed_rotation_detail {
struct AlignedStoreFault{
    std::uint32_t instruction,address,execution_flags,saved_fault_flags;
};
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class Scope>struct Io{
    Scope*scope;
    template<class Callback>void operator()(Callback&&callback)const{scope->io(std::forward<Callback>(callback));}
};
template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>>{
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::io;using Base::before_;
    __m128 converted_high_{};
    void returned_boundary(std::uint32_t pc,bool memory){
        // A real estimate provider can revoke/rebind instruction policy.
        // Revalidate the current callable/cursor before each remaining source
        // observation instead of publishing the entry's cached position.
        io([&]{s_.instruction_literal(pc);});
        if(memory)io([&]{before_(s_,pc);});
    }
public:
    Frame(State&s,Before&before,Scope&scope):Base{s,before,{&scope}}{}
    __m128 input(){
        auto&g=s_.gp;g[4]=s_.flags.minus(g[4],8u,4);
        at(0x296183u);g[0]=word(0x510f94u);
        at(0x296188u);g[1]=word(0x510f60u);g[2]=g[1]+g[0]*8u+0x20u;
        at(0x296192u);g[0]=word(0x510fa0u);
        Base::store(0x296197u,g[4]+4u,g[2],[]{});
        Base::store(0x29619bu,g[4],g[0],[]{});
        at(0x29619eu);g[1]=word(g[4]+4u);
        at(0x2961a2u);g[0]=word(g[4]);
        at(0x2961a5u);std::array<std::byte,8>bytes{};io([&]{s_.memory.read(g[1],bytes);});
        std::array<std::int16_t,4>packed{};std::memcpy(packed.data(),bytes.data(),8);
        // Source PSRAD16 discards every unknown interleaved destination word.
        // Every surviving signed16 value converts exactly to binary32.
        const auto converted=_mm_cvtepi32_ps(_mm_set_epi32(packed[3],packed[2],packed[1],packed[0]));
        converted_high_=_mm_movelh_ps(_mm_movehl_ps(converted,converted),_mm_movehl_ps(s_.xmm[2],s_.xmm[2]));
        // No source I/O or callee occurs between MOVQ and EMMS. The completed
        // authored interval empties the file and leaves TOP at zero.
        s_.emms();s_.fp.top=0;
        return converted;
    }
    __m128 multiply(__m128 left,__m128 right)const{
        asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    __m128 add(__m128 left,__m128 right)const{
        asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    __m128 opposite(__m128 value)const{return _mm_shuffle_ps(value,value,78);}
    __m128 alternate(__m128 value)const{return _mm_shuffle_ps(value,value,17);}
    __m128 scale([[maybe_unused]] __m128 quaternion,__m128 inverse){
        s_.xmm[0]=inverse;returned_boundary(0x2961deu,false);
        return multiply(s_.xmm[1],s_.xmm[0]);
    }
    __m128 inverse(__m128 norm,__m128 quaternion,__m128 opposite,__m128 alternate){
        s_.xmm[0]=norm;s_.xmm[1]=quaternion;s_.xmm[2]=converted_high_;s_.xmm[3]=opposite;s_.xmm[4]=alternate;
        at(0x2961dbu);
        if(!s_.services.reciprocal_sqrt)throw std::runtime_error("Required original contact RSQRTSS owner unavailable");
        auto lanes=std::bit_cast<std::array<std::uint32_t,4>>(norm);
        for(auto&lane:lanes)io([&]{lane=s_.services.reciprocal_sqrt(lane);});
        return std::bit_cast<__m128>(lanes);
    }
    void output(__m128 value,__m128){
        s_.xmm[1]=value;returned_boundary(0x2961e1u,true);
        const auto output_address=s_.gp[0];
        if(output_address&15u){
            const auto flags=s_.other_flags|s_.flags.bits();
            throw AlignedStoreFault{0x2961e1u,output_address,flags,flags|0x10000u};
        }
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&s_.xmm[1],16);
        // Preserve the retained vw protocol. Hardware whole-vector write
        // atomicity and the first-half completed-observer boundary are distinct.
        for(unsigned offset:{0u,8u}){
            const auto address=output_address+offset;SourcePhysicsContactRecordStore event{0x2961e1u,address,{},{}};
            if(s_.observe){event.before.resize(8);io([&]{s_.memory.read(address,event.before);});}
            const auto part=std::span(bytes).subspan(offset,8);io([&]{s_.memory.write(address,part);});
            if(s_.observe){event.written.assign(part.begin(),part.end());io([&]{s_.observe(event);});}
        }
        returned_boundary(0x2961e4u,false);s_.gp[4]=s_.flags.plus(s_.gp[4],8u,4);
        returned_boundary(0x2961e7u,true);(void)word(s_.gp[4]);s_.gp[4]+=4u;
    }
};
// Arithmetic/frame entry; the separate source owner enforces admission.
template<class State,class Before=Noop>void run_private(State&s,Before before={}){
    native_integer_fp_detail::Scope scope{s};Frame view{s,before,scope};decode(view);
}
} // namespace halo2::native_packed_rotation_detail
#endif

#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"

namespace halo2::native_mopp_ray_setup_detail {

inline constexpr std::uint32_t setup_complete=0xffffffffu;

inline bool reference_setup() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_SETUP");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State>
class SetupProvider {
    State &s_;
    const typename State::InstructionCursor *cursor_;

    void publish(std::uint32_t pc) const {
        if(cursor_)*cursor_->position=pc;
    }
    template<class Value>
    bool read(std::uint32_t address,Value &value) const {
        return native_raw_x87_detail::direct_read(s_.memory,address,std::as_writable_bytes(std::span{&value,1}));
    }
    bool can_continue(unsigned depth) const {
        return s_.fp.count==depth && native_raw_x87_detail::supported(s_) &&
            native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_);
    }
    bool scalar(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4u,bool pop=false) {
        publish(pc);
        const auto depth=s_.fp.count;
        const auto bytes=std::as_bytes(std::span{&value,1}).first(width);
        const bool direct=s_.memory.try_callback_free_write(address,bytes);
        if(!direct)physics_step_detail::w(s_.memory,s_.observe,pc,address,width,value);
        if(pop)s_.fp.pop();
        return direct || can_continue(depth-unsigned(pop));
    }
    bool vector(std::uint32_t pc,std::uint32_t address,__m128 value) {
        const auto depth=s_.fp.count;
        publish(pc);
        physics_step_detail::vw(s_.memory,s_.observe,pc,address,value);
        return can_continue(depth);
    }
    bool push(std::uint32_t pc,std::uint32_t value) {
        s_.gp[4]-=4u;
        return scalar(pc,s_.gp[4],value);
    }
    bool pop(std::uint32_t &value) {
        if(!read(s_.gp[4],value))return false;
        s_.gp[4]+=4u;
        return true;
    }
public:
    SetupProvider(State &s,const typename State::InstructionCursor *cursor):s_(s),cursor_(cursor) {}

    // A refused read or changed provider policy returns the first unretired
    // source instruction. The caller keeps every completed write and the
    // original stack and x87 state, including during exceptions.
    template<class Child,class Spill>
    std::uint32_t run(Child child,Spill spill) {
        using namespace physics_step_detail;
        using native_tu15_leaves::ordered_mul_ps;
        auto &g=s_.gp;
        auto &x=s_.xmm;
        std::uint32_t word;
        if(!push(0x30b890u,g[5]))return 0x30b891u;
        g[5]=g[4];
        g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);
        g[4]=s_.flags.minus(g[4],0x48u,4);

        if(!read(g[5]+0x8u,g[0]))return 0x30b899u;
        if(!read(0x46a0d0u,word))return 0x30b89cu;
        s_.fp.push(ext(f(word)));
        if(!scalar(0x30b8a2u,g[1]+0x64u,g[0]))return 0x30b8a5u;
        if(!read(g[5]+0x14u,g[0]))return 0x30b8a5u;
        if(!scalar(0x30b8a8u,g[1]+0x60u,g[0]))return 0x30b8abu;
        if(!read(g[5]+0x18u,g[0]))return 0x30b8abu;
        if(!scalar(0x30b8aeu,g[1]+0x5cu,g[0]))return 0x30b8b1u;
        g[2]=s_.flags.logic(0,4);
        if(!scalar(0x30b8b3u,g[1]+0x58u,g[2]))return 0x30b8b6u;
        if(!read(g[0]+0x4u,g[0]))return 0x30b8b6u;
        if(!scalar(0x30b8b9u,g[1]+0x54u,g[0]))return 0x30b8bcu;
        if(!read(g[5]+0xcu,g[0]))return 0x30b8bcu;
        if(!scalar(0x30b8bfu,g[1]+0x10u,g[0]))return 0x30b8c2u;
        x[0]=_mm_setzero_ps();
        if(!vector(0x30b8c5u,g[4]+0x8u,x[0]))return 0x30b8cau;

        if(!read(g[0]+0xcu,word))return 0x30b8cau;
        s_.fp.st(0)=physics_solver_detail::divide(s_.fp.st(0),ext(f(word)));
        if(!push(0x30b8cdu,g[6]))return 0x30b8ceu;
        g[6]=g[0]+0x30u;
        if(!push(0x30b8d1u,g[7]))return 0x30b8d2u;
        publish(0x30b8d2u);
        if(!scalar(0x30b8d2u,g[1]+0x14u,spill(s_.fp.st(0),s_.fp.status),4u,true))return 0x30b8d5u;
        if(!read(g[0]+0xcu,word))return 0x30b8d5u;
        s_.fp.push(ext(f(word)));
        if(!read(g[0],x[0]))return 0x30b8d8u;
        if(!read(0x46a1bcu,word))return 0x30b8dbu;
        s_.fp.st(0)=mul(s_.fp.st(0),ext(f(word)));
        if(!read(g[5]+0x10u,g[0]))return 0x30b8e1u;
        if(!read(g[0],x[3]))return 0x30b8e4u;
        if(!read(g[0]+0x10u,x[2]))return 0x30b8e7u;
        x[2]=_mm_sub_ps(x[2],x[0]);
        publish(0x30b8eeu);
        if(!scalar(0x30b8eeu,g[4]+0x24u,spill(s_.fp.st(0),s_.fp.status)))return 0x30b8f2u;
        x[1]=_mm_sub_ps(x[3],x[0]);
        publish(0x30b8f8u);
        if(!scalar(0x30b8f8u,g[4]+0xcu,spill(s_.fp.st(0),s_.fp.status),4u,true))return 0x30b8fcu;
        if(!read(g[4]+0xcu,word))return 0x30b8fcu;
        x[0]=_mm_set_ss(f(word));
        x[4]=ordered_mul_ps(_mm_shuffle_ps(x[0],x[0],0),x[1]);
        if(!vector(0x30b90cu,g[1]+0x20u,x[3]))return 0x30b910u;
        x[1]=_mm_shuffle_ps(x[0],x[0],0);
        if(!read(g[0]+0x10u,x[0]))return 0x30b917u;
        if(!vector(0x30b91bu,g[1]+0x30u,x[0]))return 0x30b91fu;

        if(!read(g[0]+0x20u,g[7]))return 0x30b91fu;
        if(!scalar(0x30b922u,g[1]+0x40u,g[7]))return 0x30b925u;
        if(!read(g[0]+0x24u,g[0]))return 0x30b925u;
        if(!scalar(0x30b928u,g[1]+0x44u,g[0]))return 0x30b92bu;
        if(!scalar(0x30b92bu,g[1]+0x50u,g[2],1u))return 0x30b92eu;
        if(!scalar(0x30b92eu,g[4]+0x28u,g[2]))return 0x30b932u;
        if(!scalar(0x30b932u,g[4]+0x20u,g[2]))return 0x30b936u;
        if(!scalar(0x30b936u,g[4]+0x2cu,g[2]))return 0x30b93au;
        g[2]=g[4]+0x30u;
        if(!push(0x30b93eu,g[2]))return 0x30b93fu;
        if(!push(0x30b93fu,g[6]))return 0x30b940u;
        g[0]=g[4]+0x18u;
        x[1]=ordered_mul_ps(x[1],x[2]);
        if(!push(0x30b947u,g[0]))return 0x30b948u;
        if(!vector(0x30b948u,g[4]+0x3cu,x[4]))return 0x30b94du;
        if(!vector(0x30b94du,g[4]+0x4cu,x[1]))return 0x30b952u;

        // Dispatch still belongs to this CALL even if its return-address
        // write changes policy. The child admits its entry independently.
        push(0x30b952u,0x30b957u);
        child();
        if(!native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_) || !pop(g[7]))
            return 0x30b957u;
        if(!pop(g[6]))return 0x30b958u;
        g[4]=g[5];
        if(!pop(g[5]))return 0x30b95bu;
        if(!pop(word))return 0x30b95cu;
        g[4]+=20u;
        publish(0x30b95cu);
        return setup_complete;
    }
};
#endif

template<class State,class Child,class Spill>
std::uint32_t setup_provider(State &s,Child child,Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference_setup() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.fp.count>=8u || !native_raw_x87_detail::supported(s))return 0;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return 0;
    return SetupProvider<State>{s,cursor}.run(child,spill);
#else
    (void)s;(void)child;(void)spill;
    return 0;
#endif
}

} // namespace halo2::native_mopp_ray_setup_detail

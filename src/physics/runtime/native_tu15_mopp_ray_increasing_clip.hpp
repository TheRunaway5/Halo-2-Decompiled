#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"

namespace halo2::native_mopp_ray_increasing_clip_detail {

enum class Phase { before_child, after_child, shortened };

inline bool reference_increasing_clip() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_INCREASING_CLIP");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State>
class IncreasingClip {
    State &s_;
    const typename State::InstructionCursor *cursor_;

    void publish(std::uint32_t pc) const {
        if(cursor_)*cursor_->position=pc;
    }
    template<class Value>
    bool read(std::uint32_t address,Value &value) const {
        return native_raw_x87_detail::direct_read(s_.memory,address,std::as_writable_bytes(std::span{&value,1}));
    }
    bool can_continue() const {
        return native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_) &&
            native_raw_x87_detail::supported(s_);
    }
    bool scalar(std::uint32_t pc,std::uint32_t address,std::uint32_t value,bool pop=false) {
        publish(pc);
        const bool direct=s_.memory.try_callback_free_write(address,std::as_bytes(std::span{&value,1}));
        if(!direct)physics_step_detail::w(s_.memory,s_.observe,pc,address,4,value);
        if(pop)s_.fp.pop();
        return direct || can_continue();
    }
    void vector(std::uint32_t pc,std::uint32_t address,__m128 value) {
        publish(pc);
        physics_step_detail::vw(s_.memory,s_.observe,pc,address,value);
    }
public:
    IncreasingClip(State &s,const typename State::InstructionCursor *cursor):s_(s),cursor_(cursor) {}

    template<bool After,class Spill>
    std::uint32_t ratio(Spill spill) {
        constexpr auto upper=After?0x2cu:0x34u;
        constexpr auto lower=After?0x18u:0x30u;
        auto &g=s_.gp;
        auto &x=s_.xmm;
        using namespace physics_step_detail;
        using namespace native_tu15_leaves;
        std::uint32_t word;
        if(!read(g[4]+upper,word))return 0;
        s_.fp.push(ext(f(word)));
        if constexpr(After) {
            if(!read(g[7]+0x10u,x[1]))return 0x30af46u;
        }
        if(!read(g[4]+lower,word))return After?0x30af4au:0x30aee2u;
        s_.fp.st(0)=sub(s_.fp.st(0),ext(f(word)));
        if constexpr(After) {
            if(!read(0x41c500u,x[3]))return 0x30af4eu;
        }
        if(!read(g[4]+upper,word))return After?0x30af55u:0x30aee6u;
        s_.fp.st(0)=physics_solver_detail::divide(ext(f(word)),s_.fp.st(0));
        constexpr auto spill_pc=After?0x30af59u:0x30aeeau;
        constexpr auto reload_pc=After?0x30af5du:0x30aeeeu;
        publish(spill_pc);
        if(!scalar(spill_pc,g[4]+0x14u,spill(s_.fp.st(0),s_.fp.status),true))return reload_pc;
        if(!read(g[4]+0x14u,word))return reload_pc;
        x[0]=_mm_set1_ps(f(word));
        if constexpr(After) {
            x[2]=ordered_mul_ps(x[0],x[1]);
            if(!read(g[7],x[1]))return 0x30af6du;
            x[3]=ordered_add_ps(ordered_mul_ps(_mm_sub_ps(x[3],x[0]),x[1]),x[2]);
            vector(0x30af79u,g[7],x[3]);
            return 0x30af7cu;
        } else {
            x[3]=ordered_mul_ps(x[0],x[2]);
            if(!read(0x41c500u,x[2]))return 0x30aefeu;
            x[2]=ordered_add_ps(ordered_mul_ps(_mm_sub_ps(x[2],x[0]),x[1]),x[3]);
            vector(0x30af0eu,g[4]+0x90u,x[2]);
            return 0x30af16u;
        }
    }

    std::uint32_t shortened() {
        auto &g=s_.gp;
        auto &x=s_.xmm;
        using namespace physics_step_detail;
        using namespace native_tu15_leaves;
        std::uint32_t word;
        if(!read(g[1]+0x54u,g[2]))return 0;
        if(!read(g[1]+0x30u,x[2]))return 0x30af97u;
        if(!read(g[1]+0x10u,g[0]))return 0x30af9bu;
        if(!read(0x41c500u,x[4]))return 0x30af9eu;
        if(!read(g[0],x[1]))return 0x30afa5u;
        if(!read(g[4]+0x3cu,g[0]))return 0x30afa8u;
        s_.flags.minus(g[0],3u,4);
        if(!scalar(0x30afafu,g[4]+0x70u,g[2]))return 0x30afb3u;
        if(!read(g[4]+0x70u,word))return 0x30afb3u;
        x[0]=_mm_set1_ps(f(word));
        x[3]=ordered_mul_ps(x[0],x[2]);
        if(!read(g[1]+0x20u,x[2]))return 0x30afc3u;
        x[4]=ordered_add_ps(ordered_mul_ps(_mm_sub_ps(x[4],x[0]),x[2]),x[3]);
        x[4]=_mm_sub_ps(x[4],x[1]);
        vector(0x30afd3u,g[7]+0x10u,x[4]);
        if(!can_continue() || !read(g[3]+0x14u,g[1]))return 0x30afd7u;
        if(!read(g[7]+0x10u,x[1]))return 0x30afdau;
        if(!scalar(0x30afdeu,g[4]+0x7cu,g[1]))return 0x30afe2u;
        if(!read(g[4]+0x7cu,word))return 0x30afe2u;
        x[0]=_mm_set_ss(f(word));
        x[2]=ordered_mul_ps(_mm_shuffle_ps(x[0],x[0],0),x[1]);
        vector(0x30aff2u,g[7]+0x10u,x[2]);
        if(!can_continue() || !read(g[3],x[0]))return 0x30aff6u;
        if(!read(g[7]+0x10u,x[1]))return 0x30aff9u;
        x[1]=_mm_sub_ps(x[1],x[0]);
        vector(0x30b000u,g[7]+0x10u,x[1]);
        if(!can_continue())return 0x30b004u;
        if(s_.flags.sf==s_.flags.of) {
            publish(0x30b004u);
            return 0x30ae02u;
        }
        std::uint32_t endpoint;
        if(s_.fp.count>=8u || !read(g[7]+0x10u+g[0]*4u,endpoint) ||
            !read(g[4]+0x1cu,word))return 0x30b00au;
        s_.fp.push(ext(f(endpoint)));
        s_.fp.comparison(ext(f(word)),1);
        s_.set(0,s_.fp.word(),2,0);
        s_.flags.logic(s_.reg(0,1,8)&5u,1);
        publish(s_.flags.pf?0x30b01du:0x30b017u);
        return s_.flags.pf?0x30ae02u:0x30b6afu;
    }
};
#endif

template<Phase phase,class State,class Spill>
std::uint32_t increasing_clip(State &s,Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference_increasing_clip() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.fp.count>=8u || !native_raw_x87_detail::supported(s))return 0;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return 0;
    IncreasingClip<State> frame{s,cursor};
    if constexpr(phase==Phase::shortened)return frame.shortened();
    else return frame.template ratio<phase==Phase::after_child>(spill);
#else
    (void)s;(void)spill;
    return 0;
#endif
}

} // namespace halo2::native_mopp_ray_increasing_clip_detail

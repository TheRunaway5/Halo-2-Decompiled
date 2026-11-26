#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_contact_reduction_30caa0_detail {

enum class Scan { first, second, cross, fourth };
enum class FrameStage { initial, first_anchor, second_anchor, cross_anchor, finish_selection };

struct ScanSites {
    std::uint32_t cursor, candidate, vector, anchor, spill, reload, best_reload, branch, next;
};
inline constexpr std::array sites{
    ScanSites{0x30cad0u,0x30cadcu,0x30cadfu,0x30cae2u,0x30cb05u,0x30cb09u,0x30cb1au,0x30cb22u,0x30cb24u},
    ScanSites{0x30cb40u,0x30cb48u,0x30cb4bu,0x30cb4eu,0x30cb71u,0x30cb75u,0x30cb86u,0x30cb8eu,0x30cb90u},
    ScanSites{0x30cbb0u,0x30cbb8u,0x30cbbbu,0x30cbbeu,0x30cc09u,0x30cc0du,0x30cc1eu,0x30cc26u,0x30cc28u},
    ScanSites{0x30cc40u,0x30cc48u,0x30cc4bu,0x30cc4eu,0x30cc71u,0x30cc75u,0x30cc86u,0x30cc8eu,0x30cc90u},
};

inline bool reference_scan() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_CONTACT_REDUCTION_30CAA0_SCAN");
        return (value&&*value) || native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline constexpr std::uint32_t complete=0xffffffffu;

inline bool reference_removal() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_CONTACT_REDUCTION_30CAA0_REMOVAL");
        return (value&&*value) || native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_frame() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_CONTACT_REDUCTION_30CAA0_FRAME");
        return (value&&*value) || native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State>
class Reducer {
    State &s_;
    const typename State::InstructionCursor *cursor_;

    template<class Value>
    bool read(std::uint32_t address,Value &value) const {
        return native_raw_x87_detail::direct_read(s_.memory,address,std::as_writable_bytes(std::span{&value,1}));
    }
    void publish(std::uint32_t pc) const {
        if(cursor_)*cursor_->position=pc;
    }
    template<class Value>
    bool write(std::uint32_t pc,std::uint32_t address,Value value) {
        publish(pc);
        if(s_.memory.try_callback_free_write(address,std::as_bytes(std::span{&value,1})))return true;
        const auto depth=s_.fp.count;
        physics_step_detail::w(s_.memory,s_.observe,pc,address,sizeof(Value),value);
        return s_.fp.count==depth && can_continue();
    }
    bool can_continue() const {
        return native_raw_x87_detail::supported(s_) &&
            native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_);
    }
    bool spill(std::uint32_t pc,std::uint32_t address,__m128 lane) {
        return write(pc,address,std::bit_cast<std::uint32_t>(_mm_cvtss_f32(lane)));
    }
    bool copy(std::uint32_t pc,std::uint32_t address,__m128 vector) {
        publish(pc);
        // Each original half-store can fault or start observing the second
        // half. Both still belong to the same original instruction.
        const auto depth=s_.fp.count;
        physics_step_detail::vw(s_.memory,s_.observe,pc,address,vector);
        return s_.fp.count==depth && can_continue();
    }
    bool push(std::uint32_t pc,std::uint32_t value) {
        s_.gp[4]-=4u;
        return write(pc,s_.gp[4],value);
    }
    bool pop(std::uint32_t &value) {
        if(!read(s_.gp[4],value))return false;
        s_.gp[4]+=4u;
        return true;
    }

    bool push_epsilon() {
        std::uint32_t value;
        if(!read(0x46a0e8u,value))return false;
        s_.fp.push(physics_step_detail::ext(physics_step_detail::f(value)));
        return true;
    }
    bool mark(std::uint32_t pc) {
        return write(pc,s_.gp[4]+0x18u+s_.gp[1],std::uint8_t(1));
    }

    template<class Dispatch>
    std::uint32_t remove_point(Dispatch &dispatch) {
        auto &g=s_.gp;
        auto &x=s_.xmm;
        if(!read(g[6]+4u*g[7],g[0]))return 0x30cca8u;
        std::uint16_t point;
        if(!read(g[0]+0x1cu,point))return 0x30ccabu;
        g[0]=point;
        if(!read(g[5]+0x14u,g[1]))return 0x30ccafu;
        if(!read(g[1],g[2]))return 0x30ccb2u;
        if(!push(0x30ccb4u,g[0]))return 0x30ccb5u;
        std::uint32_t target;
        if(!read(g[2]+8u,target))return 0x30ccb5u;
        // A policy change during this write applies to the child and to the
        // successor. It does not split the original indirect CALL.
        push(0x30ccb5u,0x30ccb8u);
        dispatch(target);
        if(!can_continue() || !read(g[3],g[0]))return 0x30ccb8u;
        if(!read(g[0],x[0]))return 0x30ccbau;
        if(!read(g[6]+4u*g[7],g[1]))return 0x30ccbdu;
        if(!copy(0x30ccc0u,g[1],x[0]))return 0x30ccc3u;
        if(!read(g[0]+0x10u,x[0]))return 0x30ccc3u;
        if(!copy(0x30ccc7u,g[1]+0x10u,x[0]))return 0x30cccbu;
        if(!read(g[0]+0x20u,x[0]))return 0x30cccbu;
        if(!read(g[4]+0x10u,g[0]))return 0x30cccfu;
        if(!copy(0x30ccd3u,g[1]+0x20u,x[0]))return 0x30ccd7u;
        std::uint8_t last_selected;
        if(!read(g[4]+0x17u+g[0],last_selected))return 0x30ccd7u;
        s_.set(1,last_selected,1,0);
        g[0]=s_.flags.increment(g[0],4,true);
        if(!write(0x30ccdcu,g[4]+0x18u+g[7],std::uint8_t(g[1])))return 0x30cce0u;
        if(!write(0x30cce0u,g[4]+0x10u,g[0]))return 0x30cce4u;
        g[3]=s_.flags.minus(g[3],4u,4);
        return 0;
    }

    // The source retains all lanes of these intermediate vectors through the
    // metric spill and x87 comparison. Preserve the ordered SSE destinations.
    __m128 squared_distance() {
        using native_world_leaves_detail::ordered_add_ss;
        auto &x=s_.xmm;
        x[0]=_mm_sub_ps(x[0],x[1]);
        x[0]=_mm_mul_ps(x[0],x[0]);
        x[1]=_mm_shuffle_ps(x[0],x[0],85);
        x[2]=_mm_shuffle_ps(x[0],x[0],170);
        s_.gp[0]=s_.gp[4]+0x14u;
        x[1]=ordered_add_ss(x[1],x[0]);
        x[2]=ordered_add_ss(x[2],x[1]);
        return x[2];
    }

    __m128 squared_cross() {
        using native_tu15_leaves::ordered_mul_ps;
        using native_world_leaves_detail::ordered_add_ss;
        auto &x=s_.xmm;
        x[0]=_mm_sub_ps(x[0],x[2]);
        x[3]=_mm_shuffle_ps(x[0],x[0],210);
        x[2]=_mm_shuffle_ps(x[1],x[1],201);
        x[3]=ordered_mul_ps(x[3],x[2]);
        x[4]=_mm_shuffle_ps(x[0],x[0],201);
        x[2]=_mm_shuffle_ps(x[1],x[1],210);
        x[0]=ordered_mul_ps(x[4],x[2]);
        x[0]=_mm_sub_ps(x[0],x[3]);
        x[0]=_mm_mul_ps(x[0],x[0]);
        x[2]=ordered_add_ss(_mm_shuffle_ps(x[0],x[0],85),x[0]);
        x[3]=_mm_shuffle_ps(x[0],x[0],170);
        s_.gp[2]=s_.gp[4]+0x14u;
        x[3]=ordered_add_ss(x[3],x[2]);
        return x[3];
    }

public:
    Reducer(State &s,const typename State::InstructionCursor *cursor):s_(s),cursor_(cursor) {}

    std::uint32_t initialize() {
        auto &g=s_.gp;
        if(!push(0x30caa0u,g[5]))return 0x30caa1u;
        g[5]=g[4];
        g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);
        g[4]=s_.flags.minus(g[4],0x14u,4);
        if(!push_epsilon())return 0x30caa9u;
        if(!push(0x30caafu,g[3]))return 0x30cab0u;
        g[0]=s_.flags.logic(0,4);
        if(!push(0x30cab2u,g[6]))return 0x30cab3u;
        if(!read(g[5]+8u,g[6]))return 0x30cab3u;
        if(!read(g[6],g[2]))return 0x30cab6u;
        if(!write(0x30cab8u,g[4]+0x14u,g[0]))return 0x30cabcu;
        if(!push(0x30cabcu,g[7]))return 0x30cabdu;
        g[1]=s_.flags.logic(0,4);
        if(!write(0x30cabfu,g[4]+0x10u,5u))return 0x30cac7u;
        if(!write(0x30cac7u,g[4]+0x1cu,std::uint8_t(g[0])))return 0x30cacbu;
        g[7]=s_.flags.logic(0,4);
        publish(0x30cacdu);
        return 0x30cad0u;
    }

    template<FrameStage Stage>
    std::uint32_t transition() {
        auto &g=s_.gp;
        if constexpr(Stage==FrameStage::first_anchor) {
            g[2]=g[1]*4u;
            s_.fp.pop();
            if(!push_epsilon())return 0x30cb2du;
            if(!read(g[2]+g[6],g[3]))return 0x30cb33u;
            if(!mark(0x30cb36u))return 0x30cb3bu;
            g[7]=s_.flags.logic(0,4);
            publish(0x30cb3du);
            return 0x30cb40u;
        }
        if constexpr(Stage==FrameStage::second_anchor) {
            auto &x=s_.xmm;
            if(!read(g[6]+g[1]*4u,g[3]))return 0;
            s_.fp.pop();
            if(!read(g[2]+g[6],g[2]))return 0x30cb95u;
            if(!push_epsilon())return 0x30cb98u;
            if(!read(g[3],x[0]))return 0x30cb9eu;
            if(!read(g[2],x[1]))return 0x30cba1u;
            if(!mark(0x30cba4u))return 0x30cba9u;
            x[1]=_mm_sub_ps(x[1],x[0]);
            g[7]=s_.flags.logic(0,4);
            publish(0x30cbaeu);
            return 0x30cbb0u;
        }
        if constexpr(Stage==FrameStage::cross_anchor) {
            if(!read(g[6]+g[1]*4u,g[2]))return 0;
            s_.fp.pop();
            if(!push_epsilon())return 0x30cc2du;
            if(!mark(0x30cc33u))return 0x30cc38u;
            g[7]=s_.flags.logic(0,4);
            publish(0x30cc3au);
            return 0x30cc40u;
        }
        if constexpr(Stage==FrameStage::finish_selection) {
            s_.fp.pop();
            if(!mark(0x30cc92u))return 0x30cc97u;
            g[7]=s_.flags.logic(0,4);
            g[3]=g[6]+0x10u;
            publish(0x30cc9cu);
            return 0x30cca0u;
        }
    }

    template<class Dispatch>
    std::uint32_t remove(Dispatch dispatch) {
        auto &g=s_.gp;
        do {
            std::uint8_t selected;
            if(!read(g[4]+0x18u+g[7],selected))return 0x30cca0u;
            s_.set(0,selected,1,0);
            s_.flags.logic(selected,1);
            if(s_.flags.zf) {
                if(const auto continuation=remove_point(dispatch))return continuation;
            } else {
                g[7]=s_.flags.increment(g[7],4,false);
            }
            if(!read(g[4]+0x10u,g[0]))return 0x30cceau;
            s_.flags.minus(g[7],g[0],4);
        }while(s_.flags.sf!=s_.flags.of);
        if(!pop(g[7]))return 0x30ccf2u;
        if(!pop(g[6]))return 0x30ccf3u;
        if(!pop(g[3]))return 0x30ccf4u;
        g[4]=g[5];
        if(!pop(g[5]))return 0x30ccf7u;
        std::uint32_t address;
        if(!pop(address))return 0x30ccf8u;
        publish(0x30ccf8u);
        return complete;
    }

    template<Scan Stage>
    std::uint32_t run() {
        using namespace physics_step_detail;
        constexpr auto pc=sites[static_cast<unsigned>(Stage)];
        constexpr auto anchor_reg=Stage==Scan::first || Stage==Scan::fourth ? 2u : 3u;
        constexpr auto candidate_lane=Stage==Scan::cross ? 2u : 1u;
        auto &g=s_.gp;
        auto &x=s_.xmm;
        do {
            std::uint8_t selected;
            if(!read(g[4]+0x18u+g[7],selected))return pc.cursor;
            s_.set(0,selected,1,0);
            s_.flags.logic(selected,1);
            bool available=s_.flags.zf;
            if constexpr(Stage==Scan::first)
                if(available) {
                    s_.flags.minus(g[7],g[1],4);
                    available=!s_.flags.zf;
                }
            if(available) {
                if(!read(g[6]+g[7]*4u,g[0]))return pc.candidate;
                if(!read(g[0],x[candidate_lane]))return pc.vector;
                if(!read(g[anchor_reg],x[0]))return pc.anchor;
                if constexpr(Stage==Scan::cross) {
                    const auto metric=squared_cross();
                    if(!spill(pc.spill,g[2],metric))return pc.reload;
                } else {
                    const auto metric=squared_distance();
                    if(!spill(pc.spill,g[0],metric))return pc.reload;
                }
                std::uint32_t word;
                if(!read(g[4]+0x14u,word))return pc.reload;
                s_.fp.push(ext(f(word)));
                s_.fp.comparison(s_.fp.st(1),1);
                s_.set(0,s_.fp.word(),2,0);
                s_.flags.logic(s_.reg(0,1,8)&0x41u,1);
                if(s_.flags.zf) {
                    s_.fp.pop();
                    g[1]=g[7];
                    if(!read(g[4]+0x14u,word))return pc.best_reload;
                    s_.fp.push(ext(f(word)));
                }
            }
            g[7]=s_.flags.increment(g[7],4,false);
            s_.flags.minus(g[7],5u,4);
        }while(s_.flags.sf!=s_.flags.of);
        publish(pc.branch);
        return pc.next;
    }
};
#endif

template<Scan Stage,class State>
std::uint32_t select(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference_scan() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || !s.fp.count || s.fp.count>=8u || !native_raw_x87_detail::supported(s))return 0;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return 0;
    return Reducer<State>{s,cursor}.template run<Stage>();
#else
    (void)s;
    return 0;
#endif
}

template<class State,class Dispatch>
std::uint32_t remove(State &s,Dispatch dispatch) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference_removal() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || !native_raw_x87_detail::supported(s))return 0;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return 0;
    return Reducer<State>{s,cursor}.remove(dispatch);
#else
    (void)s;(void)dispatch;
    return 0;
#endif
}

template<FrameStage Stage,class State>
std::uint32_t configure_frame(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference_frame() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || !native_raw_x87_detail::supported(s))return 0;
    if constexpr(Stage==FrameStage::initial) {
        if(s.fp.count>=8u)return 0;
    } else if(!s.fp.count) {
        return 0;
    }
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return 0;
    Reducer<State> reducer{s,cursor};
    if constexpr(Stage==FrameStage::initial)return reducer.initialize();
    else return reducer.template transition<Stage>();
#else
    (void)s;
    return 0;
#endif
}

} // namespace halo2::native_contact_reduction_30caa0_detail

#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include <bit>

namespace halo2::native_mopp_ray_scale_vectors_detail {
enum class Continuation {
    original, read_end, read_factor, read_start, reload_factor, read_end_scaled,
    read_origin, scale_tail
};

inline bool reference_scale_vectors() {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_SCALE_VECTORS");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State> class ScaleVectors {
    State &s_;
    const typename State::InstructionCursor *cursor_;

    void publish(std::uint32_t pc) const {
        if(cursor_)*cursor_->position=pc;
    }
    template<class Value> bool read(std::uint32_t address,Value &value) {
        return native_mopp_ray_scale_tail_detail::read_operand(s_,address,value);
    }
    template<std::size_t Size>
    void half(std::uint32_t pc,std::uint32_t address,std::span<const std::byte,Size> bytes) {
        if(s_.observe) {
            std::uint64_t value{};
            std::memcpy(&value,bytes.data(),Size);
            physics_step_detail::w(s_.memory,s_.observe,pc,address,Size,value);
        } else {
            s_.memory.write(address,bytes);
        }
    }
    bool continuation_allowed() const {
        return (_mm_getcsr()&0x1f80u)==0x1f80u &&
            native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_);
    }
    void vector(std::uint32_t pc,std::uint32_t address,__m128 value) {
        publish(pc);
        const auto bytes=std::bit_cast<std::array<std::byte,16>>(value);
        const std::span<const std::byte,16> words{bytes};
        // Independently write the two original widths. Recheck the guest
        // observer between halves after any provider callback.
        half(pc,address,words.first<8u>());
        half(pc,address+8u,words.last<8u>());
    }
    bool scalar(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        publish(pc);
        if(s_.memory.try_callback_free_write(address,std::as_bytes(std::span{&value,1})))
            return true;
        physics_step_detail::w(s_.memory,s_.observe,pc,address,4,value);
        return !native_mopp_ray_scale_tail_detail::reference_scale_provider() &&
            continuation_allowed();
    }
public:
    ScaleVectors(State &s,const typename State::InstructionCursor *cursor):s_(s),cursor_(cursor) {}

    template<class Spill> Continuation run(Spill spill) {
        auto &g=s_.gp;
        auto &x=s_.xmm;
        if(!read(g[4]+0x60u,x[1]))return Continuation::original;
        publish(0x30b3b3u);
        x[0]=_mm_sub_ps(x[0],x[1]);
        vector(0x30b3b6u,g[7],x[0]);
        if(!continuation_allowed())return Continuation::read_end;

        if(!read(g[7]+0x10u,x[0]))return Continuation::read_end;
        if(!scalar(0x30b3bdu,g[4]+0x14u,g[2]))return Continuation::read_factor;
        std::int32_t factor;
        if(!read(g[4]+0x14u,factor))return Continuation::read_factor;
        publish(0x30b3c1u);
        s_.fp.push(static_cast<long double>(factor));
        publish(0x30b3c5u);
        x[0]=_mm_sub_ps(x[0],x[1]);
        vector(0x30b3c8u,g[7]+0x10u,x[0]);
        if(!continuation_allowed())return Continuation::read_start;
        if(!read(g[7],x[2]))return Continuation::read_start;

        publish(0x30b3cfu);
        const auto scale=spill(s_.fp.st(0),s_.fp.status);
        if(!scalar(0x30b3cfu,g[4]+0x14u,scale))return Continuation::reload_factor;
        std::uint32_t word;
        if(!read(g[4]+0x14u,word))return Continuation::reload_factor;
        x[0]=_mm_set_ss(std::bit_cast<float>(word));
        publish(0x30b3e0u);
        x[3]=native_tu15_leaves::ordered_mul_ps(_mm_shuffle_ps(x[0],x[0],0),x[2]);
        vector(0x30b3e3u,g[7],x[3]);
        if(!continuation_allowed())return Continuation::read_end_scaled;
        if(!read(g[7]+0x10u,x[2]))return Continuation::read_end_scaled;
        publish(0x30b3f1u);
        x[3]=native_tu15_leaves::ordered_mul_ps(_mm_shuffle_ps(x[0],x[0],0),x[2]);
        vector(0x30b3f4u,g[7]+0x10u,x[3]);
        if(!continuation_allowed())return Continuation::read_origin;
        if(!read(g[3],x[2]))return Continuation::read_origin;

        publish(0x30b3fbu);
        x[2]=native_tu15_leaves::ordered_add_ps(x[2],x[1]);
        publish(0x30b405u);
        x[1]=native_tu15_leaves::ordered_mul_ps(_mm_shuffle_ps(x[0],x[0],0),x[2]);
        vector(0x30b408u,g[4]+0x40u,x[1]);
        return Continuation::scale_tail;
    }
};
#endif

template<class State,class Spill>
Continuation scale_vectors(State &s,Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_scale_vectors() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.active_mmx || (_mm_getcsr()&0x1f80u)!=0x1f80u)return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    return ScaleVectors<State>{s,cursor}.run(spill);
#else
    (void)s;(void)spill;
    return Continuation::original;
#endif
}
} // namespace halo2::native_mopp_ray_scale_vectors_detail

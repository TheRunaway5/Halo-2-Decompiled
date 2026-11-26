#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include <bit>

namespace halo2::native_mopp_ray_axis_plane_detail {
enum class Continuation {
    original, store_lower, adjust_axis, store_axis, read_lower, spill_lower, read_upper,
    read_start, read_end, offsets, clear_left, read_left, store_right, compare
};

inline bool reference_axis_plane() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_AXIS_PLANE");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_axis_plane_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_AXIS_PLANE_PROVIDER");
        return value && *value;
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State> class Plane {
    State &s_;
    const typename State::InstructionCursor *cursor_;

    void publish(std::uint32_t pc) const {
        if(cursor_)*cursor_->position=pc;
    }

    template<class Value> bool read(std::uint32_t address,Value &value) const {
        const auto *page=s_.memory.direct_page(address,sizeof(Value));
        if(!page)return false;
        std::memcpy(&value,page->bytes+(address&4095u),sizeof(Value));
        return true;
    }

    bool fast_store(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        publish(pc);
        return s_.memory.try_callback_free_write(address,std::as_bytes(std::span{&value,1}));
    }

    bool continue_after_store(unsigned maximum_depth) const {
        return !reference_axis_plane_provider() &&
            native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_) &&
            s_.fp.count<=maximum_depth && native_raw_x87_detail::supported(s_);
    }

    std::optional<Continuation> axis_store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,
        Continuation before,Continuation after) {
        if(fast_store(pc,address,value))return {};
        if(reference_axis_plane_provider())return before;
        physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);
        if(!continue_after_store(6u))return after;
        return {};
    }

    bool spill_byte(std::uint32_t pc,std::uint32_t address,std::int32_t encoded) {
        publish(pc);
        std::uint32_t bits;
        std::uint16_t status;
        asm volatile("fildl %2; fstps %0; fnstsw %1"
            :"=m"(bits),"=m"(status):"m"(encoded):"memory","st");
        s_.fp.status=(s_.fp.status&~0x200u)|(status&0x200u);
        if(s_.memory.try_callback_free_write(address,std::as_bytes(std::span{&bits,1})))return true;
        // The narrowly admitted integer is exact in both formats, including
        // at a refused store. Publish the original live lane before w().
        s_.fp.push(static_cast<long double>(encoded));
        physics_step_detail::w(s_.memory,s_.observe,pc,address,4,bits);
        s_.fp.pop();
        return continue_after_store(6u);
    }

    template<class Spill> bool spill_lane(std::uint32_t pc,std::uint32_t address,Spill spill) {
        publish(pc);
        const auto bits=spill(s_.fp.st(0),s_.fp.status);
        const bool direct=fast_store(pc,address,bits);
        if(!direct)physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,bits);
        s_.fp.pop();
        return direct || continue_after_store(6u);
    }

public:
    Plane(State &s,const typename State::InstructionCursor *cursor):s_(s),cursor_(cursor) {}

    template<class Spill> Continuation axis(Spill spill) {
        using namespace physics_step_detail;
        auto &g=s_.gp;
        std::uint8_t lower,upper;
        if(!read(g[6]+1u,lower)||!read(g[6]+2u,upper))return Continuation::original;
        g[1]=lower;g[2]=upper;
        if(const auto next=axis_store(0x30ada3u,g[4]+0x14u,g[1],
            Continuation::store_lower,Continuation::adjust_axis))return *next;

        g[0]=s_.flags.plus(g[0],0xfffffff0u,4);
        if(const auto next=axis_store(0x30adaau,g[4]+0x3cu,g[0],
            Continuation::store_axis,Continuation::read_lower))return *next;
        std::int32_t encoded;
        const auto scratch=g[4]+0x14u;
        if(!read(scratch,encoded)||std::uint32_t(encoded)>255u)return Continuation::read_lower;
        if(fast_store(0x30adb2u,scratch,g[2])) {
            if(!spill_byte(0x30adb6u,g[4]+0x20u,encoded))return Continuation::read_upper;
        } else {
            if(reference_axis_plane_provider())return Continuation::read_lower;
            // The original FILD lane is live during this store. A callback
            // may modify the lane, so spill the current value after return.
            s_.fp.push(static_cast<long double>(encoded));
            w(s_.memory,s_.observe,0x30adb2u,scratch,4u,g[2]);
            if(!s_.fp.count || !continue_after_store(7u))return Continuation::spill_lower;
            if(!spill_lane(0x30adb6u,g[4]+0x20u,spill))return Continuation::read_upper;
        }
        if(!read(g[4]+0x14u,encoded)||std::uint32_t(encoded)>255u)return Continuation::read_upper;
        if(!spill_byte(0x30adbeu,g[4]+0x1cu,encoded))return Continuation::read_start;

        std::uint32_t endpoint;
        const auto start=g[7]+g[0]*4u;
        if(!read(start,endpoint))return Continuation::read_start;
        s_.fp.push(ext(f(endpoint)));
        if(!read(start+0x10u,endpoint))return Continuation::read_end;
        s_.fp.push(ext(f(endpoint)));
        publish(0x30adc5u);
        return Continuation::offsets;
    }

    Continuation offsets() {
        auto &g=s_.gp;
        std::uint8_t right;
        if(!read(g[6]+3u,right))return Continuation::original;
        g[0]=right;
        g[6]=s_.flags.plus(g[6],4u,4);
        const auto address=g[4]+0x24u;
        if(!fast_store(0x30add0u,address,0u)) {
            if(reference_axis_plane_provider())return Continuation::clear_left;
            physics_step_detail::w(s_.memory,s_.observe,0x30add0u,address,4u,0u);
            if(!native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_))
                return Continuation::read_left;
        }
        return finish_offsets();
    }

    Continuation finish_offsets() {
        auto &g=s_.gp;
        std::uint32_t left;
        if(!read(g[4]+0x24u,left))return Continuation::read_left;
        g[1]=left;
        const auto address=g[4]+0x14u;
        if(!fast_store(0x30addcu,address,g[0])) {
            if(reference_axis_plane_provider())return Continuation::store_right;
            physics_step_detail::w(s_.memory,s_.observe,0x30addcu,address,4u,g[0]);
        }
        return Continuation::compare;
    }
};

template<class State>
bool cursor_admitted(const State &s,const typename State::InstructionCursor *cursor) {
    if(reference_axis_plane() || s.image!=SourcePhysicsContactImage::TitleUpdate15 || s.observe)
        return false;
    return s.services.instruction?cursor!=nullptr:!s.instruction_cursor;
}
#endif

template<class State,class Spill>
Continuation axis_plane(State &s,Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(!cursor_admitted(s,cursor) || s.active_mmx || s.fp.count>6u)
        return Continuation::original;
    std::uint16_t control;
    asm volatile("fnstcw %0":"=m"(control));
    if((control&0x3fu)!=0x3fu)return Continuation::original;
    return Plane<State>{s,cursor}.axis(spill);
#else
    (void)s;(void)spill;
    return Continuation::original;
#endif
}

template<class State>
Continuation plane_offsets(State &s, bool shared_tail = false) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(!cursor_admitted(s,cursor))return Continuation::original;
    Plane<State> plane{s,cursor};
    return shared_tail ? plane.finish_offsets() : plane.offsets();
#else
    (void)s;
    (void)shared_tail;
    return Continuation::original;
#endif
}
} // namespace halo2::native_mopp_ray_axis_plane_detail

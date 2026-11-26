#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include <bit>

namespace halo2::native_mopp_ray_wide_plane_detail {
enum class Continuation {
    original, save_low, read_left_code, read_lower, save_high, read_right_code,
    read_upper, save_axis, advance_program, read_start, read_end, read_left_high,
    save_left, read_right_high, offsets
};

inline bool reference_wide_plane() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_WIDE_PLANE");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_wide_plane_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_WIDE_PLANE_PROVIDER");
        return value && *value;
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State> class WidePlane {
    State &s_;
    const typename State::InstructionCursor *cursor_;
    bool provider_store_{};

    void publish(std::uint32_t pc) const {
        if(cursor_)*cursor_->position=pc;
    }

    template<class Value> bool read(std::uint32_t address,Value &value) const {
        const auto *page=s_.memory.direct_page(address,sizeof(Value));
        if(!page)return false;
        std::memcpy(&value,page->bytes+(address&4095u),sizeof(Value));
        return true;
    }

    bool store(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        publish(pc);
        return s_.memory.try_callback_free_write(address,std::as_bytes(std::span{&value,1}));
    }

    void provider_store(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        provider_store_=true;
        physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);
    }

    bool continue_after_store(unsigned maximum_depth) const {
        return !reference_wide_plane_provider() &&
            native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_) &&
            s_.fp.count<=maximum_depth && native_raw_x87_detail::supported(s_);
    }

    std::optional<Continuation> guarded_store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,
        Continuation before,Continuation after,unsigned maximum_depth) {
        if(store(pc,address,value))return {};
        if(reference_wide_plane_provider())return before;
        provider_store(pc,address,value);
        if(!continue_after_store(maximum_depth))return after;
        return {};
    }

    std::optional<Continuation> interposed_store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,
        std::int32_t encoded,Continuation before,Continuation after,bool &live_lane) {
        if(store(pc,address,value)) {
            live_lane=false;
            return {};
        }
        s_.fp.push(static_cast<long double>(encoded));
        live_lane=true;
        if(reference_wide_plane_provider())return before;
        provider_store(pc,address,value);
        if(!s_.fp.count || !continue_after_store(7u))return after;
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
        s_.fp.push(static_cast<long double>(encoded));
        provider_store(pc,address,bits);
        s_.fp.pop();
        return continue_after_store(6u);
    }

    template<class Spill>
    bool spill_component(std::uint32_t pc,std::uint32_t address,std::int32_t encoded,
                         bool live_lane,Spill spill) {
        if(!live_lane)return spill_byte(pc,address,encoded);
        publish(pc);
        const auto value=spill(s_.fp.st(0),s_.fp.status);
        const bool direct=store(pc,address,value);
        if(!direct)provider_store(pc,address,value);
        s_.fp.pop();
        return direct || continue_after_store(6u);
    }

public:
    WidePlane(State &s,const typename State::InstructionCursor *cursor):s_(s),cursor_(cursor) {}

    template<class Spill> Continuation run(Spill spill) {
        using namespace physics_step_detail;
        auto &g=s_.gp;
        const auto first=g[6]+1u;
        const auto *page=s_.memory.direct_page(first,6u);
        if(!page)return Continuation::original;
        const auto *code=page->bytes+(first&4095u);
        const auto byte=[&](unsigned original_offset,std::uint32_t live_offset,std::uint32_t &word) {
            if(!provider_store_) {
                word=std::to_integer<std::uint32_t>(code[original_offset-1u]);
                return true;
            }
            // A real write can replace ESI or its backing. Borrow only the
            // current byte, immediately before its original source use.
            std::uint8_t value;
            if(!read(g[6]+live_offset,value))return false;
            word=value;
            return true;
        };
        g[1]=std::to_integer<std::uint32_t>(code[0]);
        g[2]=std::to_integer<std::uint32_t>(code[1]);
        if(const auto next=guarded_store(0x30ad52u,g[4]+0x14u,g[1],
            Continuation::save_low,Continuation::read_left_code,6u))return *next;
        if(!byte(4u,4u,g[1]))return Continuation::read_left_code;
        std::int32_t encoded;
        if(!read(g[4]+0x14u,encoded)||std::uint32_t(encoded)>255u)return Continuation::read_lower;
        g[0]=s_.flags.plus(g[0],0xffffffddu,4);
        bool live_lane;
        if(const auto next=interposed_store(0x30ad61u,g[4]+0x14u,g[2],encoded,
            Continuation::save_high,Continuation::read_right_code,live_lane))return *next;
        if(!byte(6u,6u,g[2])) {
            if(!live_lane)s_.fp.push(static_cast<long double>(encoded));
            return Continuation::read_right_code;
        }
        if(!spill_component(0x30ad69u,g[4]+0x20u,encoded,live_lane,spill))return Continuation::read_upper;
        if(!read(g[4]+0x14u,encoded)||std::uint32_t(encoded)>255u)return Continuation::read_upper;
        if(const auto next=interposed_store(0x30ad71u,g[4]+0x3cu,g[0],encoded,
            Continuation::save_axis,Continuation::advance_program,live_lane))return *next;
        g[6]=s_.flags.plus(g[6],7u,4);
        if(!spill_component(0x30ad78u,g[4]+0x1cu,encoded,live_lane,spill))return Continuation::read_start;

        // Use the actual endpoint and code bytes after earlier stores. This
        // order also covers guest aliases between the encoded data and frame.
        std::uint32_t word;
        const auto point=g[7]+g[0]*4u;
        if(!read(point,word))return Continuation::read_start;
        s_.fp.push(ext(f(word)));
        if(!read(point+0x10u,word))return Continuation::read_end;
        s_.fp.push(ext(f(word)));
        if(!byte(3u,0xfffffffcu,g[0]))return Continuation::read_left_high;
        g[0]=s_.flags.shift(g[0],8u,4);
        g[0]=s_.flags.plus(g[0],g[1],4);
        if(const auto next=guarded_store(0x30ad8cu,g[4]+0x24u,g[0],
            Continuation::save_left,Continuation::read_right_high,8u))return *next;
        if(!byte(5u,0xfffffffeu,g[0]))return Continuation::read_right_high;
        g[0]=s_.flags.shift(g[0],8u,4);
        g[0]=s_.flags.plus(g[0],g[2],4);
        publish(0x30ad99u);
        return Continuation::offsets;
    }
};
#endif

template<class State,class Spill>
Continuation wide_plane(State &s,Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_wide_plane() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.active_mmx || s.fp.count>6u)return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    std::uint16_t control;
    asm volatile("fnstcw %0":"=m"(control));
    if((control&0x3fu)!=0x3fu)return Continuation::original;
    return WidePlane<State>{s,cursor}.run(spill);
#else
    (void)s;(void)spill;
    return Continuation::original;
#endif
}
} // namespace halo2::native_mopp_ray_wide_plane_detail

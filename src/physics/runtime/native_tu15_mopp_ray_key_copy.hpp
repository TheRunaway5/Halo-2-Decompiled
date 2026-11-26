#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include <bit>

namespace halo2::native_mopp_ray_key_copy_detail {
enum class Continuation {
    original, read_vector, read_shift, write_shift, read_scale, write_scale,
    read_old_key, write_old_key, read_property, write_property, set_cache, write_key, advance_key, resume
};

inline bool reference_key_copy() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_KEY_COPY");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_key_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_KEY_PROVIDER");
        return value && *value;
    }();
    return reference;
}

inline bool reference_key_copy_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_KEY_COPY_PROVIDER");
        return value && *value;
    }();
    return reference;
}

inline bool reference_offset_key_node() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_OFFSET_KEY_NODE");
        return value && *value;
    }();
    return reference;
}

inline bool reference_offset_key_copy() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_OFFSET_KEY_COPY");
        return value && *value;
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State> class KeyCopy {
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
    bool scalar(std::uint32_t pc,std::uint32_t offset,std::uint32_t value) {
        publish(pc);
        return s_.memory.try_callback_free_write(s_.gp[4]+offset,std::as_bytes(std::span{&value,1}));
    }
    std::optional<Continuation> field(std::uint32_t pc,std::uint32_t offset,std::uint32_t value,
        Continuation before,Continuation after) {
        if(scalar(pc,offset,value))return {};
        if(reference_key_copy_provider())return before;
        physics_step_detail::w(s_.memory,s_.observe,pc,s_.gp[4]+offset,4u,value);
        if(!native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_))return after;
        return {};
    }
    bool copy_vector(std::uint32_t site) {
        publish(site);
        // Use both ordered original provider operations. Either half may
        // install an observer or change the service cursor for the next PC.
        physics_step_detail::vw(s_.memory,s_.observe,site,s_.gp[4]+0x40u,s_.xmm[0]);
        if(s_.observe)return false;
        if(!cursor_)return !s_.services.instruction && !s_.instruction_cursor;
        return native_mopp_ray_scale_tail_detail::admitted_cursor(s_)==cursor_;
    }

    template<bool Offset>
    std::optional<Continuation> copy_variables() {
        constexpr auto sites=Offset?std::array{0x30b48fu,0x30b497u,0x30b49eu,0x30b4a5u,0x30b4acu}:
            std::array{0x30b4eau,0x30b4f2u,0x30b4f9u,0x30b500u,0x30b507u};
        auto &g=s_.gp;
        if(!read(g[3],s_.xmm[0]))return Continuation::read_vector;
        if(!copy_vector(sites[0]))return Continuation::read_shift;
        // Reload every field after the preceding store. The two frames may overlap.
        if(!read(g[3]+0x10u,g[2]))return Continuation::read_shift;
        if(const auto next=field(sites[1],0x50u,g[2],
            Continuation::write_shift,Continuation::read_scale))return next;
        if(!read(g[3]+0x14u,g[1]))return Continuation::read_scale;
        if(const auto next=field(sites[2],0x54u,g[1],
            Continuation::write_scale,Continuation::read_old_key))return next;
        if(!read(g[3]+0x18u,g[2]))return Continuation::read_old_key;
        if(const auto next=field(sites[3],0x58u,g[2],
            Continuation::write_old_key,Continuation::read_property))return next;
        if(!read(g[3]+0x1cu,g[1]))return Continuation::read_property;
        if(const auto next=field(sites[4],0x5cu,g[1],
            Continuation::write_property,Continuation::set_cache))return next;
        g[3]=g[4]+0x40u;
        return {};
    }

public:
    KeyCopy(State &s,const typename State::InstructionCursor *cursor):s_(s),cursor_(cursor) {}

    Continuation run() {
        auto &g=s_.gp;
        std::array<std::uint8_t,4> bytes;
        if(!read(g[6]+1u,bytes))return Continuation::original;
        // The ordinary bytes cannot call a provider and CMP overwrites the
        // intermediate shift/add flags before the next observable access.
        g[0]=(std::uint32_t(bytes[0])<<24u)|(std::uint32_t(bytes[1])<<16u)|
             (std::uint32_t(bytes[2])<<8u)|bytes[3];
        g[2]=bytes[3];
        g[1]=g[4]+0x40u;
        s_.flags.minus(g[3],g[1],4);
        if(!s_.flags.zf) {
            if(const auto next=copy_variables<false>())return *next;
        }
        return finish();
    }

    Continuation finish() {
        auto &g=s_.gp;
        if(!scalar(0x30b50fu,0x58u,g[0])) {
            if(reference_key_provider())return Continuation::write_key;
            physics_step_detail::w(s_.memory,s_.observe,0x30b50fu,g[4]+0x58u,4u,g[0]);
            if(!native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_))
                return Continuation::advance_key;
        }
        g[6]=s_.flags.plus(g[6],5u,4);
        publish(0x30b516u);
        return Continuation::resume;
    }

    Continuation offset() {
        auto &g=s_.gp;
        const auto cached=g[4]+0x40u;
        const bool needs_copy=g[3]!=cached;
        if(needs_copy && reference_offset_key_copy())return Continuation::original;
        std::array<std::uint8_t,2> bytes;
        std::uint32_t old_key;
        if(!read(g[6]+1u,bytes) || (!needs_copy && !read(g[4]+0x58u,old_key)))
            return Continuation::original;
        g[1]=cached;
        g[2]=bytes[1];
        g[0]=(std::uint32_t(bytes[0])<<8u)|bytes[1];
        if(needs_copy) {
            s_.flags.minus(g[3],g[1],4);
            if(const auto next=copy_variables<true>())return *next;
            if(!read(g[4]+0x58u,old_key))return Continuation::write_key;
        }
        const auto key=s_.flags.plus(old_key,g[0],4u);
        if(!scalar(0x30b4b4u,0x58u,key)) {
            physics_step_detail::w(s_.memory,s_.observe,0x30b4b4u,g[4]+0x58u,4u,key);
            if(!native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_))
                return Continuation::advance_key;
        }
        g[6]=s_.flags.plus(g[6],3u,4u);
        publish(0x30b4bbu);
        return Continuation::resume;
    }
};
#endif

template<bool Tail=false,bool Offset=false,class State>
Continuation key_copy(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_key_copy() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe)return Continuation::original;
    if constexpr(Tail)
        if(reference_key_provider())return Continuation::original;
    if constexpr(Offset)
        if(reference_offset_key_node())return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    KeyCopy<State> frame{s,cursor};
    if constexpr(Offset)return frame.offset();
    else if constexpr(Tail)return frame.finish();
    else return frame.run();
#else
    (void)s;
    return Continuation::original;
#endif
}
} // namespace halo2::native_mopp_ray_key_copy_detail

#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include <bit>

namespace halo2::native_mopp_ray_collector_detail {
enum class Part { select, shape_call, shape_result, collector_call, collector_result };
enum class Continuation {
    original, done, alternate, read_key, read_filter, read_collection_table, push_shape_output,
    push_shape_key, set_shape_receiver, read_output, read_shape_table, save_parent, read_parent_value,
    save_parent_value, read_collector, push_collector, resolve_query, push_query, resolve_ray,
    push_ray, set_collector_receiver, save_shape, save_key, read_fraction
};

inline bool reference_collector() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_COLLECTOR");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_collector_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_COLLECTOR_PROVIDER");
        return value && *value;
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State> class Collector {
    State &s_;
    const typename State::InstructionCursor *cursor_;

    void publish(std::uint32_t pc) const {
        if(cursor_)*cursor_->position=pc;
    }

    bool read(std::uint32_t address,std::uint32_t &word) const {
        if(const auto *page=s_.memory.direct_page(address,4u)) {
            std::memcpy(&word,page->bytes+(address&4095u),4u);
            return true;
        }
        const auto view=s_.memory.try_read_view(address,4u);
        if(!view)return false;
        std::memcpy(&word,view->data(),4u);
        return true;
    }

    bool store(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        publish(pc);
        return s_.memory.try_callback_free_write(address,std::as_bytes(std::span{&value,1}));
    }

    std::optional<Continuation> guarded_store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,
        Continuation before,Continuation after) {
        if(store(pc,address,value))return {};
        if(reference_collector_provider())return before;
        physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);
        if(!native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_))return after;
        return {};
    }

    std::optional<Continuation> guarded_push(std::uint32_t pc,std::uint32_t value,
        Continuation before,Continuation after) {
        const auto address=s_.gp[4]-4u;
        if(store(pc,address,value)) {
            s_.gp[4]=address;
            return {};
        }
        if(reference_collector_provider())return before;
        // The actual PUSH exposes the decremented ESP to a guard or fault.
        s_.push(pc,value);
        if(!native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_))return after;
        return {};
    }

public:
    Collector(State &s,const typename State::InstructionCursor *cursor):s_(s),cursor_(cursor) {}

    Continuation select() {
        auto &g=s_.gp;
        if(!read(g[4]+0x28u,g[0]))return Continuation::original;
        if(!read(g[3]+0x18u,g[6]))return Continuation::read_key;
        if(!read(g[0]+0x44u,g[1]))return Continuation::read_filter;
        g[7]=s_.flags.plus(g[7],g[6],4);
        s_.flags.logic(g[1],4);
        publish(0x30b603u);
        return s_.flags.zf?Continuation::done:Continuation::alternate;
    }

    Continuation shape_call() {
        auto &g=s_.gp;
        if(!read(g[0]+0x64u,g[0]))return Continuation::original;
        if(!read(g[0],g[2]))return Continuation::read_collection_table;
        g[1]=g[4]+0xa0u;
        if(const auto next=guarded_push(0x30b630u,g[1],
            Continuation::push_shape_output,Continuation::push_shape_key))return *next;
        if(const auto next=guarded_push(0x30b631u,g[7],
            Continuation::push_shape_key,Continuation::set_shape_receiver))return *next;
        g[1]=g[0];
        publish(0x30b632u);
        return Continuation::done;
    }

    Continuation shape_result() {
        auto &g=s_.gp;
        if(!read(g[4]+0x28u,g[6]))return Continuation::original;
        if(!read(g[6]+0x58u,g[1]))return Continuation::read_output;
        s_.flags.logic(g[1],4);
        if(!read(g[0],g[2]))return Continuation::read_shape_table;
        publish(0x30b642u);
        return s_.flags.zf?Continuation::done:Continuation::alternate;
    }

    Continuation collector_call() {
        auto &g=s_.gp;
        if(!read(g[6]+0x60u,g[1]))return Continuation::original;
        if(const auto next=guarded_store(0x30b674u,g[4]+0x6cu,g[1],
            Continuation::save_parent,Continuation::read_parent_value))return *next;
        if(!read(g[1]+8u,g[1]))return Continuation::read_parent_value;
        if(const auto next=guarded_store(0x30b67bu,g[4]+0x68u,g[1],
            Continuation::save_parent_value,Continuation::read_collector))return *next;
        if(!read(g[6]+0x5cu,g[1]))return Continuation::read_collector;
        if(const auto next=guarded_push(0x30b682u,g[1],
            Continuation::push_collector,Continuation::resolve_query))return *next;
        g[1]=g[4]+0x64u;
        if(const auto next=guarded_push(0x30b687u,g[1],
            Continuation::push_query,Continuation::resolve_ray))return *next;
        g[1]=g[6]+0x20u;
        if(const auto next=guarded_push(0x30b68bu,g[1],
            Continuation::push_ray,Continuation::set_collector_receiver))return *next;
        g[1]=g[0];
        if(const auto next=guarded_store(0x30b68eu,g[4]+0x6cu,g[0],
            Continuation::save_shape,Continuation::save_key))return *next;
        if(const auto next=guarded_store(0x30b692u,g[4]+0x70u,g[7],
            Continuation::save_key,Continuation::done))return *next;
        return Continuation::done;
    }

    template<bool Shape,class Dispatch>
    bool dispatch_call(Dispatch invoke) {
        constexpr auto pc=Shape?0x30b634u:0x30b696u;
        constexpr auto returned=Shape?0x30b637u:0x30b699u;
        constexpr auto table_offset=Shape?0x34u:0x24u;
        std::uint32_t target;
        if(!read(s_.gp[2]+table_offset,target))return false;
        publish(pc);
        // Keep the real return PUSH and owner call. The pre-push target
        // survives callback re-entry and the owner's unwind propagates.
        s_.push(pc,returned);
        invoke(s_,target,pc,false);
        return true;
    }

    Continuation collector_result() {
        auto &g=s_.gp;
        if(!read(g[6]+0x5cu,g[2]))return Continuation::original;
        if(!read(g[2]+4u,g[0]))return Continuation::read_fraction;
        publish(0x30b69fu);
        physics_step_detail::w(s_.memory,s_.observe,0x30b69fu,g[6]+0x54u,4u,g[0]);
        return Continuation::done;
    }
};
#endif

template<Part part,class State>
Continuation collector(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_collector() || s.image!=SourcePhysicsContactImage::TitleUpdate15)return Continuation::original;
    if constexpr(part!=Part::select&&part!=Part::shape_result)
        if(s.observe)return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    Collector<State> frame{s,cursor};
    if constexpr(part==Part::select)return frame.select();
    if constexpr(part==Part::shape_call)return frame.shape_call();
    if constexpr(part==Part::shape_result)return frame.shape_result();
    if constexpr(part==Part::collector_call)return frame.collector_call();
    if constexpr(part==Part::collector_result)return frame.collector_result();
#else
    (void)s;
    return Continuation::original;
#endif
}

template<bool Shape,class State,class Dispatch>
bool dispatch_call(State &s,Dispatch invoke) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return false;
    if(reference_collector() || reference_collector_provider() ||
       s.image!=SourcePhysicsContactImage::TitleUpdate15 || s.observe)return false;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return false;
    return Collector<State>{s,cursor}.template dispatch_call<Shape>(invoke);
#else
    (void)s;(void)invoke;
    return false;
#endif
}
}

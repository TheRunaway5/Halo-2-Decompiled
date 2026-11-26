#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_collision_cache_309060_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_world_leaves_detail::ordered_sub_ps;

inline constexpr auto first = 0x309060u;
inline constexpr auto last = 0x30914au;
inline constexpr auto observation_offset = 0x3b00u;

template<class State, bool Canonical>
struct CachePort : Port<State> {
    using Port<State>::Port;
    static constexpr std::uint32_t observation(std::uint32_t pc) {
        return Canonical ? pc - observation_offset : pc;
    }
    std::uint32_t executable(std::uint32_t tu15, std::uint32_t disc) const {
        if constexpr (Canonical)
            return this->s.image == SourcePhysicsContactImage::MapPack11 ? disc : tu15;
        return tu15;
    }
    std::uint32_t resume(std::uint32_t tu15) const {
        return executable(tu15,tu15 - observation_offset);
    }
    std::uint32_t read(std::uint32_t pc, std::uint32_t address, unsigned width) {
        return Port<State>::read(observation(pc),address,width);
    }
    void store(std::uint32_t pc, std::uint32_t address, unsigned width, std::uint32_t value) {
        Port<State>::store(observation(pc),address,width,value);
    }
    __m128 read_vector(std::uint32_t pc, std::uint32_t address) {
        return Port<State>::read_vector(observation(pc),address);
    }
    void store_vector(std::uint32_t pc, std::uint32_t address, __m128 value) {
        Port<State>::store_vector(observation(pc),address,value);
    }
    void push(std::uint32_t pc, std::uint32_t value) { Port<State>::push(observation(pc),value); }
    std::uint32_t pop(std::uint32_t pc) { return Port<State>::pop(observation(pc)); }
    void ret(std::uint32_t pc, std::uint32_t bytes) { Port<State>::ret(observation(pc),bytes); }
    void publish(std::uint32_t pc) { Port<State>::publish(observation(pc)); }
};

template<class State, class Dispatch, class Transform, class Direction, bool Canonical = false>
struct Cache : CachePort<State,Canonical> {
    Dispatch &dispatch;
    Transform &transform;
    Direction &direction;
    Cache(State &s, std::uint32_t *cursor, Trace<State> *trace, Dispatch &router, Transform &point, Direction &vector)
        : CachePort<State,Canonical>(s,cursor,trace,{}), dispatch(router), transform(point), direction(vector) {}

    template<class Action> void guarded(Action &&action) {
        try {
            action();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
    auto &x() { return this->s.xmm; }
    std::uint32_t argument(std::uint32_t pc, std::uint32_t offset) {
        return this->read(pc,this->gp[ebp] + offset,4);
    }
    __m128 local_vector(std::uint32_t pc, std::uint32_t offset) {
        return this->read_vector(pc,this->gp[esp] + offset);
    }
    void write_vector(std::uint32_t pc, std::uint32_t offset, __m128 value) {
        this->store_vector(pc,this->gp[esp] + offset,value);
    }
    void cache_word(std::uint32_t pc, std::uint32_t offset, unsigned width, std::uint32_t value) {
        this->store(pc,this->gp[esi] + offset,width,value);
    }
    void call_shape(std::uint32_t pc, std::uint32_t resume, std::uint32_t vtable, std::uint32_t slot) {
        std::uint32_t target{};
        guarded([&] {
            target = this->read(pc,vtable + slot,4);
            this->push(pc,this->resume(resume));
        });
        // The World router consumes a canonical call-site address. Stores
        // and the instruction cursor expose the selected TU1.5 address.
        this->enter(target,[&](State &s) { dispatch(s,target,pc - observation_offset,false); });
    }
    void open_frame() {
        auto &gp = this->gp;
        guarded([&] {
            this->push(first,gp[ebp]);
            gp[ebp] = gp[esp];
            gp[esp] = this->flags.logic(gp[esp] & 0xfffffff0u,4);
            gp[esp] = this->flags.minus(gp[esp],0x74u,4);
            this->push(0x309069u,gp[ebx]);
            this->push(0x30906au,gp[esi]);
            gp[esi] = gp[ecx];
            this->push(0x30906du,gp[edi]);
        });
    }
    void support_centers() {
        auto &gp = this->gp;
        guarded([&] {
            gp[edi] = argument(0x30906eu,8u);
            gp[eax] = this->read(0x309071u,gp[edi],4);
            gp[ecx] = gp[esp] + 0x60u;
            this->push(0x309077u,gp[ecx]);
            gp[ecx] = gp[edi];
        });
        call_shape(0x30907au,0x30907du,gp[eax],0x38u);
        guarded([&] {
            gp[ebx] = argument(0x30907du,12u);
            gp[edx] = this->read(0x309080u,gp[ebx],4);
            gp[eax] = gp[esp] + 0x50u;
            this->push(0x309086u,gp[eax]);
            gp[ecx] = gp[ebx];
        });
        call_shape(0x309089u,0x30908cu,gp[edx],0x38u);
    }
    void first_shape() {
        auto &gp = this->gp;
        auto &lanes = x();
        guarded([&] {
            gp[edx] = argument(0x30908cu,16u);
            gp[ecx] = gp[esp] + 0x50u;
            this->push(0x309093u,gp[ecx]);
            this->push(0x309094u,gp[edx]);
            gp[ecx] = gp[esp] + 0x48u;
            this->push(0x309099u,this->resume(0x30909eu));
        });
        this->enter(this->executable(0x2e4690u,0x2e0ba0u),transform);
        guarded([&] {
            lanes[0] = local_vector(0x30909eu,0x60u);
            lanes[1] = local_vector(0x3090a3u,0x40u);
            gp[eax] = this->read(0x3090a8u,gp[edi],4);
            gp[ecx] = gp[esp] + 0x30u;
            this->push(0x3090aeu,gp[ecx]);
            gp[edx] = gp[esp] + 0x14u;
            lanes[1] = ordered_sub_ps(lanes[1],lanes[0]);
            this->push(0x3090b6u,gp[edx]);
            gp[ecx] = gp[edi];
            write_vector(0x3090b9u,0x18u,lanes[1]);
        });
        call_shape(0x3090beu,0x3090c1u,gp[eax],0x30u);
    }
    void second_shape() {
        auto &gp = this->gp;
        auto &lanes = x();
        guarded([&] {
            gp[edx] = argument(0x3090c1u,16u);
            gp[eax] = (gp[eax] & 0xffff0000u) | this->read(0x3090c4u,gp[esp] + 0x3cu,2);
            lanes[0] = local_vector(0x3090c9u,0x30u);
            lanes[1] = local_vector(0x3090ceu,0x40u);
            gp[ecx] = gp[esp] + 0x10u;
            this->push(0x3090d7u,gp[ecx]);
            lanes[1] = ordered_sub_ps(lanes[1],lanes[0]);
            this->push(0x3090dbu,gp[edx]);
            gp[ecx] = gp[esp] + 0x28u;
            cache_word(0x3090e0u,0u,2,gp[eax] & 0xffffu);
            write_vector(0x3090e3u,0x18u,lanes[1]);
            this->push(0x3090e8u,this->resume(0x3090edu));
        });
        this->enter(this->executable(0x2e47b0u,0x2e0cc0u),direction);
        guarded([&] {
            lanes[1] = local_vector(0x3090edu,0x20u);
            lanes[0] = this->read_vector(0x3090f2u,this->executable(0x461960u,0x418ae0u));
            gp[eax] = this->read(0x3090f9u,gp[ebx],4);
            gp[ecx] = gp[esp] + 0x70u;
            this->push(0x3090ffu,gp[ecx]);
            gp[edx] = gp[esp] + 0x24u;
            lanes[1] = _mm_xor_ps(lanes[1],lanes[0]);
            this->push(0x309107u,gp[edx]);
            gp[ecx] = gp[ebx];
            write_vector(0x30910au,0x28u,lanes[1]);
        });
        call_shape(0x30910fu,0x309112u,gp[eax],0x30u);
    }
    void finish_cache() {
        auto &gp = this->gp;
        guarded([&] {
            gp[eax] = (gp[eax] & 0xffff0000u) | this->read(0x309112u,gp[esp] + 0x7cu,2);
            cache_word(0x309117u,2u,2,gp[eax] & 0xffffu);
            gp[eax] = 0xffffu;
            cache_word(0x309120u,4u,2,gp[eax]);
            cache_word(0x309124u,6u,2,gp[eax]);
            gp[eax] = 0xff01u;
            cache_word(0x30912au,8u,1,1u);
            cache_word(0x30912du,9u,1,1u);
            gp[edx] = this->read(0x309130u,gp[edi],4);
            gp[ecx] = gp[edi];
        });
        call_shape(0x309134u,0x309137u,gp[edx],0x3cu);
        guarded([&] {
            cache_word(0x309137u,0xau,1,gp[eax] & 0xffu);
            gp[eax] = this->read(0x30913au,gp[ebx],4);
            gp[ecx] = gp[ebx];
        });
        call_shape(0x30913eu,0x309141u,gp[eax],0x3cu);
        guarded([&] {
            gp[edi] = this->pop(0x309141u);
            cache_word(0x309142u,0xbu,1,gp[eax] & 0xffu);
            gp[esi] = this->pop(0x309145u);
            gp[ebx] = this->pop(0x309146u);
            gp[esp] = gp[ebp];
            gp[ebp] = this->pop(0x309149u);
            this->ret(last,12u);
        });
        this->publish(last);
    }
    void run() {
        open_frame();
        support_centers();
        first_shape();
        second_shape();
        finish_cache();
        this->reached(0);
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Collision cache 309060"};
    return value;
}
template<class State>
bool canonical_cursor(const State &s, std::uint32_t *&cursor) {
    cursor = nullptr;
    if (s.image != SourcePhysicsContactImage::MapPack11 && s.image != SourcePhysicsContactImage::TitleUpdate15)
        return false;
    if ((_mm_getcsr() & 0x1f80u) != 0x1f80u)
        return false;
    if constexpr (requires {s.active_mmx;})
        if (s.active_mmx)
            return false;
    if (!s.services.instruction)
        return true;
    const auto *policy = s.live_instruction_cursor();
    if (!policy || (policy->first_trace && *policy->first_trace) ||
        (policy->second_trace && *policy->second_trace) || (s.observe && !policy->observation_only_stores))
        return false;
    for (const auto site : policy->service_sites)
        if (site >= first - observation_offset && site <= last - observation_offset)
            return false;
    cursor = policy->position;
    return true;
}

template<class State, class Reference, class Dispatch, class Transform, class Direction>
bool run_canonical(State &s, bool reference_mode, bool verify, native_small_queries_detail::VerifySummary &stats,
                   Reference reference, Dispatch dispatch, Transform transform, Direction direction) {
    std::uint32_t *cursor;
    if (reference_mode || !canonical_cursor(s,cursor))
        return false;
    native_object_tick_detail::execute(s,cursor,verify,stats.hit,stats.calls,stats,
        [&](State &v) {
            if constexpr (requires {v.selected_observation_pcs;})
                v.selected_observation_pcs = s.selected_observation_pcs;
            reference(v);
        },
        [&](State &v,std::uint32_t *position,Trace<State> *trace) {
            return Cache<State,Dispatch,Transform,Direction,true>{v,position,trace,dispatch,transform,direction};
        });
    return true;
}

inline auto &contact_summary() {
    static native_small_queries_detail::VerifySummary stats{"Contact collision cache 305560"};
    return stats;
}
inline auto &legacy_world_summary() {
    static native_small_queries_detail::VerifySummary stats{"World collision cache Map Pack 1.1"};
    return stats;
}

template<class State, class Reference, class Dispatch, class Transform, class Direction>
bool bridge_contact(State &s, Reference reference, Dispatch dispatch, Transform transform, Direction direction) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_CONTACT_CACHE_305560");
    static const bool verify = flag("HALO2_VERIFY_CONTACT_CACHE_305560");
    return run_canonical(s,force_reference,verify,contact_summary(),reference,dispatch,transform,direction);
}

template<class State, class Reference, class Dispatch, class Transform, class Direction>
bool bridge(State &s, Reference reference, Dispatch dispatch, Transform transform, Direction direction) {
    using native_small_queries_detail::flag;
    if (s.image == SourcePhysicsContactImage::MapPack11) {
        static const bool force_reference = flag("HALO2_REFERENCE_COLLISION_CACHE_MAPPACK11");
        static const bool verify = flag("HALO2_VERIFY_COLLISION_CACHE_MAPPACK11");
        return run_canonical(s,force_reference,verify,legacy_world_summary(),reference,dispatch,transform,direction);
    }
    if (!s.selected_observation_pcs || (_mm_getcsr() & 0x1f80u) != 0x1f80u)
        return false;
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return false;
    }
    static const bool force_reference = flag("HALO2_REFERENCE_COLLISION_CACHE_309060");
    static const bool verify = flag("HALO2_VERIFY_COLLISION_CACHE_309060");
    return native_small_queries_detail::run_owner(s,first,last,force_reference,verify,coverage(),summary(),
        [&](State &v) {
            v.selected_observation_pcs = true;
            reference(v);
        },
        [&](State &v,std::uint32_t *cursor,Trace<State> *trace) {
            return Cache<State,Dispatch,Transform,Direction>{v,cursor,trace,dispatch,transform,direction};
        });
}

} // namespace halo2::native_collision_cache_309060_detail

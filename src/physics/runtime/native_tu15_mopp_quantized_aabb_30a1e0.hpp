#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"

namespace halo2::native_mopp_quantized_aabb_30a1e0_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;

inline constexpr auto first = 0x30a1e0u;
inline constexpr auto last = 0x30a310u;
inline constexpr auto traverse_aabb = 0x309940u;

struct BoundSites {
    std::uint32_t bound, code, translation, scale, spill, reload, integer, result, cache;
    std::uint32_t coordinate, cache_offset;
};
// The lower and upper coordinates are interleaved in the original. Each
// loads the live source coordinate before reading the translation pointer.
inline constexpr std::array bounds{
    BoundSites{0x30a213u,0x30a216u,0x30a219u,0x30a21bu,0x30a21eu,0x30a222u,0x30a226u,
               0x30a22au,0x30a22fu,0x10u,0x10u},
    BoundSites{0x30a232u,0x30a235u,0x30a238u,0x30a23bu,0x30a23eu,0x30a242u,0x30a246u,
               0x30a24au,0x30a24fu,0x04u,0x24u},
    BoundSites{0x30a252u,0x30a255u,0x30a258u,0x30a25bu,0x30a25eu,0x30a262u,0x30a266u,
               0x30a26au,0x30a26fu,0x14u,0x14u},
    BoundSites{0x30a272u,0x30a275u,0x30a278u,0x30a27bu,0x30a27eu,0x30a282u,0x30a286u,
               0x30a28au,0x30a28fu,0x08u,0x28u},
};
inline constexpr BoundSites upper_z{
    0x30a292u,0x30a295u,0x30a298u,0x30a29bu,0x30a29eu,0x30a2a2u,0x30a2a6u,
    0u,0u,0x18u,0u};

template<class State, class Traverse>
struct AabbSetup : Port<State> {
    Traverse &traverse;
    AabbSetup(State &s, std::uint32_t *cursor, Trace<State> *trace, Traverse &child)
        : Port<State>(s,cursor,trace,{}), traverse(child) {}

    template<class Action> void preserve_fault_state(Action &&action) {
        try {
            action();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
    auto &fp() { return this->s.fp; }
    std::uint32_t local(std::uint32_t pc, std::uint32_t offset) {
        return this->read(pc,this->gp[esp] + offset,4);
    }
    void save_local(std::uint32_t pc, std::uint32_t offset, std::uint32_t value) {
        this->store(pc,this->gp[esp] + offset,4,value);
    }
    std::uint32_t cache_word(std::uint32_t pc, std::uint32_t offset) {
        return this->read(pc,this->gp[ecx] + offset,4);
    }
    void save_cache(std::uint32_t pc, std::uint32_t offset, std::uint32_t value) {
        this->store(pc,this->gp[ecx] + offset,4,value);
    }
    std::uint32_t signed_high(std::uint32_t pc, std::uint32_t offset) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(
            std::uint16_t(this->read(pc,this->gp[ecx] + offset,2)))));
    }
    long double extended(std::uint32_t pc, std::uint32_t address) {
        return physics_step_detail::ext(physics_step_detail::f(this->read(pc,address,4)));
    }
    void spill_bound(std::uint32_t spill, std::uint32_t reload, std::uint32_t integer,
                     std::uint32_t float_slot, std::uint32_t integer_slot) {
        this->at(spill);
        save_local(spill,float_slot,physics_step_detail::u(physics_step_detail::spill(fp().st(0))));
        fp().pop();
        fp().push(extended(reload,this->gp[esp] + float_slot));
        this->at(integer);
        save_local(integer,integer_slot,physics_contact_runtime_detail::integer_spill(fp().st(0)));
        fp().pop();
    }
    void open_and_quantize_lower_x() {
        using namespace physics_step_detail;
        auto &gp = this->gp;
        this->push(first,gp[ebp]);
        gp[ebp] = gp[esp];
        gp[esp] = this->flags.logic(gp[esp] & 0xfffffff0u,4);
        gp[esp] = this->flags.minus(gp[esp],0x4cu,4);
        gp[eax] = this->read(0x30a1e9u,gp[ebp] + 0x10u,4);
        gp[edx] = this->read(0x30a1ecu,gp[ebp] + 0x08u,4);
        save_cache(0x30a1efu,0x00u,gp[eax]);
        gp[eax] = this->read(0x30a1f1u,gp[ebp] + 0x0cu,4);
        save_cache(0x30a1f4u,0x30u,gp[edx]);
        fp().push(extended(0x30a1f7u,gp[eax]));
        fp().st(0) = sub(fp().st(0),extended(0x30a1f9u,gp[edx]));
        this->push(0x30a1fbu,gp[esi]);
        fp().st(0) = mul(fp().st(0),extended(0x30a1fcu,gp[edx] + 0x0cu));
        spill_bound(0x30a1ffu,0x30a203u,0x30a207u,8u,12u);
        gp[esi] = local(0x30a20bu,12u);
        gp[esi] = this->flags.increment(gp[esi],4,true);
        save_cache(0x30a210u,0x20u,gp[esi]);
    }
    void quantize(const BoundSites &site) {
        using namespace physics_step_detail;
        auto &gp = this->gp;
        fp().push(extended(site.bound,gp[eax] + site.coordinate));
        gp[esi] = cache_word(site.code,0x30u);
        fp().st(0) = sub(fp().st(0),extended(site.translation,gp[esi] + (site.coordinate & 15u)));
        fp().st(0) = mul(fp().st(0),extended(site.scale,gp[esi] + 0x0cu));
        spill_bound(site.spill,site.reload,site.integer,12u,8u);
    }
    void stage_intervals() {
        auto &gp = this->gp;
        gp[esi] = signed_high(0x30a2aau,0x22u);
        gp[eax] = local(0x30a2aeu,8u);
        save_local(0x30a2b2u,0x20u,gp[esi]);
        gp[esi] = signed_high(0x30a2b6u,0x12u);
        gp[eax] = this->flags.increment(gp[eax],4,true);
        gp[esi] = this->flags.increment(gp[esi],4,false);
        save_local(0x30a2bcu,0x10u,gp[esi]);
        gp[esi] = signed_high(0x30a2c0u,0x26u);
        save_local(0x30a2c4u,0x24u,gp[esi]);
        gp[esi] = signed_high(0x30a2c8u,0x16u);
        gp[esi] = this->flags.increment(gp[esi],4,false);
        save_cache(0x30a2cdu,0x18u,gp[eax]);
        gp[eax] = this->flags.right(gp[eax],16u,4,true);
        save_local(0x30a2d3u,0x14u,gp[esi]);
        gp[esi] = signed_high(0x30a2d7u,0x2au);
        gp[eax] = this->flags.increment(gp[eax],4,false);
        save_local(0x30a2dcu,0x18u,gp[eax]);
        save_local(0x30a2e0u,0x28u,gp[esi]);
    }
    void stage_traversal() {
        auto &gp = this->gp;
        gp[eax] = this->flags.logic(0u,4);
        gp[edx] = this->flags.plus(gp[edx],0x30u,4);
        this->push(0x30a2e9u,gp[edx]);
        gp[edx] = gp[esp] + 0x14u;
        this->push(0x30a2eeu,gp[edx]);
        constexpr std::array sites{0x30a2efu,0x30a2f3u,0x30a2f7u,0x30a2fbu,0x30a2ffu,0x30a303u};
        for (unsigned i = 0; i < sites.size(); ++i)
            save_local(sites[i],0x38u + 4u*i,0u);
        this->push(0x30a307u,0x30a30cu);
    }
    void run() {
        preserve_fault_state([&] {
            open_and_quantize_lower_x();
            for (const auto &site : bounds) {
                quantize(site);
                this->gp[esi] = local(site.result,8u);
                this->gp[esi] = this->flags.increment(this->gp[esi],4,true);
                save_cache(site.cache,site.cache_offset,this->gp[esi]);
            }
            quantize(upper_z);
            stage_intervals();
            stage_traversal();
        });
        this->enter(traverse_aabb,traverse);
        preserve_fault_state([&] {
            this->gp[esi] = this->pop(0x30a30cu);
            this->gp[esp] = this->gp[ebp];
            this->gp[ebp] = this->pop(0x30a30fu);
            this->ret(last,12u);
        });
        this->publish(last);
        this->reached(0);
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"MOPP quantized AABB 30A1E0"};
    return value;
}
template<class State, class Reference, class Traverse>
bool bridge(State &s, Reference reference, Traverse traverse) {
    using native_small_queries_detail::flag;
    if (!s.selected_observation_pcs)
        return false;
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return false;
    }
    static const bool force_reference = flag("HALO2_REFERENCE_MOPP_QUANTIZED_AABB_30A1E0");
    static const bool verify = flag("HALO2_VERIFY_MOPP_QUANTIZED_AABB_30A1E0");
    return native_small_queries_detail::run_owner(s,first,last,force_reference,verify,coverage(),summary(),
        [&](State &v) {
            v.selected_observation_pcs = true;
            reference(v);
        },
        [&](State &v,std::uint32_t *cursor,Trace<State> *trace) {
            return AabbSetup<State,Traverse>{v,cursor,trace,traverse};
        });
}

} // namespace halo2::native_mopp_quantized_aabb_30a1e0_detail

#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_contact_cache_update_30cd00_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_tu15_leaves::ordered_add_ps;
using native_tu15_leaves::ordered_mul_ps;
using native_world_leaves_detail::ordered_add_ss;
using native_world_leaves_detail::ordered_sub_ps;
using physics_contact_runtime_detail::source_load_x87_exception_flags;
using physics_contact_runtime_detail::source_x87_exception_flags;

inline constexpr auto first = 0x30cd00u;
inline constexpr auto last = 0x30cf31u;
inline constexpr auto transform_point = 0x2e4690u;
inline constexpr auto reduce_candidates = 0x30caa0u;
inline constexpr auto observation_offset = 0x3af0u;

struct DistanceSites {
    std::uint32_t input, spill, reload;
    std::uint32_t input_offset, spill_offset;
    unsigned origin_lane;
};
inline constexpr DistanceSites initial_distance{0x30cd40u,0x30cd63u,0x30cd67u,0u,0x18u,1u};
inline constexpr DistanceSites secondary_distance{0x30cd78u,0x30cd9cu,0x30cda0u,0x20u,0x14u,2u};

struct X87Replay {
    struct Environment {
        std::uint16_t control{}, exceptions{};
        static Environment capture() {
            Environment value;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
            asm volatile("fnstcw %0":"=m"(value.control));
#else
            throw std::runtime_error("Contact cache x87 control backend unavailable");
#endif
            value.exceptions = source_x87_exception_flags();
            return value;
        }
        void restore() const {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
            asm volatile("fldcw %0"::"m"(control):"memory");
#else
            throw std::runtime_error("Contact cache x87 control backend unavailable");
#endif
            source_load_x87_exception_flags(exceptions);
        }
        bool operator==(const Environment &) const = default;
    };
    std::array<Environment,4> children{};
    unsigned recorded{}, replayed{};
    void record() {
        children.at(recorded++) = Environment::capture();
    }
    template<class State> void child(State &s,std::uint32_t target) {
        native_object_tick_detail::replay(s,target);
        if (replayed == recorded)
            throw std::runtime_error("Contact cache x87 replay made an extra child call");
        children[replayed++].restore();
    }
};

template<class State, class Dispatch, class Transform>
struct Update : Port<State> {
    Dispatch &dispatch;
    Transform &transform;
    X87Replay &x87;
    Update(State &s, std::uint32_t *cursor, Trace<State> *trace, Dispatch &route, Transform &child,X87Replay &fp)
        : Port<State>(s,cursor,trace,{}), dispatch(route), transform(child), x87(fp) {}

    template<class Action> void preserve_fault_state(Action &&action) {
        try {
            action();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
    auto &x() { return this->s.xmm; }
    auto &fp() { return this->s.fp; }
    std::uint32_t arg(std::uint32_t pc,std::uint32_t offset) {
        return this->read(pc,this->gp[ebp] + offset,4);
    }
    std::uint32_t local(std::uint32_t pc,std::uint32_t offset) {
        return this->read(pc,this->gp[esp] + offset,4);
    }
    void save_local(std::uint32_t pc,std::uint32_t offset,std::uint32_t value) {
        this->store(pc,this->gp[esp] + offset,4,value);
    }
    __m128 local_vector(std::uint32_t pc,std::uint32_t offset) {
        return this->read_vector(pc,this->gp[esp] + offset);
    }
    void save_vector(std::uint32_t pc,std::uint32_t offset,__m128 value) {
        this->store_vector(pc,this->gp[esp] + offset,value);
    }
    long double load_float(std::uint32_t pc,std::uint32_t address) {
        return physics_step_detail::ext(physics_step_detail::f(this->read(pc,address,4)));
    }
    void discard_fp(std::uint32_t pc) {
        this->at(pc);
        (void)fp().st(0);
        fp().pop();
    }
    bool below_threshold() {
        auto &eax_value = this->gp[eax];
        eax_value = (eax_value & 0xffff0000u) | fp().word();
        this->flags.logic((eax_value >> 8u) & 5u,1);
        return !this->flags.pf;
    }
    void close_frame(std::uint32_t pc) {
        auto &gp = this->gp;
        gp[edi] = this->pop(pc);
        gp[esi] = this->pop(pc + 1u);
        gp[ebx] = this->pop(pc + 2u);
        gp[esp] = gp[ebp];
        gp[ebp] = this->pop(pc + 5u);
        this->ret(pc + 6u,0u);
        this->publish(pc + 6u);
    }
    void route(std::uint32_t pc,std::uint32_t target) {
        this->enter(target,[&](State &s) { dispatch(s,target,pc - observation_offset,false); });
        if (this->trace)
            x87.record();
    }
    void transform_contact() {
        this->enter(transform_point,transform);
        if (this->trace)
            x87.record();
    }
    bool open_frame() {
        auto &gp = this->gp;
        this->push(first,gp[ebp]);
        gp[ebp] = gp[esp];
        gp[esp] = this->flags.logic(gp[esp] & 0xfffffff0u,4);
        gp[esp] = this->flags.minus(gp[esp],0xa4u,4);
        gp[eax] = arg(0x30cd0cu,0x10u);
        gp[ecx] = this->read(0x30cd0fu,gp[eax] + 0x0cu,4);
        fp().push(load_float(0x30cd12u,gp[ecx] + 4u));
        gp[eax] = arg(0x30cd15u,0x20u);
        gp[ecx] = this->read(0x30cd18u,gp[eax],4);
        this->at(0x30cd1au);
        fp().push(fp().st(0));
        this->push(0x30cd1cu,gp[ebx]);
        this->at(0x30cd1du);
        fp().st(1) = physics_step_detail::mul(fp().st(1),fp().st(0));
        fp().pop();
        this->push(0x30cd1fu,gp[esi]);
        gp[edx] = this->flags.logic(0xffffffffu,4);
        gp[esi] = this->flags.logic(0u,4);
        this->test32(gp[ecx]);
        this->push(0x30cd27u,gp[edi]);
        gp[edi] = arg(0x30cd28u,0x14u);
        return !this->flags.zf && this->flags.sf == this->flags.of;
    }
    void distance(const DistanceSites &site) {
        auto &gp = this->gp;
        auto &lanes = x();
        lanes[0] = this->read_vector(site.input,gp[ecx] + site.input_offset);
        lanes[0] = ordered_sub_ps(lanes[0],lanes[site.origin_lane]);
        lanes[0] = ordered_mul_ps(lanes[0],lanes[0]);
        lanes[2] = _mm_shuffle_ps(lanes[0],lanes[0],85);
        lanes[3] = lanes[0];
        gp[eax] = gp[esp] + site.spill_offset;
        lanes[2] = ordered_add_ss(lanes[2],lanes[0]);
        lanes[3] = _mm_shuffle_ps(lanes[3],lanes[0],170);
        lanes[3] = ordered_add_ss(lanes[3],lanes[2]);
        this->store(site.spill,gp[eax],4,physics_step_detail::u(_mm_cvtss_f32(lanes[3])));
        fp().push(load_float(site.reload,gp[esp] + site.spill_offset));
    }
    void scan() {
        auto &gp = this->gp;
        x()[1] = this->read_vector(0x30cd31u,gp[edi] + 0x10u);
        gp[ebx] = gp[ecx];
        gp[ecx] = arg(0x30cd37u,0x1cu);
        do {
            distance(initial_distance);
            this->at(0x30cd6bu);
            fp().comparison(fp().st(1),1);
            if (below_threshold()) {
                this->reached(1);
                x()[2] = this->read_vector(0x30cd74u,gp[edi] + 0x40u);
                distance(secondary_distance);
                fp().comparison(load_float(0x30cda4u,0x46a560u),1);
                if (below_threshold()) {
                    this->reached(2);
                    discard_fp(0x30cdb1u);
                    gp[edx] = gp[esi];
                    fp().push(load_float(0x30cdb5u,gp[esp] + 0x18u));
                }
            }
            gp[esi] = this->flags.increment(gp[esi],4,false);
            gp[ecx] = this->flags.plus(gp[ecx],0x30u,4);
            this->compare32(gp[esi],gp[ebx]);
        } while (this->flags.sf != this->flags.of);
        this->test32(gp[edx]);
        discard_fp(0x30cdc7u);
    }
    void replace_matching() {
        auto &gp = this->gp;
        auto &lanes = x();
        gp[ecx] = arg(0x30cdcbu,0x1cu);
        gp[eax] = this->flags.shift(gp[edx] * 3u,4u,4);
        fp().push(load_float(0x30cdd4u,gp[eax] + gp[ecx] + 0x1cu));
        gp[eax] = this->flags.plus(gp[eax],gp[ecx],4);
        this->store_vector(0x30cddau,gp[eax],lanes[1]);
        lanes[0] = this->read_vector(0x30cdddu,gp[edi] + 0x30u);
        this->store_vector(0x30cde1u,gp[eax] + 0x10u,lanes[0]);
        lanes[0] = this->read_vector(0x30cde5u,gp[edi] + 0x40u);
        this->store_vector(0x30cde9u,gp[eax] + 0x20u,lanes[0]);
        gp[ecx] = this->read(0x30cdedu,gp[edi] + 0x20u,4);
        this->store(0x30cdf0u,gp[eax] + 0x2cu,4,gp[ecx]);
        gp[edx] = this->read(0x30cdf3u,gp[edi] + 0x24u,4);
        this->at(0x30cdf6u);
        this->store(0x30cdf6u,gp[eax] + 0x1cu,4,physics_step_detail::u(physics_step_detail::spill(fp().st(0))));
        fp().pop();
        this->store(0x30cdf9u,gp[eax] + 0x0cu,4,gp[edx]);
        close_frame(0x30cdfcu);
    }
    void allocate_slot(bool scanned) {
        auto &gp = this->gp;
        if (scanned) {
            this->compare32(gp[esi],4u);
            if (this->flags.sf == this->flags.of) {
                gp[esi] = gp[esp] + 0x80u;
                this->reached(4);
                return;
            }
        } else {
            discard_fp(0x30cf24u);
            this->reached(0);
        }
        gp[eax] = arg(0x30cf26u,0x1cu);
        gp[esi] = this->flags.shift(gp[esi] * 3u,4u,4);
        gp[esi] = this->flags.plus(gp[esi],gp[eax],4);
        this->reached(5);
    }
    void copy_and_transform() {
        auto &gp = this->gp;
        auto &lanes = x();
        preserve_fault_state([&] {
            lanes[0] = this->read_vector(0x30ce13u,gp[edi] + 0x10u);
            gp[edx] = arg(0x30ce17u,0x0cu);
            this->store_vector(0x30ce1au,gp[esi],lanes[0]);
            lanes[0] = this->read_vector(0x30ce1du,gp[edi] + 0x30u);
            this->store_vector(0x30ce21u,gp[esi] + 0x10u,lanes[0]);
            lanes[0] = this->read_vector(0x30ce25u,gp[edi] + 0x40u);
            this->store_vector(0x30ce29u,gp[esi] + 0x20u,lanes[0]);
            gp[eax] = this->read(0x30ce2du,gp[edi] + 0x24u,4);
            this->store(0x30ce30u,gp[esi] + 0x0cu,4,gp[eax]);
            gp[ecx] = this->read(0x30ce33u,gp[edi] + 0x20u,4);
            gp[eax] = arg(0x30ce36u,8u);
            this->store(0x30ce39u,gp[esi] + 0x2cu,4,gp[ecx]);
            gp[ecx] = this->read(0x30ce3cu,gp[eax] + 8u,4);
            gp[ebx] = this->read(0x30ce3fu,gp[edx] + 8u,4);
            gp[ecx] = this->flags.plus(gp[ecx],0x20u,4);
            this->push(0x30ce45u,gp[esi]);
            this->push(0x30ce46u,gp[ecx]);
            gp[ecx] = gp[esp] + 0x48u;
            gp[ebx] = this->flags.plus(gp[ebx],0x20u,4);
            this->push(0x30ce4eu,0x30ce53u);
        });
        transform_contact();
        preserve_fault_state([&] {
            gp[eax] = gp[esi] + 0x10u;
            this->push(0x30ce56u,gp[eax]);
            this->push(0x30ce57u,gp[ebx]);
            gp[ecx] = gp[esp] + 0x38u;
            this->push(0x30ce5cu,0x30ce61u);
        });
        transform_contact();
    }
    void add_to_receiver() {
        auto &gp = this->gp;
        auto &lanes = x();
        std::uint32_t target{};
        preserve_fault_state([&] {
            gp[edx] = this->read(0x30ce61u,gp[edi] + 0x24u,4);
            lanes[1] = local_vector(0x30ce64u,0x40u);
            lanes[3] = this->read_vector(0x30ce69u,0x41c500u);
            gp[eax] = this->read(0x30ce70u,gp[esi] + 0x2cu,4);
            gp[ecx] = arg(0x30ce73u,0x10u);
            gp[edi] = arg(0x30ce76u,0x18u);
            save_local(0x30ce79u,0x14u,gp[edx]);
            lanes[0] = _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(local(0x30ce7du,0x14u))));
            gp[edx] = this->read(0x30ce83u,gp[edi],4);
            lanes[0] = _mm_shuffle_ps(lanes[0],lanes[0],0);
            lanes[2] = ordered_mul_ps(lanes[0],lanes[1]);
            lanes[1] = local_vector(0x30ce8fu,0x30u);
            lanes[3] = ordered_sub_ps(lanes[3],lanes[0]);
            lanes[0] = this->read_vector(0x30ce97u,gp[esi] + 0x20u);
            lanes[3] = ordered_mul_ps(lanes[3],lanes[1]);
            lanes[3] = ordered_add_ps(lanes[3],lanes[2]);
            save_vector(0x30cea1u,0x50u,lanes[3]);
            save_local(0x30cea6u,0x5cu,gp[eax]);
            gp[eax] = gp[esp] + 0x50u;
            this->push(0x30ceaeu,gp[eax]);
            gp[eax] = arg(0x30ceafu,0x0cu);
            this->push(0x30ceb2u,gp[ecx]);
            gp[ecx] = arg(0x30ceb3u,8u);
            this->push(0x30ceb6u,gp[eax]);
            this->push(0x30ceb7u,gp[ecx]);
            gp[ecx] = gp[edi];
            save_vector(0x30cebau,0x70u,lanes[0]);
            target = this->read(0x30cebfu,gp[edx] + 4u,4);
            this->push(0x30cebfu,0x30cec2u);
        });
        route(0x30cebfu,target);
    }
    bool record_contact() {
        auto &gp = this->gp;
        this->flags.minus(gp[eax] & 0xffffu,0xffffu,2);
        if (this->flags.zf) {
            this->reached(6);
            return false;
        }
        gp[ebx] = arg(0x30cec8u,0x20u);
        gp[edx] = this->flags.logic((gp[eax] & 0xffffu) | 0x3f000000u,4);
        this->store(0x30ced4u,gp[esi] + 0x1cu,4,gp[edx]);
        gp[ecx] = this->read(0x30ced7u,gp[ebx],4);
        gp[ecx] = this->flags.increment(gp[ecx],4,false);
        gp[eax] = gp[ecx];
        this->compare32(gp[eax],4u);
        this->store(0x30cedfu,gp[ebx],4,gp[ecx]);
        const bool reduce = !this->flags.zf && this->flags.sf == this->flags.of;
        this->reached(reduce ? 8u : 7u);
        return reduce;
    }
    void trim_cache() {
        auto &gp = this->gp;
        preserve_fault_state([&] {
            gp[eax] = arg(0x30cee3u,0x1cu);
            gp[ecx] = gp[eax] + 0x30u;
            gp[edx] = gp[eax] + 0x60u;
            save_local(0x30ceecu,0x1cu,gp[eax]);
            gp[eax] = this->flags.plus(gp[eax],0x90u,4);
            save_local(0x30cef5u,0x28u,gp[eax]);
            gp[eax] = arg(0x30cef9u,0x0cu);
            this->push(0x30cefcu,gp[edi]);
            save_local(0x30cefdu,0x24u,gp[ecx]);
            gp[ecx] = arg(0x30cf01u,8u);
            this->push(0x30cf04u,gp[eax]);
            save_local(0x30cf05u,0x2cu,gp[edx]);
            this->push(0x30cf09u,gp[ecx]);
            gp[edx] = gp[esp] + 0x28u;
            this->push(0x30cf0eu,gp[edx]);
            save_local(0x30cf0fu,0x3cu,gp[esi]);
            this->push(0x30cf13u,0x30cf18u);
        });
        route(0x30cf13u,reduce_candidates);
        preserve_fault_state([&] {
            gp[esp] = this->flags.plus(gp[esp],0x10u,4);
            this->store(0x30cf1bu,gp[ebx],4,gp[eax]);
        });
    }
    void run() {
        bool existing{};
        preserve_fault_state([&] {
            const bool scanned = open_frame();
            if (scanned) {
                scan();
                existing = this->flags.sf == this->flags.of;
            }
            if (existing) {
                this->reached(3);
                replace_matching();
            } else {
                allocate_slot(scanned);
            }
        });
        if (existing)
            return;
        copy_and_transform();
        add_to_receiver();
        bool reduce{};
        preserve_fault_state([&] { reduce = record_contact(); });
        if (reduce)
            trim_cache();
        preserve_fault_state([&] { close_frame(0x30cf1du); });
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Contact cache update 30CD00"};
    return value;
}
template<class State, class Reference, class Dispatch, class Transform>
bool bridge(State &s, Reference reference, Dispatch dispatch, Transform transform) {
    using native_small_queries_detail::flag;
    if (!s.selected_observation_pcs || (_mm_getcsr() & 0x1f80u) != 0x1f80u)
        return false;
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return false;
    }
    static const bool force_reference = flag("HALO2_REFERENCE_CONTACT_CACHE_UPDATE_30CD00");
    static const bool verify = flag("HALO2_VERIFY_CONTACT_CACHE_UPDATE_30CD00");
    X87Replay x87;
    const auto entry_x87 = verify ? X87Replay::Environment::capture() : X87Replay::Environment{};
    return native_small_queries_detail::run_owner(s,first,last,force_reference,verify,coverage(),summary(),
        [&](State &v) {
            // FCOM exposes the physical sticky bits while the logical x87
            // stack is replayed separately. Start from the original entry
            // flags and reapply the flags returned by each genuine child.
            const auto live_x87 = X87Replay::Environment::capture();
            entry_x87.restore();
            v.selected_observation_pcs = true;
            try {
                reference(v,x87);
            } catch (...) {
                live_x87.restore();
                throw;
            }
            const auto expected_x87 = X87Replay::Environment::capture();
            live_x87.restore();
            if (x87.recorded != x87.replayed || expected_x87 != live_x87)
                throw std::runtime_error("Contact cache x87 replay differs");
        },
        [&](State &v,std::uint32_t *cursor,Trace<State> *trace) {
            return Update<State,Dispatch,Transform>{v,cursor,trace,dispatch,transform,x87};
        });
}

} // namespace halo2::native_contact_cache_update_30cd00_detail

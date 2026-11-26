#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"

namespace halo2::native_convex_process_prefix_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;

constexpr std::uint32_t first = 0x2f8b60u;
constexpr std::uint32_t last = 0x2f8c87u;
enum class Continuation { retained, compose_transforms, remove_cached };

struct ClockSamples {
    std::array<std::uint64_t,3> values{};
    unsigned recorded{}, replayed{};
    void record(std::uint64_t value) { values.at(recorded++) = value; }
    std::uint64_t read() {
        if (replayed == recorded)
            throw std::runtime_error("Convex process timestamp replay exhausted");
        return values[replayed++];
    }
};

struct Profile {
    std::uint32_t marker, label, timestamp, save, restore, stamp, advance;
    unsigned scratch, stamp_register;
};
constexpr Profile entry_profile{
    0x41cd44u,0x2f8b80u,0x2f8b86u,0x2f8b88u,0x2f8b8cu,0x2f8b90u,0x2f8b96u,0x1cu,eax};
constexpr Profile separation_profile{
    0x41cd3cu,0x2f8bcau,0x2f8bd0u,0x2f8bd2u,0x2f8bd6u,0x2f8bdau,0x2f8be0u,0x14u,edx};
constexpr Profile transform_profile{
    0x41cd34u,0x2f8c5du,0x2f8c63u,0x2f8c65u,0x2f8c69u,0x2f8c6du,0x2f8c73u,0x14u,eax};

template<class State,bool Record = true>
struct ConvexProcess : Port<State> {
    ClockSamples &clocks;
    Continuation &next;
    ConvexProcess(State &s,std::uint32_t *cursor,Trace<State> *trace,ClockSamples &samples,Continuation &result)
        : Port<State>(s,cursor,trace,{}), clocks(samples), next(result) {}

    // Use Port only for ordered memory and optional replay. Keep the live
    // architectural state current before every provider/observer boundary.
    void load() {}
    void push(std::uint32_t pc,std::uint32_t value) {
        const auto address = this->s.gp[esp] -= 4u;
        this->store(pc,address,value);
    }
    void at(std::uint32_t pc) {
        if constexpr (Record)
            Port<State>::at(pc);
        else if (this->cursor)
            *this->cursor = pc;
    }
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width) {
        if constexpr (Record)
            return Port<State>::read(pc,address,width);
        at(pc);
        return physics_step_detail::r(this->s.memory,address,width);
    }
    __m128 read_vector(std::uint32_t pc,std::uint32_t address) {
        if constexpr (Record)
            return Port<State>::read_vector(pc,address);
        at(pc);
        return physics_step_detail::v(this->s.memory,address);
    }
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        if constexpr (Record) {
            Port<State>::store(pc,address,4,value);
        } else {
            at(pc);
            if constexpr (std::endian::native == std::endian::little) {
                if (!this->s.observe && this->s.memory.try_callback_free_write(
                        address,std::as_bytes(std::span{&value,1})))
                    return;
            }
            physics_step_detail::w(this->s.memory,this->s.observe,pc,address,4,value);
        }
    }
    void branch(unsigned bit) {
        if constexpr (Record)
            this->reached(bit);
    }
    std::uint32_t local(std::uint32_t pc,unsigned offset) {
        return this->read(pc,this->s.gp[esp] + offset,4);
    }
    void save(std::uint32_t pc,unsigned offset,std::uint32_t value) {
        this->store(pc,this->s.gp[esp] + offset,value);
    }
    long double scalar(std::uint32_t pc,std::uint32_t address) {
        using namespace physics_step_detail;
        return ext(f(this->read(pc,address,4)));
    }
    void profile(const Profile &site) {
        auto &g = this->s.gp;
        this->store(site.label,g[ecx],site.marker);
        this->at(site.timestamp);
        if (!this->s.services.timestamp_counter)
            throw std::runtime_error("Required original contact RDTSC owner unavailable");
        const auto stamp = this->s.services.timestamp_counter();
        if (this->trace)
            clocks.record(stamp);
        g[eax] = std::uint32_t(stamp);
        g[edx] = std::uint32_t(stamp >> 32u);
        save(site.save,site.scratch,g[eax]);
        g[site.stamp_register] = local(site.restore,site.scratch);
        this->store(site.stamp,g[ecx] + 4u,g[site.stamp_register]);
        g[ecx] = this->s.flags.plus(g[ecx],12u,4);
        this->store(site.advance,0x48c974u,g[ecx]);
    }
    void compare_limit(std::uint32_t pc) {
        this->s.flags.minus(this->s.gp[ecx],this->read(pc,0x48c978u,4),4);
    }
    void open_frame() {
        auto &g = this->s.gp;
        this->push(first,g[ebp]);
        g[ebp] = g[esp];
        g[esp] = this->s.flags.logic(g[esp] & 0xfffffff0u,4);
        g[esp] = this->s.flags.minus(g[esp],0x224u,4);
        g[eax] = this->read(0x2f8b6cu,0x48c978u,4);
        this->push(0x2f8b71u,g[ebx]);
        this->push(0x2f8b72u,g[esi]);
        g[esi] = g[ecx];
        g[ecx] = this->read(0x2f8b75u,0x48c974u,4);
        this->s.flags.minus(g[ecx],g[eax],4);
        this->push(0x2f8b7du,g[edi]);
        if (this->s.flags.cf) {
            branch(0);
            profile(entry_profile);
        }
    }
    void compare_distance(std::uint32_t pc) {
        this->s.fp.comparison(scalar(pc,0x46a0e8u),1);
    }
    void set_ax_from_status() {
        auto &a = this->s.gp[eax];
        a = (a & 0xffff0000u) | this->s.fp.word();
        this->s.flags.logic((a >> 8u) & 0x41u,1);
    }
    bool prepared_shapes_need_projection() {
        auto &g = this->s.gp;
        this->s.fp.push(scalar(0x2f8b9cu,g[esi] + 0x2cu));
        g[ebx] = this->read(0x2f8b9fu,g[ebp] + 12u,4);
        compare_distance(0x2f8ba2u);
        g[eax] = this->read(0x2f8ba8u,g[ebx],4);
        g[edi] = this->read(0x2f8baau,g[ebp] + 8u,4);
        g[edx] = this->read(0x2f8badu,g[edi],4);
        save(0x2f8bafu,0x1cu,g[eax]);
        g[eax] = (g[eax] & 0xffff0000u) | this->s.fp.word();
        save(0x2f8bb5u,0x18u,g[edx]);
        this->s.flags.logic((g[eax] >> 8u) & 0x41u,1);
        return this->s.flags.zf;
    }
    bool separated_after_motion() {
        using namespace physics_step_detail;
        using native_tu15_leaves::ordered_mul_ps;
        auto &g = this->s.gp;
        auto &x = this->s.xmm;
        auto &fp = this->s.fp;
        compare_limit(0x2f8bc2u);
        if (this->s.flags.cf) {
            branch(1);
            profile(separation_profile);
        }

        g[eax] = this->read(0x2f8be6u,g[edi] + 8u,4);
        x[0] = this->read_vector(0x2f8be9u,g[eax]);
        g[ecx] = this->read(0x2f8becu,g[ebx] + 8u,4);
        x[2] = this->read_vector(0x2f8befu,g[eax] + 16u);
        fp.push(scalar(0x2f8bf3u,g[ecx] + 12u));
        x[3] = this->read_vector(0x2f8bf6u,g[ecx] + 16u);
        x[1] = this->read_vector(0x2f8bfau,g[esi] + 0x20u);
        g[edx] = this->read(0x2f8bfeu,g[edi] + 8u,4);
        fp.st(0) = add(fp.st(0),scalar(0x2f8c01u,g[edx] + 12u));
        x[2] = _mm_sub_ps(x[2],x[0]);
        x[0] = this->read_vector(0x2f8c07u,g[ecx]);
        x[3] = _mm_sub_ps(x[3],x[0]);
        x[0] = ordered_mul_ps(_mm_sub_ps(x[3],x[2]),x[1]);
        x[1] = _mm_add_ss(_mm_shuffle_ps(x[0],x[0],85),x[0]);
        g[eax] = g[esp] + 0x14u;
        x[2] = _mm_add_ss(_mm_shuffle_ps(x[0],x[0],170),x[1]);
        save(0x2f8c30u,0x14u,u(_mm_cvtss_f32(x[2])));
        fp.st(0) = add(fp.st(0),scalar(0x2f8c34u,g[esp] + 0x14u));
        fp.st(0) = sub(scalar(0x2f8c38u,g[esi] + 0x2cu),fp.st(0));
        this->at(0x2f8c3bu);
        this->store(0x2f8c3bu,g[esi] + 0x2cu,u(spill(fp.st(0))));
        compare_distance(0x2f8c3eu);
        set_ax_from_status();
        return this->s.flags.zf;
    }
    void stage_transforms() {
        auto &g = this->s.gp;
        compare_limit(0x2f8c55u);
        if (this->s.flags.cf) {
            branch(2);
            profile(transform_profile);
        }
        g[ecx] = this->read(0x2f8c79u,g[ebx] + 8u,4);
        g[edx] = this->read(0x2f8c7cu,g[edi] + 8u,4);
        g[ecx] = this->s.flags.plus(g[ecx],0x20u,4);
        this->push(0x2f8c82u,g[ecx]);
        g[edx] = this->s.flags.plus(g[edx],0x20u,4);
        this->push(0x2f8c86u,g[edx]);
        g[ecx] = g[esp] + 0x88u;
    }
    void run() {
        open_frame();
        if (prepared_shapes_need_projection()) {
            branch(3);
            if (separated_after_motion()) {
                branch(4);
                next = Continuation::remove_cached;
                this->at(0x2f8c49u);
                return;
            }
            this->s.gp[ecx] = this->read(0x2f8c4fu,0x48c974u,4);
        }
        stage_transforms();
        next = Continuation::compose_transforms;
        this->at(last);
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Convex contact prefix 2F8B60"};
    return value;
}

template<class State,class Reference>
Continuation prefix(State &s,Reference reference) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_CONVEX_PROCESS_PREFIX_2F8B60");
    static const bool verify = flag("HALO2_VERIFY_CONVEX_PROCESS_PREFIX_2F8B60");
    if (force_reference || s.image != SourcePhysicsContactImage::TitleUpdate15 ||
        s.active_mmx || (_mm_getcsr() & 0x1f80u) != 0x1f80u)
        return Continuation::retained;

    std::uint32_t *position{};
    if (s.services.instruction) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !s.literal_service_sites || !State::has_literal_service_sites(*cursor) ||
            (cursor->first_trace && *cursor->first_trace) ||
            (cursor->second_trace && *cursor->second_trace) ||
            (s.observe && !cursor->observation_only_stores))
            return Continuation::retained;
        position = cursor->position;
    } else if (s.observe || s.instruction_cursor) {
        return Continuation::retained;
    }

    ClockSamples clocks;
    auto next = Continuation::retained;
    if (!verify) {
        ConvexProcess<State,false>{s,position,nullptr,clocks,next}.run();
        return next;
    }
    auto &totals = summary();
    native_object_tick_detail::execute(s,position,true,coverage(),totals.calls,totals,
        [&](State &v) {
            if (reference(v,clocks) != next || clocks.replayed != clocks.recorded)
                throw std::runtime_error("Convex process prefix chose a different continuation or clock path");
        }, [&](State &v,std::uint32_t *cursor,Trace<State> *trace) {
            return ConvexProcess<State>{v,cursor,trace,clocks,next};
        });
    totals.hit = coverage().load();
    return next;
}
} // namespace halo2::native_convex_process_prefix_detail

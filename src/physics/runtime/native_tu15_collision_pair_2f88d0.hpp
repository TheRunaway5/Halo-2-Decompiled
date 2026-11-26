#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"

namespace halo2::native_collision_pair_2f88d0_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_tu15_leaves::ordered_add_ps;
using native_tu15_leaves::ordered_mul_ps;

inline constexpr auto first = 0x2f88d0u;
inline constexpr auto last = 0x2f8b5eu;

struct ClockSamples {
    struct Sample { std::uint32_t pc; std::uint64_t value; };
    std::array<Sample,2> samples{};
    unsigned recorded{}, replayed{};
    void record(std::uint32_t pc, std::uint64_t value) {
        samples.at(recorded++) = {pc,value};
    }
    std::uint64_t read(std::uint32_t pc) {
        if (replayed == recorded || samples[replayed].pc != pc)
            throw std::runtime_error("Collision pair timestamp replay differs");
        return samples[replayed++].value;
    }
};
struct ProfileSites {
    std::uint32_t mark, timestamp, scratch_write, scratch_read, value, advance, marker;
};
inline constexpr ProfileSites begin_profile{
    0x2f88edu,0x2f88f3u,0x2f88f5u,0x2f88f9u,0x2f88fdu,0x2f8903u,0x41cd44u};
inline constexpr ProfileSites end_profile{
    0x2f8b3cu,0x2f8b42u,0x2f8b44u,0x2f8b48u,0x2f8b4cu,0x2f8b52u,0x41cabcu};

template<class State, class Dispatch, class Direct>
struct Pair : Port<State> {
    Dispatch &dispatch;
    Direct &direct;
    ClockSamples &clocks;
    Pair(State &s, std::uint32_t *cursor, Trace<State> *trace, Dispatch &route, Direct &compound, ClockSamples &samples)
        : Port<State>(s,cursor,trace,{}), dispatch(route), direct(compound), clocks(samples) {}
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
    auto &fp() { return this->s.fp; }
    std::uint32_t local(std::uint32_t pc, std::uint32_t offset) {
        return this->read(pc,this->gp[esp] + offset,4);
    }
    void save_local(std::uint32_t pc, std::uint32_t offset, std::uint32_t value, unsigned width = 4) {
        this->store(pc,this->gp[esp] + offset,width,value);
    }
    __m128 vector(std::uint32_t pc, std::uint32_t offset) {
        return this->read_vector(pc,this->gp[esp] + offset);
    }
    void save_vector(std::uint32_t pc, std::uint32_t offset, __m128 value) {
        this->store_vector(pc,this->gp[esp] + offset,value);
    }
    long double extended(std::uint32_t pc, std::uint32_t address) {
        using namespace physics_step_detail;
        return ext(f(this->read(pc,address,4)));
    }
    void spill_top(std::uint32_t pc, std::uint32_t offset, bool pop) {
        using namespace physics_step_detail;
        this->at(pc);
        save_local(pc,offset,u(spill(fp().st(0))));
        if (pop)
            fp().pop();
    }
    __m128 low_scalar(std::uint32_t pc, std::uint32_t offset) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(local(pc,offset))));
    }
    void route(std::uint32_t pc, std::uint32_t resume, std::uint32_t target) {
        guarded([&] { this->push(pc,resume); });
        this->enter(target,[&](State &v) { dispatch(v,target,pc,false); });
    }
    void read_clock(std::uint32_t pc) {
        this->publish(pc);
        if (!this->s.services.timestamp_counter)
            throw std::runtime_error("Required original contact RDTSC owner unavailable");
        const auto value = this->s.services.timestamp_counter();
        if (this->trace)
            clocks.record(pc,value);
        this->load();
        this->gp[eax] = std::uint32_t(value);
        this->gp[edx] = std::uint32_t(value >> 32u);
    }
    void profile(const ProfileSites &site) {
        auto &gp = this->gp;
        guarded([&] { this->store(site.mark,gp[ecx],4,site.marker); });
        read_clock(site.timestamp);
        guarded([&] {
            save_local(site.scratch_write,0x10u,gp[eax]);
            gp[eax] = local(site.scratch_read,0x10u);
            this->store(site.value,gp[ecx] + 4u,4,gp[eax]);
            gp[ecx] = this->flags.plus(gp[ecx],12u,4);
            this->store(site.advance,0x48c974u,4,gp[ecx]);
        });
    }
    void open_frame() {
        auto &gp = this->gp;
        guarded([&] {
            this->push(first,gp[ebp]);
            gp[ebp] = gp[esp];
            gp[esp] = this->flags.logic(gp[esp] & 0xfffffff0u,4);
            gp[esp] = this->flags.minus(gp[esp],0x254u,4);
            gp[ecx] = this->read(0x2f88dcu,0x48c974u,4);
            this->compare32(gp[ecx],this->read(0x2f88e2u,0x48c978u,4));
            this->push(0x2f88e8u,gp[ebx]);
            this->push(0x2f88e9u,gp[esi]);
            this->push(0x2f88eau,gp[edi]);
        });
        if (this->flags.cf) {
            this->reached(0);
            profile(begin_profile);
        }
    }
    void pair_transforms() {
        auto &gp = this->gp;
        guarded([&] {
            gp[edi] = this->read(0x2f8909u,gp[ebp] + 12u,4);
            gp[esi] = this->read(0x2f890cu,gp[ebp] + 8u,4);
            gp[edx] = this->read(0x2f890fu,gp[edi] + 8u,4);
            gp[ecx] = this->read(0x2f8912u,gp[esi],4);
            gp[eax] = this->read(0x2f8914u,gp[esi] + 8u,4);
            gp[ebx] = this->read(0x2f8917u,gp[edi],4);
            gp[edx] = this->flags.plus(gp[edx],0x20u,4);
            this->push(0x2f891cu,gp[edx]);
            gp[eax] = this->flags.plus(gp[eax],0x20u,4);
            save_local(0x2f8920u,0x14u,gp[ecx]);
            this->push(0x2f8924u,gp[eax]);
            gp[ecx] = gp[esp] + 0x28u;
        });
        route(0x2f8929u,0x2f892eu,0x2e4550u);
        std::uint32_t type{};
        guarded([&] {
            gp[edx] = this->read(0x2f892eu,gp[ebx],4);
            gp[ecx] = gp[ebx];
            type = this->read(0x2f8932u,gp[edx] + 0x14u,4);
        });
        route(0x2f8932u,0x2f8935u,type);
        this->compare32(gp[eax],5u);
        if (this->flags.zf) {
            this->reached(1);
            guarded([&] {
                gp[ecx] = local(0x2f893au,0x10u);
                gp[eax] = gp[esp] + 0x20u;
                this->push(0x2f8942u,gp[eax]);
                this->push(0x2f8943u,gp[ebx]);
                this->push(0x2f8944u,gp[ecx]);
                gp[ecx] = gp[esp] + 0x20u;
                this->push(0x2f8949u,0x2f894eu);
            });
            this->enter(0x309150u,direct);
        } else {
            this->reached(2);
            guarded([&] {
                gp[eax] = local(0x2f8950u,0x10u);
                gp[edx] = gp[esp] + 0x20u;
                this->push(0x2f8958u,gp[edx]);
                this->push(0x2f8959u,gp[ebx]);
                this->push(0x2f895au,gp[eax]);
                gp[ecx] = gp[esp] + 0x20u;
            });
            route(0x2f895fu,0x2f8964u,0x309060u);
        }
    }
    bool separation_query() {
        auto &gp = this->gp;
        guarded([&] {
            gp[eax] = this->read(0x2f8964u,gp[edi],4);
            gp[ebx] = this->read(0x2f8966u,gp[esi],4);
            gp[ecx] = gp[esp] + 0xe0u;
            this->push(0x2f896fu,gp[ecx]);
            gp[edx] = gp[esp] + 0x18u;
            this->push(0x2f8974u,gp[edx]);
            gp[ecx] = gp[esp] + 0x28u;
            this->push(0x2f8979u,gp[ecx]);
            this->push(0x2f897au,gp[eax]);
            this->push(0x2f897bu,gp[ebx]);
            gp[ecx] = gp[esp] + 0x104u;
            save_local(0x2f8983u,0x24u,gp[eax]);
            save_local(0x2f8987u,0x110u,0u,1);
        });
        route(0x2f898fu,0x2f8994u,0x30e930u);
        this->test32(gp[eax]);
        this->reached(this->flags.zf ? 4 : 3);
        return this->flags.zf;
    }
    void contact_vectors() {
        auto &gp = this->gp;
        guarded([&] {
            gp[edx] = gp[esp] + 0x60u;
            this->push(0x2f89a0u,gp[edx]);
            gp[eax] = gp[esp] + 0xe4u;
            this->push(0x2f89a8u,gp[eax]);
            gp[ecx] = gp[esp] + 0x1cu;
            this->push(0x2f89adu,gp[ecx]);
            gp[ecx] = gp[esp] + 0xfcu;
        });
        route(0x2f89b5u,0x2f89bau,0x30d350u);
        guarded([&] {
            gp[eax] = this->read(0x2f89bau,gp[esi] + 8u,4);
            gp[edx] = gp[esp] + 0x60u;
            this->push(0x2f89c1u,gp[edx]);
            gp[eax] = this->flags.plus(gp[eax],0x20u,4);
            this->push(0x2f89c5u,gp[eax]);
            gp[ecx] = gp[esp] + 0xa8u;
        });
        route(0x2f89cdu,0x2f89d2u,0x2e4760u);
    }
    bool contact_distance() {
        using namespace physics_step_detail;
        auto &gp = this->gp;
        fp().push(extended(0x2f89d2u,gp[esp] + 0x80u));
        fp().st(0) = sub(fp().st(0),extended(0x2f89d9u,gp[ebx] + 12u));
        gp[ecx] = local(0x2f89dcu,0x10u);
        gp[edx] = this->read(0x2f89e0u,gp[ebp] + 16u,4);
        fp().st(0) = sub(fp().st(0),extended(0x2f89e3u,gp[ecx] + 12u));
        spill_top(0x2f89e6u,0x80u,false);
        this->at(0x2f89edu);
        fp().push(fp().st(0));
        fp().comparison(extended(0x2f89efu,gp[edx] + 8u),1);
        gp[eax] = (gp[eax] & 0xffff0000u) | fp().word();
        this->flags.logic((gp[eax] >> 8u) & 5u,1);
        if (this->flags.pf) {
            this->reached(5);
            this->at(0x2f8b2cu);
            (void)fp().st(0);
            fp().pop();
            return false;
        }
        this->reached(6);
        return true;
    }
    void rotate_normal() {
        auto &gp = this->gp;
        auto &lanes = x();
        fp().push(extended(0x2f89fdu,gp[ebx] + 12u));
        lanes[2] = vector(0x2f8a00u,0x60u);
        lanes[3] = vector(0x2f8a05u,0x50u);
        this->at(0x2f8a0au);
        fp().st(0) = -fp().st(0);
        lanes[5] = vector(0x2f8a0cu,0x30u);
        spill_top(0x2f8a11u,0x10u,true);
        lanes[0] = low_scalar(0x2f8a15u,0x10u);
        gp[eax] = this->read(0x2f8a1bu,gp[edi] + 8u,4);
        this->at(0x2f8a1eu);
        fp().push(fp().st(0));
        lanes[1] = lanes[0];
        this->at(0x2f8a23u);
        fp().st(0) = -fp().st(0);
        lanes[1] = _mm_shuffle_ps(lanes[1],lanes[0],0);
        spill_top(0x2f8a29u,0x10u,true);
        lanes[0] = vector(0x2f8a2du,0x70u);
        lanes[1] = ordered_mul_ps(lanes[1],lanes[2]);
        lanes[0] = ordered_add_ps(lanes[0],lanes[1]);
        lanes[1] = low_scalar(0x2f8a38u,0x10u);
        save_vector(0x2f8a3eu,0x70u,lanes[0]);
        lanes[4] = _mm_shuffle_ps(lanes[1],lanes[1],0);
        lanes[4] = ordered_mul_ps(lanes[4],lanes[2]);
        lanes[0] = ordered_add_ps(lanes[0],lanes[4]);
        lanes[4] = vector(0x2f8a50u,0x40u);
        lanes[0] = _mm_sub_ps(lanes[0],lanes[3]);
        lanes[3] = vector(0x2f8a58u,0x20u);
        gp[ecx] = this->read(0x2f8a5du,gp[ebp] + 20u,4);
        lanes[1] = _mm_shuffle_ps(lanes[3],lanes[5],68);
        lanes[3] = _mm_shuffle_ps(lanes[3],lanes[5],238);
        lanes[5] = lanes[1];
        lanes[2] = _mm_shuffle_ps(lanes[4],lanes[4],68);
        lanes[7] = _mm_shuffle_ps(lanes[4],lanes[4],238);
        lanes[4] = _mm_shuffle_ps(lanes[0],lanes[0],85);
        lanes[1] = _mm_shuffle_ps(lanes[1],lanes[2],136);
        lanes[6] = _mm_shuffle_ps(lanes[0],lanes[0],170);
        lanes[5] = _mm_shuffle_ps(lanes[5],lanes[2],221);
        lanes[5] = ordered_mul_ps(lanes[5],lanes[4]);
        lanes[4] = _mm_shuffle_ps(lanes[0],lanes[0],0);
        lanes[1] = ordered_mul_ps(lanes[1],lanes[4]);
        lanes[0] = ordered_add_ps(lanes[1],lanes[5]);
        lanes[3] = _mm_shuffle_ps(lanes[3],lanes[7],136);
        lanes[3] = ordered_mul_ps(lanes[3],lanes[6]);
        lanes[0] = ordered_add_ps(lanes[0],lanes[3]);
        save_vector(0x2f8aafu,0x90u,lanes[0]);
    }
    std::uint32_t contact_packet() {
        auto &gp = this->gp;
        auto &lanes = x();
        gp[eax] = this->flags.plus(gp[eax],0x20u,4);
        lanes[2] = _mm_shuffle_ps(lanes[0],lanes[0],170);
        save_local(0x2f8ac1u,0xd0u,gp[esi]);
        save_local(0x2f8ac8u,0xd4u,gp[edi]);
        lanes[1] = this->read_vector(0x2f8acfu,gp[eax] + 0x30u);
        lanes[3] = this->read_vector(0x2f8ad3u,gp[eax] + 0x20u);
        lanes[3] = ordered_mul_ps(lanes[3],lanes[2]);
        lanes[2] = this->read_vector(0x2f8adau,gp[eax] + 0x10u);
        lanes[3] = ordered_add_ps(lanes[3],lanes[1]);
        lanes[1] = _mm_shuffle_ps(lanes[0],lanes[0],85);
        lanes[2] = ordered_mul_ps(lanes[2],lanes[1]);
        lanes[1] = _mm_shuffle_ps(lanes[0],lanes[0],0);
        lanes[0] = this->read_vector(0x2f8af2u,gp[eax]);
        gp[eax] = this->read(0x2f8af5u,gp[ecx],4);
        lanes[0] = ordered_mul_ps(lanes[0],lanes[1]);
        lanes[0] = ordered_add_ps(lanes[0],lanes[2]);
        lanes[0] = ordered_add_ps(lanes[0],lanes[3]);
        save_vector(0x2f8b00u,0xb0u,lanes[0]);
        spill_top(0x2f8b08u,0xbcu,true);
        lanes[0] = vector(0x2f8b0fu,0xa0u);
        gp[edx] = gp[esp] + 0xb0u;
        this->push(0x2f8b1eu,gp[edx]);
        save_vector(0x2f8b1fu,0xc4u,lanes[0]);
        return this->read(0x2f8b27u,gp[eax] + 4u,4);
    }
    void close_frame() {
        auto &gp = this->gp;
        guarded([&] {
            gp[ecx] = this->read(0x2f8b2eu,0x48c974u,4);
            this->compare32(gp[ecx],this->read(0x2f8b34u,0x48c978u,4));
        });
        if (this->flags.cf) {
            this->reached(7);
            profile(end_profile);
        }
        guarded([&] {
            gp[edi] = this->pop(0x2f8b58u);
            gp[esi] = this->pop(0x2f8b59u);
            gp[ebx] = this->pop(0x2f8b5au);
            gp[esp] = gp[ebp];
            gp[ebp] = this->pop(0x2f8b5du);
            this->ret(last,0u);
        });
        this->publish(last);
    }
    void run() {
        open_frame();
        pair_transforms();
        if (separation_query()) {
            contact_vectors();
            bool output{};
            std::uint32_t target{};
            guarded([&] {
                output = contact_distance();
                if (output) {
                    rotate_normal();
                    target = contact_packet();
                }
            });
            if (output)
                route(0x2f8b27u,0x2f8b2au,target);
        }
        close_frame();
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Collision pair 2F88D0"};
    return value;
}
template<class State, class Reference, class Dispatch, class Direct>
bool bridge(State &s, Reference reference, Dispatch dispatch, Direct direct) {
    using native_small_queries_detail::flag;
    if ((_mm_getcsr() & 0x1f80u) != 0x1f80u)
        return false;
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return false;
    }
    static const bool force_reference = flag("HALO2_REFERENCE_COLLISION_PAIR_2F88D0");
    static const bool verify = flag("HALO2_VERIFY_COLLISION_PAIR_2F88D0");
    ClockSamples clocks;
    return native_small_queries_detail::run_owner(s,first,last,force_reference,verify,coverage(),summary(),
        [&](State &v) {
            reference(v,clocks);
            if (clocks.replayed != clocks.recorded)
                throw std::runtime_error("Collision pair did not replay both timestamp branches");
        }, [&](State &v,std::uint32_t *cursor,Trace<State> *trace) {
            return Pair<State,Dispatch,Direct>{v,cursor,trace,dispatch,direct,clocks};
        });
}

} // namespace halo2::native_collision_pair_2f88d0_detail

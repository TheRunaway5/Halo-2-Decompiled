#pragma once

#include "core/native_tu15_authored_powered_parameter_systems.hpp"
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_live_support_systems.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include <cmath>
#include <optional>

namespace halo2::native_authored_powered_parameter {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace fp = physics_step_detail;
using systems::Outcome;
using systems::Pair;

enum class Layer { base, blend };
enum class Spill { world, lens };

struct CallSite { std::uint32_t pc, return_address; };
struct PointCall { std::uint32_t index_pc, layer_pc; CallSite site; };
struct PairReads { std::uint32_t y, next_y, x, next_x; };
struct MagnitudeCheck { std::uint32_t input, difference, absolute, limit, compare, discard; };
struct RootSites {
    std::uint32_t save_start, reload_start, save_approximation, save_end, estimate, spill_estimate;
    std::uint32_t load_estimate, multiply_by_approximation, save_exponent, load_parameter, load_exponent;
};
struct PowerSites {
    CallSite call;
    std::uint32_t store, load_zero, compare_zero, discard, reload, load_unit;
};
struct LayerSites {
    std::uint32_t entry, last;
    std::array<PointCall, 4> points;
    std::uint32_t save_second_point, starting_point_slot, select_starting_pair;
    PairReads ending_pair, starting_pair;
    std::uint32_t end_epsilon, store_ending, store_starting, unit_shortcut;
    MagnitudeCheck zero_start, matching;
    RootSites root;
    PowerSites power;
    std::array<std::uint32_t, 3> shortcut_exits;
};

inline constexpr LayerSites base_sites{
    .entry = 0x13e2b0u, .last = 0x13e3e5u,
    .points = {{{0x13e2b0u, 0x13e2b2u, {0x13e2b6u, 0x13e2bbu}},
                {0x13e2bbu, 0x13e2bfu, {0x13e2c3u, 0x13e2c8u}},
                {0x13e2c8u, 0x13e2ceu, {0x13e2d2u, 0x13e2d7u}},
                {0x13e2d7u, 0x13e2dbu, {0x13e2dfu, 0x13e2e4u}}}},
    .save_second_point = 0x13e2cau, .starting_point_slot = 0x10u, .select_starting_pair = 0x13e318u,
    .ending_pair = {0x13e2e4u, 0x13e2e9u, 0x13e2eeu, 0x13e2f2u},
    .starting_pair = {0x13e31cu, 0x13e321u, 0x13e326u, 0x13e32bu},
    .end_epsilon = 0x13e305u, .store_ending = 0x13e30cu, .store_starting = 0x13e33eu, .unit_shortcut = 0x13e356u,
    .zero_start = {0x13e344u, 0u, 0x13e348u, 0x13e34au, 0x13e350u, 0x13e352u},
    .matching = {0x13e363u, 0x13e367u, 0x13e36bu, 0x13e36du, 0x13e373u, 0x13e375u},
    .root = {0x13e379u, 0x13e37fu, 0x13e38bu, 0x13e38fu, 0x13e395u, 0x13e39bu,
             0x13e3a1u, 0x13e3a5u, 0x13e3a9u, 0x13e3adu, 0x13e3b1u},
    .power = {{0x13e3b5u, 0x13e3bau}, 0x13e3bau, 0x13e3beu, 0x13e3c4u, 0x13e3c6u, 0x13e3cfu, 0x13e3d5u},
    .shortcut_exits = {0x13e312u, 0x13e35eu, 0x13e377u},
};

inline constexpr LayerSites blend_sites{
    .entry = 0x13e43eu, .last = 0x13e571u,
    .points = {{{0x13e43eu, 0x13e440u, {0x13e444u, 0x13e449u}},
                {0x13e449u, 0x13e44du, {0x13e451u, 0x13e456u}},
                {0x13e456u, 0x13e45cu, {0x13e460u, 0x13e465u}},
                {0x13e465u, 0x13e469u, {0x13e46du, 0x13e472u}}}},
    .save_second_point = 0x13e458u, .starting_point_slot = 0x18u, .select_starting_pair = 0x13e4a6u,
    .ending_pair = {0x13e472u, 0x13e477u, 0x13e47cu, 0x13e480u},
    .starting_pair = {0x13e4aau, 0x13e4afu, 0x13e4b4u, 0x13e4b8u},
    .end_epsilon = 0x13e493u, .store_ending = 0x13e49au, .store_starting = 0x13e4cbu, .unit_shortcut = 0x13e4e3u,
    .zero_start = {0x13e4d1u, 0u, 0x13e4d5u, 0x13e4d7u, 0x13e4ddu, 0x13e4dfu},
    .matching = {0x13e4f0u, 0x13e4f4u, 0x13e4f8u, 0x13e4fau, 0x13e500u, 0x13e502u},
    .root = {0x13e506u, 0x13e50cu, 0x13e517u, 0x13e51bu, 0x13e521u, 0x13e527u,
             0x13e52du, 0x13e531u, 0x13e535u, 0x13e539u, 0x13e53du},
    .power = {{0x13e541u, 0x13e546u}, 0x13e546u, 0x13e54au, 0x13e550u, 0x13e552u, 0x13e55bu, 0x13e561u},
    .shortcut_exits = {0x13e4a0u, 0x13e4ebu, 0x13e504u},
};

inline constexpr auto control_point_owner = 0x13e9a0u;
inline constexpr auto power_owner = 0x37cde8u;
inline constexpr auto one_address = 0x46a0d0u;
inline constexpr auto zero_address = 0x46a0e8u;
inline constexpr auto epsilon_address = 0x46a0ecu;
inline constexpr unsigned outcomes = 6u;

struct X87Environment {
    std::uint16_t control_word{}, exceptions{};

    static X87Environment capture() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        X87Environment value;
        asm volatile("fnstcw %0" : "=m"(value.control_word));
        value.exceptions = physics_contact_runtime_detail::source_x87_exception_flags();
        return value;
#else
        throw std::runtime_error("Required TU1.5 authored x87 backend unavailable");
#endif
    }
    void apply() const {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("fnclex\n\tfldcw %0" :: "m"(control_word));
        physics_contact_runtime_detail::source_load_x87_exception_flags(exceptions);
#else
        throw std::runtime_error("Required TU1.5 authored x87 backend unavailable");
#endif
    }
    bool operator==(const X87Environment &) const = default;
};

struct ExternalTrace {
    struct Call {
        std::uint32_t site;
        X87Environment before, after;
    };
    struct Estimate {
        std::uint32_t input{}, result{}, mxcsr_before{}, mxcsr_after{};
        X87Environment before, after;
    };
    std::array<Call, 5> calls{};
    unsigned count{};
    std::optional<Estimate> estimate;
};

inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State, class Dispatch>
struct Adapter : Port<State> {
    Layer layer;
    Spill spill_mode;
    Dispatch &dispatch;
    const LayerSites &sites;
    ExternalTrace &external;

    Adapter(State &state, std::uint32_t *cursor, Trace<State> *trace, Layer selected, Spill spill,
        Dispatch &route, ExternalTrace &calls) : Port<State>{state, cursor, trace, {}}, layer(selected),
        spill_mode(spill), dispatch(route), sites(selected == Layer::base ? base_sites : blend_sites), external(calls) {}

    std::uint32_t local(std::uint32_t offset) const { return this->gp[esp] + offset; }
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(fp::f(this->read(pc, address, 4)));
    }
    void write_scalar(std::uint32_t pc, std::uint32_t stack_offset, __m128 value) {
        this->store(pc, local(stack_offset), 4, fp::u(_mm_cvtss_f32(value)));
    }
    unsigned first_point_register() const { return layer == Layer::base ? ebp : ebx; }
    void call_owner(CallSite site, std::uint32_t target) {
        this->call(site.pc, site.return_address, target, [&](State &state) {
            try {
                if (this->trace) {
                    auto &record = external.calls[external.count++];
                    record.site = site.pc;
                    record.before = X87Environment::capture();
                    dispatch(state, target, site.pc);
                    record.after = X87Environment::capture();
                } else dispatch(state, target, site.pc);
            } catch (...) {
                this->gp = state.gp;
                this->flags = state.flags;
                throw;
            }
        });
    }
    void load_control_points() {
        auto &gp = this->gp;
        for (unsigned index = 0; index < sites.points.size(); ++index) {
            const auto &point = sites.points[index];
            this->push(point.index_pc, index);
            if (index == 1) gp[first_point_register()] = gp[eax];
            if (index == 2) this->store(sites.save_second_point, local(sites.starting_point_slot + 4u), 4, gp[eax]);
            if (index == 3) gp[esi] = gp[eax];
            this->push(point.layer_pc, unsigned(layer));
            gp[eax] = gp[edi];
            call_owner(point.site, control_point_owner);
        }
    }
    const PairReads &reads(Pair pair) const { return pair == Pair::starting ? sites.starting_pair : sites.ending_pair; }
    std::uint32_t point(Pair pair) const { return this->gp[pair == Pair::starting ? first_point_register() : esi]; }
    __m128 point_y(Pair pair) { return scalar(reads(pair).y, point(pair) + 4u); }
    __m128 next_point_y(Pair pair) { return scalar(reads(pair).next_y, this->gp[eax] + 4u); }
    __m128 point_x(Pair pair) { return scalar(reads(pair).x, point(pair)); }
    __m128 next_point_x(Pair pair) { return scalar(reads(pair).next_x, this->gp[eax]); }

    bool above() const { return !this->flags.cf && !this->flags.zf; }
    bool store_ending_distance(__m128 distance) {
        this->flags.scalar_compare(distance, scalar(sites.end_epsilon, epsilon_address), false);
        write_scalar(sites.store_ending, 0x14u, distance);
        return above();
    }
    void select_starting_pair() {
        this->gp[eax] = this->read(sites.select_starting_pair, local(sites.starting_point_slot), 4);
    }
    void store_starting_distance(__m128 distance) { write_scalar(sites.store_starting, 0x10u, distance); }

    long double extended(std::uint32_t pc, std::uint32_t address) {
        return fp::ext(fp::f(this->read(pc, address, 4)));
    }
    void push_extended(std::uint32_t pc, std::uint32_t address) { this->s.fp.push(extended(pc, address)); }
    void clear_lens_c1() {
        if (spill_mode == Spill::lens) this->s.fp.status &= ~0x200u;
    }
    void compare_and_discard(std::uint32_t compare, std::uint32_t discard) {
        auto &state = this->s;
        this->at(compare);
        state.compare_and_pop(state.fp.st(1));
        this->flags = state.flags;
        this->at(discard);
        (void)state.fp.st(0);
        clear_lens_c1();
        state.fp.pop();
    }
    bool compare_magnitude(const MagnitudeCheck &test) {
        auto &stack = this->s.fp;
        push_extended(test.input, local(0x10u));
        if (test.difference) stack.st(0) = fp::sub(stack.st(0), extended(test.difference, local(0x14u)));
        this->at(test.absolute);
        stack.st(0) = std::fabs(stack.st(0));
        clear_lens_c1();
        push_extended(test.limit, epsilon_address);
        compare_and_discard(test.compare, test.discard);
        return above();
    }
    bool start_is_near_zero() { return compare_magnitude(sites.zero_start); }
    bool distances_are_near_equal() { return compare_magnitude(sites.matching); }
    __m128 unit_for_shortcut() { return scalar(sites.unit_shortcut, one_address); }

    std::uint32_t reciprocal_estimate(std::uint32_t input) {
        const auto &estimate = this->s.services.reciprocal_sqrt;
        if (!estimate) throw std::runtime_error("Required original contact RSQRTSS owner unavailable");
        if (!this->trace) return estimate(input);
        auto &record = external.estimate.emplace();
        record.input = input;
        record.mxcsr_before = _mm_getcsr();
        record.before = X87Environment::capture();
        record.result = estimate(input);
        record.after = X87Environment::capture();
        record.mxcsr_after = _mm_getcsr();
        return record.result;
    }
    void estimate_exponent(__m128 &parameter, __m128 start, __m128 end) {
        const auto &root = sites.root;
        const auto approximate_slot = layer == Layer::base ? 0x18u : 0x10u;
        const auto end_slot = layer == Layer::base ? 0x14u : 0x18u;
        const auto inverse_slot = layer == Layer::base ? 0x10u : 0x14u;
        auto &bits = this->gp[layer == Layer::base ? edx : eax];
        write_scalar(root.save_start, end_slot, start);
        bits = this->read(root.reload_start, local(end_slot), 4);
        bits = this->flags.right(bits, 1u, 4, true);
        bits = this->flags.plus(bits, 0x1fc00000u, 4);
        this->store(root.save_approximation, local(approximate_slot), 4, bits);
        write_scalar(root.save_end, end_slot, end);
        const auto inverse = reciprocal_estimate(this->read(root.estimate, local(end_slot), 4));
        parameter = _mm_move_ss(parameter, _mm_set_ss(fp::f(inverse)));
        write_scalar(root.spill_estimate, inverse_slot, parameter);

        push_extended(root.load_estimate, local(inverse_slot));
        auto &stack = this->s.fp;
        stack.st(0) = fp::mul(stack.st(0), extended(root.multiply_by_approximation, local(approximate_slot)));
        spill_extended(root.save_exponent, 0x18u);
        stack.pop();
    }
    void spill_extended(std::uint32_t pc, std::uint32_t offset) {
        this->at(pc);
        auto &stack = this->s.fp;
        std::uint32_t value;
        if (spill_mode == Spill::lens) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
            const auto stored = native_live_support::x87::store_single(stack.st(0));
            value = std::uint32_t(stored.bits);
            stack.status = std::uint16_t((stack.status & ~0x200u) | (stored.rounded_up ? 0x200u : 0u));
#else
            throw std::runtime_error("Required TU1.5 FSTP32 backend unavailable");
#endif
        } else value = fp::u(fp::spill(stack.st(0)));
        this->store(pc, local(offset), 4, value);
    }
    void raise_parameter() {
        push_extended(sites.root.load_parameter, local(0x20u));
        push_extended(sites.root.load_exponent, local(0x18u));
        call_owner(sites.power.call, power_owner);
        spill_extended(sites.power.store, layer == Layer::base ? 0x10u : 0x20u);
    }
    bool power_is_negative() {
        push_extended(sites.power.load_zero, zero_address);
        compare_and_discard(sites.power.compare_zero, sites.power.discard);
        return above();
    }
    __m128 stored_power() { return scalar(sites.power.reload, local(layer == Layer::base ? 0x10u : 0x20u)); }
    __m128 unit_for_power() { return scalar(sites.power.load_unit, one_address); }
    bool power_exceeds_unit(__m128 power, __m128 unit) {
        this->flags.scalar_compare(power, unit, false);
        return above();
    }

    void run() {
        auto &x = this->s.xmm;
        try {
            const auto branch = unsigned(systems::evaluate(*this, {x[0], x[1], x[2], x[3], x[4], x[5]}));
            this->reached(branch + unsigned(layer) * outcomes);
            this->publish(branch < sites.shortcut_exits.size() ? sites.shortcut_exits[branch] : sites.last);
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

// Branch-specific reference adapters use these functions to replay the exact
// real services without calling an owner again or altering the live x87 state.
template<class State, class Reference, class Dispatch>
bool bridge(State &state, Layer layer, Spill spill, Reference reference, Dispatch dispatch) {
    using namespace native_small_queries_detail;
    static const bool force_reference = flag("HALO2_REFERENCE_AUTHORED_FUNCTION_BOUNDED") ||
        flag("HALO2_REFERENCE_AUTHORED_POWERED_PARAMETER");
    static const bool verify = flag("HALO2_VERIFY_AUTHORED_POWERED_PARAMETER");
    static VerifySummary summary{"Authored function powered parameter"};
    const auto initial_x87 = verify && !force_reference ? X87Environment::capture() : X87Environment{};
    ExternalTrace recorded;
    auto replay = [&](State &original) {
        struct RestoreX87 {
            X87Environment live = X87Environment::capture();
            ~RestoreX87() { live.apply(); }
        } restore;
        initial_x87.apply();
        unsigned calls{};
        bool estimated{};
        const auto replay_child = [&](State &child, std::uint32_t target, std::uint32_t site) {
            if (calls >= recorded.count || site != recorded.calls[calls].site ||
                X87Environment::capture() != recorded.calls[calls].before)
                throw std::runtime_error("Authored powered parameter child entry differs from original");
            native_object_tick_detail::replay(child, target);
            recorded.calls[calls++].after.apply();
        };
        const auto replay_estimate = [&](State &, std::uint32_t input) {
            if (estimated || !recorded.estimate || input != recorded.estimate->input ||
                _mm_getcsr() != recorded.estimate->mxcsr_before || X87Environment::capture() != recorded.estimate->before)
                throw std::runtime_error("Authored powered parameter reciprocal estimate differs from original");
            estimated = true;
            recorded.estimate->after.apply();
            _mm_setcsr(recorded.estimate->mxcsr_after);
            return recorded.estimate->result;
        };
        reference(original, replay_child, replay_estimate);
        if (calls != recorded.count || estimated != recorded.estimate.has_value() || X87Environment::capture() != restore.live)
            throw std::runtime_error("Authored powered parameter completed with a different service or x87 state");
    };
    const auto &sites = layer == Layer::base ? base_sites : blend_sites;
    return run_owner(state, sites.entry, sites.last, force_reference, verify, coverage(), summary, replay,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Adapter<State, Dispatch>{value, cursor, trace, layer, spill, dispatch, recorded};
        });
}
} // namespace halo2::native_authored_powered_parameter

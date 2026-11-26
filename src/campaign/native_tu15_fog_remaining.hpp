#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_fog_remaining_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

enum class Entry : std::uint32_t {
    setup = 0x131ac0u, environment = 0x131c89u, environment_patch = 0x131f80u,
    projection_call = 0x132183u, sky_patch = 0x132492u, sky_bloom = 0x1324d9u,
    missing_sky = 0x132597u, empty = 0x132628u,
};
enum class Next : std::uint32_t {
    fallback, cluster = 0x131c31u, empty = 0x132628u, weight = 0x131d38u, missing_sky = 0x132597u,
    environment_bloom = 0x131fc7u, projected_merge = 0x132188u, sky_bloom = 0x1324d9u,
    recorded = 0x132084u, uncovered = 0x13208cu, resolve = 0x13262bu,
};

template<Entry entry> constexpr std::uint32_t last_pc() {
    switch (entry) {
    case Entry::setup: return 0x131c2au;
    case Entry::environment: return 0x131d32u;
    case Entry::environment_patch: return 0x131fc2u;
    case Entry::projection_call: return 0x132183u;
    case Entry::sky_patch: return 0x1324d4u;
    case Entry::sky_bloom: return 0x132592u;
    case Entry::missing_sky: return 0x132623u;
    case Entry::empty: return 0x132628u;
    }
}

struct NoChild {};
struct Slot { std::uint32_t pc, offset; };
inline constexpr std::array initial_zeros{
    Slot{0x131b32u,0x14u}, Slot{0x131b36u,0x44u}, Slot{0x131b3au,0x18u}, Slot{0x131b3eu,0x48u},
    Slot{0x131b42u,0x4cu}, Slot{0x131b46u,0x1cu}, Slot{0x131b4au,0x50u}, Slot{0x131b4eu,0x20u},
    Slot{0x131b52u,0x90u}, Slot{0x131b59u,0x24u},
};
inline constexpr std::array accumulator_zeros{
    Slot{0x131b61u,0x94u}, Slot{0x131b68u,0x28u}, Slot{0x131b6cu,0x70u}, Slot{0x131b70u,0x2cu},
    Slot{0x131b74u,0x98u}, Slot{0x131b7bu,0x30u}, Slot{0x131b7fu,0x74u}, Slot{0x131b83u,0x9cu},
    Slot{0x131b8au,0x78u}, Slot{0x131b8eu,0x54u}, Slot{0x131b92u,0xa0u}, Slot{0x131b99u,0x7cu},
    Slot{0x131b9du,0x58u}, Slot{0x131ba1u,0xa4u}, Slot{0x131ba8u,0x80u}, Slot{0x131bafu,0xa8u},
    Slot{0x131bb6u,0x5cu}, Slot{0x131bbau,0xacu}, Slot{0x131bc1u,0x84u}, Slot{0x131bc8u,0x60u},
    Slot{0x131bccu,0x88u},
};
inline constexpr std::array attenuation{
    Slot{0x131bf0u,0x50u}, Slot{0x131bf6u,0x30u}, Slot{0x131bfcu,0x8cu},
    Slot{0x131c05u,0xacu}, Slot{0x131c0eu,0xecu}, Slot{0x131c17u,0x68u},
};
struct PatchSites {
    std::array<std::uint32_t,4> read, push;
    std::uint32_t call, return_to, cluster_read, cluster_compare, weight_read, datum_store, weight_store, present_store;
    unsigned lane;
    Next next;
};
inline constexpr PatchSites environment_patch{
    {0x131f80u,0x131f83u,0x131f87u,0x131f8bu}, {0x131f86u,0x131f8au,0x131f8eu,0x131f8fu},
    0x131f9du,0x131fa2u,0x131fa2u,0x131fa5u,0x131faeu,0x131fb5u,0x131fbcu,0x131fc2u,4,
    Next::environment_bloom,
};
inline constexpr PatchSites sky_patch{
    {0x132492u,0x132495u,0x132499u,0x13249du}, {0x132498u,0x13249cu,0x1324a0u,0x1324a1u},
    0x1324afu,0x1324b4u,0x1324b4u,0x1324b7u,0x1324c0u,0x1324c7u,0x1324ceu,0x1324d4u,6,
    Next::sky_bloom,
};

template<Entry entry, class State, class Child> struct Adapter : Port<State> {
    using Base = Port<State>;
    Next &next;
    Child &child;
    Adapter(State &s, std::uint32_t *cursor, Trace<State> *trace, Next &result, Child &route)
        : Base(s, cursor, trace, {}), next(result), child(route) {}
    auto &x() { return this->s.xmm; }
    template<class Action> void guarded(Action &&action) {
        try {
            action();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
    std::uint32_t local(std::uint32_t pc, std::uint32_t offset) {
        return this->read(pc, this->gp[esp] + offset, 4);
    }
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(this->read(pc, address, 4))));
    }
    __m128 local_float(std::uint32_t pc, std::uint32_t offset) {
        return scalar(pc, this->gp[esp] + offset);
    }
    static std::uint32_t bits(__m128 value) {
        return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));
    }
    void assign(Slot slot, std::uint32_t value) {
        this->store(slot.pc, this->gp[esp] + slot.offset, 4, value);
    }
    bool above(__m128 left, __m128 right) {
        this->flags.scalar_compare(left, right, false);
        return !this->flags.cf && !this->flags.zf;
    }
    void finish(Next destination, std::uint32_t pc, unsigned branch) {
        next = destination;
        this->reached(branch);
        this->publish(pc);
    }
    void invoke(std::uint32_t pc, std::uint32_t continuation, std::uint32_t target) {
        guarded([&] { this->push(pc, continuation); });
        // The callee's faults retain its live register state and cursor.
        this->enter(target, [&](State &v) { child(v, target, pc); });
    }
    void prepare_buffers() {
        auto &gp = this->gp;
        gp[ecx] = this->flags.logic(0u, 4);
        assign({0x131b18u,0x34u}, 0u);
        assign({0x131b1cu,0x38u}, 0u);
        assign({0x131b20u,0x3cu}, 0u);
        gp[edx] = this->flags.logic(0u, 4);
        assign({0x131b26u,0x40u}, 0u);
        x()[3] = scalar(0x131b2au, 0x46a0d0u);
        for (const auto slot : initial_zeros)
            assign(slot, 0u);
        gp[eax] = this->flags.logic(0u, 4);
        this->test32(gp[ebx]);
        for (const auto slot : accumulator_zeros)
            assign(slot, 0u);
        gp[ecx] = 12u;
        gp[edi] = gp[esp] + 0xc0u;
        assign({0x131bdfu,0x64u}, 0u);
        assign({0x131be3u,0x8cu}, 0u);
        const auto reverse = (this->s.other_flags & 0x400u) != 0;
        const auto stride = reverse ? 0xfffffffcu : 4u;
        this->reached(reverse ? 0 : 1);
        while (gp[ecx]) {
            this->store(0x131beau, gp[edi], 4, 0u);
            gp[edi] += stride;
            --gp[ecx];
        }
        assign({0x131becu,0x68u}, 0u);
        for (const auto slot : attenuation)
            assign(slot, bits(x()[3]));
        if (this->flags.zf || this->flags.sf != this->flags.of) {
            finish(Next::empty, 0x131c1du, 2);
            return;
        }
        gp[ebp] = gp[esp] + 0x1838u;
        assign({0x131c2au,0xb4u}, gp[ebx]);
        finish(Next::cluster, 0x131c2au, 3);
    }
    void setup() {
        auto &gp = this->gp;
        gp[eax] = 0x4820u;
        invoke(0x131ac5u, 0x131acau, 0x32a660u);
        guarded([&] {
            x()[0] = _mm_setzero_ps();
            this->push(0x131acdu, gp[ebx]);
            this->push(0x131aceu, gp[ebp]);
            this->push(0x131acfu, gp[esi]);
            gp[esi] = local(0x131ad0u, 0x4838u);
            this->push(0x131ad7u, gp[edi]);
            gp[eax] = gp[esp] + 0x1830u;
            this->push(0x131adfu, gp[eax]);
            gp[eax] = local(0x131ae0u, 0x4838u);
            gp[ebp] = this->flags.logic(0xffffffffu, 4);
            this->push(0x131aeau, gp[ecx]);
            assign({0x131aebu,0xb8u}, gp[ebp]);
            assign({0x131af2u,0x74u}, bits(x()[0]));
        });
        invoke(0x131af8u, 0x131afdu, 0x131470u);
        guarded([&] {
            gp[ebx] = gp[eax];
            gp[edx] = gp[esp] + 0x150u;
            this->push(0x131b06u, gp[edx]);
            gp[eax] = gp[esp] + 0x1834u;
            this->push(0x131b0eu, gp[eax]);
            gp[edx] = gp[ebx];
        });
        invoke(0x131b11u, 0x131b16u, 0x131610u);
        guarded([&] { prepare_buffers(); });
    }
    void find_environment() {
        auto &gp = this->gp;
        gp[edx] = this->read(0x131c89u, gp[ebp] + 12u, 4);
        this->compare32(gp[edx], gp[eax]);
        gp[ecx] = std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(gp[ecx]))));
        if (!this->flags.zf) {
            this->reached(0);
            gp[edx] = this->flags.product(gp[edx], 0xf4u);
            gp[ebx] = gp[esp] + gp[edx] + 0x150u;
            return;
        }
        gp[edx] = this->read(0x131ca2u, 0x4ecc20u, 4);
        gp[ebx] = this->flags.logic(0u, 4);
        this->test32(gp[edx]);
        if (this->flags.zf)
            return;
        this->test32(gp[ecx]);
        if (this->flags.sf != this->flags.of)
            return;
        this->compare32(gp[ecx], this->read(0x131cb2u, gp[edx] + 0x340u, 4));
        if (this->flags.sf == this->flags.of)
            return;
        this->reached(1);
        gp[ebx] = this->read(0x131cbau, gp[edx] + 0x344u, 4);
        gp[ecx] = this->flags.product(gp[ecx], 0xf4u);
        gp[ecx] = this->flags.plus(gp[ecx], gp[ebx], 4);
        gp[ebx] = gp[ecx];
    }
    void environment() {
        auto &gp = this->gp;
        find_environment();
        this->test32(gp[ebx]);
        if (this->flags.zf) {
            finish(Next::missing_sky, 0x131cccu, 2);
            return;
        }
        const auto distance = gp[ebx] + 0x10u;
        x()[0] = scalar(0x131cd2u, distance);
        if (above(x()[0], x()[7])) {
            x()[0] = scalar(0x131cdcu, 0x46a110u);
            if (above(x()[0], scalar(0x131ce4u, distance))) {
                this->reached(3);
                x()[0] = scalar(0x131ceau, distance);
            } else {
                this->reached(4);
            }
        } else {
            this->reached(5);
            x()[0] = scalar(0x131cf1u, 0x556e10u);
        }
        x()[1] = scalar(0x131cf9u, gp[ebp] - 4u);
        x()[1] = _mm_div_ss(x()[1], x()[0]);
        x()[4] = _mm_sub_ss(x()[3], x()[1]);
        if (above(x()[7], x()[4])) {
            this->reached(6);
            x()[4] = x()[7];
        } else if (above(x()[4], x()[3])) {
            this->reached(7);
            x()[4] = x()[3];
        }
        this->compare32(gp[edi], local(0x131d1bu, 0x4834u));
        if (!this->flags.zf) {
            finish(Next::weight, 0x131d22u, 8);
            return;
        }
        gp[ecx] = this->read(0x131d24u, gp[ebx] + 0xf0u, 2);
        gp[edx] = local(0x131d2bu, 0x483cu);
        this->store(0x131d32u, gp[edx] + 0xf0u, 4, gp[ecx]);
        finish(Next::weight, 0x131d32u, 9);
    }
    void patch(const PatchSites &pc) {
        auto &gp = this->gp;
        guarded([&] {
            gp[eax] = this->read(pc.read[0], gp[edx] + 0x24u, 4);
            gp[ecx] = this->read(pc.read[1], gp[edx] + 0x20u, 4);
            this->push(pc.push[0], gp[eax]);
            gp[eax] = this->read(pc.read[2], gp[edx] + 0x1cu, 4);
            this->push(pc.push[1], gp[ecx]);
            gp[ecx] = this->read(pc.read[3], gp[edx] + 0x18u, 4);
            this->push(pc.push[2], gp[eax]);
            this->push(pc.push[3], gp[ecx]);
            gp[ecx] = gp[edx] + 12u;
            gp[eax] = gp[esp] + 0xd0u;
            x()[0] = x()[pc.lane];
        });
        invoke(pc.call, pc.return_to, 0x1342b0u);
        guarded([&] {
            gp[eax] = this->read(pc.cluster_read, gp[ebp] - 8u, 4);
            this->compare32(gp[eax], local(pc.cluster_compare, 0x4834u));
            if (this->flags.zf || above(x()[pc.lane], local_float(pc.weight_read, 0x6cu))) {
                this->reached(1);
                assign({pc.datum_store,0xb0u}, gp[edi]);
                assign({pc.weight_store,0x6cu}, bits(x()[pc.lane]));
            }
            this->store(pc.present_store, gp[esp] + 0x13u, 1, 1u);
            finish(pc.next, pc.present_store, 0);
        });
    }
    void bloom() {
        auto &gp = this->gp;
        x()[0] = scalar(0x1324d9u, gp[esi] + 0x68u);
        if (above(x()[0], x()[7])) {
            this->reached(0);
            const bool cap = above(x()[6], x()[5]);
            x()[1] = scalar(0x1324eau, gp[esi] + 0x74u);
            x()[2] = scalar(0x1324efu, gp[esi] + 0x70u);
            x()[3] = scalar(0x1324f4u, gp[esi] + 0x6cu);
            x()[4] = scalar(0x1324f9u, gp[esi] + 0x68u);
            x()[0] = cap ? x()[5] : x()[6];
            this->reached(cap ? 1 : 2);
            this->flags.scalar_compare(x()[1], x()[7], true);
            const auto ah = (this->flags.bits() | 2u) & 0xffu;
            gp[eax] = (gp[eax] & ~0xff00u) | (ah << 8);
            this->flags.logic(ah & 0x44u, 1);
            if (!this->flags.pf) {
                this->reached(3);
                x()[1] = x()[5];
            }
            x()[1] = ordered_mul_ss(x()[1], x()[0]);
            x()[1] = ordered_add_ss(x()[1], local_float(0x132516u, 0x60u));
            assign({0x13251cu,0x60u}, bits(x()[1]));
            x()[4] = ordered_mul_ss(x()[4], x()[0]);
            x()[4] = ordered_add_ss(x()[4], local_float(0x132526u, 0x54u));
            x()[3] = ordered_mul_ss(x()[3], x()[0]);
            x()[3] = ordered_add_ss(x()[3], local_float(0x132530u, 0x58u));
            x()[2] = ordered_mul_ss(x()[2], x()[0]);
            x()[2] = ordered_add_ss(x()[2], local_float(0x13253au, 0x5cu));
            x()[1] = x()[0];
            x()[1] = ordered_add_ss(x()[1], local_float(0x132543u, 0x64u));
            x()[5] = _mm_sub_ss(x()[5], x()[0]);
            x()[5] = ordered_mul_ss(x()[5], local_float(0x13254du, 0x68u));
            assign({0x132553u,0x54u}, bits(x()[4]));
            assign({0x132559u,0x58u}, bits(x()[3]));
            assign({0x13255fu,0x5cu}, bits(x()[2]));
            assign({0x132565u,0x64u}, bits(x()[1]));
            assign({0x13256bu,0x68u}, bits(x()[5]));
        }
        gp[edx] = local(0x132571u, 0xbcu);
        x()[3] = scalar(0x132578u, 0x46a0d0u);
        x()[7] = _mm_setzero_ps();
        this->store(0x132583u, gp[ebp], 4, 3u);
        this->store(0x13258au, gp[ebp] + 4u, 4, gp[edx]);
        this->store(0x13258du, gp[ebp] + 8u, 4, bits(x()[6]));
        finish(Next::recorded, 0x132592u, 4);
    }
    void missing() {
        auto &gp = this->gp;
        x()[1] = scalar(0x132597u, gp[ebp] - 4u);
        x()[1] = _mm_div_ss(x()[1], scalar(0x13259cu, 0x556e10u));
        x()[0] = _mm_sub_ss(x()[3], x()[1]);
        if (above(x()[7], x()[0])) {
            this->reached(1);
            x()[0] = x()[7];
        }
        constexpr std::array weights{
            Slot{0x1325b3u,0x4cu}, Slot{0x1325c3u,0x2cu}, Slot{0x1325d6u,0x88u},
            Slot{0x1325e8u,0xa8u}, Slot{0x1325feu,0xe8u},
        };
        constexpr std::array stores{0x1325bdu,0x1325cdu,0x1325dfu,0x1325f5u,0x13260bu};
        for (unsigned i = 0; i < weights.size(); ++i) {
            const auto [pc, offset] = weights[i];
            if (i == 2) {
                x()[1] = x()[0];
                x()[1] = ordered_add_ss(x()[1], local_float(pc, offset));
            } else {
                x()[1] = local_float(pc, offset);
                x()[1] = ordered_add_ss(x()[1], x()[0]);
            }
            assign({stores[i],offset}, bits(x()[1]));
        }
        this->store(0x132614u, gp[ebp], 4, 1u);
        this->store(0x13261bu, gp[ebp] + 4u, 4, gp[eax]);
        this->store(0x13261eu, gp[ebp] + 8u, 4, bits(x()[0]));
        finish(Next::uncovered, 0x132623u, 0);
    }
    void run() {
        if constexpr (entry == Entry::setup) {
            setup();
        } else if constexpr (entry == Entry::environment) {
            guarded([&] { environment(); });
        } else if constexpr (entry == Entry::environment_patch || entry == Entry::sky_patch) {
            patch(entry == Entry::environment_patch ? environment_patch : sky_patch);
        } else if constexpr (entry == Entry::projection_call) {
            invoke(0x132183u, 0x132188u, 0x133b40u);
            // The completed child owns the current cursor until the next parent instruction.
            next = Next::projected_merge;
            this->reached(0);
        } else if constexpr (entry == Entry::sky_bloom) {
            guarded([&] { bloom(); });
        } else if constexpr (entry == Entry::missing_sky) {
            guarded([&] { missing(); });
        } else if constexpr (entry == Entry::empty) {
            x()[7] = _mm_setzero_ps();
            finish(Next::resolve, 0x132628u, 0);
        }
    }
};

template<Entry entry> inline auto &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}
template<Entry entry> inline auto &summary() {
    constexpr const char *name = [] {
        switch (entry) {
        case Entry::setup: return "Campaign fog setup";
        case Entry::environment: return "Campaign fog environment selection";
        case Entry::environment_patch: return "Campaign fog environment patch";
        case Entry::projection_call: return "Campaign fog projection call";
        case Entry::sky_patch: return "Campaign fog sky patch";
        case Entry::sky_bloom: return "Campaign fog sky bloom";
        case Entry::missing_sky: return "Campaign fog missing sky";
        case Entry::empty: return "Campaign fog empty clusters";
        }
    }();
    static native_small_queries_detail::VerifySummary value{name};
    return value;
}
template<Entry entry, class State, class Reference, class Child = NoChild>
Next bridge(State &s, Reference reference, Child child = {}) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_FOG_REMAINING");
    static const bool verify = flag("HALO2_VERIFY_FOG_REMAINING");
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return Next::fallback;
    }
    if constexpr (entry == Entry::environment || entry == Entry::environment_patch || entry == Entry::sky_patch ||
                  entry == Entry::sky_bloom || entry == Entry::missing_sky)
        if ((_mm_getcsr() & 0x1f80u) != 0x1f80u)
            return Next::fallback;
    Next next{};
    native_small_queries_detail::run_owner(s, std::uint32_t(entry), last_pc<entry>(), force_reference, verify,
        coverage<entry>(), summary<entry>(),
        [&](State &v) {
            if (reference(v) != next)
                throw std::runtime_error("Campaign fog continuation differs");
        }, [&](State &v, std::uint32_t *cursor, Trace<State> *trace) {
            return Adapter<entry,State,Child>{v, cursor, trace, next, child};
        });
    return next;
}

} // namespace halo2::native_fog_remaining_detail

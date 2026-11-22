// Differential test of 2C2AD0 against its complete retained TU1.5 body.
// The test includes the same prefix and child routes installed by the binder.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "rendering/effects/native_tu15_particle_motion.hpp"
#include <iostream>
#include <random>
#include <set>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
using U = std::uint32_t;
using Registers = native_object_tick_detail::Registers<WorldState>;
struct Event {
    U pc, address, bits, kind, width;
    bool operator==(const Event &) const = default;
};
struct Bank : SourcePhysicsContactMemory {
    // A small circular fixture bank allows stack, global and datum aliases.
    std::array<std::byte, 65536> bytes{};
    mutable std::vector<Event> events;
    U pc{}, stop{};
    void event(U address, U value, U kind, U width) const {
        if (events.size() >= 3000)
            throw std::logic_error("particle traversal exceeded fixture");
        events.push_back({pc, address, value, kind, width});
        if (stop && events.size() == stop)
            throw std::runtime_error("provider stopped");
    }
    void read(U at, std::span<std::byte> out) const override {
        U value = 0;
        for (unsigned k = 0; k < out.size(); ++k) {
            out[k] = bytes[(at + k) & 65535];
            value |= std::to_integer<U>(out[k]) << (8 * k);
        }
        event(at, value, 0, out.size());
    }
    void write(U at, std::span<const std::byte> in) override {
        U value = 0;
        for (unsigned k = 0; k < in.size(); ++k)
            value |= std::to_integer<U>(in[k]) << (8 * k);
        event(at, value, 1, in.size());
        put(at, value, in.size());
    }
    void put(U at, U value, unsigned width = 4) {
        for (unsigned k = 0; k < width; ++k)
            bytes[(at + k) & 65535] = std::byte(value >> (8 * k));
    }
};
struct ChildCall {
    U target, pc;
    Registers abi;
};
struct Result {
    std::vector<ChildCall> calls;
    Registers final;
    std::string error;
};
Result *result;
unsigned trial;
std::set<U> visited;
constexpr U stack = 0x9800, definition = 0x1200, particles = 0x2800, table = 0x1600;
constexpr U routes[]{0x2c2adf, 0x2c2af4, 0x2c2b09, 0x2c2b1f};
void child(WorldState &s, U target, U pop_bytes) {
    auto &bank = static_cast<Bank &>(s.memory);
    result->calls.push_back({target, bank.pc, Registers::capture(s)});
    const U return_pc = physics_step_detail::r(s.memory, s.gp[4]);
    // Expose a child's storage and caller-saved register changes. The
    // channel evaluator supplies random scalar bits, including exceptional values.
    s.gp[4] += 4 + pop_bytes;
    s.gp[0] = return_pc * 17u;
    s.gp[1] = ~s.gp[0];
    s.gp[2] = target;
    s.flags.logic(return_pc, 4);
    const U bits = trial * 2654435761u + return_pc;
    s.xmm[0] = _mm_castsi128_ps(_mm_set_epi32(-1, 0x12345, 17, bits));
    if (target == 0x24ffe0 && (trial & 8))
        bank.put(stack - 0x60, bits);
}
static void run_176a40(WorldState &s) {
    child(s, 0x176a40, 20);
}
static void run_24ffe0(WorldState &s) {
    child(s, 0x24ffe0, 0);
}
static void run_2c2d90(WorldState &s) {
    child(s, 0x2c2d90, 20);
}
static void dispatch(WorldState &s, U target, U, bool) {
    child(s, target, 16);
}
#include "motion.inc"

constexpr U fp_values[] = {0,           0x80000000u, 1,           0x807fffffu, 0x3f000000u, 0x40000000u,
                           0xbf800000u, 0x7fc00001u, 0xffc00002u, 0x7f800003u, 0xff800000u, 0x7f800000u};
void prepare(Bank &m, unsigned n, std::mt19937 &rng) {
    const auto scalar = [&]() -> U { return rng() % 3 ? fp_values[rng() % std::size(fp_values)] : rng(); };
    const U kind = n % 11 < 6 ? 0 : n % 5;
    for (U i = 0; i < 4; ++i)
        m.put(0x2c2b30 + i * 4, routes[i]);
    if (n % 23 == 0)
        m.put(0x2c2b30 + 4 * kind, n % 2 ? 0xdeadbeef : routes[rng() % 4]);
    m.put(definition, kind, 2);
    m.put(definition + 8, 0x4800);
    m.put(definition + 12, rng() % 16);
    m.put(definition + 16, rng());
    m.put(stack, 0xbeef1234);
    m.put(stack + 4, scalar());
    m.put(stack + 8, scalar());
    m.put(stack + 12, scalar());
    m.put(0x52d714, table);
    m.put(table + 0x44, particles);
    m.put(0x52be68, 0x6e00);
    m.put(0x6e00, scalar());
    m.put(0x46a0d0, n % 19 ? 0x3f800000 : scalar());
    for (U i = 0; i < 3; ++i) {
        const U p = particles + 64 * i;
        m.put(p + 2, n % 7 == i ? (n % 3 == 0 ? 8 : n % 3 == 1 ? 1 : 9) : 0, 1);
        m.put(p + 4, i == 2 ? 0xffffffffu : (0xabcd0001u + i));
        m.put(p + 8, (n + i) % 2 ? 0 : scalar());
        for (U j = 0; j < 4; ++j)
            m.put(p + 0x28 + 4 * j, scalar());
    }
}
Result execute(Bank &m, bool bound, bool census, bool forced, const SourcePhysicsWorldRuntimeAbi &abi,
               U mxcsr) {
    Result out;
    result = &out;
    const SourcePhysicsContactRecordObserver observe = [&](const auto &e) {
        if (e.instruction != m.pc && !census)
            throw std::logic_error("store cursor");
        m.event(e.address, std::to_integer<U>(e.written.front()), 2, e.written.size());
    };
    SourcePhysicsWorldRuntimeServices services;
    constexpr std::array sites{0x38846eu, 0x21f590u, 0x33b595u};
    services.instruction = [&](U pc) {
        if (bound && !forced)
            throw std::logic_error("bound owner fell back");
        visited.insert(pc);
        m.pc = pc;
    };
    if (bound && !census && !forced) {
        auto &cursor = services.instruction_cursor;
        cursor.position = &m.pc;
        cursor.service_sites = sites;
        cursor.callback_type = &services.instruction.target_type();
        cursor.executor = std::this_thread::get_id();
        cursor.literal_service_sites = true;
        cursor.observation_only_stores = true;
    }
    WorldState s{m, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    s.load(abi);
    _mm_setcsr(mxcsr);
    try {
        if (bound)
            bound_run(s);
        else
            run_2c2ad0(s);
    } catch (const std::runtime_error &e) {
        out.error = e.what();
    }
    out.final = Registers::capture(s);
    return out;
}
// Rejection must happen before the first memory access or ABI change.
bool guards() {
    for (unsigned mode = 0; mode < 6; ++mode) {
        Bank bank;
        bank.put(0x1200, 5, 2);
        SourcePhysicsWorldRuntimeServices services;
        SourcePhysicsContactRecordObserver observer;
        bool trace = mode == 1;
        constexpr std::array sites{0x38846eu, 0x21f590u, 0x33b595u};
        services.instruction = [](U) {};
        auto &c = services.instruction_cursor;
        c.position = &bank.pc;
        c.service_sites = sites;
        c.callback_type = &services.instruction.target_type();
        c.executor = std::this_thread::get_id();
        c.literal_service_sites = true;
        c.first_trace = &trace;
        if (mode == 2)
            services.required_owner = [](auto &, const auto &, auto &, const auto &) {};
        if (mode == 3)
            observer = [](const auto &) {};
        WorldState state{bank, observer, services,
                         mode == 0 ? SourcePhysicsContactImage::MapPack11
                                   : SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp[0] = 0x1200;
        abi.gp[4] = stack;
        state.load(abi);
        if (mode == 4)
            services.instruction = [](U) {}; // replaced after state cached the witness
        if (mode == 5)
            state.active_mmx.emplace();
        const auto before = Registers::capture(state);
        const auto inert = [](auto &&...) {};
        if (native_particle_motion_detail::bridge_2c2ad0(state, inert, inert, inert, inert, inert,
                                                         canonical_branch) ||
            !bank.events.empty() || !Registers::capture(state).same(before))
            return false;
    }
    return true;
}
} // namespace
int main(int argc, char **argv) {
    const unsigned count = argc > 1 ? std::stoul(argv[1]) : 8000;
    const bool census = std::getenv("HALO2_CENSUS_NATIVE_BRIDGES");
    const bool forced = std::getenv("HALO2_REFERENCE_EFFECT_OWNERS");
    if (!forced && !guards())
        return 3;
    std::mt19937 rng(0x2c2ad0);
    U faults = 0, calls = 0;
    for (trial = 0; trial < count; ++trial) {
        Bank a;
        prepare(a, trial, rng);
        if (trial % 4 == 0)
            a.stop = 1 + rng() % 260;
        Bank b = a;
        SourcePhysicsWorldRuntimeAbi abi{};
        for (auto &v : abi.gp)
            v = rng();
        abi.gp[0] = definition;
        abi.gp[1] = trial % 3 ? rng() : 0;
        abi.gp[4] = stack;
        abi.gp[6] = trial % 9 ? 0x12340000 : 0xffffffffu;
        for (auto &reg : abi.xmm)
            for (auto &lane : reg)
                lane = rng();
        abi.eflags = 0x202 | (rng() & 0xcd5u);
        U mxcsr = 0x1f80 | ((trial % 4) << 13) | (trial & 0x40) | ((trial & 0x20) ? 0x8000 : 0);
        auto x = execute(a, false, census, forced, abi, mxcsr);
        auto y = execute(b, true, census, forced, abi, mxcsr);
        if (census) {
            for (auto &e : a.events)
                e.pc = 0;
            for (auto &e : b.events)
                e.pc = 0;
        }
        bool ok = a.bytes == b.bytes && a.events == b.events && x.error == y.error && x.final.same(y.final) &&
                  x.calls.size() == y.calls.size();
        for (unsigned i = 0; ok && i < x.calls.size(); ++i)
            ok = x.calls[i].target == y.calls[i].target && (census || x.calls[i].pc == y.calls[i].pc) &&
                 x.calls[i].abi.same(y.calls[i].abi);
        if (!ok) {
            std::cerr << "trial " << trial << " pc " << std::hex << a.pc << "/" << b.pc << " error " << x.error
                      << "/" << y.error << " flags " << x.final.flags.bits() << "/" << y.final.flags.bits()
                      << " mxcsr " << x.final.mxcsr << "/" << y.final.mxcsr << "\n";
            return 1;
        }
        faults += x.error == "provider stopped";
        calls += x.calls.size();
    }
    if (visited.size() != motion_sites || !calls || !faults)
        return 2;
    std::cout << "cases " << count << " sites " << visited.size() << " child boundaries " << calls
              << " interrupted calls " << faults << "\n";
}

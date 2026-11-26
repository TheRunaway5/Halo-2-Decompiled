#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage = native_mopp_ray_scale_tail_detail;
using Route = stage::Continuation;
constexpr std::uint32_t base = 0x62000000u, stack = base + 0x2300u;
constexpr std::array service_sites{0x38846eu, 0x21f590u, 0x33b595u};

std::uint32_t spill(long double value, std::uint16_t &status) {
    std::uint32_t bits;
    std::uint16_t actual;
    asm volatile("fldt %2; fstps %0; fnstsw %1"
        :"=m"(bits),"=m"(actual):"m"(value):"st");
    status = (status & ~0x200u) | (actual & 0x200u);
    return bits;
}

void retained(WorldState &s, Route route) {
    switch (route) {
    case Route::original: goto shift;
    case Route::write_shift: goto store_shift;
    case Route::read_scale: goto scale;
    case Route::read_property: goto property;
    case Route::write_property: goto store_property;
    case Route::read_key: goto key;
    case Route::write_key: goto store_key;
    case Route::set_variables: goto variables;
    case Route::resume: return;
    }
shift:
    s.instruction_literal(0x30b40du); s.gp[1] = r(s.memory, s.gp[3] + 0x10u);
    s.instruction_literal(0x30b410u); s.gp[1] = s.flags.plus(s.gp[1], s.gp[0], 4);
store_shift:
    s.instruction_literal(0x30b412u); w(s.memory, s.observe, 0x30b412u, s.gp[4] + 0x50u, 4, s.gp[1]);
scale:
    s.instruction_literal(0x30b416u); s.fp.st(0) = mul(s.fp.st(0), ext(f(r(s.memory, s.gp[3] + 0x14u))));
    s.instruction_literal(0x30b419u); s.gp[6] = s.flags.plus(s.gp[6], 4u, 4);
    s.instruction_literal(0x30b41cu); w(s.memory, s.observe, 0x30b41cu, s.gp[4] + 0x54u, 4, spill(s.fp.st(0), s.fp.status)); s.fp.pop();
property:
    s.instruction_literal(0x30b420u); s.gp[2] = r(s.memory, s.gp[3] + 0x1cu);
store_property:
    s.instruction_literal(0x30b423u); w(s.memory, s.observe, 0x30b423u, s.gp[4] + 0x5cu, 4, s.gp[2]);
key:
    s.instruction_literal(0x30b427u); s.gp[0] = r(s.memory, s.gp[3] + 0x18u);
store_key:
    s.instruction_literal(0x30b42au); w(s.memory, s.observe, 0x30b42au, s.gp[4] + 0x58u, 4, s.gp[0]);
variables:
    s.instruction_literal(0x30b42eu); s.gp[3] = s.gp[4] + 0x40u;
    s.instruction_literal(0x30b432u);
}

struct Failure {};
struct Fixture {
    std::uint32_t vars = base + 0x180u;
    std::uint32_t deny_store{}, fault_pc{};
    bool install_observer{}, initial_observer{}, trace{};
    bool trace_after_store{},move_variables{};
    unsigned rounding{}, precision{3};
    std::uint32_t scale = 0x3f8032ffu;
    long double factor = 127.125L;
};

struct Store {
    std::uint32_t pc, address, value;
    bool operator==(const Store &) const = default;
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;
    std::vector<Store> stores;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> provider;
    std::vector<SourcePhysicsContactRecordStore> observed;
    std::vector<std::uint32_t> instructions;
    std::array<std::byte, 16> bytes{};
    std::uint32_t pc{0x30b408u};
    Route route{};
    bool fault{};
};

struct Memory final : SourcePhysicsContactMemory {
    const Fixture &fixture;
    Result &result;
    SourcePhysicsContactRecordObserver &observer;
    std::array<std::byte, 0x3000> bytes{};
    SourceDirectPageIndex index;
    SourcePhysicsContactRecordObserver record = [&](const auto &store) { result.observed.push_back(store); };
    std::function<void()> after_write;

    Memory(const Fixture &input, Result &output, SourcePhysicsContactRecordObserver &observe)
        : fixture(input), result(output), observer(observe) {
        direct_pages_ = &index;
        auto &block = index.blocks[base >> 22u];
        block = std::make_unique<SourceDirectPageIndex::Block>();
        for (unsigned n = 0; n < 3; ++n)
            (*block)[((base >> 12u) & 1023u) + n] = {bytes.data() + n * 4096u, true};
        set(fixture.vars + 0x10u, 0xfffffff3u);
        set(fixture.vars + 0x14u, fixture.scale);
        set(fixture.vars + 0x18u, 0x87654321u);
        set(fixture.vars + 0x1cu, 0x01234567u);
        if (fixture.initial_observer) observer = record;
    }
    void set(std::uint32_t address, std::uint32_t word) {
        std::memcpy(bytes.data() + address - base, &word, 4);
    }
    void read(std::uint32_t address, std::span<std::byte> output) const override {
        result.provider.emplace_back(result.pc, address);
        if (result.pc == fixture.fault_pc) throw Failure{};
        std::memcpy(output.data(), bytes.data() + address - base, output.size());
    }
    void commit(std::uint32_t address, std::span<const std::byte> input) {
        std::uint32_t word;
        std::memcpy(&word, input.data(), input.size());
        result.stores.push_back({result.pc, address, word});
        std::memcpy(bytes.data() + address - base, input.data(), input.size());
    }
    bool try_callback_free_write(std::uint32_t address, std::span<const std::byte> input) noexcept override {
        if (fixture.deny_store == result.pc || observer) return false;
        commit(address, input);
        return true;
    }
    void write(std::uint32_t address, std::span<const std::byte> input) override {
        if (try_callback_free_write(address, input)) return;
        result.provider.emplace_back(result.pc, address);
        if (result.pc == fixture.fault_pc) throw Failure{};
        commit(address, input);
        if (fixture.install_observer) observer = record;
        if (fixture.deny_store == result.pc) after_write();
    }
};

Result run(const Fixture &fixture, bool native) {
    Result result;
    result.stores.reserve(4);
    SourcePhysicsContactRecordObserver observer;
    Memory bank{fixture, result, observer};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) {
        result.pc = pc;
        result.instructions.push_back(pc);
    };
    bool trace=fixture.trace;
    services.instruction_cursor = {&result.pc, &trace, nullptr,
        service_sites, std::this_thread::get_id(), &services.instruction.target_type(), true};
    const auto cw = std::uint16_t(0x7fu | fixture.precision << 8u | fixture.rounding << 10u);
    asm volatile("fnclex; fldcw %0"::"m"(cw));
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp = {0x80000010u, 5u, 7u, fixture.vars, stack, 0x1089u, 0xfffffffdu, 643u};
    input.eflags = 0x2d7u;
    input.x87_depth = 2;
    input.x87[0] = fixture.factor;
    input.x87[1] = -1.25L;
    input.x87_status = 0x3200u;
    input.floating_point = SourcePhysicsFactoryFloatingPoint{cw, 0x1f80u};
    WorldState state{bank, observer, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(input);
    bank.after_write=[&] {
        if(fixture.trace_after_store)trace=true;
        if(fixture.move_variables) {
            state.gp[3]=base+0x1180u;
            bank.set(state.gp[3]+0x14u,0x40000000u);
            bank.set(state.gp[3]+0x18u,0xaabbccddu);
            bank.set(state.gp[3]+0x1cu,0x11223344u);
        }
    };
    try {
        result.route = native ? stage::scale_tail(state, spill) : Route::original;
        retained(state, result.route);
    } catch (Failure &) {
        result.fault = true;
    }
    result.abi = state.save();
    std::memcpy(result.bytes.data(), bank.bytes.data() + stack - base + 0x50u, result.bytes.size());
    return result;
}

void compare(const Fixture &f, Route expected, bool fault = false) {
    const auto translated = run(f, false);
    const auto native = run(f, true);
    const auto &a = translated.abi, &b = native.abi;
    if (stage::reference_scale_tail()) expected = Route::original;
    const bool route = native.fault || native.route == expected;
    if (!route || native.fault != fault || native.fault != translated.fault ||
        native.pc != translated.pc || native.stores != translated.stores ||
        native.provider != translated.provider || native.observed != translated.observed ||
        native.instructions != translated.instructions || native.bytes != translated.bytes ||
        a.gp != b.gp || a.eflags != b.eflags || a.xmm != b.xmm || a.x87_depth != b.x87_depth ||
        a.x87_status != b.x87_status || a.floating_point->mxcsr != b.floating_point->mxcsr ||
        a.floating_point->x87_control_word != b.floating_point->x87_control_word)
        throw std::runtime_error("MOPP scale-tail fault, store or continuation mismatch");
    for (unsigned i = 0; i < a.x87_depth; ++i) {
        if (std::memcmp(&a.x87[i], &b.x87[i], 10))
            throw std::runtime_error("MOPP scale-tail live x87 differs");
    }
}
}

int main() try {
    constexpr std::array writes{0x30b412u, 0x30b41cu, 0x30b423u, 0x30b42au};
    constexpr std::array write_routes{Route::write_shift, Route::read_property, Route::write_property, Route::write_key};
    constexpr std::array after_writes{Route::read_scale, Route::read_property, Route::read_key, Route::set_variables};
    constexpr std::array reads{0x30b40du, 0x30b416u, 0x30b420u, 0x30b427u};
    constexpr std::array offsets{0x10u, 0x14u, 0x1cu, 0x18u};
    constexpr std::array read_routes{Route::original, Route::read_scale, Route::read_property, Route::read_key};
    unsigned cases{};
    for (const auto p : {0u, 2u, 3u}) for (unsigned rc = 0; rc < 4u; ++rc) {
        Fixture f; f.rounding = rc; f.precision = p;
        for (const auto bits : {0x3f8032ffu, 0x80000000u, 0x7f812345u, 0x80000003u, 0x7f800000u, 0x7fc11112u}) {
            f.scale = bits;
            compare(f, Route::resume); ++cases;
        }
        for (unsigned i = 0; i < writes.size(); ++i) {
            auto store = f; store.deny_store = writes[i];
            const bool rollback=stage::reference_scale_provider();
            const auto fall_back=rollback?write_routes[i]:after_writes[i];
            const auto continued=rollback?write_routes[i]:Route::resume;
            compare(store, continued); ++cases;
            store.move_variables = true;
            compare(store, continued); ++cases;
            store.move_variables = false;
            store.trace_after_store = true;
            compare(store, fall_back); ++cases;
            store.trace_after_store = false;
            store.install_observer = true;
            compare(store, fall_back); ++cases;
            store.fault_pc = writes[i];
            compare(store, write_routes[i], true); ++cases;
            auto read = f; read.vars = base + 0xffeu - offsets[i];
            compare(read, read_routes[i]); ++cases;
            read.fault_pc = reads[i];
            compare(read, read_routes[i], true); ++cases;
        }
        auto alias = f; alias.vars = stack + 0x40u;
        compare(alias, Route::resume); ++cases;
        auto observer = f; observer.initial_observer = true;
        compare(observer, Route::original); ++cases;
        auto traced = f; traced.trace = true;
        compare(traced, Route::original); ++cases;
    }
    std::cout << "30B40D scale tail: " << cases << " exact continuation and fault comparisons\n";
    return 0;
} catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
}

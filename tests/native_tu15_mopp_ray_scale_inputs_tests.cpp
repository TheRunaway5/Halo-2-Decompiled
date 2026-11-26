#include "physics/runtime/native_tu15_mopp_ray_scale_inputs.hpp"

#include <iostream>
#include <random>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage = native_mopp_ray_scale_inputs_detail;
using stage::Continuation;

constexpr std::uint32_t base = 0x10000000u;
constexpr std::uint32_t stack = base + 0x200u;
constexpr std::uint32_t spill = stack + 0x14u;
constexpr std::array service_sites{0x38846eu, 0x21f590u, 0x33b595u};

struct Fixture {
    std::uint32_t program = base + 0x40u, endpoint = base + 0x100u;
    std::uint32_t fault_pc{}, fault_address{};
    bool components = true, vector = true, final_component = true, fast_store = true;
    bool trace{}, observe{}, authentic = true;
};

struct Access {
    std::uint32_t pc, address;
    std::size_t width;
    bool write;
    std::array<std::uint32_t, 8> registers;
    bool operator==(const Access &) const = default;
};

struct Result {
    std::array<std::byte, 1024> bytes{};
    std::array<std::uint32_t, 8> registers{};
    std::array<std::array<std::uint32_t, 4>, 8> xmm{};
    std::vector<Access> effects;
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<std::uint32_t> instructions;
    std::uint32_t pc = 0x30aaa7u, flags{}, mxcsr{};
    Continuation continuation = Continuation::original;
    std::string fault;
};

struct Bank final : SourcePhysicsContactMemory {
    const Fixture &fixture;
    Result &result;
    WorldState *active{};

    Bank(const Fixture &fixture, Result &result): fixture(fixture), result(result) {}

    bool contains(std::uint32_t address, std::size_t count) const noexcept {
        return count <= result.bytes.size() && address - base <= result.bytes.size() - count;
    }

    std::optional<std::span<const std::byte>> try_read_view(
            std::uint32_t address, std::size_t count) const noexcept override {
        if (!contains(address, count)) return std::nullopt;
        if (address == fixture.program + 1u && count == 2u && fixture.components)
            return std::span<const std::byte>{result.bytes}.subspan(address - base, count);
        if (address == fixture.endpoint && count == 16u && fixture.vector)
            return std::span<const std::byte>{result.bytes}.subspan(address - base, count);
        if (address == fixture.program + 3u && count == 1u && fixture.final_component)
            return std::span<const std::byte>{result.bytes}.subspan(address - base, count);
        return std::nullopt;
    }

    void checked_access(std::uint32_t address, std::size_t count, bool is_write) const {
        result.effects.push_back({result.pc, address, count, is_write, active->gp});
        if (!contains(address, count) || (result.pc == fixture.fault_pc && address == fixture.fault_address))
            throw std::runtime_error("Scale input provider fault");
    }

    void read(std::uint32_t address, std::span<std::byte> out) const override {
        const auto pc = result.pc;
        if (!((fixture.components && (pc == 0x30b36au || pc == 0x30b36eu)) ||
              (fixture.vector && pc == 0x30b372u) ||
              (fixture.final_component && pc == 0x30b379u)))
            checked_access(address, out.size(), false);
        if (!contains(address, out.size())) throw std::runtime_error("Scale input provider fault");
        std::memcpy(out.data(), result.bytes.data() + (address - base), out.size());
    }

    bool try_callback_free_write(std::uint32_t address, std::span<const std::byte> bytes) noexcept override {
        if (!fixture.fast_store || address != spill || bytes.size() != 4u) return false;
        std::memcpy(result.bytes.data() + (address - base), bytes.data(), bytes.size());
        return true;
    }

    void write(std::uint32_t address, std::span<const std::byte> bytes) override {
        if (!try_callback_free_write(address, bytes)) {
            checked_access(address, bytes.size(), true);
            std::memcpy(result.bytes.data() + (address - base), bytes.data(), bytes.size());
        }
    }
};

Result run(const Fixture &fixture, unsigned seed, bool native) {
    Result result;
    std::mt19937 random(seed);
    for (auto &byte : result.bytes) byte = std::byte(random() & 255u);
    SourcePhysicsWorldRuntimeAbi input{};
    for (auto &reg : input.gp) reg = std::uint32_t(random());
    input.gp[4] = stack;
    input.gp[6] = fixture.program;
    input.gp[7] = fixture.endpoint;
    for (auto &lanes : input.xmm) for (auto &lane : lanes) lane = std::uint32_t(random());
    input.eflags = 0x202u | (std::uint32_t(random()) & 0x8d5u);
    const auto saved_mxcsr = _mm_getcsr();
    _mm_setcsr(0x1f80u | (std::uint32_t(random()) & 63u));

    Bank bank(fixture, result);
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) { result.pc = pc; result.instructions.push_back(pc); };
    services.instruction_cursor = {&result.pc, &fixture.trace, nullptr, service_sites,
        std::this_thread::get_id(), &services.instruction.target_type(), true};
    if (!fixture.authentic) services.instruction_cursor = {};
    SourcePhysicsContactRecordObserver observe;
    if (fixture.observe) observe = [&](const auto &record) { result.stores.push_back(record); };
    WorldState state{bank, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(input);
    bank.active = &state;
    try {
        if (native) result.continuation = stage::read_scale_inputs(state);
        if (result.continuation == Continuation::original) {
            state.instruction_literal(0x30b36au);
            state.gp[1] = r(bank, state.gp[6] + 1u, 1u);
            state.instruction_literal(0x30b36eu);
            state.gp[2] = r(bank, state.gp[6] + 2u, 1u);
        }
        if (result.continuation == Continuation::original || result.continuation == Continuation::load_endpoint) {
            state.instruction_literal(0x30b372u);
            state.xmm[0] = v(bank, state.gp[7]);
            state.instruction_literal(0x30b375u);
            w(bank, observe, 0x30b375u, state.gp[4] + 0x14u, 4u, state.gp[1]);
        }
        if (result.continuation != Continuation::spill_components) {
            state.instruction_literal(0x30b379u);
            state.gp[1] = r(bank, state.gp[6] + 3u, 1u);
        }
    } catch (const std::runtime_error &error) { result.fault = error.what(); }

    const auto abi = state.save();
    result.registers = abi.gp;
    result.xmm = abi.xmm;
    result.flags = abi.eflags;
    result.mxcsr = _mm_getcsr();
    _mm_setcsr(saved_mxcsr);
    return result;
}

void check(const Fixture &fixture, unsigned seed, Continuation expected) {
    if (stage::reference_mode()) expected = Continuation::original;
    const auto before = run(fixture, seed, false);
    const auto after = run(fixture, seed, true);
    if (before.bytes != after.bytes || before.registers != after.registers || before.xmm != after.xmm ||
            before.effects != after.effects || before.stores != after.stores ||
            before.instructions != after.instructions || before.pc != after.pc || before.flags != after.flags ||
            before.mxcsr != after.mxcsr || before.fault != after.fault ||
            (before.fault.empty() && after.continuation != expected)) {
        std::cerr << "Scale input seed " << seed << ", original PC " << std::hex << before.pc
                  << ", native PC " << after.pc << std::dec << '\n';
        throw std::runtime_error("Scale input continuation differs from original");
    }
}

unsigned prove() {
    unsigned calls = 0;
    for (unsigned seed = 1; seed <= 128u; ++seed) {
        auto test = [&](const Fixture &fixture, Continuation expected) { check(fixture, seed, expected); ++calls; };
        Fixture fixture;
        test(fixture, Continuation::spill_components);
        fixture.program = spill - 3u;
        fixture.endpoint = spill - 8u;
        test(fixture, Continuation::spill_components);
        fixture.final_component = false;
        test(fixture, Continuation::read_final_component);
        fixture.fault_pc = 0x30b379u;
        fixture.fault_address = spill;
        test(fixture, Continuation::read_final_component);
        fixture.fault_pc = 0;
        fixture.vector = false;
        test(fixture, Continuation::load_endpoint);
        fixture.fault_pc = 0x30b372u;
        fixture.fault_address = fixture.endpoint + 8u;
        test(fixture, Continuation::load_endpoint);
        fixture = Fixture{};
        fixture.components = false;
        fixture.fault_pc = 0x30b36eu;
        fixture.fault_address = fixture.program + 2u;
        test(fixture, Continuation::original);
        fixture.components = true;
        fixture.fast_store = false;
        fixture.fault_pc = 0x30b375u;
        fixture.fault_address = spill;
        test(fixture, Continuation::original);
        fixture.fault_pc = 0;
        test(fixture, Continuation::spill_components);
        fixture.observe = true;
        test(fixture, Continuation::original);
        fixture.observe = false;
        fixture.trace = true;
        test(fixture, Continuation::original);
        fixture.trace = false;
        fixture.authentic = false;
        test(fixture, Continuation::original);
    }
    return calls;
}
} // namespace

int main() try {
    std::cout << "30B36A MOPP scale inputs: " << prove() << " ordered continuation comparisons\n";
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}

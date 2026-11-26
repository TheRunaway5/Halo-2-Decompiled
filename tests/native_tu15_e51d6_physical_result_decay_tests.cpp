#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_physical_result_decay_e51d6.hpp"

#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <tuple>

namespace {
using namespace frame_misc_harness;
using halo2::native_physical_result_decay::Entry;
using physics_step_detail::f;
using physics_step_detail::r;
using physics_step_detail::u;
using physics_step_detail::w;

enum class Access { read, store, observer, instruction };
struct Rejected {};
struct Probe : Memory {
    using Operation = std::tuple<Access, std::uint32_t, std::uint32_t, std::size_t>;
    mutable std::vector<Operation> operations;
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::uint32_t pc{};
    std::size_t reject{};

    void visit(Access access, std::uint32_t address = 0, std::size_t bytes = 0) const {
        operations.emplace_back(access, pc, address, bytes);
        if (operations.size() == reject) throw Rejected{};
    }
    void read(std::uint32_t address, std::span<std::byte> bytes) const override {
        visit(Access::read, address, bytes.size());
        Memory::read(address, bytes);
    }
    void write(std::uint32_t address, std::span<const std::byte> bytes) override {
        visit(Access::store, address, bytes.size());
        Memory::write(address, bytes);
    }
    void observe(const SourcePhysicsContactRecordStore &store) {
        stores.push_back(store);
        visit(Access::observer, store.address, store.written.size());
    }
};

constexpr std::uint32_t stack = 0x62000400u, object = 0x61002000u;
constexpr std::uint32_t clock = 0x61000400u, context = 0x61000500u;
constexpr std::uint32_t normal_ret = 0xe52bau, blocked_ret = 0xe52cbu;

struct Fixture { Case state; Entry route{Entry::decay}; };

Fixture fixture(std::mt19937 &random, unsigned path, unsigned variant) {
    Fixture test;
    auto &state = test.state;
    auto &memory = state.memory;
    const auto sp = (variant & 8u ? 0x62000ffcu : stack) + (variant & 3u);
    random_abi(random, state, sp);
    const auto body = variant & 16u ? sp - 0x3a4u : object;
    state.abi.gp[5] = body;
    if (path == 0) test.route = Entry::blocked;
    constexpr std::array values{0x00000000u, 0x80000000u, 0x3e800000u, 0x3f800000u,
        0xbf000000u, 0x00000001u, 0x80000225u, 0x7f800000u, 0xff800000u,
        0x7f855722u, 0x7fc32203u, 0xffc75678u};
    const auto scalar = [&](unsigned n){return values[(variant + n) % values.size()];};
    memory.put(0x4f3218u, context);
    memory.put(context + 8u, path < 4 ? 2u : 5u);
    memory.put(0x51df54u, clock);
    memory.put(clock + 4u, scalar(0));
    memory.put(0x46a0d0u, 0x3f800000u);
    memory.put(0x46a0dcu, 0xbf800000u);
    memory.put(0x46a10cu, 0x3e800000u);
    memory.put(0x46a120u, 0x3f000000u);
    // LAHF/TEST admits the directional path only for an ordered comparison equal to zero.
    const auto direction = path == 7 ? scalar(5) : path >= 4 || (variant & 1u) ? 0u : 0x3f800000u;
    memory.put(body + 0x2c4u, direction);
    memory.put(body + 0x148u, path == 4 ? 0x80u : path == 5 ? 0x100u : 0u);
    memory.put(body + 0x3a4u, path == 1 ? 0x3f000000u : path == 2 ? 0xbf000000u : scalar(3));
    for (unsigned i = 0; i < 4; ++i) memory.put(sp + 4u * i, random());
    memory.put(sp + 0x13u, random(), 1);
    memory.put(sp + 0xe0u, 0x63000000u);
    return test;
}

#include "physical_result_decay_reference.inc"

struct Outcome {
    Probe memory;
    SourcePhysicsWorldRuntimeAbi abi;
    bool rejected{};
};

Outcome execute(const Fixture &test, bool native, std::size_t reject) {
    Outcome out;
    static_cast<Memory &>(out.memory) = test.state.memory.clone();
    out.memory.reject = reject;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) {out.memory.pc = pc; out.memory.visit(Access::instruction);};
    const SourcePhysicsContactRecordObserver observer = [&](const auto &store) {out.memory.observe(store);};
    WorldState state{out.memory, observer, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(test.state.abi);
    try {
        if (native) {
            if (!selected(state, test.route)) throw std::runtime_error("Bound physical-result tail refused fixture");
        } else retained(state, test.route);
    } catch (const Rejected &) {out.rejected = true;}
    out.abi = state.save();
    return out;
}

Outcome check(const Fixture &test, std::size_t reject) {
    auto original = execute(test, false, reject);
    auto native = execute(test, true, reject);
    if (original.rejected != bool(reject) || original.rejected != native.rejected ||
        original.memory.operations != native.memory.operations || original.memory.stores != native.memory.stores ||
        !original.memory.same(native.memory) || !same_abi(original.abi, native.abi) ||
        original.abi.floating_point->mxcsr != native.abi.floating_point->mxcsr)
        throw std::runtime_error("Physical-result tail provider or ABI differs at " + std::to_string(native.memory.pc));
    if (!reject && (native.abi.gp[4] != test.state.abi.gp[4] + 0xf0u ||
            native.memory.pc != (test.route == Entry::blocked ? blocked_ret : normal_ret)))
        throw std::runtime_error("Physical-result tail returned through a different frame");
    return native;
}
}

int main() try {
    std::mt19937 random{0xe51d6u};
    std::set<std::uint32_t> sites, exits;
    std::set<Probe::Operation> faults;
    unsigned cases{}, stores{};
    for (unsigned path = 0; path < 8; ++path) for (unsigned variant = 0; variant < 64; ++variant) try {
        const auto test = fixture(random, path, variant);
        const auto normal = check(test, 0);
        ++cases;
        stores += normal.memory.stores.size();
        exits.insert(normal.memory.pc);
        for (unsigned n = 0; n < normal.memory.operations.size(); ++n) {
            const auto &[phase, pc, address, size] = normal.memory.operations[n];
            if (phase == Access::instruction) sites.insert(pc);
            if (faults.emplace(phase, pc, 0, size).second) (void)check(test, n + 1);
        }
    } catch (const std::exception &error) {
        throw std::runtime_error(std::to_string(path) + "/" + std::to_string(variant) + " " + error.what());
    }
    std::cout << nlohmann::json{{"cases",cases},{"sites",sites},{"exits",exits},
                               {"fault_cases",faults.size()},{"stores",stores}}.dump() << '\n';
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}

#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_physical_result_activity.hpp"

#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <tuple>

namespace {
using namespace frame_misc_harness;
using physics_step_detail::f;
using physics_step_detail::r;
using physics_step_detail::si;
using physics_step_detail::u;
using physics_step_detail::w;

enum class Access { read, store, observer, instruction, child };
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

constexpr std::uint32_t stack = 0x62000500u, manager = 0x61000100u, slots = 0x61000200u;
constexpr std::uint32_t group = 0x61000600u, tick = 0x61000900u, output = 0x61000a00u;
constexpr std::uint32_t object = 0x61002000u, target = 0x61003000u;
constexpr std::array child_sites{0xe4dccu, 0xe4e8cu, 0xe4ef8u, 0xe4f19u, 0xe4f45u};
constexpr std::array child_targets{0xe3310u, 0x1cc310u, 0x1cc310u, 0xdfd20u, 0xe65a0u};
constexpr std::array child_resumes{0xe4dd1u, 0xe4e91u, 0xe4efdu, 0xe4f1eu, 0xe4f4au};
constexpr std::array child_arguments{0u, 2u, 2u, 0u, 0u};
constexpr std::array stack_stores{0xe4dccu, 0xe4e83u, 0xe4e85u, 0xe4e8cu, 0xe4ee5u,
    0xe4ef3u, 0xe4ef8u, 0xe4f19u, 0xe4f45u};

struct Fixture {Case state; bool request_in_progress{}, force_response{}; std::uint32_t response{};};

Fixture fixture(std::mt19937 &random, unsigned path, unsigned variant) {
    Fixture test;
    auto &state = test.state;
    auto &memory = state.memory;
    random_abi(random, state, stack + (variant & 3u));
    const auto sp = state.abi.gp[4];
    const auto index = variant & 7u;
    state.abi.gp[5] = object;
    state.abi.gp[6] = (std::uint32_t(random()) & 0xffff0000u) | index;
    test.request_in_progress = path == 1;
    test.force_response = path == 14;
    constexpr std::array values{0x00000000u, 0x80000000u, 0x3e800000u, 0x3f800000u,
        0xbf000000u, 0x00000001u, 0x80000225u, 0x7f800000u, 0x7f855722u, 0x7fc32203u, 0xffc75678u};
    const auto scalar = [&](unsigned n){return values[(variant + n) % values.size()];};
    test.response = scalar(0);
    std::uint32_t mode = 3;
    if (path >= 3 && path <= 5) mode = 6;
    if (path == 6) mode = 4;
    const bool status = path == 3 || path == 11 || path == 12;
    if (status) mode = 6;
    memory.put(0x4ecbd0u, manager);
    memory.put(manager + 0x44u, slots);
    memory.put(slots + 12u * index + 8u, target);
    memory.put(0x51df54u, tick);
    memory.put(tick + 2u, random(), 2);
    memory.put(tick + 4u, scalar(2));
    memory.put(0x44a8a0u, scalar(3));
    memory.put(0x46a0ccu, scalar(4));
    memory.put(target + 0x3dcu, mode, 1);
    memory.put(target + 0x3f8u, status || path == 4, 1);
    memory.put(target + 0x348u, status ? 0x2000u : 0u, 2);
    memory.put(object + 0x3dcu, path == 6 ? 4u : 3u, 1);
    memory.put(object + 0x3aau, path == 1 || path == 2 ? (variant & 1u ? 0xfeu : 3u) : 0u, 1);
    memory.put(object + 0x134u, ((path >= 8 && path <= 11) ? 0x800000u : 0u) | (random() & 0x7fffffu));
    memory.put(object + 0x148u, path == 7 || path == 9, 1);
    memory.put(object + 0x348u, path == 12 ? 0x4000u : random() & 0x9fffu, 2);
    memory.put(group + 0x264u, path == 13 ? 2u : 0u);
    memory.put(group + 0x268u, scalar(5));
    memory.put(group + 0x26cu, scalar(6));
    memory.put(sp + 0x14u, 12u * index);
    memory.put(sp + 0x2cu, group);
    memory.put(sp + 0x30u, path == 8 ? 2u : 0u);
    memory.put(sp + 0x44u, scalar(7));
    memory.put(sp + 0xe8u, variant & 8u ? object + 0x340u : output);
    memory.put(output + 9u, random(), 1);
    return test;
}

struct ChildContext {Probe &memory; const Fixture &fixture; std::vector<std::uint64_t> log;};
void child(WorldState &, std::uint32_t, std::uint32_t, ChildContext &);
#include "contact_transition_reference.inc"

void child(WorldState &state, std::uint32_t site, std::uint32_t target_pc, ChildContext &ctx) {
    const auto index = std::size_t(std::ranges::find(child_sites, site) - child_sites.begin());
    if (index == child_sites.size() || target_pc != child_targets[index])
        throw std::runtime_error("Unexpected contact transition child");
    const auto sp = state.gp[4];
    if (ctx.memory.get(sp) != child_resumes[index]) throw std::runtime_error("Child resume word differs");
    auto hash = entry_hash(state, target_pc);
    for (unsigned argument = 0; argument < child_arguments[index]; ++argument)
        hash = mix(hash, ctx.memory.get(sp + 4u + 4u * argument));
    ctx.log.push_back(hash);
    ctx.memory.visit(Access::child, target_pc);
    // Hold child service outcomes fixed between the two parents. The i386
    // replay separately exercises all four genuine original children.
    if (index == 0) state.set(0, ctx.fixture.request_in_progress, 1);
    if (index == 3) state.xmm[0] = _mm_set_ss(f(ctx.fixture.response));
    if (index == 4) state.set(0, ctx.fixture.force_response, 1);
    state.gp[4] += 4u * (child_arguments[index] + 1u);
}

struct Outcome {
    Probe memory;
    SourcePhysicsWorldRuntimeAbi abi;
    std::vector<std::uint64_t> calls;
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
    ChildContext ctx{out.memory, test, {}};
    try {
        if (native) {
            if (!selected(state, ctx)) throw std::runtime_error("Bound contact phase refused fixture");
        } else retained(state, ctx);
    } catch (const Rejected &) {out.rejected = true;}
    out.abi = state.save();
    out.calls = std::move(ctx.log);
    return out;
}

Outcome check(const Fixture &test, std::size_t reject) {
    auto original = execute(test, false, reject);
    auto native = execute(test, true, reject);
    if (original.rejected != bool(reject) || original.rejected != native.rejected ||
        original.memory.operations != native.memory.operations || original.memory.stores != native.memory.stores ||
        original.calls != native.calls || !original.memory.same(native.memory) || !same_abi(original.abi, native.abi) ||
        original.abi.floating_point->mxcsr != native.abi.floating_point->mxcsr)
        throw std::runtime_error("Contact provider or ABI differs at " + std::to_string(native.memory.pc));
    if (reject) {
        const auto [phase, pc, address, size] = native.memory.operations.back();
        if (size == 4 && std::ranges::find(stack_stores, pc) != stack_stores.end()) {
            if (phase == Access::store && native.abi.gp[4] != address + 4u)
                throw std::runtime_error("Denied contact frame store advanced ESP");
            if (phase == Access::observer && native.abi.gp[4] != address)
                throw std::runtime_error("Committed contact frame store kept old ESP");
        }
    } else if (native.abi.gp[4] != test.state.abi.gp[4]) throw std::runtime_error("Contact phase unbalanced its stack");
    return native;
}
}

int main() try {
    std::mt19937 random{0xe4dc0u};
    std::set<std::uint32_t> sites, calls;
    std::set<Probe::Operation> faults;
    unsigned cases{}, stores{};
    for (unsigned path = 0; path < 16; ++path) for (unsigned variant = 0; variant < 64; ++variant) try {
        const auto test = fixture(random, path, variant);
        const auto normal = check(test, 0);
        ++cases;
        stores += normal.memory.stores.size();
        for (unsigned n = 0; n < normal.memory.operations.size(); ++n) {
            const auto &[phase, pc, address, size] = normal.memory.operations[n];
            if (phase == Access::instruction) sites.insert(pc);
            if (phase == Access::child) calls.insert(pc);
            if (faults.emplace(phase, pc, phase == Access::child ? address : 0, size).second) (void)check(test, n + 1);
        }
    } catch (const std::exception &error) {
        throw std::runtime_error(std::to_string(path) + "/" + std::to_string(variant) + " " + error.what());
    }
    std::cout << nlohmann::json{{"cases",cases},{"sites",sites},{"call_sites",calls},
                               {"fault_cases",faults.size()},{"stores",stores}}.dump() << '\n';
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}

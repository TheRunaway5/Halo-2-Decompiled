#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_physical_result_activity.hpp"

#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <tuple>

namespace {
using namespace frame_misc_harness;
using halo2::native_physical_result_activity::Continuation;
using physics_step_detail::r;
using physics_step_detail::w;

enum class Access { read, write, committed, instruction, child };
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
        visit(Access::write, address, bytes.size());
        Memory::write(address, bytes);
    }
    void observe(const SourcePhysicsContactRecordStore &store) {
        stores.push_back(store);
        visit(Access::committed, store.address, store.written.size());
    }
};

constexpr std::uint32_t stack = 0x62000600u, manager = 0x61000100u, objects = 0x61000200u;
constexpr std::uint32_t definitions = 0x61000400u, group = 0x61000600u, attached = 0x61000800u;
constexpr std::uint32_t object = 0x61002000u, result = 0x61003000u;
constexpr std::array sites{0xe4aaau, 0xe4ac4u, 0xe4b0au, 0xe4b58u, 0xe4b7cu,
    0xe4d2fu, 0xe4d3fu, 0xe4d52u, 0xe4d68u, 0xe5046u};
constexpr std::array children{0xe0500u, 0xbbbc0u, 0x1ed880u, 0x1edda0u, 0xe53e0u,
    0x144210u, 0x1452f0u, 0x1d5b50u, 0x1d5db0u, 0xe0500u};
constexpr std::array resumes{0xe4aafu, 0xe4ac9u, 0xe4b0fu, 0xe4b5du, 0xe4b81u,
    0xe4d34u, 0xe4d44u, 0xe4d57u, 0xe4d6du, 0xe504bu};
constexpr std::array<unsigned, sites.size()> arguments{0, 1, 2, 1, 3, 0, 0, 1, 2, 0};

enum class Phase { open, project, activity };
struct Fixture { Case state; bool active{}; unsigned variation{}; Phase phase{Phase::open}; };
Fixture fixture(std::mt19937 &rng, unsigned route, unsigned variant) {
    Fixture test;
    auto &state = test.state;
    auto &bank = state.memory;
    random_abi(rng, state, stack + (variant & 3u));
    test.active = route & 1u;
    test.variation = variant;
    const auto sp = state.abi.gp[4];
    const auto id = rng() & 0xffff0000u;
    const auto slot = variant & 7u;
    const auto shape = (5u + (variant & 7u));
    const auto datum = variant & 8u ? object + 0x7cu : result;
    const auto descriptor = variant & 16u ? object - 0x100u : object + 0x180u;
    const auto kind = variant & 32u ? object - 0x80u : object + 0x280u;
    bank.put(sp, 0x63000000u);
    bank.put(sp + 4u, id | slot);
    bank.put(sp + 8u, attached);
    bank.put(sp + 12u, datum);
    for (unsigned n = 4; n <= 0xc0u; n += 4) bank.put(sp - n, rng());
    bank.put(0x4ecbd0u, manager);
    bank.put(manager + 0x44u, objects);
    bank.put(objects + 12 * slot + 8u, object);
    bank.put(0x4f0414u, definitions);
    bank.put(definitions + 16 * shape + 8u, group);
    bank.put(object, shape | (rng() & 0xffff0000u));
    bank.put(object + 0x12au, descriptor - object, 2);
    bank.put(object + 0x33eu, kind - object, 2);
    bank.put(object + 0x348u, rng(), 1);
    bank.put(object + 0xc1u, route >= 2u, 1);
    bank.put(object + 0x10au, route >= 4u ? 4u : 0u, 1);
    bank.put(descriptor + 0x6cu, route == 6u || route == 7u, 1);
    bank.put(kind, route >= 8u ? 4u : 0u, 1);
    bank.put(attached, 0u);
    bank.put(result, 0u);
    return test;
}

Fixture motion_fixture(std::mt19937 &rng, Phase phase, unsigned route, unsigned variant) {
    auto test = fixture(rng, route, variant);
    test.phase = phase;
    auto &state = test.state;
    auto &bank = state.memory;
    const auto sp = state.abi.gp[4], slot = variant & 7u;
    state.abi.gp[5] = object;
    state.abi.gp[6] = variant & 1u ? 0xffffffffu : 0xc5670000u | slot;
    bank.put(sp + 0x14u, 12 * slot);
    bank.put(sp + 0x28u, attached);
    bank.put(sp + 0x2cu, group);
    bank.put(sp + 0x30u, variant & 1u, 1);
    bank.put(sp + 0xe4u, 0xc5670000u | slot);
    bank.put(sp + 0xe8u, attached);
    bank.put(sp + 0xecu, result);
    bank.put(attached + 0x68u, route == 0 ? 0xffffffffu : 5u);
    bank.put(attached, route == 1 ? 0xffffffffu : 5u);
    bank.put(attached + 6u, route == 2 ? 0xffffu : 5u, 2);
    bank.put(attached + 0x7cu, route == 3 ? 0xe0000c2u : 5u);
    bank.put(object + 0xb3u, route == 4, 1);
    bank.put(group + 0x264u, route == 5 ? 0u : 4u);
    bank.put(object + 0x3dcu, variant & 2u ? 1u : 3u, 1);
    bank.put(object + 0xc0u, route == 0 ? 0u : 0x40u, 1);
    bank.put(object + 0x34bu, route == 2, 1);
    bank.put(object + 0xb4u, slot);
    bank.put(0x52be5cu, manager);
    bank.put(objects + 160 * slot + 4u, route == 1 ? 0x800u : 0u);
    bank.put(0x46a0e8u, 0x3f800000u);
    constexpr std::array values{0u, 0x80000000u, 0x3f800000u, 0x40000000u,
        0x7f800000u, 0x7fc23282u, 0x7f812349u, 0x00000167u};
    for (unsigned axis = 0; axis < 3; ++axis) {
        bank.put(result + 0x138u + axis * 4u, values[(variant + axis) % values.size()]);
        bank.put(sp + 0x3cu + axis * 4u, values[(variant + axis + 2u) % values.size()]);
        bank.put(sp + 0x94u + axis * 4u, values[(variant + axis + 3u) % values.size()]);
    }
    return test;
}

struct ChildContext { Probe &memory; const Fixture &test; std::vector<std::uint64_t> calls; };
void child(WorldState &, std::uint32_t, std::uint32_t, ChildContext &);
#include "physical_result_activity_alias.inc"

void child(WorldState &state, std::uint32_t site, std::uint32_t target, ChildContext &ctx) {
    const auto index = std::size_t(std::ranges::find(sites, site) - sites.begin());
    if (index == sites.size() || target != children[index]) throw std::logic_error("Unexpected physical-result child");
    auto &memory = ctx.memory;
    const auto sp = state.gp[4];
    if (memory.get(sp) != resumes[index]) throw std::logic_error("Physical-result child return word differs");
    auto entry = entry_hash(state, target);
    for (unsigned n = 0; n <= arguments[index]; ++n) entry = mix(entry, memory.get(sp + n * 4u));
    ctx.calls.push_back(entry);
    memory.visit(Access::child, target);
    if (target == 0xe0500u) state.set(0, ctx.test.active, 1);
    if (index == 2) {
        const auto frame = sp + (arguments[index] + 1) * 4u;
        for (unsigned n : {0x34u, 0x38u, 0x48u, 0x4cu, 0x50u, 0x54u, 0x58u, 0x5cu})
            w(state.memory, state.observe, target, frame + n, 4, 0x40111200u + n + ctx.test.variation);
    }
    state.gp[4] += (arguments[index] + 1) * 4u;
}

struct Outcome {
    Probe memory;
    SourcePhysicsWorldRuntimeAbi abi;
    std::vector<std::uint64_t> calls;
    std::uint32_t route{};
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
        if (test.phase == Phase::open) {
            const auto next = native ? selected(state, ctx) : retained(state, ctx);
            if (next == Continuation::tick) out.route = 0xe4b81u;
            if (next == Continuation::blocked) out.route = 0xe52bdu;
        } else {
            const bool projection = test.phase == Phase::project;
            out.route = native ? selected_motion(state, ctx, projection) : retained_motion(state, ctx, projection);
        }
    }
    catch (const Rejected &) {out.rejected = true;}
    out.abi = state.save();
    out.calls = std::move(ctx.calls);
    return out;
}

Outcome check(const Fixture &test, std::size_t reject) {
    auto original = execute(test, false, reject);
    auto native = execute(test, true, reject);
    if (original.rejected != bool(reject) || original.rejected != native.rejected || original.route != native.route ||
        original.memory.operations != native.memory.operations || original.memory.stores != native.memory.stores ||
        original.calls != native.calls || !original.memory.same(native.memory) || !same_abi(original.abi, native.abi) ||
        original.abi.floating_point->mxcsr != native.abi.floating_point->mxcsr)
        throw std::runtime_error("Physical-result access/ABI differs at translated/native PCs " +
                                 std::to_string(original.memory.pc) + "/" + std::to_string(native.memory.pc));
    if (!reject && !native.route)
        throw std::runtime_error("Physical-result setup refused its verified fixture");
    return native;
}
}

int main() try {
    std::mt19937 random{0xe4a40u};
    std::set<std::uint32_t> instructions, routed_children, destinations;
    std::set<Probe::Operation> faults;
    unsigned cases{}, stores{};
    for (const auto phase : {Phase::open, Phase::project, Phase::activity})
      for (unsigned route = 0; route < 10; ++route) for (unsigned variant = 0; variant < 64; ++variant) try {
        const auto test = phase == Phase::open ? fixture(random, route, variant) : motion_fixture(random, phase, route, variant);
        const auto normal = check(test, 0);
        ++cases;
        stores += normal.memory.stores.size();
        destinations.insert(normal.route);
        for (unsigned n = 0; n < normal.memory.operations.size(); ++n) {
            const auto &[phase, pc, address, width] = normal.memory.operations[n];
            if (phase == Access::instruction) instructions.insert(pc);
            if (phase == Access::child) routed_children.insert(address);
            if (faults.emplace(phase, pc, phase == Access::child ? address : 0, width).second)
                (void)check(test, n + 1);
        }
    } catch (const std::exception &error) {
        throw std::runtime_error(std::to_string(route) + "/" + std::to_string(variant) + " " + error.what());
    }
    std::cout << nlohmann::json{{"cases",cases},{"sites",instructions},{"children",routed_children},
                               {"fault_cases",faults.size()},{"stores",stores},{"destinations",destinations}}.dump() << '\n';
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}

#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_physical_result_classification_e4b81.hpp"

#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <tuple>

namespace {
using namespace frame_misc_harness;

enum class Access { read, store, observer, instruction, child };
struct Rejected {};
struct Probe : Memory {
    using Operation = std::tuple<Access, std::uint32_t, std::uint32_t, std::size_t>;
    mutable std::vector<Operation> operations;
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::map<std::uint32_t, std::uint64_t> children;
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

enum class Scenario { ordinary, variant_four, variant_five, distinct_class, clear_duration, advance_duration,
                      restore, restore_without_release, unordered, aliased, count };

constexpr std::uint32_t caller_stack = 0x62000400u, object = 0x61002000u;
constexpr std::uint32_t kind = 0x61000200u, prior_contact = 0x61000400u, clock = 0x61000600u;

Case fixture(std::mt19937 &random, Scenario scenario, unsigned variation) {
    Case test;
    auto &memory = test.memory;
    const auto sp = (variation & 4u ? 0x62001002u : caller_stack) + (variation & 3u);
    random_abi(random, test, sp);
    test.abi.gp[5] = object;
    test.abi.gp[6] = scenario == Scenario::aliased ? sp + 0x84u : kind;
    test.abi.gp[7] = scenario == Scenario::aliased ? object + 0x3a8u : prior_contact;
    for (unsigned offset = 4; offset <= 16; offset += 4) memory.put(sp - offset, random());
    memory.put(kind, scenario == Scenario::variant_four ? 4u : scenario == Scenario::variant_five ? 5u : 7u, 1);
    memory.put(sp + 0x84u, scenario == Scenario::aliased ? 4u : scenario == Scenario::variant_five, 1);
    memory.put(sp + 0xe4u, 0xabc12345u);
    memory.put(sp + 0x28u, prior_contact + 0x200u);
    memory.put(sp + 0x24u, random());
    memory.put(sp + 0x2cu, random());
    memory.put(prior_contact + 0x27cu, 0xe0000c2u);
    memory.put(test.abi.gp[7], scenario >= Scenario::distinct_class ? 0x400000cu : 0x800001eu);
    memory.put(object + 0x34au, scenario != Scenario::distinct_class, 1);
    memory.put(object + 0x3a9u, scenario == Scenario::clear_duration ? 1u : 0u, 1);
    memory.put(object + 0x3a8u, scenario == Scenario::aliased ? 0x0cu : variation & 8u ? 255u : 5u, 1);
    memory.put(0x51df54u, clock);
    memory.put(0x46a178u, scenario == Scenario::unordered ? 0x7f850021u : 0x3f800000u);
    memory.put(clock + 4u, (scenario == Scenario::advance_duration || scenario == Scenario::clear_duration) && !(variation & 8u)
        ? 0x3f800000u : 0x3d000000u);
    test.policy = scenario == Scenario::restore_without_release;
    return test;
}

using Route = std::function<void(WorldState &, std::uint32_t, std::uint32_t)>;

#include "physical_result_classification_reference.inc"

struct Outcome {
    Probe memory;
    SourcePhysicsWorldRuntimeAbi abi;
    bool rejected{};
};

Outcome execute(const Case &test, bool native, std::size_t reject) {
    Outcome out;
    static_cast<Memory &>(out.memory) = test.memory.clone();
    out.memory.reject = reject;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) {out.memory.pc = pc; out.memory.visit(Access::instruction);};
    const SourcePhysicsContactRecordObserver observer = [&](const auto &store) {out.memory.observe(store);};
    WorldState state{out.memory, observer, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(test.abi);
    // This fixture compares the parent's exact ABI at its route boundary and
    // supplies deterministic child outcomes to cover both parent continuations.
    const Route route = [&](WorldState &child, std::uint32_t site, std::uint32_t target) {
        out.memory.visit(Access::child, target);
        out.memory.children.emplace(site, entry_hash(child, target));
        const auto resume = child.pop();
        switch (site) {
        case 0xe4bb0u:
            if (target != 0xb95d0u || resume != 0xe4bb5u) throw std::runtime_error("Variant child route differs");
            child.gp[4] += 12u;
            break;
        case 0xe4c29u:
            if (target != 0x1d3c20u || resume != 0xe4c2eu) throw std::runtime_error("Contact child route differs");
            child.gp[4] += 8u;
            out.memory.put(child.gp[4] + 0x26u, test.policy ? 0xffffu : 4u, 2);
            break;
        case 0xe4c3fu:
            if (target != 0x1d6490u || resume != 0xe4c44u) throw std::runtime_error("Release child route differs");
            child.gp[4] += 4u;
            break;
        case 0xe4c4du:
            if (target != 0xdf820u || resume != 0xe4c52u) throw std::runtime_error("Classification child route differs");
            child.set(0, 0x3fu, 1, 0);
            break;
        default:
            throw std::runtime_error("Unexpected classification child");
        }
    };
    try {
        if (native) {
            if (!selected(state, route)) throw std::runtime_error("Bound classification refused fixture");
        } else retained(state, route);
    } catch (const Rejected &) {out.rejected = true;}
    out.abi = state.save();
    return out;
}

Outcome check(const Case &test, std::size_t reject) {
    auto original = execute(test, false, reject);
    auto native = execute(test, true, reject);
    if (original.rejected != bool(reject) || original.rejected != native.rejected ||
        original.memory.operations != native.memory.operations || original.memory.stores != native.memory.stores ||
        original.memory.children != native.memory.children || !original.memory.same(native.memory) ||
        !same_abi(original.abi, native.abi) || original.abi.floating_point->mxcsr != native.abi.floating_point->mxcsr)
        throw std::runtime_error("Classification provider, child or ABI differs at " + std::to_string(native.memory.pc));
    if (!reject && (native.abi.gp[4] != test.abi.gp[4] || native.memory.pc != 0xe4c52u))
        throw std::runtime_error("Classification did not restore the parent frame");
    return native;
}
}

int main() try {
    std::mt19937 random{0xe4b81u};
    std::set<std::uint32_t> sites, children;
    std::set<std::tuple<Access, std::uint32_t, unsigned>> faults;
    unsigned cases{}, stores{};
    for (unsigned scenario = 0; scenario < unsigned(Scenario::count); ++scenario)
        for (unsigned variation = 0; variation < 32; ++variation) try {
            const auto test = fixture(random, Scenario(scenario), variation);
            const auto normal = check(test, 0);
            ++cases;
            stores += normal.memory.stores.size();
            for (const auto &[site, abi] : normal.memory.children) children.insert(site);
            std::map<std::pair<Access, std::uint32_t>, unsigned> ordinal;
            for (unsigned n = 0; n < normal.memory.operations.size(); ++n) {
                const auto &[phase, pc, address, size] = normal.memory.operations[n];
                if (phase == Access::instruction) sites.insert(pc);
                if (faults.emplace(phase, pc, ordinal[{phase, pc}]++).second) (void)check(test, n + 1);
            }
        } catch (const std::exception &error) {
            throw std::runtime_error(std::to_string(scenario) + "/" + std::to_string(variation) + " " + error.what());
        }
    std::cout << nlohmann::json{{"cases",cases},{"sites",sites},{"children",children},
                               {"fault_cases",faults.size()},{"stores",stores}}.dump() << '\n';
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}

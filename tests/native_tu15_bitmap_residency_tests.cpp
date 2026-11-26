#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/models/native_tu15_bitmap_residency.hpp"

#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <tuple>

namespace {
using namespace frame_misc_harness;
using Json = nlohmann::json;
using physics_step_detail::r;
using physics_step_detail::w;

struct Stop {};
enum class Access { none, read, write, observed_write, instruction, child };
struct Probe : Memory {
    using Operation = std::tuple<Access, std::uint32_t, std::uint32_t, unsigned>;
    mutable std::vector<Operation> operations;
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::uint32_t pc{};
    std::size_t reject{};

    void visit(Access phase, std::uint32_t address, unsigned width) const {
        operations.emplace_back(phase, pc, address, width);
        if (operations.size() == reject) throw Stop{};
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
        visit(Access::observed_write, store.address, store.written.size());
    }
};

constexpr std::array targets{0x12f330u, 0x12f7c0u, 0x12f620u, 0x12f4a0u};
constexpr std::array resumes{0x12f9d1u, 0x12fa2au, 0x12fa47u, 0x12fa9fu};
constexpr std::array stack_stores{0x12f91fu, 0x12f926u, 0x12f939u, 0x12f9a5u, 0x12f9ccu,
    0x12fa25u, 0x12fa42u, 0x12fa83u, 0x12fa88u, 0x12fa8du, 0x12fa92u, 0x12fa99u, 0x12fa9au};
struct Test {
    Case input;
    unsigned mode{}, selection{};
    bool force_mode_seven{}, retry{}, resident_result{}, skip_scan{};
};

constexpr std::uint32_t stack = 0x62000400u, row = 0x61000100u;
constexpr std::uint32_t pool = 0x61000200u, cache = 0x61000300u;
constexpr std::uint32_t page_pool = 0x61000400u, tracking = 0x61000500u, page_bank = 0x61000600u;
constexpr std::uint32_t stop = 0x63000000u;

Test fixture(std::mt19937 &random, unsigned route, unsigned variant) {
    Test test;
    random_abi(random, test.input, stack + (variant & 3u));
    auto &input = test.input;
    auto &memory = input.memory;
    const auto sp = input.abi.gp[4];
    test.mode = route == 8 ? 4u : route == 9 ? 8u : route == 10 ? 16u : variant & 3u;
    test.selection = variant % 3;
    test.force_mode_seven = route == 11;
    test.retry = route == 4 || route == 9;
    test.resident_result = route == 4 || route == 8;
    test.skip_scan = route == 5;
    memory.put(sp, stop);
    memory.put(sp + 4u, row);
    memory.put(sp + 8u, test.mode);
    memory.put(0x474da1u, variant & 1u, 1);
    memory.put(0x474d9du, variant & 2u, 1);
    memory.put(0x474d9cu, variant & 4u, 1);
    memory.put(0x46a18cu, variant & 8u ? 0x7fc31556u : 0x3a500000u);
    memory.put(0x46a0d0u, 0x3f800000u);
    memory.put(0x4f2d4cu, 0x3b800000u);
    memory.put(0x4f2d38u, route == 1 || route == 2 || route == 3, 1);
    memory.put(0x4f2d40u, route == 6 || route == 7 || route == 12 ? 1u : 0u);
    memory.put(0x4f2d44u, route == 12 ? 1u : variant % 4);
    memory.put(0x4f2d4bu, route == 3 || route == 11, 1);
    memory.put(0x4f2d34u, cache);
    memory.put(0x4f2d24u, pool);
    memory.put(0x492304u, 0x12345670u);
    memory.put(0x492308u, 0x12345680u);
    memory.put(0x49230cu, 0x12345690u);
    memory.put(row + 0xf, route == 0 ? 0 : 2, 1);
    memory.put(row + 0xe, variant & 1u ? 0x80 : 0, 1);
    memory.put(row + 0x16, variant, 1);
    memory.put(row + 0x17, route == 12 ? 3 : variant, 1);
    memory.put(row + 0x50, 0xf00d437au);
    memory.put(row + 0x5c, route == 1 || route == 3 ? 1 : variant & 1u ? 0xffffffffu : 0);
    memory.put(row + 0x64, variant & 2u ? 8 : 0);
    memory.put(row + 0x58, 0x3f000000u);
    memory.put(row + 0xa, variant % 3, 2);
    memory.put(row + 0x28, route == 7 || route == 13 ? 0 : 0xffffffffu);
    memory.put(row + 0x2c, route == 7 ? 1 : 0xffffffffu);
    memory.put(row + 0x30, 0xffffffffu);
    memory.put(cache + 0x64, page_pool);
    memory.put(cache + 0x38, 0x12340001u);
    memory.put(page_pool + 0x44, page_bank);
    memory.put(pool + 0x44, route == 13 ? 0xffffffecu : tracking);
    memory.put(tracking + 3, 1, 1);
    memory.put(0xffffffefu, 1, 1);
    input.abi.xmm[1][0] = route == 1 || route == 3 ? 0x3f800000u :
        variant & 8u ? 0x7f812340u : 0x3e800000u;
    return test;
}

struct ChildContext {
    Probe &memory;
    const Test &test;
    std::vector<std::uint64_t> log;
};

void child(WorldState &, std::uint32_t, ChildContext &);
#include "bitmap_residency_alias.inc"

void child(WorldState &state, std::uint32_t target, ChildContext &context) {
    const auto index = std::size_t(std::ranges::find(targets, target) - targets.begin());
    if (index == targets.size()) throw std::logic_error("Unexpected bitmap child");
    auto &memory = context.memory;
    const auto sp = state.gp[4];
    std::uint64_t entry = entry_hash(state, target);
    for (unsigned n = 0; n < (index == 3 ? 6u : 1u); ++n) entry = mix(entry, memory.get(sp + n * 4u));
    context.log.push_back(entry);
    memory.visit(Access::child, target, 0);
    if (memory.get(sp) != resumes[index]) throw std::logic_error("Wrong bitmap child return word");
    // Keep the same deterministic resource outcomes for both bodies while
    // retaining their complete observed child entry state and caller stack.
    if (index == 1) state.set(0, context.test.selection == 0, 1);
    if (index == 2) {
        state.gp[0] = context.test.selection;
        if (context.test.force_mode_seven) w(state.memory, state.observe, memory.pc, sp + 0x18u, 1, 1);
    }
    if (index == 3) {
        const auto retry = memory.get(sp + 20u);
        w(state.memory, state.observe, memory.pc, retry, 1, context.test.retry);
        w(state.memory, state.observe, memory.pc, retry + 2u, 1, context.test.skip_scan);
        state.gp[0] = context.test.resident_result ? 0x12340a20u : 0;
    }
    state.gp[4] += index == 3 ? 24u : 4u;
}

struct Outcome {
    Probe memory;
    SourcePhysicsWorldRuntimeAbi abi;
    std::vector<std::uint64_t> children;
    bool rejected{};
};

Outcome execute(const Test &test, bool native, std::size_t reject) {
    Outcome out;
    static_cast<Memory &>(out.memory) = test.input.memory.clone();
    out.memory.reject = reject;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) {
        out.memory.pc = pc;
        out.memory.visit(Access::instruction, 0, 0);
    };
    const SourcePhysicsContactRecordObserver observer = [&](const auto &store){out.memory.observe(store);};
    WorldState state{out.memory, observer, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(test.input.abi);
    ChildContext context{out.memory, test, {}};
    try {
        if (native) selected(state, context);
        else retained(state, context);
    } catch(const Stop &) {out.rejected = true;}
    out.abi = state.save();
    out.children = std::move(context.log);
    return out;
}

Outcome check(const Test &test, std::size_t reject) {
    auto original = execute(test, false, reject);
    auto native = execute(test, true, reject);
    const char *difference = nullptr;
    if (original.rejected != native.rejected || original.rejected != bool(reject)) difference = "fault outcome";
    else if (original.memory.operations != native.memory.operations) difference = "instruction/provider order";
    else if (original.memory.stores != native.memory.stores) difference = "ordered stores/preimages";
    else if (original.children != native.children) difference = "child entry ABI";
    else if (!original.memory.same(native.memory)) difference = "memory banks";
    else if (!same_abi(original.abi, native.abi) || original.abi.floating_point->mxcsr != native.abi.floating_point->mxcsr)
        difference = "GP/EFLAGS/XMM/x87/MXCSR";
    if (difference) throw std::runtime_error(std::string(difference) + " differs at native/retained PC " +
        std::to_string(native.memory.pc) + "/" + std::to_string(original.memory.pc));
    if (reject) {
        const auto [phase, pc, address, width] = native.memory.operations.back();
        if (width == 4 && std::ranges::find(stack_stores, pc) != stack_stores.end()) {
            if (phase == Access::write && native.abi.gp[4] != address + 4u)
                throw std::runtime_error("Rejected bitmap stack store advanced ESP");
            if (phase == Access::observed_write && native.abi.gp[4] != address)
                throw std::runtime_error("Committed bitmap stack store reached the observer with stale ESP");
        }
    }
    if (!reject && native.abi.gp[4] != test.input.abi.gp[4] + 12u) throw std::runtime_error("Bitmap RET8 stack escaped");
    return original;
}
}

int main() try {
    std::mt19937 random{0x12f910};
    std::set<std::uint32_t> sites, children, before_stack, after_stack;
    std::set<Probe::Operation> faults;
    unsigned cases = 0, stores = 0;
    for (unsigned route = 0; route < 14; ++route) {
        for (unsigned variant = 0; variant < 16; ++variant) try {
            const auto test = fixture(random, route, variant);
            const auto result = check(test, 0);
            ++cases;
            stores += result.memory.stores.size();
            const auto &operations = result.memory.operations;
            for (unsigned n = 0; n < operations.size(); ++n) {
                const auto &[phase, pc, address, width] = operations[n];
                if (phase == Access::instruction) sites.insert(pc);
                if (phase == Access::child) children.insert(address);
                if (faults.emplace(phase, pc, phase == Access::child ? address : 0, width).second) {
                    (void)check(test, n + 1);
                    if (width == 4 && std::ranges::find(stack_stores, pc) != stack_stores.end()) {
                        if (phase == Access::write) before_stack.insert(pc);
                        if (phase == Access::observed_write) after_stack.insert(pc);
                    }
                }
            }
        } catch (const std::exception &e) {
            throw std::runtime_error("Route " + std::to_string(route) + "/" + std::to_string(variant) + " " + e.what());
        }
    }
    std::cout << Json{{"cases", cases}, {"sites", sites}, {"child_entries", children}, {"stores", stores},
                      {"fault_cases", faults.size()}, {"stack_store_denials", before_stack},
                      {"committed_stack_observer_rejections", after_stack}}.dump() << '\n';
} catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
}

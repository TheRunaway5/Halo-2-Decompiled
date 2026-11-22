#include "halo2/native_campaign_ai_behavior_start_42_tu15.hpp"

#include <iostream>
#include <map>
#include <stdexcept>

namespace {
using namespace halo2;
using Word = std::uint32_t;
constexpr auto image = SourcePhysicsContactImage::TitleUpdate15;
constexpr SourcePhysicsContactRuntimeCall call{0x1b0d1cu, 0x1c0230u, false};
constexpr Word stack = 0x600eff30u, request = 0x600eff78u, actor = 0xe3620000u, descriptor = 0x48afc0u;
constexpr Word pool = 0x82fad850u, rows = 0x82fad89cu, resume = 0x1b0d1eu;

void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

struct Fault {};

struct Access {
    char kind;
    Word pc, address;
    std::size_t width;
    bool operator==(const Access &) const = default;
};

struct Memory final : SourcePhysicsContactMemory {
    std::map<Word, std::byte> bytes;
    std::vector<Word> sites;
    mutable std::vector<Access> accesses;
    Word pc{};
    std::optional<Access> fault;

    void access(char kind, Word address, std::size_t width) const {
        accesses.push_back({kind, pc, address, width});
        if (fault == accesses.back()) throw Fault{};
    }
    void read(Word address, std::span<std::byte> output) const override {
        access('r', address, output.size());
        for (std::size_t i = 0; i < output.size(); ++i) output[i] = bytes.at(address + Word(i));
    }
    void write(Word address, std::span<const std::byte> input) override {
        access('w', address, input.size());
        for (std::size_t i = 0; i < input.size(); ++i) bytes.at(address + Word(i)) = input[i];
    }
    void put(Word address, Word value, unsigned width = 4) {
        for (unsigned i = 0; i < width; ++i) bytes[address + i] = std::byte(value >> (i * 8u));
    }
    Word get(Word address, unsigned width = 4) const {
        Word value{};
        for (unsigned i = 0; i < width; ++i) value |= std::to_integer<Word>(bytes.at(address + i)) << (i * 8u);
        return value;
    }
};

struct Run {
    Memory memory;
    SourcePhysicsWorldRuntimeAbi abi{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<Word> observed_sp;
    std::optional<Word> stop_after;
    Word row;
    bool change_sentinel_after_store{};

    explicit Run(Word datum = actor, Word gate = 0, Word output = request)
        : row(rows + (datum & 0xffffu) * 0x888u) {
        abi.gp = {call.target, descriptor, datum, 1u, stack, 0xffff002au, rows, request};
        abi.eflags = 0x607u;
        abi.fs_base = 0x60200000u;
        abi.floating_point = SourcePhysicsFactoryFloatingPoint{0x027fu, 0x1f80u};
        abi.x87_depth = 1;
        abi.x87_status = 0x2800u;
        abi.x87[0] = 1.5L;
        for (unsigned n = 0; n < abi.xmm.size(); ++n)
            abi.xmm[n] = {0x3f800000u + n, 0x80000000u, n, 0x7fc12345u};

        for (Word p = stack - 8u; p <= stack + 8u; p += 4u) memory.put(p, 0xcafe0000u | (p & 0xffffu));
        memory.put(stack, resume);
        memory.put(stack + 4u, datum);
        memory.put(stack + 8u, output);
        memory.put(output + 0x0cu, 0xc5u, 1);
        memory.put(0x5020c8u, pool);
        memory.put(pool + 0x44u, rows);
        memory.put(0x47d6c0u, 0xffffffffu);
        memory.put(row + 0x225u, gate, 1);
        memory.put(row + 0x220u, 0xf0u, 1);
        memory.put(row + 0x84u, 0x3456u, 2);
        for (Word p = row + 0x3feu; p != row + 0x420u; ++p) memory.put(p, p & 255u, 1);
    }

    bool run(const SourcePhysicsContactRuntimeCall &actual = call, SourcePhysicsContactImage mode = image,
             bool observe = true) {
        SourcePhysicsWorldRuntimeServices services;
        services.instruction = [&](Word pc) { memory.pc = pc; memory.sites.push_back(pc); };
        services.required_owner = [](auto &, const auto &, auto &, const auto &) {
            throw std::runtime_error("Descriptor-42 leaf called another owner");
        };
        SourcePhysicsContactRecordObserver observer;
        if (observe) observer = [&](const auto &store) {
            check(memory.pc == store.instruction, "Completed store lost its original cursor");
            stores.push_back(store);
            observed_sp.push_back(abi.gp[4]);
            if (change_sentinel_after_store && store.instruction == 0x1c0296u)
                memory.put(0x47d6c0u, Word(stores.size()));
            if (stop_after == store.instruction) throw Fault{};
        };
        return native_campaign_ai_behavior_start_42_tu15_try_run(memory, mode, actual, abi, services, observer);
    }
};

void completed(Run &r, Word result) {
    const auto before = r.abi;
    check(r.run(), "Actual descriptor-42 owner refused");
    check((r.abi.gp[0] & 255u) == result && r.abi.gp[4] == stack + 12u && r.memory.pc == 0x1c02a2u,
        "Original AL or RET 8 continuation changed");
    for (const unsigned reg : {3u, 5u, 6u, 7u})
        check(r.abi.gp[reg] == before.gp[reg], "Callee-saved register changed");
    check(r.abi.eflags == (result ? 0x646u : 0x682u), "Defined CMP/DEC flags changed");
    check(r.abi.xmm == before.xmm && r.abi.fs_base == before.fs_base &&
        r.abi.x87[0] == before.x87[0] && r.abi.x87_status == before.x87_status &&
        r.abi.x87_depth == before.x87_depth && r.abi.floating_point->mxcsr == before.floating_point->mxcsr &&
        r.abi.floating_point->x87_control_word == before.floating_point->x87_control_word,
        "Integer leaf changed FP or FS state");
}

void branches() {
    Run rejected(actor, 0x80u);
    completed(rejected, 0);
    check(rejected.abi.gp[0] == 0x1c0200u && rejected.abi.gp[1] == 0 && rejected.abi.gp[2] == rows &&
        rejected.stores.size() == 2u && rejected.memory.get(request + 0x0cu, 1) == 0xc5u,
        "Gate failure changed actor or request");

    Run accepted(0xc1230001u);
    accepted.change_sentinel_after_store = true;
    completed(accepted, 1);
    check(accepted.stores.size() == 10u && accepted.observed_sp[0] == stack - 4u &&
        accepted.observed_sp[1] == stack - 8u && accepted.memory.get(request + 0x0cu, 1) == 1u &&
        accepted.memory.get(accepted.row + 0x84u, 2) == 4u &&
        accepted.memory.get(accepted.row + 0x220u, 1) == 0u &&
        accepted.memory.get(accepted.row + 0x3feu, 2) == 3u,
        "Start callback did not publish ordered actor/request fields");
    for (unsigned i = 0; i < 4u; ++i) {
        const auto address = accepted.row + 0x402u + i * 6u;
        check(accepted.memory.get(address) == (i ? 6u + i : 0xffffffffu) &&
            accepted.memory.get(address + 4u, 2) == ((address + 4u) & 255u) + (((address + 5u) & 255u) << 8u),
            "Sentinel was not reread or six-byte record padding changed");
    }
    check(accepted.abi.gp[0] == (((accepted.row + 0x41au) & ~255u) | 1u) &&
        accepted.abi.gp[1] == 0 && accepted.abi.gp[2] == 9u, "Successful full register result changed");

    Run aliased(actor, 0, pool + 0x44u - 0x0cu);
    constexpr auto redirected = (rows & ~255u) | 1u;
    for (Word p = redirected + 0x3feu; p != redirected + 0x41au; ++p) aliased.memory.put(p, 0x7au, 1);
    completed(aliased, 1);
    check(aliased.memory.get(pool + 0x44u) == redirected &&
        aliased.memory.get(redirected + 0x3feu, 2) == 3u &&
        aliased.memory.get(redirected + 0x402u) == 0xffffffffu &&
        aliased.memory.get(rows + 0x3feu, 2) != 3u, "Published pool base was not reread");
}

void faults_and_guards() {
    const auto expect_fault = [](Run &r) {
        try { (void)r.run(); } catch (const Fault &) { return; }
        throw std::runtime_error("Faulting original access completed");
    };
    Run push_fault;
    push_fault.memory.fault = Access{'w', 0x1c0241u, stack - 8u, 4};
    expect_fault(push_fault);
    check(push_fault.abi.gp[4] == stack - 4u && push_fault.stores.size() == 1u,
        "Refused PUSH changed ESP or completed a store");
    Run post_store_fault;
    post_store_fault.stop_after = 0x1c0241u;
    expect_fault(post_store_fault);
    check(post_store_fault.abi.gp[4] == stack - 8u && post_store_fault.stores.size() == 2u,
        "Completed PUSH was lost on observer stop");
    Run pool_fault;
    pool_fault.memory.fault = Access{'r', 0x1c0271u, pool + 0x44u, 4};
    expect_fault(pool_fault);
    check(pool_fault.abi.gp[0] == request && pool_fault.abi.gp[4] == stack - 8u && pool_fault.stores.size() == 5u,
        "Second pool read fault changed the completed prefix");
    Run return_fault;
    return_fault.memory.fault = Access{'r', 0x1c02a2u, stack, 4};
    expect_fault(return_fault);
    check(return_fault.abi.gp[4] == stack && return_fault.stores.size() == 10u,
        "Faulting return consumed its continuation");

    Run wrong;
    check(!wrong.run({call.instruction + 1u, call.target, false}) &&
        !wrong.run({call.instruction, call.target + 1u, false}) &&
        !wrong.run({call.instruction, call.target, true}) &&
        !wrong.run(call, SourcePhysicsContactImage::MapPack11), "Unrelated caller was admitted");
    wrong.abi.gp[1] = 0x48a638u;
    check(!wrong.run() && wrong.memory.sites.empty() && wrong.memory.accesses.empty(),
        "Unrelated descriptor slot changed the original state");
}

enum class Selection { no_prop, actor_busy, other_prop, matching_prop, same_context, mask_excluded, bit_disabled, alternate };

struct SelectionRun {
    static constexpr SourcePhysicsContactRuntimeCall call{0x1afce4u, 0x1c0190u, false};
    static constexpr Word sp = 0x600eff08u, continuation = 0x1afce6u, prop = 0xefee0003u;
    static constexpr Word selected = 0x82fad96cu;
    static constexpr Word prop_pool = 0x84101000u, prop_rows = 0x84102000u, context = 0x84103120u;
    static constexpr Word default_eax = 0xa511ffffu, sentinel = 0x9bf6u;
    Memory memory;
    SourcePhysicsWorldRuntimeAbi abi{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<Word> observed_sp;
    std::optional<Word> stop_after;

    explicit SelectionRun(Selection scenario) {
        constexpr Word datum = actor | 9u, row = rows + 9u * 0x888u;
        abi.gp = {call.target, request, datum, descriptor, sp, 0x400u, selected, rows};
        abi.eflags = 0x607u;
        abi.fs_base = 0x60200000u;
        abi.floating_point = SourcePhysicsFactoryFloatingPoint{0x027fu, 0x5f85u};
        abi.x87_depth = 1;
        abi.x87_status = 0x2804u;
        abi.x87[0] = 1.5L;
        for (unsigned n = 0; n < abi.xmm.size(); ++n)
            abi.xmm[n] = {0x3f800000u + n, 0x80000000u, n, 0x7fc12345u};
        memory.put(sp - 8u, 0xdedededeu);
        memory.put(sp - 4u, 0xbabababau);
        memory.put(sp, continuation);
        memory.put(sp + 4u, datum);
        memory.put(sp + 8u, selected);
        memory.put(sp + 12u, request);
        memory.put(0x5020c8u, pool);
        memory.put(0x47c1a4u, default_eax);
        memory.put(0x47c1a8u, sentinel, 2);
        memory.put(pool + 0x44u, rows);
        memory.put(row + 0x338u, scenario == Selection::no_prop ? 0xffffffffu : prop);
        memory.put(row + 0x225u, scenario == Selection::actor_busy ? 0x80u : 0, 1);
        memory.put(selected + 0xdu, scenario >= Selection::same_context ? 1u : 0, 1);
        memory.put(0x50eef0u, prop_pool);
        memory.put(prop_pool + 0x44u, prop_rows);
        memory.put(prop_rows + 3u * 0x3cu + 0x18u, scenario == Selection::matching_prop ? 1u : 2u, 2);
        memory.put(0x47b550u, context);
        memory.put(0x47b908u, 0xde000123u);
        memory.put(context + 8u, scenario == Selection::same_context ? 0xde000123u : 0xde000124u);
        memory.put(context + 4u, scenario == Selection::mask_excluded ? 0x1100u : 0x1120u);
        memory.put(0x4fafc0u, 0x1020u);
        memory.put(0x56ae88u, scenario == Selection::bit_disabled ? 0x80000000u : 0x80400000u);
    }

    bool run(SourcePhysicsContactRuntimeCall actual = call, SourcePhysicsContactImage mode = image) {
        SourcePhysicsWorldRuntimeServices services;
        services.instruction = [&](Word pc) { memory.pc = pc; memory.sites.push_back(pc); };
        services.required_owner = [](auto &, const auto &, auto &, const auto &) {
            throw std::runtime_error("Descriptor-42 selector called an unrelated owner");
        };
        return native_campaign_ai_behavior_select_42_tu15_try_run(memory, mode, actual, abi, services,
            [&](const auto &store) {
                check(memory.pc == store.instruction, "Selector store lost its original cursor");
                stores.push_back(store);
                observed_sp.push_back(abi.gp[4]);
                if (stop_after == store.instruction) throw Fault{};
            });
    }
};

void selector_branches() {
    using Case = Selection;
    for (const auto scenario : {Case::no_prop, Case::actor_busy, Case::other_prop, Case::matching_prop, Case::same_context,
             Case::mask_excluded, Case::bit_disabled, Case::alternate}) {
        SelectionRun r(scenario);
        const auto before = r.abi;
        check(r.run(), "Actual descriptor-42 selector refused");
        const auto expected = scenario == Case::alternate ? 0x36u : scenario == Case::matching_prop ?
            (SelectionRun::default_eax & 0xffff0000u) | SelectionRun::sentinel : SelectionRun::default_eax;
        check(r.abi.gp[0] == expected && r.abi.gp[4] == SelectionRun::sp + 16u &&
              r.memory.pc == (scenario == Case::alternate ? 0x1c0208u : 0x1c022du), "Selector result or RET 12 differs");
        for (unsigned i : {3u, 5u, 6u, 7u})
            check(r.abi.gp[i] == before.gp[i], "Selector lost a callee-save");
        check(r.abi.xmm == before.xmm && r.abi.x87 == before.x87 && r.abi.x87_depth == before.x87_depth &&
              r.abi.x87_status == before.x87_status && r.abi.fs_base == before.fs_base &&
              r.abi.floating_point->mxcsr == before.floating_point->mxcsr &&
              r.abi.floating_point->x87_control_word == before.floating_point->x87_control_word,
              "Selector changed the original vector, x87, or segment state");
        check(r.stores.size() == (scenario == Case::no_prop ? 1u : 2u) &&
            r.stores[0].instruction == 0x1c01abu && r.observed_sp[0] == SelectionRun::sp - 4u &&
            (r.stores.size() == 1u || (r.stores[1].instruction == 0x1c01bdu && r.observed_sp[1] == SelectionRun::sp - 8u)),
            "Selector changed preserved-register store order");
        for (const auto &access : r.memory.accesses)
            check(access.address != SelectionRun::sp + 12u, "Unused third argument was accessed");
    }
}

void selector_faults_and_guards() {
    const auto expect_fault = [](SelectionRun &r) {
        try { (void)r.run(); } catch (const Fault &) { return; }
        throw std::runtime_error("Selector fault was not retained");
    };
    SelectionRun denied(Selection::matching_prop);
    denied.memory.fault = Access{'w',0x1c01bdu,SelectionRun::sp - 8u,4};
    expect_fault(denied);
    check(denied.abi.gp[4] == SelectionRun::sp - 4 && denied.stores.size() == 1u, "Refused EBX save advanced the stack");

    SelectionRun observed(Selection::matching_prop);
    observed.stop_after = 0x1c01bdu;
    expect_fault(observed);
    check(observed.abi.gp[4] == SelectionRun::sp - 8 && observed.stores.size() == 2u,
        "Successful EBX save disappeared after observer fault");

    SelectionRun actor_fault(Selection::matching_prop);
    actor_fault.memory.fault = Access{'r',0x1c01afu,rows + 9u * 0x888u + 0x338u,4};
    expect_fault(actor_fault);
    check(actor_fault.abi.gp[4] == SelectionRun::sp - 4 && actor_fault.stores.size() == 1u &&
          actor_fault.abi.gp[2] == pool && actor_fault.memory.pc == 0x1c01afu, "Actor read lost the actual failure prefix");

    SelectionRun wrong(Selection::alternate);
    const auto c = SelectionRun::call;
    check(!wrong.run({c.instruction+1,c.target,false}) && !wrong.run({c.instruction,c.target+1,false}) &&
          !wrong.run({c.instruction,c.target,true}) && !wrong.run(c,SourcePhysicsContactImage::MapPack11),
          "Selector accepted the wrong call boundary");
    wrong.abi.gp[0] = 0;
    check(!wrong.run(), "Selector accepted the wrong indirect target");
    wrong.abi.gp[0] = c.target;
    wrong.abi.gp[3] = descriptor + 4u;
    check(!wrong.run() && wrong.memory.accesses.empty() && wrong.memory.sites.empty() && wrong.stores.empty(),
          "Selector changed state before declining unrelated descriptor");
}
}

int main() try {
    branches();
    faults_and_guards();
    selector_branches();
    selector_faults_and_guards();
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}

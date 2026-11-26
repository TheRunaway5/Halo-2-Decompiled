#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/native_tu15_fog_remaining.hpp"

using namespace frame_misc_harness;
namespace fog = halo2::native_fog_remaining_detail;
using fog::Entry;
using fog::Next;

static void dispatch(WorldState &, std::uint32_t, std::uint32_t, bool);
static void run_131470(WorldState &);
static void run_131610(WorldState &);
static void run_133b40(WorldState &);
static void run_1342b0(WorldState &);
#include "fog_remaining_body.inc"

namespace {
constexpr std::array entries{Entry::setup, Entry::environment, Entry::environment_patch, Entry::projection_call,
    Entry::sky_patch, Entry::sky_bloom, Entry::missing_sky, Entry::empty};
constexpr std::array<std::uint64_t,8> expected_coverage{15u,1023u,3u,1u,3u,31u,3u,1u};
constexpr auto unit = 0x3f800000u;
constexpr auto scenario = 0x510000u;
constexpr auto record = 0x520000u;
constexpr auto patch = 0x530000u;
constexpr auto datum = 0x540000u;

struct Access {
    std::uint32_t address;
    std::size_t width;
    bool write;
    bool operator==(const Access &) const = default;
};
struct FaultMemory : Memory {
    mutable std::vector<Access> accesses;
    unsigned fault_at{};
    FaultMemory(const Memory &source, unsigned fault) : fault_at(fault) { pages = source.clone().pages; }
    void access(std::uint32_t at, std::size_t size, bool write) const {
        accesses.push_back({at, size, write});
        if (accesses.size() == fault_at)
            throw std::runtime_error("Injected campaign fog backing fault");
    }
    void read(std::uint32_t at, std::span<std::byte> output) const override {
        access(at, output.size(), false);
        Memory::read(at, output);
    }
    void write(std::uint32_t at, std::span<const std::byte> input) override {
        access(at, input.size(), true);
        Memory::write(at, input);
    }
};
struct Child {
    std::uint32_t target, call_site;
    SourcePhysicsWorldRuntimeAbi entry;
};
struct Observation {
    SourcePhysicsWorldRuntimeAbi abi{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<Child> calls;
    std::uint32_t cursor{}, literals{};
    Next next{};
    std::string fault;
};

thread_local Observation *active{};
thread_local Entry scope{};
thread_local unsigned variant{};

bool identical(const SourcePhysicsWorldRuntimeAbi &a, const SourcePhysicsWorldRuntimeAbi &b) {
    return same_abi(a,b) && a.fs_base == b.fs_base && a.floating_point->mxcsr == b.floating_point->mxcsr;
}
// Exercise the adapter's callback boundary with deterministic child returns
// and faults. The separate linked owner fixture enters the original children.
void child(WorldState &s, std::uint32_t target, std::uint32_t site) {
    active->calls.push_back({target, site, s.save()});
    if (s.services.instruction)
        s.services.instruction(target);
    s.set(0, r(s.memory, s.gp[4], 4), 4, 0);
    s.flags.logic(s.gp[0], 4);
    s.set(1, s.gp[1] ^ 0x12345678u, 4, 0);
    s.xmm[2] = _mm_setzero_ps();
    s.fp.status = std::uint16_t(s.fp.status ^ 0x0100u);
    s.pop();
    switch (target) {
    case 0x32a660u:
        s.gp[4] -= 0x4820u;
        return;
    case 0x131470u:
        s.gp[0] = std::array{0u,1u,0xffffffffu,4u}[variant % 4u];
        s.gp[4] += 8u;
        return;
    case 0x131610u:
    case 0x133b40u:
        s.gp[4] += 8u;
        return;
    case 0x1342b0u:
        s.gp[4] += 16u;
        return;
    default:
        throw std::logic_error("Unknown fog fixture target");
    }
}

Next execute_stage(WorldState &s, Entry entry, bool native) {
#define FOG_STAGE(name) case Entry::name: \
    return native ? fog::bridge<Entry::name>(s, native_fog_remaining_##name##_reference_131ac0, child) \
                  : literal_fog_remaining_##name##_131ac0(s)
    switch (entry) {
    FOG_STAGE(setup);
    FOG_STAGE(environment);
    FOG_STAGE(environment_patch);
    FOG_STAGE(projection_call);
    FOG_STAGE(sky_patch);
    FOG_STAGE(sky_bloom);
    FOG_STAGE(missing_sky);
    FOG_STAGE(empty);
    }
#undef FOG_STAGE
    throw std::logic_error("Unknown fog fixture stage");
}

std::uint32_t last(Entry entry) {
    switch (entry) {
#define FOG_LAST(name) case Entry::name: return fog::last_pc<Entry::name>()
    FOG_LAST(setup);
    FOG_LAST(environment);
    FOG_LAST(environment_patch);
    FOG_LAST(projection_call);
    FOG_LAST(sky_patch);
    FOG_LAST(sky_bloom);
    FOG_LAST(missing_sky);
    FOG_LAST(empty);
#undef FOG_LAST
    }
    throw std::logic_error("Unknown fog fixture stage");
}

Observation run(FaultMemory &bank, const Case &c, Entry entry, unsigned row, bool native, unsigned store_fault) {
    Observation observation;
    SourcePhysicsContactRecordObserver observer = [&](const auto &event) {
        observation.stores.push_back(event);
        if (observation.stores.size() == store_fault)
            throw std::runtime_error("Injected campaign fog observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) {
        observation.cursor = pc;
        observation.literals += pc >= std::uint32_t(entry) && pc <= last(entry);
    };
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if (!census_mode) {
        auto &policy = services.instruction_cursor;
        policy.position = &observation.cursor;
        policy.callback_type = &services.instruction.target_type();
        policy.executor = std::this_thread::get_id();
        policy.service_sites = sites;
        policy.literal_service_sites = true;
        policy.observation_only_stores = true;
    }
    FpScope fp(*c.abi.floating_point);
    WorldState state{bank, observer, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(c.abi);
    active = &observation;
    scope = entry;
    variant = row;
    try {
        observation.next = execute_stage(state, entry, native);
    } catch (const std::exception &error) {
        observation.fault = error.what();
    }
    active = nullptr;
    observation.abi = state.save();
    return observation;
}

Case fixture(std::mt19937 &rng, Entry entry, unsigned row) {
    const auto modes = row % 10u;
    Case c;
    const auto stack = row % 17u == 0 ? 0x8ff002u : 0x900ff8u + row % 17u;
    const auto frame = 0x710ff8u + row % 17u;
    random_abi(rng, c, stack);
    c.abi.eflags = (c.abi.eflags & ~0x400u) | (row % 2u ? 0x400u : 0u);
    c.abi.fs_base = std::uint32_t(rng());
    c.abi.gp[0] = 0xffffffffu;
    c.abi.gp[1] = modes == 4u ? 0x105793ffu : 0x10579300u;
    c.abi.gp[2] = patch;
    c.abi.gp[5] = frame;
    c.abi.gp[6] = datum;
    c.abi.gp[7] = modes % 2u;
    c.abi.xmm[3][0] = unit;
    c.abi.xmm[4][0] = modes % 2 ? unit : 0u;
    c.abi.xmm[5][0] = modes % 3u ? unit : 0x3f000000u;
    c.abi.xmm[6][0] = modes % 2u ? unit : 0x3e800000u;
    c.abi.xmm[7][0] = 0u;
    for (auto at = stack - 0x6000u; at < stack + 0x6000u; at += 0x1000u)
        c.memory.put(at, std::uint32_t(rng()));
    for (auto offset : {0x13u,0x2cu,0x4cu,0x54u,0x58u,0x5cu,0x60u,0x64u,0x68u,0x6cu,0x88u,0xa8u,0xb0u,0xbcu,0xe8u})
        c.memory.put(stack + offset, unit);
    c.memory.put(stack + 0x4834u, 1u);
    c.memory.put(stack + 0x483cu, datum);
    c.memory.put(frame - 8u, modes % 2u);
    const auto signed_distance = modes == 5u ? 0x40400000u : modes == 6u ? 0xbf800000u : 0x3f000000u;
    c.memory.put(frame - 4u, signed_distance);
    c.memory.put(frame, 0u);
    c.memory.put(frame + 12u, modes == 0u || modes == 5u || modes == 6u ? 0u : 0xffffffffu);
    c.memory.put(0x4ecc20u, modes == 2u ? 0u : scenario);
    c.memory.put(scenario + 0x340u, modes == 3u ? 0u : 1u);
    c.memory.put(scenario + 0x344u, record);
    c.memory.put(stack + 0x160u, modes == 5u ? 0u : 0x40000000u);
    c.memory.put(stack + 0x240u, 0x8765u, 2);
    c.memory.put(record + 0x10u, 0x3f000000u);
    c.memory.put(record + 0xf0u, 0x4321u, 2);
    c.memory.put(0x46a0d0u, unit);
    c.memory.put(0x46a110u, unit);
    c.memory.put(0x556e10u, unit);
    for (unsigned offset = 0x18u; offset <= 0x24u; offset += 4u)
        c.memory.put(patch + offset, std::uint32_t(rng()));
    c.memory.put(datum + 0x68u, modes == 9u ? 0u : unit);
    c.memory.put(datum + 0x6cu, 0x3e800000u);
    c.memory.put(datum + 0x70u, 0x3f800000u);
    c.memory.put(datum + 0x74u, modes % 2u ? unit : 0u);
    c.memory.put(datum + 0xf0u, 0u);
    if (entry != Entry::setup && row >= 20u && row % 3u != 0u) {
        std::mt19937_64 edges{rng()};
        for (unsigned lane : {3u,4u,5u,6u,7u})
            c.abi.xmm[lane][0] = rng() % 2 ? special_float(edges) : std::uint32_t(rng());
        c.memory.put(frame - 4u, special_float(edges));
        c.memory.put(0x556e10u, special_float(edges));
        c.memory.put(datum + 0x6cu, special_float(edges));
        c.memory.put(datum + 0x70u, special_float(edges));
        c.memory.put(datum + 0x74u, special_float(edges));
        c.memory.put(stack + 0x68u, special_float(edges));
    }
    return c;
}

bool same_children(const std::vector<Child> &a, const std::vector<Child> &b) {
    if (a.size() != b.size())
        return false;
    for (std::size_t index = 0; index < a.size(); ++index)
        if (a[index].target != b[index].target || a[index].call_site != b[index].call_site ||
            !identical(a[index].entry, b[index].entry))
            return false;
    return true;
}

struct Counts { unsigned cases{}, faults{}, accesses{}, stores{}, children{}; };
std::pair<unsigned,unsigned> compare(Counts &count, const Case &c, Entry entry, unsigned row,
                                     unsigned access = 0, unsigned store = 0) {
    FaultMemory baseline(c.memory, access), actual(c.memory, access);
    const auto ref = run(baseline, c, entry, row, false, store);
    const auto got = run(actual, c, entry, row, true, store);
    if ((got.fault.empty() && got.next == Next::fallback) || ref.fault.empty() == bool(access || store) ||
        ref.next != got.next || ref.fault != got.fault || !identical(ref.abi, got.abi) ||
        !same_children(ref.calls, got.calls) || ref.stores != got.stores || baseline.accesses != actual.accesses ||
        !baseline.same(actual) || got.literals != 0 || ref.literals == 0 || (!census_mode && ref.cursor != got.cursor)) {
        std::fprintf(stderr,"entry %x row %u access %u/store %u: pc %x/%x next %x/%x MXCSR %x/%x error '%s'/'%s'\n",
            unsigned(entry),row,access,store,got.cursor,ref.cursor,unsigned(got.next),unsigned(ref.next),
            got.abi.floating_point->mxcsr,ref.abi.floating_point->mxcsr,got.fault.c_str(),ref.fault.c_str());
        throw std::runtime_error("Native campaign fog stage differs");
    }
    ++count.cases;
    count.faults += bool(access || store);
    count.accesses += unsigned(baseline.accesses.size());
    count.stores += unsigned(ref.stores.size());
    count.children += unsigned(ref.calls.size());
    return {unsigned(baseline.accesses.size()),unsigned(ref.stores.size())};
}
} // namespace

static void dispatch(WorldState &s, std::uint32_t target, std::uint32_t site, bool tail) {
    if (tail || target != 0x32a660u)
        throw std::logic_error("Unexpected fog stack-probe route");
    child(s, target, site);
}
static void run_131470(WorldState &s) { child(s, 0x131470u, 0x131af8u); }
static void run_131610(WorldState &s) { child(s, 0x131610u, 0x131b11u); }
static void run_133b40(WorldState &s) { child(s, 0x133b40u, 0x132183u); }
static void run_1342b0(WorldState &s) { child(s, 0x1342b0u, scope == Entry::sky_patch ? 0x1324afu : 0x131f9du); }

int main(int argc, char **argv) try {
    const std::string mode = argc > 1 ? argv[1] : "literal";
    const auto rows = argc > 2 ? unsigned(std::stoul(argv[2])) : 4000u;
    set_mode(mode,"HALO2_VERIFY_FOG_REMAINING");
    std::mt19937 rng{0xc0131ac0u};
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_FOG_REMAINING","1",1);
        for (auto entry : entries) {
            const auto c = fixture(rng,entry,0);
            FaultMemory bank(c.memory,0);
            const auto result = run(bank,c,entry,0,true,0);
            if (result.next != Next::fallback || !bank.accesses.empty() || !result.calls.empty() ||
                !result.fault.empty() || result.cursor || !identical(result.abi,c.abi))
                throw std::runtime_error("Forced reference touched a fog stage");
        }
        std::puts("{\"forced_reference_refusals\":8}");
        return 0;
    }
    Counts count;
    for (auto entry : entries) {
        if (!census_mode)
            check_refusals(fixture(rng,entry,0),[&](WorldState &s) { return execute_stage(s,entry,true) != Next::fallback; });
        for (unsigned row = 0; row < rows; ++row) {
            const auto c = fixture(rng,entry,row);
            const auto [accesses,stores] = compare(count,c,entry,row);
            if (row >= 10u)
                continue;
            for (unsigned access = 1; access <= accesses; ++access)
                compare(count,c,entry,row,access);
            for (unsigned store = 1; store <= stores; ++store)
                compare(count,c,entry,row,0,store);
        }
    }
    const std::array coverage{
        fog::coverage<Entry::setup>().load(),fog::coverage<Entry::environment>().load(),
        fog::coverage<Entry::environment_patch>().load(),fog::coverage<Entry::projection_call>().load(),
        fog::coverage<Entry::sky_patch>().load(),fog::coverage<Entry::sky_bloom>().load(),
        fog::coverage<Entry::missing_sky>().load(),fog::coverage<Entry::empty>().load(),
    };
    if (!std::ranges::equal(coverage, expected_coverage)) {
        for (const auto mask : coverage)
            std::fprintf(stderr," %llx",static_cast<unsigned long long>(mask));
        std::fprintf(stderr,"\n");
        throw std::runtime_error("Incomplete fog branch coverage");
    }
    const auto replayed = fog::summary<Entry::setup>().children.load() + fog::summary<Entry::environment_patch>().children.load() +
                          fog::summary<Entry::projection_call>().children.load() + fog::summary<Entry::sky_patch>().children.load();
    if (mode == "verify" && replayed != 6u * rows)
        throw std::runtime_error("Incomplete fog child replay coverage");
    std::printf("{\"mode\":\"%s\",\"cases\":%u,\"faults\":%u,\"ordered_accesses\":%u,\"ordered_stores\":%u,"
        "\"compared_child_calls\":%u,\"verified_child_replays\":%llu}\n",mode.c_str(),count.cases,count.faults,
        count.accesses,count.stores,count.children,static_cast<unsigned long long>(replayed));
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr,"%s\n",error.what());
    return 1;
}

#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_damage_descriptor_refresh.hpp"

using namespace frame_misc_harness;
namespace refresh = halo2::native_damage_descriptor_refresh_detail;

namespace original {
struct ObservedDamageState : WorldState {
    using WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t damage_observation_pc(std::uint32_t disc, std::uint32_t tu15, const ObservedDamageState &s) {
    return s.selected_observation_pcs && s.image == SourcePhysicsContactImage::TitleUpdate15 ? tu15 : disc;
}
#include "damage_descriptor_refresh_children.inc"
static void run_1e83a0(ObservedDamageState &s);
static void run_d7cd0(ObservedDamageState &s);
static void run_d7e00(ObservedDamageState &s);
#include "damage_descriptor_refresh_reference.inc"
} // namespace original

namespace {
using original::ObservedDamageState;
constexpr auto frame_globals = 0x5020c8u;
constexpr auto object_globals = 0x4ecbd0u;
constexpr auto tag_globals = 0x4f0414u;
constexpr auto header = 0x610000u;
constexpr auto descriptors = 0x620000u;
constexpr auto tag_table = 0x630000u;
constexpr auto tag = 0x640000u;
constexpr auto object_table = 0x650000u;
constexpr auto object = 0x660000u;
constexpr auto defaults = 0x670000u;
constexpr std::array targets{0x1eccc0u, 0xd8c20u, 0xd8d50u};

struct Access {
    std::uint32_t address;
    std::size_t width;
    bool write;
    bool operator==(const Access &) const = default;
};
struct FaultMemory : Memory {
    mutable std::vector<Access> accesses;
    unsigned fail_at{};
    FaultMemory(const Memory &source, unsigned fault) : fail_at(fault) { pages = source.clone().pages; }
    void access(std::uint32_t address, std::size_t width, bool write) const {
        accesses.push_back({address, width, write});
        if (accesses.size() == fail_at)
            throw std::runtime_error("Injected descriptor backing fault");
    }
    void read(std::uint32_t address, std::span<std::byte> out) const override {
        access(address, out.size(), false);
        Memory::read(address, out);
    }
    void write(std::uint32_t address, std::span<const std::byte> bytes) override {
        access(address, bytes.size(), true);
        Memory::write(address, bytes);
    }
};
struct Child {
    std::uint32_t target;
    SourcePhysicsWorldRuntimeAbi entry;
};
struct Observation {
    SourcePhysicsWorldRuntimeAbi abi{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<Child> calls;
    std::uint32_t cursor{}, literals{};
    bool admitted{};
    std::string fault;
};
thread_local Observation *active{};

bool identical(const SourcePhysicsWorldRuntimeAbi &a, const SourcePhysicsWorldRuntimeAbi &b) {
    return same_abi(a,b) && a.fs_base == b.fs_base && a.floating_point->mxcsr == b.floating_point->mxcsr;
}
bool identical(const std::vector<Child> &a, const std::vector<Child> &b) {
    if (a.size() != b.size())
        return false;
    for (std::size_t index = 0; index < a.size(); ++index)
        if (a[index].target != b[index].target || !identical(a[index].entry,b[index].entry))
            return false;
    return true;
}
bool bridge(ObservedDamageState &s) {
    return refresh::bridge(s, original::native_damage_descriptor_refresh_reference_d73e0,
                           original::run_1e83a0, original::run_d7cd0, original::run_d7e00);
}
Observation execute(FaultMemory &memory, const Case &c, bool native, unsigned observer_fault = 0,
                    bool selected = true, SourcePhysicsContactImage image = SourcePhysicsContactImage::TitleUpdate15) {
    Observation observed;
    SourcePhysicsContactRecordObserver observer = [&](const auto &event) {
        observed.stores.push_back(event);
        if (observed.stores.size() == observer_fault)
            throw std::runtime_error("Injected descriptor observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) {
        observed.cursor = pc;
        observed.literals += pc >= refresh::first && pc <= refresh::last;
    };
    if (!census_mode) {
        auto &cursor = services.instruction_cursor;
        cursor.position = &observed.cursor;
        cursor.callback_type = &services.instruction.target_type();
        cursor.executor = std::this_thread::get_id();
        cursor.service_sites = halo2::native_census_detail::service_sites;
        cursor.literal_service_sites = true;
        cursor.observation_only_stores = true;
    }
    FpScope fp(*c.abi.floating_point);
    ObservedDamageState state{memory, observer, services, image};
    state.load(c.abi);
    state.selected_observation_pcs = selected;
    active = &observed;
    try {
        if (native)
            observed.admitted = bridge(state);
        else
            original::native_damage_descriptor_refresh_reference_d73e0_live(state);
    } catch (const std::exception &error) {
        observed.fault = error.what();
    }
    active = nullptr;
    observed.abi = state.save();
    return observed;
}

Case fixture(std::mt19937 &random, unsigned row) {
    Case c;
    const auto sp = row % 11u == 9u ? 0x2u : 0x701ff0u + row % 19u;
    const auto bp = 0x720ff0u + row % 19u;
    random_abi(random,c,sp);
    c.abi.gp[5] = bp;
    c.abi.fs_base = std::uint32_t(random());
    const auto descriptor_case = row % 5u;
    const auto object_case = row / 10u % 3u;
    const bool pending = row / 5u % 2u != 0;
    const auto category = descriptor_case == 0u ? 2u : row / 30u % 2u;
    const auto slot = row % 7u == 6u ? 0xabcfffffu : 0xabc01234u;
    const auto object_id = object_case == 0u ? 0xffffffffu : 0x12340007u;
    for (auto offset = 0; offset <= 0xc0u; offset += 4u) {
        c.memory.put(sp - offset, std::uint32_t(random()));
        c.memory.put(sp + offset, std::uint32_t(random()));
    }
    c.memory.put(sp + 0x84u, object_id);
    c.memory.put(bp + 0xaau, category + (row % 3u == 1u ? 32u : 0u), 1);
    c.memory.put(bp + 0x10au, (std::uint32_t(random()) & 0xdfffu) | (pending ? 0x2000u : 0u), 2);
    c.memory.put(bp + 0x12cu, descriptor_case == 1u ? 0xffffffffu : slot);
    c.memory.put(frame_globals, header);
    c.memory.put(header + 0x44u, descriptors);
    const auto id = descriptor_case == 2u ? 0xffffffffu : 0xfedc0003u;
    c.memory.put(descriptors + (slot & 0xffffu) * 0x888u + 0x54u, id);
    c.memory.put(tag_globals, tag_table);
    c.memory.put(tag_table + 3u * 16u + 8u, tag);
    c.memory.put(tag_table + 4u * 16u + 8u, tag + 0x1000u);
    c.memory.put(tag + 0x8u, descriptor_case == 4u ? 4u : 0xffffffffu);
    c.memory.put(tag + 0x3cu, descriptor_case == 3u ? 1u : 0u);
    c.memory.put(tag + 0x40u, std::uint32_t(random()));
    c.memory.put(tag + 0x103cu, 1u);
    c.memory.put(tag + 0x1040u, std::uint32_t(random()));
    c.memory.put(object_globals, header + 0x1000u);
    c.memory.put(header + 0x1044u, object_table);
    c.memory.put(object_table + (object_id & 0xffffu) * 12u + 8u, object);
    c.memory.put(object + 0xaau, object_case == 1u ? 2u : 0u, 1);
    c.memory.put(object + 0xc2u, std::uint32_t(random()), 2);
    c.memory.put(object + 0xc4u, std::uint32_t(random()));
    c.memory.put(object + 0xc8u, std::uint32_t(random()));
    c.memory.put(object + 0x248u, 0xffffffffu);
    c.memory.put(object + 0x12cu, 5u);
    c.memory.put(object + 0x138u, std::uint32_t(random()), 2);
    c.memory.put(object + 0x13cu, std::uint32_t(random()));
    // The genuine D8D50 already-refreshed exit restores its frame without
    // requiring unrelated downstream world owners.
    c.memory.put(object + 0x10au, 4u, 1);
    c.memory.put(0x473980u, defaults);
    for (auto offset = 0; offset <= 8; offset += 4)
        c.memory.put(defaults + offset, std::uint32_t(random()));
    return c;
}

struct Counts {unsigned comparisons{}, faults{}, calls{}, stores{}, accesses{};};
std::pair<unsigned,unsigned> compare(const Case &c, unsigned row, Counts &count, unsigned access = 0, unsigned store = 0) {
    FaultMemory expected(c.memory,access), actual(c.memory,access);
    const auto ref = execute(expected,c,false,store);
    const auto got = execute(actual,c,true,store);
    if (ref.fault.empty() == bool(access || store) || ref.fault != got.fault ||
        (got.fault.empty() && !got.admitted) || !identical(ref.abi,got.abi) || !identical(ref.calls,got.calls) ||
        ref.stores != got.stores || expected.accesses != actual.accesses || !expected.same(actual) ||
        got.literals || !ref.literals || (!census_mode && got.cursor != ref.cursor)) {
        std::fprintf(stderr,"descriptor row %u access %u/store %u cursor %x/%x child count %zu/%zu fault '%s'/'%s'\n",
            row,access,store,got.cursor,ref.cursor,got.calls.size(),ref.calls.size(),got.fault.c_str(),ref.fault.c_str());
        throw std::runtime_error("Selected damage descriptor refresh differs");
    }
    if (!access && !store) {
        const bool lookup = row % 5u >= 2u, pending = row / 5u % 2u != 0;
        std::vector<std::uint32_t> seen, required;
        for (const auto &call : got.calls)
            seen.push_back(call.target);
        if (lookup)
            required.push_back(targets[0]);
        if (pending)
            required.insert(required.end(),targets.begin()+1,targets.end());
        if (seen != required)
            throw std::runtime_error("Missing original descriptor child");
    }
    ++count.comparisons;
    count.faults += bool(access || store);
    count.calls += unsigned(ref.calls.size());
    count.stores += unsigned(ref.stores.size());
    count.accesses += unsigned(expected.accesses.size());
    return {unsigned(expected.accesses.size()),unsigned(ref.stores.size())};
}
} // namespace

namespace original {
static void run_1e83a0(ObservedDamageState &s) { active->calls.push_back({targets[0],s.save()}); fixture_1e83a0(s); }
static void run_d7cd0(ObservedDamageState &s) { active->calls.push_back({targets[1],s.save()}); fixture_d7cd0(s); }
static void run_d7e00(ObservedDamageState &s) { active->calls.push_back({targets[2],s.save()}); fixture_d7e00(s); }
} // namespace original

int main(int argc, char **argv) try {
    const auto cases = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000u;
    const std::string mode = argc > 2 ? argv[2] : "literal";
    set_mode(mode,"HALO2_VERIFY_DAMAGE_DESCRIPTOR_REFRESH");
    std::mt19937 random{0xd84f4u};
    const auto initial = fixture(random,8u);
    const auto refuse = [&](bool selected, SourcePhysicsContactImage image) {
        FaultMemory memory(initial.memory,0);
        const auto got = execute(memory,initial,true,0,selected,image);
        if (got.admitted || !got.fault.empty() || got.cursor || !got.calls.empty() || !memory.accesses.empty() ||
            !identical(got.abi,initial.abi))
            throw std::runtime_error("Descriptor refresh touched an unsupported route");
    };
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_DAMAGE_DESCRIPTOR_REFRESH","1",1);
        refuse(true,SourcePhysicsContactImage::TitleUpdate15);
        std::puts("{\"forced_reference_refusals\":1}");
        return 0;
    }
    refuse(false,SourcePhysicsContactImage::TitleUpdate15);
    refuse(true,SourcePhysicsContactImage::MapPack11);
    Counts count;
    unsigned success_children{};
    for (unsigned row = 0; row < cases; ++row) {
        const auto c = fixture(random,row);
        const auto [accesses,stores] = compare(c,row,count);
        success_children += unsigned(row % 5u >= 2u) + 2u * (row / 5u % 2u);
        if (row >= 30)
            continue;
        for (unsigned step = 1; step <= accesses; ++step)
            compare(c,row,count,step);
        for (unsigned step = 1; step <= stores; ++step)
            compare(c,row,count,0,step);
    }
    const auto covered = refresh::coverage().load();
    const auto replays = refresh::summary().children.load();
    if (covered != 31u || (mode == "verify" && replays != success_children))
        throw std::runtime_error("Incomplete descriptor branches or child replay");
    std::printf("{\"mode\":\"%s\",\"comparisons\":%u,\"faults\":%u,\"accesses\":%u,\"stores\":%u,"
        "\"genuine_child_calls\":%u,\"verified_replays\":%llu,\"coverage\":%llu}\n",mode.c_str(),
        count.comparisons,count.faults,count.accesses,count.stores,count.calls,
        static_cast<unsigned long long>(replays),static_cast<unsigned long long>(covered));
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr,"%s\n",error.what());
    return 1;
}

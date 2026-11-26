#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/source_difficulty_parameter_native_tu15_internal.hpp"
#include "gameplay/units/native_tu15_damage_response_secondary.hpp"
#include "halo2/native_weapon_effect_dispatch_tu15.hpp"

using namespace frame_misc_harness;
namespace secondary = halo2::native_damage_response_secondary_detail;
using secondary::Entry;

namespace original {
struct ObservedDamageState : WorldState {
    using WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t damage_observation_pc(std::uint32_t disc, std::uint32_t tu15, const ObservedDamageState &s) {
    return s.selected_observation_pcs && s.image == SourcePhysicsContactImage::TitleUpdate15 ? tu15 : disc;
}
static void run_1ed0b0(ObservedDamageState &);
static void run_d7cd0(ObservedDamageState &);
static void run_1781f0(ObservedDamageState &);
static void run_a87e0(ObservedDamageState &);
static void dispatch(ObservedDamageState &, std::uint32_t, std::uint32_t, bool);
#include "damage_response_secondary_children.inc"
#include "damage_response_secondary_reference.inc"
} // namespace original

namespace {
using original::ObservedDamageState;
constexpr std::array entries{Entry::load,Entry::scaled};
constexpr std::uint32_t definition = 0x610000u, parameters = 0x620000u, globals = 0x630000u,
    table = 0x640000u, objects = 0x650000u, object = 0x660000u;
constexpr std::uint32_t one = 0x3f800000u, half = 0x3f000000u, two = 0x40000000u;

struct Access {
    std::uint32_t address;
    std::size_t width;
    bool write;
    bool operator==(const Access &) const = default;
};
struct FaultMemory : Memory {
    mutable std::vector<Access> accessed;
    unsigned fail_at{};
    FaultMemory(const Memory &source, unsigned failure) : fail_at(failure) { pages = source.clone().pages; }
    void access(std::uint32_t address, std::size_t width, bool write) const {
        accessed.push_back({address,width,write});
        if (accessed.size() == fail_at)
            throw std::runtime_error("Injected secondary response backing fault");
    }
    void read(std::uint32_t address, std::span<std::byte> bytes) const override {
        access(address,bytes.size(),false);
        Memory::read(address,bytes);
    }
    void write(std::uint32_t address, std::span<const std::byte> bytes) override {
        access(address,bytes.size(),true);
        Memory::write(address,bytes);
    }
};
struct Child {
    std::uint32_t target, site;
    SourcePhysicsWorldRuntimeAbi abi;
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
    return a.size()==b.size() && std::ranges::equal(a,b,[](const Child &left,const Child &right) {
        return left.target==right.target && left.site==right.site && identical(left.abi,right.abi);
    });
}
bool bridge(ObservedDamageState &s, Entry entry) {
    return secondary::bridge(s,entry,original::native_damage_response_secondary_reference_d73e0,
        original::run_1ed0b0,original::run_d7cd0,original::run_1781f0,original::run_a87e0,
        original::dispatch,original::damage_mulss,original::damage_addss);
}
unsigned scenario(unsigned row) { return row / entries.size() % 12u; }
bool supplies_required(unsigned mode) { return mode != 3u && mode != 6u; }

Observation execute(FaultMemory &memory, const Case &c, bool native, Entry entry, unsigned scenario,
                    unsigned observer_fault = 0, bool selected = true,
                    SourcePhysicsContactImage image = SourcePhysicsContactImage::TitleUpdate15) {
    Observation out;
    SourcePhysicsContactRecordObserver observer = [&](const auto &event) {
        out.stores.push_back(event);
        if (out.stores.size() == observer_fault)
            throw std::runtime_error("Injected secondary response observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) {
        out.cursor = pc;
        out.literals += pc >= std::uint32_t(entry) && pc <= secondary::last;
    };
    if (!census_mode) {
        auto &cursor = services.instruction_cursor;
        cursor.position = &out.cursor;
        cursor.callback_type = &services.instruction.target_type();
        cursor.executor = std::this_thread::get_id();
        cursor.service_sites = halo2::native_census_detail::service_sites;
        cursor.literal_service_sites = true;
        cursor.observation_only_stores = true;
    }
    if (supplies_required(scenario)) {
        services.required_owner = [&](auto &bank, const auto &call, auto &abi, const auto &) {
            if (&bank != &memory || call.tail ||
                !((call.target == 0xbc820u && call.instruction == 0xd78f1u) ||
                  (call.target == 0x179610u && call.instruction == 0xd8889u)))
                throw std::runtime_error("Wrong secondary response required route");
            out.calls.push_back({call.target,call.instruction,abi});
            services.instruction(call.target);
            const auto expected = call.target == 0xbc820u ? 0xd8846u : 0xd888eu;
            if (r(bank,abi.gp[4],4) != expected)
                throw std::runtime_error("Wrong secondary response required return word");
            abi.gp[4] += 20u;
            abi.gp[0] = 0xbabc1234u;
            abi.gp[1] ^= 0x5591ab77u;
            abi.xmm[3][2] ^= 0x80808080u;
        };
    }
    FpScope fp(*c.abi.floating_point);
    ObservedDamageState state{memory,observer,services,image};
    state.load(c.abi);
    state.selected_observation_pcs = selected;
    active = &out;
    try {
        if (native)
            out.admitted = bridge(state,entry);
        else
            original::native_damage_response_secondary_reference_d73e0_live(state,entry);
    } catch (const std::exception &error) {
        out.fault = error.what();
    }
    active = nullptr;
    out.abi = state.save();
    return out;
}

Case fixture(std::mt19937 &random, unsigned row) {
    Case c;
    const auto mode = scenario(row);
    const auto sp = row % 17u == 9u ? 2u : 0x711ff0u + row % 19u;
    const auto bp = 0x720ff0u + row % 19u;
    random_abi(random,c,sp);
    c.abi.gp[5] = bp;
    c.abi.gp[7] = 0x9ab00007u;
    c.abi.fs_base = std::uint32_t(random());
    for (unsigned offset = 0; offset <= 0x130u; offset += 4u) {
        c.memory.put(sp + offset,std::uint32_t(random()));
        c.memory.put(sp - offset,std::uint32_t(random()));
    }
    auto threshold = two;
    auto source = mode == 0 ? half : two;
    auto previous = half;
    auto factor = one;
    auto difficulty = one;
    if (row >= 24u && mode >= 7u) {
        std::mt19937_64 edges{random()};
        threshold = special_float(edges);
        source = special_float(edges);
        previous = special_float(edges);
        factor = special_float(edges);
        difficulty = special_float(edges);
    }
    c.abi.xmm[0][0] = source;
    c.abi.xmm[2][0] = threshold;
    c.memory.put(sp + 0x14u,std::uint32_t(random()));
    c.memory.put(sp + 0x18u,definition);
    c.memory.put(sp + 0x1cu,threshold);
    c.memory.put(sp + 0x84u,0x9ab00007u);
    c.memory.put(definition + 0xb8u,std::uint32_t(random()));
    c.memory.put(definition + 0xd0u,source);
    c.memory.put(bp + 0xf0u,previous);
    c.memory.put(bp + 0x10au,((mode <= 2u || mode == 6u) ? 0x200u : 0u) |
         ((mode == 2u || mode == 5u || mode == 6u) ? 8u : 0u),2);
    c.memory.put(bp + 0xaau,mode % 2u ? 1u : 2u,1);
    c.memory.put(bp + 0x138u,std::uint32_t(random()),2);
    c.memory.put(0x51df54u,parameters);
    c.memory.put(parameters + 4u,factor);
    c.memory.put(0x4f3218u,globals);
    c.memory.put(globals + 8u,2u);
    c.memory.put(0x4ecc1cu,0u);
    c.memory.put(0x46a0d0u,difficulty);
    c.memory.put(0x4ecbd0u,table);
    c.memory.put(table + 0x44u,objects);
    c.memory.put(objects + 12u*7u + 8u,object);
    c.memory.put(object + 0xaau,2u,1);
    c.memory.put(object + 0xc2u,std::uint32_t(random()),2);
    c.memory.put(object + 0xc4u,std::uint32_t(random()));
    c.memory.put(object + 0xc8u,std::uint32_t(random()));
    c.memory.put(object + 0xd4u,0xffffffffu);
    return c;
}

struct Counts {unsigned cases{}, faults{}, stores{}, accesses{}, required{}, direct{};};
std::pair<unsigned,unsigned> compare(const Case &c, unsigned row, Entry entry, Counts &count,
                                     unsigned access = 0, unsigned store = 0) {
    FaultMemory ref_bank(c.memory,access), got_bank(c.memory,access);
    const auto mode = scenario(row);
    const auto ref = execute(ref_bank,c,false,entry,mode,store);
    const auto got = execute(got_bank,c,true,entry,mode,store);
    const bool fault = access || store || !supplies_required(mode);
    if (ref.fault.empty() == fault || ref.fault != got.fault || (!fault && !got.admitted) ||
        !identical(ref.abi,got.abi) || !identical(ref.calls,got.calls) || ref.stores != got.stores ||
        ref_bank.accessed != got_bank.accessed || !ref_bank.same(got_bank) || got.literals || !ref.literals ||
        (!census_mode && ref.cursor != got.cursor)) {
        std::fprintf(stderr,"secondary row %u entry %x access %u/store %u cursor %x/%x children %zu/%zu '%s'/'%s'\n",
            row,unsigned(entry),access,store,got.cursor,ref.cursor,got.calls.size(),ref.calls.size(),
            got.fault.c_str(),ref.fault.c_str());
        throw std::runtime_error("Selected secondary response differs");
    }
    if (!access && !store && mode == 3u &&
        (ref.cursor != 0x1796a9u || ref.fault != "Native weapon effect requires its actual callee owner"))
        throw std::runtime_error("Direct 179610 did not reach its authentic missing-service fault");
    for (const auto &child : got.calls) {
        count.direct += child.target == 0x179610u && !child.site;
        count.required += child.target == 0x179610u && child.site == 0xd8889u;
    }
    ++count.cases;
    count.faults += fault;
    count.stores += unsigned(ref.stores.size());
    count.accesses += unsigned(ref_bank.accessed.size());
    return {unsigned(ref_bank.accessed.size()),unsigned(ref.stores.size())};
}
} // namespace

namespace original {
void run_1ed0b0(ObservedDamageState &s) { active->calls.push_back({0x1f19d0u,0u,s.save()}); fixture_1ed0b0(s); }
void run_d7cd0(ObservedDamageState &s) { active->calls.push_back({0xd8c20u,0u,s.save()}); fixture_d7cd0(s); }
void run_1781f0(ObservedDamageState &s) { active->calls.push_back({0x179610u,0u,s.save()}); fixture_1781f0(s); }
void run_a87e0(ObservedDamageState &s) { active->calls.push_back({0xb1c30u,0u,s.save()}); fixture_a87e0(s); }
void dispatch(ObservedDamageState &s, std::uint32_t target, std::uint32_t site, bool tail) {
    if (target != 0xbc820u)
        throw std::logic_error("Unqualified secondary response dispatch reached");
    s.required(site,target,tail);
}
} // namespace original

int main(int argc, char **argv) try {
    const auto rows = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000u;
    const std::string mode = argc > 2 ? argv[2] : "literal";
    set_mode(mode,"HALO2_VERIFY_DAMAGE_RESPONSE_SECONDARY");
    std::mt19937 random{0xd87ceu};
    const auto initial = fixture(random,1u);
    const auto refuse = [&](bool selected, SourcePhysicsContactImage image) {
        for (auto entry : entries) {
            FaultMemory bank(initial.memory,0);
            const auto got = execute(bank,initial,true,entry,0,0,selected,image);
            if (got.admitted || !got.fault.empty() || got.cursor || !got.calls.empty() ||
                !bank.accessed.empty() || !identical(got.abi,initial.abi))
                throw std::runtime_error("Secondary response touched an unsupported route");
        }
    };
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_DAMAGE_RESPONSE_SECONDARY","1",1);
        refuse(true,SourcePhysicsContactImage::TitleUpdate15);
        std::puts("{\"forced_reference_refusals\":2}");
        return 0;
    }
    refuse(false,SourcePhysicsContactImage::TitleUpdate15);
    refuse(true,SourcePhysicsContactImage::MapPack11);
    Counts count;
    for (unsigned row = 0; row < rows; ++row) {
        const auto c = fixture(random,row);
        const auto entry = entries[row % entries.size()];
        const auto [accesses,stores] = compare(c,row,entry,count);
        if (row >= 24u)
            continue;
        for (unsigned step = 1; step <= accesses; ++step)
            compare(c,row,entry,count,step);
        for (unsigned step = 1; step <= stores; ++step)
            compare(c,row,entry,count,0,step);
    }
    const auto covered = secondary::coverage().load();
    const auto replayed = secondary::summary().children.load();
    // The pinned direct route necessarily faults at its first required
    // grandchild. It is verified above and never enters success coverage.
    if (covered != 0xdfu || !count.required || !count.direct || (mode == "verify" && !replayed)) {
        std::fprintf(stderr,"coverage %llx, direct %u, required %u, replays %llu\n",
            static_cast<unsigned long long>(covered),count.direct,count.required,static_cast<unsigned long long>(replayed));
        throw std::runtime_error("Incomplete secondary-response branch or child coverage");
    }
    std::printf("{\"mode\":\"%s\",\"comparisons\":%u,\"faults\":%u,\"accesses\":%u,\"stores\":%u,"
        "\"direct_effect_calls\":%u,\"required_effect_calls\":%u,\"verified_replays\":%llu,\"coverage\":%llu}\n",
        mode.c_str(),count.cases,count.faults,count.accesses,count.stores,count.direct,count.required,
        static_cast<unsigned long long>(replayed),static_cast<unsigned long long>(covered));
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr,"%s\n",error.what());
    return 1;
}

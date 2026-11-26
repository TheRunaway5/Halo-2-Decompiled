#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_collision_cache_309060.hpp"
#include "collision_cache_accounting.inc"

using namespace frame_misc_harness;
namespace cache = halo2::native_collision_cache_309060_detail;

namespace original {
#ifdef HALO2_COLLISION_CACHE_CONTACT_FIXTURE
using State = halo2::physics_contact_runtime_detail::State;
using ObservedWorldState = State;
#else
struct ObservedWorldState : WorldState {
    using WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t world_observation_pc(std::uint32_t disc, std::uint32_t tu15, const ObservedWorldState &s) {
    return s.selected_observation_pcs && s.image == SourcePhysicsContactImage::TitleUpdate15 ? tu15 : disc;
}
#endif
static void dispatch(ObservedWorldState &, std::uint32_t, std::uint32_t, bool);
static void run_2e0ba0(ObservedWorldState &);
static void run_2e0cc0(ObservedWorldState &);
#include "collision_cache_owner.inc"
#include "collision_cache_children.inc"
} // namespace original

namespace {
using original::ObservedWorldState;
#ifdef HALO2_COLLISION_CACHE_CONTACT_FIXTURE
constexpr bool contact_api = true;
#else
constexpr bool contact_api = false;
#endif
bool map_pack{}, quiet{}, unqualified{}, tracing{}, service_observer{}, arbitrary_observer{}, foreign_executor{}, forced{};
constexpr auto service_pc = 0x30557au;
bool selected_world(bool selected) { return !contact_api && !map_pack && selected; }
bool canonical_binding() { return contact_api || map_pack; }
std::uint32_t resume_pc(std::uint32_t tu15) { return map_pack ? tu15 - 0x3b00u : tu15; }
std::uint32_t transform_target() { return map_pack ? 0x2e0ba0u : 0x2e4690u; }
std::uint32_t direction_target() { return map_pack ? 0x2e0cc0u : 0x2e47b0u; }
const char *reference_variable() {
    if (contact_api) return "HALO2_REFERENCE_CONTACT_CACHE_305560";
    if (map_pack) return "HALO2_REFERENCE_COLLISION_CACHE_MAPPACK11";
    return "HALO2_REFERENCE_COLLISION_CACHE_309060";
}
const char *verify_variable() {
    if (contact_api) return "HALO2_VERIFY_CONTACT_CACHE_305560";
    if (map_pack) return "HALO2_VERIFY_COLLISION_CACHE_MAPPACK11";
    return "HALO2_VERIFY_COLLISION_CACHE_309060";
}
template<class S> void set_observations(S &s,bool selected) {
    if constexpr (requires {s.selected_observation_pcs;})
        s.selected_observation_pcs = selected;
}
constexpr auto output = 0x610000u, shape_a = 0x620000u, shape_b = 0x630000u, relative = 0x640000u;
constexpr auto vtable_a = 0x650000u, vtable_b = 0x660000u;
constexpr std::array targets{0x889100u,0x889200u,0x2e4690u,0x889300u,0x2e47b0u,0x889400u,0x889500u,0x889600u};
constexpr std::array call_sites{0x30557au,0x305589u,0x309099u,0x3055beu,0x3090e8u,0x30560fu,0x305634u,0x30563eu};
constexpr std::array resumes{0x30907du,0x30908cu,0x30909eu,0x3090c1u,0x3090edu,0x309112u,0x309137u,0x309141u};

struct Access {
    std::uint32_t address;
    std::size_t width;
    bool write;
    bool operator==(const Access &) const = default;
};
struct FaultMemory : Memory {
    mutable std::vector<Access> accesses;
    const unsigned fail_at;
    FaultMemory(const Memory &source, unsigned fault) : fail_at(fault) { pages = source.clone().pages; }
    void access(std::uint32_t address, std::size_t width, bool write) const {
        accesses.push_back({address,width,write});
        if (accesses.size() == fail_at)
            throw std::runtime_error("Injected collision cache backing fault");
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
bool identical(const SourcePhysicsWorldRuntimeAbi &a, const SourcePhysicsWorldRuntimeAbi &b) {
    return same_abi(a,b) && a.fs_base == b.fs_base && a.floating_point->mxcsr == b.floating_point->mxcsr;
}
struct Child {
    std::uint32_t target, site;
    SourcePhysicsWorldRuntimeAbi abi;
    bool operator==(const Child &other) const {
        return target == other.target && site == other.site && identical(abi,other.abi);
    }
};
struct Observation {
    SourcePhysicsWorldRuntimeAbi abi{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<Child> calls;
    std::uint32_t cursor{}, literals{}, service_calls{};
    unsigned fault_child{};
    std::string fault;
};
thread_local Observation *active{};
thread_local bool active_selected{};

bool record_call(std::uint32_t target, std::uint32_t site, const SourcePhysicsWorldRuntimeAbi &abi) {
    const auto index = active->calls.size();
    if (index >= targets.size())
        throw std::logic_error("Extra collision cache child");
    auto expected_target = targets.at(index);
    auto expected_site = call_sites.at(index);
    if (index == 2u || index == 4u) {
        expected_target = index == 2u ? transform_target() : direction_target();
        if (!active_selected)
            expected_site -= 0x3b00u;
    }
    if (target != expected_target || site != expected_site)
        throw std::logic_error("Wrong real collision cache child route");
    active->calls.push_back({target,site,abi});
    return active->calls.size() == active->fault_child;
}

Case fixture(std::mt19937 &random, unsigned row) {
    Case c;
    const auto sp = row % 8u == 7u ? 0x30u + row % 17u : 0x70ff00u + row % 17u;
    random_abi(random,c,sp);
    c.abi.gp[1] = output;
    c.abi.fs_base = random();
    for (auto offset = 0u; offset < 0x150u; offset += 4u)
        c.memory.put(sp - offset,random());
    c.memory.put(sp,0x12345678u);
    c.memory.put(sp + 4u,shape_a);
    c.memory.put(sp + 8u,shape_b);
    c.memory.put(sp + 12u,relative);
    c.memory.put(shape_a,vtable_a);
    c.memory.put(shape_b,vtable_b);
    c.memory.put(vtable_a + 0x38u,targets[0]);
    c.memory.put(vtable_b + 0x38u,targets[1]);
    c.memory.put(vtable_a + 0x30u,targets[3]);
    c.memory.put(vtable_b + 0x30u,targets[5]);
    c.memory.put(vtable_a + 0x3cu,targets[6]);
    c.memory.put(vtable_b + 0x3cu,targets[7]);
    for (auto offset = 0u; offset < 16u; offset += 4u) {
        c.memory.put(output + offset,random());
        c.memory.put((map_pack ? 0x418ae0u : 0x461960u) + offset,0x80000000u);
    }
    constexpr std::array words{
        0x3f000000u,0x3f800000u,0x00000000u,0x80000000u,0x7fc00007u,0xff800021u,0x007fffffu,0xff800000u
    };
    for (auto offset = 0u; offset < 0x40u; offset += 4u)
        c.memory.put(relative + offset,words.at(random() % words.size()));
    return c;
}

void required(FaultMemory &memory, unsigned row, const SourcePhysicsContactRuntimeCall &call,
              SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsContactRecordObserver &observe) {
    const auto index = active->calls.size();
    if (call.tail || record_call(call.target,call.instruction,abi)) {
        abi.gp[0] = 0xbadabc0du;
        throw std::runtime_error("Injected collision cache child fault");
    }
    const auto sp = abi.gp[4];
    if (r(memory,sp) != resume_pc(resumes.at(index)))
        throw std::logic_error("Wrong collision cache child return address");
    auto args = 0u;
    if (index <= 1u) {
        const auto destination = r(memory,sp + 4u);
        args = 4u;
        for (auto offset = 0u; offset < 16u; offset += 4u)
            w(memory,observe,call.instruction,destination + offset,4,
              index == 0u ? 0x3fa00000u : 0x3f000000u);
    } else if (index == 3u || index == 5u) {
        const auto direction = r(memory,sp + 4u);
        const auto destination = r(memory,sp + 8u);
        args = 8u;
        for (auto offset = 0u; offset < 16u; offset += 4u)
            w(memory,observe,call.instruction,destination + offset,4,r(memory,direction + offset));
        w(memory,observe,call.instruction,destination + 12u,2,0x2300u + row + index);
    }
    abi.gp[0] = 0xbaad0000u | (row + index);
    abi.gp[4] += args + 4u;
    abi.eflags ^= 0x41u;
}

Observation execute(FaultMemory &memory, const Case &c, unsigned row, bool raw, bool selected,
                    unsigned store_fault, unsigned child_fault) {
    Observation result;
    result.fault_child = child_fault;
    const SourcePhysicsContactRecordObserver observe = [&](const auto &event) {
        result.stores.push_back(event);
        if (result.stores.size() == store_fault)
            throw std::runtime_error("Injected collision cache observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    if (!quiet)
    services.instruction = [&](std::uint32_t pc) {
        result.cursor = pc;
        result.service_calls += service_observer && pc == service_pc;
        const auto start = selected_world(selected) ? cache::first : cache::first - cache::observation_offset;
        result.literals += pc >= start && pc <= start + cache::last - cache::first;
    };
    if (!census_mode && !forced && !quiet && !unqualified) {
        auto &cursor = services.instruction_cursor;
        cursor.position = &result.cursor;
        cursor.callback_type = &services.instruction.target_type();
        cursor.executor = foreign_executor ? std::thread::id{} : std::this_thread::get_id();
        static constexpr std::array legacy_sites{0x337a45u};
        static constexpr std::array fault_sites{service_pc,0x337a45u};
        if (service_observer)
            cursor.service_sites = fault_sites;
        else if (canonical_binding())
            cursor.service_sites = legacy_sites;
        else {
            cursor.service_sites = halo2::native_census_detail::service_sites;
            cursor.literal_service_sites = true;
        }
        cursor.first_trace = &tracing;
        cursor.observation_only_stores = !arbitrary_observer;
    }
    services.required_owner = [&](auto &, const auto &call, auto &abi, const auto &observer) {
        required(memory,row,call,abi,observer);
    };
    FpScope fp(*c.abi.floating_point);
    const auto image = map_pack ? SourcePhysicsContactImage::MapPack11 : SourcePhysicsContactImage::TitleUpdate15;
    ObservedWorldState s{memory,observe,services,image};
    s.load(c.abi);
    set_observations(s,selected);
    active = &result;
    active_selected = selected_world(selected);
    try {
        if (raw)
#ifdef HALO2_COLLISION_CACHE_CONTACT_FIXTURE
            original::native_contact_collision_cache_reference_305560_live(s);
#else
            original::native_collision_cache_reference_305560_live(s);
#endif
        else
            original::run_305560(s);
    } catch (const std::exception &error) {
        result.fault = error.what();
    }
    active = nullptr;
    result.abi = s.save();
    return result;
}
} // namespace

namespace original {
static void dispatch(ObservedWorldState &s, std::uint32_t target, std::uint32_t site, bool tail) {
    s.required(site,target,tail);
}
static void run_2e0ba0(ObservedWorldState &s) {
    if (record_call(transform_target(),active_selected ? 0x309099u : 0x305599u,s.save())) {
        s.gp[0] = 0xbadabc0du;
        throw std::runtime_error("Injected collision cache child fault");
    }
    fixture_2e0ba0(s);
}
static void run_2e0cc0(ObservedWorldState &s) {
    if (record_call(direction_target(),active_selected ? 0x3090e8u : 0x3055e8u,s.save())) {
        s.gp[0] = 0xbadabc0du;
        throw std::runtime_error("Injected collision cache child fault");
    }
    fixture_2e0cc0(s);
}
} // namespace original

namespace {
struct CacheTotals {unsigned comparisons{}, faults{}, accesses{}, stores{}, children{};};
std::pair<unsigned,unsigned> compare(const Case &c, unsigned row, const std::string &mode, CacheTotals &counts,
                                     unsigned access = 0u, unsigned store = 0u, unsigned child = 0u, bool selected = true) {
    FaultMemory left(c.memory,access), right(c.memory,access);
    const auto ref = execute(left,c,row,true,selected,store,child);
    const auto actual = execute(right,c,row,false,selected,store,child);
    const bool injected = access || store || child;
    const bool translated = (!canonical_binding() && !selected) || forced || unqualified || tracing ||
                            service_observer || arbitrary_observer || foreign_executor ||
                            (census_mode && canonical_binding());
    const bool callback_differs = !quiet && !service_observer && !arbitrary_observer &&
                                  bool(actual.literals) != translated;
    if (ref.fault.empty() == injected || ref.fault != actual.fault || !identical(ref.abi,actual.abi) ||
        ref.calls != actual.calls || ref.stores != actual.stores || left.accesses != right.accesses || !left.same(right) ||
        (!census_mode && !quiet && ref.cursor != actual.cursor) || callback_differs ||
        (!quiet && !ref.literals) || ref.service_calls != actual.service_calls) {
        std::fprintf(stderr,"collision cache row %u mode %s access %u store %u child %u literals %u/%u "
                    "pc %x/%x fault '%s'/'%s'\n",row,mode.c_str(),access,store,child,actual.literals,ref.literals,
                    actual.cursor,ref.cursor,actual.fault.c_str(),ref.fault.c_str());
        for (auto reg = 0u; reg < 8u; ++reg)
            if (actual.abi.gp[reg] != ref.abi.gp[reg])
                std::fprintf(stderr,"GP %u native %x reference %x\n",reg,actual.abi.gp[reg],ref.abi.gp[reg]);
        throw std::runtime_error("World cache initializer differs from pinned owner");
    }
    if (!injected && (actual.calls.size() != targets.size() || actual.abi.gp[4] != c.abi.gp[4] + 16u ||
            right.get(output,2) != std::uint16_t(0x2303u + row) ||
            right.get(output + 2u,2) != std::uint16_t(0x2305u + row) ||
            right.get(output + 4u) != 0xffffffffu || right.get(output + 8u,2) != 0x0101u ||
            right.get(output + 10u,1) != std::uint8_t(row + 6u) ||
            right.get(output + 11u,1) != std::uint8_t(row + 7u)))
        throw std::runtime_error("Wrong collision cache result, return frame, or child count");
    ++counts.comparisons;
    counts.faults += injected;
    counts.accesses += unsigned(left.accesses.size());
    counts.stores += unsigned(ref.stores.size());
    counts.children += unsigned(ref.calls.size());
    return {unsigned(left.accesses.size()),unsigned(ref.stores.size())};
}
} // namespace

int main(int argc, char **argv) {
    if (argc != 3 && argc != 4)
        return 2;
    try {
        const auto count = std::stoul(argv[1]);
        const std::string mode = argv[2];
        map_pack = argc == 4 && std::string(argv[3]) == "map-pack11";
        quiet = mode == "quiet";
        unqualified = mode == "unqualified";
        tracing = mode == "trace";
        service_observer = mode == "service";
        arbitrary_observer = mode == "observer";
        foreign_executor = mode == "foreign";
        forced = mode == "reference";
        set_mode(mode,verify_variable());
        if (forced)
            setenv(reference_variable(),"1",1);
        std::mt19937 random(0x309060u);
        CacheTotals totals, canonical;
        for (auto row = 0u; row < count; ++row)
            compare(fixture(random,row),row,mode,totals);
        const auto c = fixture(random,7u);
        const auto [accesses,stores] = compare(c,7u,mode,totals);
        for (auto cut = 1u; cut <= accesses; ++cut)
            compare(c,7u,mode,totals,cut);
        for (auto cut = 1u; cut <= stores; ++cut)
            compare(c,7u,mode,totals,0u,cut);
        for (auto child = 1u; child <= targets.size(); ++child)
            compare(c,7u,mode,totals,0u,0u,child);
        if (!canonical_binding())
            compare(c,7u,mode,canonical,0u,0u,0u,false);
        const auto coverage = contact_api ? cache::contact_summary().hit.load() :
            map_pack ? cache::legacy_world_summary().hit.load() : cache::coverage().load();
        const bool translated = forced || unqualified || tracing || service_observer ||
                                arbitrary_observer || foreign_executor || (census_mode && canonical_binding());
        if (coverage != (translated ? 0u : 1u))
            throw std::runtime_error("Missing collision cache native return");
        std::printf("{\"comparisons\":%u,\"faults\":%u,\"accesses\":%u,\"stores\":%u,"
                    "\"children\":%u,\"canonical_fallbacks\":%u,\"coverage\":%llu}\n",
                    totals.comparisons,totals.faults,totals.accesses,totals.stores,totals.children,
                    canonical.comparisons,static_cast<unsigned long long>(coverage));
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr,"%s\n",error.what());
        return 1;
    }
}

#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_allocator_owner_dispatch.hpp"
#include "physics/runtime/native_tu15_collision_pair_2f88d0.hpp"
#include "collision_pair_accounting.inc"

using namespace frame_misc_harness;
namespace pair = halo2::native_collision_pair_2f88d0_detail;

namespace original {
static void dispatch(WorldState &, std::uint32_t, std::uint32_t, bool);
static void run_309150(WorldState &);
#include "collision_pair_owner.inc"
#include "collision_pair_child.inc"
#include "collision_pair_router.inc"
} // namespace original

namespace {
constexpr auto a = 0x610000u, b = 0x620000u, threshold = 0x630000u, recipient = 0x640000u;
constexpr auto shape_a = 0x650000u, shape_b = 0x660000u, motor_a = 0x670000u, motor_b = 0x680000u;
constexpr auto a_vtable = 0x690000u, b_vtable = 0x6a0000u, out_vtable = 0x6b0000u, ring = 0x6c0000u;
constexpr auto cursor_global = 0x48c974u, limit_global = 0x48c978u;
constexpr auto type = 0x880000u, output = 0x880010u;
constexpr auto support = 0x880020u, feature = 0x880030u, vertices = 0x880040u;
constexpr std::array clock_words{0x5123456789abcdefull,0x8fedcbaa76543210ull};

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
            throw std::runtime_error("Injected collision pair backing fault");
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
    SourcePhysicsWorldRuntimeAbi entry;
    bool operator==(const Child &other) const {
        return site == other.site && target == other.target && identical(entry,other.entry);
    }
};
struct Observation {
    SourcePhysicsWorldRuntimeAbi abi{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<Child> calls;
    std::vector<std::uint64_t> clocks;
    std::uint32_t cursor{}, literals{};
    unsigned fault_child{};
    std::string fault;
};
thread_local Observation *active{};

struct Policy {
    unsigned route{}, distance{}, profiling{};
    unsigned fault_store{}, fault_child{}, fault_clock{};
    bool missing_clock{}, missing_child{};
    unsigned x87_depth{};
};

Case fixture(std::mt19937 &random, unsigned row, const Policy &policy) {
    Case c;
    const auto sp = row % 64u == 63u ? 0x110u : (0x70ff00u + row % 59u);
    const auto frame = ((sp - 4u) & ~15u) - 0x260u;
    random_abi(random,c,sp);
    c.abi.fs_base = random();
    for (auto offset = 0u; offset < 0x400u; offset += 4u)
        c.memory.put(sp - offset,random());
    c.memory.put(sp,0x12345678u);
    c.memory.put(sp + 4u,a);
    c.memory.put(sp + 8u,b);
    c.memory.put(sp + 12u,threshold);
    c.memory.put(sp + 16u,recipient);
    c.memory.put(a,shape_a);
    c.memory.put(a + 8u,motor_a);
    c.memory.put(b,shape_b);
    c.memory.put(b + 8u,motor_b);
    c.memory.put(shape_a,a_vtable);
    c.memory.put(shape_b,b_vtable);
    c.memory.put(shape_a + 12u,0x3e800000u);
    c.memory.put(shape_b + 12u,0x3e800000u);
    c.memory.put(b_vtable + 0x14u,type);
    c.memory.put(a_vtable + 0x38u,support);
    c.memory.put(a_vtable + 0x30u,feature);
    c.memory.put(a_vtable + 0x3cu,vertices);
    c.memory.put(0x461960u,0x80000000u);
    c.memory.put(0x461964u,0x80000000u);
    c.memory.put(0x461968u,0x80000000u);
    c.memory.put(0x46196cu,0x80000000u);
    c.memory.put(recipient,out_vtable);
    c.memory.put(out_vtable + 4u,output);
    constexpr std::array limits{0x40000000u,0x3f800000u,0x3fc00000u,0xff800001u};
    c.memory.put(threshold + 8u,limits.at(policy.distance));
    c.memory.put(cursor_global,policy.profiling == 3u ? ring + 24u : ring);
    c.memory.put(limit_global,ring + policy.profiling * 12u - (policy.profiling == 3u ? 12u : 0u));
    for (auto offset = 0u; offset < 32u; offset += 4u)
        c.memory.put(ring + offset,random());
    constexpr std::array float_words{
        0x3e800000u,0xc0c00000u,0u,0x80000000u,0x7f800001u,0xffc00014u,0x807fffffu,0x7f800000u
    };
    for (auto offset = 0u; offset < 0x40u; offset += 4u) {
        c.memory.put(motor_a + 0x20u + offset,0x3f000000u);
        c.memory.put(motor_b + 0x20u + offset,float_words.at(random() % float_words.size()));
    }
    for (auto offset = 0x20u; offset < 0xc0u; offset += 4u)
        c.memory.put(frame + offset,float_words.at(random() % float_words.size()));
    c.memory.put(frame + 0x80u,0x40000000u);
    if (policy.x87_depth) {
        c.abi.x87_depth = policy.x87_depth;
        for (unsigned lane = 0; lane < policy.x87_depth; ++lane)
            c.abi.x87[lane] = (long double)lane;
    }
    return c;
}

void required(FaultMemory &memory, const Policy &p, Observation &observed,
              std::uint32_t site, std::uint32_t target, bool tail, SourcePhysicsWorldRuntimeAbi &abi,
              const SourcePhysicsContactRecordObserver &observe) {
    observed.calls.push_back({target,site,abi});
    if (tail)
        throw std::logic_error("Expected a collision pair call");
    if (observed.calls.size() == p.fault_child) {
        abi.gp[0] = 0xabad1deau;
        throw std::runtime_error("Injected collision pair child fault");
    }
    unsigned args{};
    std::uint32_t resume{};
    switch (target) {
    case 0x2e4550u: args = 8; resume = 0x2f892eu; break;
    case type: abi.gp[0] = p.route == 0u ? 5u : 6u; resume = 0x2f8935u; break;
    case 0x309060u: args = 12; resume = 0x2f8964u; break;
    case 0x30e930u:
        args = 20;
        resume = 0x2f8994u;
        abi.gp[0] = p.route == 2u;
        if (p.profiling == 3u)
            w(memory,observe,site,cursor_global,4,ring);
        break;
    case 0x30d350u: args = 12; resume = 0x2f89bau; break;
    case 0x2e4760u: args = 8; resume = 0x2f89d2u; break;
    case output: args = 4; resume = 0x2f8b2au; break;
    case support:
        args = 4;
        resume = 0x30921du;
        for (auto i = 0u; i < 16u; i += 4u)
            w(memory,observe,site,r(memory,abi.gp[4]+4u) + i,4,0x3f000000u);
        break;
    case feature:
        args = 8;
        resume = 0x30926du;
        for (auto i = 0u; i < 16u; i += 4u)
            w(memory,observe,site,r(memory,abi.gp[4]+8u) + i,4,0u);
        w(memory,observe,site,r(memory,abi.gp[4]+4u),2,0x1234u);
        break;
    case vertices: resume = 0x309284u; abi.gp[0] = 3u; break;
    default: throw std::logic_error("Unexpected collision pair child");
    }
    if (r(memory,abi.gp[4]) != resume)
        throw std::logic_error("Unexpected collision pair return PC");
    abi.gp[4] += 4u + args;
}

Observation execute(FaultMemory &memory, const Case &c, const Policy &p, bool raw) {
    Observation result;
    const SourcePhysicsContactRecordObserver observer = [&](const auto &event) {
        result.stores.push_back(event);
        if (result.stores.size() == p.fault_store)
            throw std::runtime_error("Injected collision pair observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) {
        result.cursor = pc;
        result.literals += pc >= pair::first && pc <= pair::last;
    };
    if (!census_mode && std::getenv("HALO2_REFERENCE_COLLISION_PAIR_2F88D0") == nullptr) {
        auto &cursor = services.instruction_cursor;
        cursor.position = &result.cursor;
        cursor.callback_type = &services.instruction.target_type();
        cursor.executor = std::this_thread::get_id();
        cursor.service_sites = halo2::native_census_detail::service_sites;
        cursor.literal_service_sites = true;
        cursor.observation_only_stores = true;
    }
    if (!p.missing_child)
        services.required_owner = [&](auto &, const auto &call, auto &abi, const auto &observe) {
            required(memory,p,result,call.instruction,call.target,call.tail,abi,observe);
        };
    if (!p.missing_clock)
        services.timestamp_counter = [&] {
            const auto index = result.clocks.size();
            if (index >= clock_words.size())
                throw std::logic_error("Collision pair called real clock during replay");
            const auto value = clock_words.at(index);
            result.clocks.push_back(value);
            if (result.clocks.size() == p.fault_clock)
                throw std::runtime_error("Injected collision pair clock fault");
            return value;
        };
    FpScope fp(*c.abi.floating_point);
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(c.abi);
    result.fault_child = p.fault_child;
    active = &result;
    try {
        if (raw)
            original::native_collision_pair_reference_2f88d0_live(state);
        else
            original::run_2f88d0(state);
    } catch (const std::exception &error) {
        result.fault = error.what();
    }
    active = nullptr;
    result.abi = state.save();
    return result;
}
} // namespace

namespace original {
static void run_309150(WorldState &s) {
    active->calls.push_back({0x309150u,0x2f8949u,s.save()});
    if (active->calls.size() == active->fault_child) {
        s.gp[0] = 0xabad1deau;
        throw std::runtime_error("Injected collision pair child fault");
    }
    fixture_309150(s);
}
} // namespace original

namespace {
struct PairTotals {
    unsigned comparisons{}, faults{}, accesses{}, stores{}, children{}, clocks{};
};
std::pair<unsigned,unsigned> compare(const Case &c, const Policy &p, unsigned row, const std::string &mode,
                                     PairTotals &counts, unsigned access = 0u) {
    FaultMemory left(c.memory,access), right(c.memory,access);
    const auto ref = execute(left,c,p,true);
    const auto actual = execute(right,c,p,false);
    const auto reference = mode == "reference";
    const bool injected = access || p.fault_store || p.fault_child || p.fault_clock ||
                          p.missing_clock || p.missing_child || p.x87_depth;
    if (ref.fault != actual.fault || ref.fault.empty() == injected ||
        !identical(ref.abi,actual.abi) || ref.calls != actual.calls || ref.clocks != actual.clocks ||
        ref.stores != actual.stores || left.accesses != right.accesses || !left.same(right) ||
        (!census_mode && ref.cursor != actual.cursor) ||
        (reference ? !actual.literals : actual.literals != 0u) || !ref.literals) {
        std::fprintf(stderr,
            "collision pair row %u mode %s access %u store %u child %u clock %u pc %x/%x visits %u/%u fault '%s'/'%s'\n",
            row,mode.c_str(),access,p.fault_store,p.fault_child,p.fault_clock,actual.cursor,ref.cursor,
            actual.literals,ref.literals,actual.fault.c_str(),ref.fault.c_str());
        for (auto reg = 0u; reg < 8u; ++reg)
            if (actual.abi.gp[reg] != ref.abi.gp[reg])
                std::fprintf(stderr,"  GP %u %x/%x\n",reg,actual.abi.gp[reg],ref.abi.gp[reg]);
        std::fprintf(stderr,"  access %zu/%zu, stores %zu/%zu, calls %zu/%zu, clocks %zu/%zu, flags %x/%x\n",
            right.accesses.size(),left.accesses.size(),actual.stores.size(),ref.stores.size(),
            actual.calls.size(),ref.calls.size(),actual.clocks.size(),ref.clocks.size(),actual.abi.eflags,ref.abi.eflags);
        throw std::runtime_error("Collision pair differs from pinned owner");
    }
    if (!injected) {
        std::vector<std::uint32_t> routes;
        for (const auto &call : actual.calls)
            routes.push_back(call.target);
        std::vector<std::uint32_t> expected{0x2e4550u,type};
        if (p.route == 0u)
            expected.insert(expected.end(),{0x309150u,support,feature,vertices});
        else
            expected.push_back(0x309060u);
        expected.push_back(0x30e930u);
        if (p.route != 2u)
            expected.insert(expected.end(),{0x30d350u,0x2e4760u});
        if (p.route != 2u && p.distance == 0u)
            expected.push_back(output);
        constexpr std::array expected_clocks{0u,1u,2u,1u};
        if (routes != expected || actual.clocks.size() != expected_clocks.at(p.profiling) ||
            actual.abi.gp[4] != c.abi.gp[4] + 4u)
            throw std::runtime_error("Wrong 2F88D0 branch, genuine child route, or timestamp count");
    }
    ++counts.comparisons;
    counts.faults += injected;
    counts.accesses += unsigned(left.accesses.size());
    counts.stores += unsigned(ref.stores.size());
    counts.children += unsigned(ref.calls.size());
    counts.clocks += unsigned(ref.clocks.size());
    return {unsigned(left.accesses.size()),unsigned(ref.stores.size())};
}
} // namespace

int main(int argc, char **argv) {
    if (argc != 3)
        return 2;
    try {
        const auto cases = std::stoul(argv[1]);
        const std::string mode = argv[2];
        set_mode(mode,"HALO2_VERIFY_COLLISION_PAIR_2F88D0");
        if (mode == "reference")
            setenv("HALO2_REFERENCE_COLLISION_PAIR_2F88D0","1",1);
        std::mt19937 random(0x2f88d0u);
        PairTotals counts;
        if (mode == "literal") {
            auto refusal_random = random;
            check_refusals(fixture(refusal_random,0u,{}),[](WorldState &s) {
                return pair::bridge(s,original::native_collision_pair_reference_2f88d0,
                                    original::dispatch,original::run_309150);
            });
        }
        for (auto row = 0u; row < cases; ++row) {
            const Policy policy{row % 3u,row / 3u % 4u,row / 12u % 4u};
            const auto c = fixture(random,row,policy);
            compare(c,policy,row,mode,counts);
        }
        for (auto row = 0u; row < 3u; ++row) {
            Policy p{row,row == 1u ? 1u : 0u,2u};
            const auto c = fixture(random,row,p);
            const auto [accesses,stores] = compare(c,p,row,mode,counts);
            for (auto step = 1u; step <= accesses; ++step)
                compare(c,p,row,mode,counts,step);
            for (auto step = 1u; step <= stores; ++step) {
                p.fault_store = step;
                compare(c,p,row,mode,counts);
            }
        }
        for (auto child = 1u; child <= 10u; ++child) {
            const Policy p{.profiling=2u,.fault_child=child};
            const auto c = fixture(random,0u,p);
            compare(c,p,0u,mode,counts);
        }
        for (auto clock = 1u; clock <= 2u; ++clock) {
            const Policy p{.profiling=2u,.fault_clock=clock};
            const auto c = fixture(random,0u,p);
            compare(c,p,0u,mode,counts);
        }
        const Policy exceptions[]{
            {.profiling=2u,.missing_clock=true}, {.profiling=2u,.missing_child=true},
            {.profiling=2u,.x87_depth=7u}, {.profiling=2u,.x87_depth=8u}
        };
        for (const auto &p : exceptions)
            compare(fixture(random,0u,p),p,0u,mode,counts);
        const auto coverage = pair::coverage().load();
        if (coverage != (mode == "reference" ? 0u : 0xffu))
            throw std::runtime_error("Missing collision pair branch coverage");
        std::printf("{\"comparisons\":%u,\"faults\":%u,\"accesses\":%u,\"stores\":%u,\"children\":%u,"
                    "\"clocks\":%u,\"coverage\":%llu}\n",counts.comparisons,counts.faults,counts.accesses,counts.stores,
                    counts.children,counts.clocks,static_cast<unsigned long long>(coverage));
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr,"%s\n",error.what());
        return 1;
    }
}

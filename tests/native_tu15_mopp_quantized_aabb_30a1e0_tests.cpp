#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_mopp_aabb_traversal.hpp"
#include "physics/runtime/native_tu15_mopp_quantized_aabb_30a1e0.hpp"
#include "mopp_quantized_aabb_accounting.inc"

using namespace frame_misc_harness;
namespace setup = halo2::native_mopp_quantized_aabb_30a1e0_detail;

namespace original {
struct ObservedWorldState : WorldState {
    using WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t world_observation_pc(std::uint32_t disc, std::uint32_t tu15, const ObservedWorldState &s) {
    return s.selected_observation_pcs && s.image == SourcePhysicsContactImage::TitleUpdate15 ? tu15 : disc;
}
static void dispatch(ObservedWorldState &, std::uint32_t, std::uint32_t, bool) {
    throw std::logic_error("MOPP quantizer fixture reached an unqualified traversal route");
}
static void run_2df780(ObservedWorldState &) {
    throw std::logic_error("MOPP quantizer fixture expected available key capacity");
}
static void run_305e40(ObservedWorldState &);
#include "mopp_quantized_aabb_owner.inc"
#include "mopp_quantized_aabb_child.inc"
} // namespace original

namespace {
using original::ObservedWorldState;
constexpr auto cache = 0x610000u, code = 0x620000u, bounds = 0x630000u;
constexpr auto keys = 0x640000u, key_buffer = 0x650000u;

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
    void accessed(std::uint32_t address, std::size_t width, bool write) const {
        accesses.push_back({address,width,write});
        if (accesses.size() == fail_at)
            throw std::runtime_error("Injected MOPP setup backing fault");
    }
    void read(std::uint32_t address, std::span<std::byte> bytes) const override {
        accessed(address,bytes.size(),false);
        Memory::read(address,bytes);
    }
    void write(std::uint32_t address, std::span<const std::byte> bytes) override {
        accessed(address,bytes.size(),true);
        Memory::write(address,bytes);
    }
};
bool identical(const SourcePhysicsWorldRuntimeAbi &a, const SourcePhysicsWorldRuntimeAbi &b) {
    return same_abi(a,b) && a.fs_base == b.fs_base && a.floating_point->mxcsr == b.floating_point->mxcsr;
}
struct Observation {
    SourcePhysicsWorldRuntimeAbi abi{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<SourcePhysicsWorldRuntimeAbi> traversals;
    std::uint32_t cursor{}, literals{};
    bool fail_child{};
    std::string fault;
};
thread_local Observation *active{};

Case fixture(std::mt19937 &random, unsigned row, unsigned depth = 0u) {
    Case c;
    const auto sp = row % 13u == 12u ? 0x30u + row % 15u : 0x70ff00u + row % 19u;
    const auto frame = ((sp - 4u) & ~15u) - 0x50u;
    const auto alias = row % 11u;
    random_abi(random,c,sp);
    c.abi.gp[1] = alias == 1u ? bounds : alias == 2u ? code : alias == 3u ? frame + 0x10u : cache;
    c.abi.fs_base = random();
    if (depth) {
        c.abi.x87_depth = depth;
        for (auto slot = 0u; slot < depth; ++slot)
            c.abi.x87[slot] = (long double)(slot + 1u) / 3.0L;
    }
    for (auto offset = 0u; offset < 0x200u; offset += 4u)
        c.memory.put(sp - offset,random());
    c.memory.put(sp,0x12345678u);
    c.memory.put(sp + 4u,code);
    c.memory.put(sp + 8u,bounds);
    c.memory.put(sp + 12u,keys);
    constexpr std::array inputs{
        0u,0x80000000u,0x3e800000u,0xbfbfffffu,0x4effffffu,0xcf800000u,
        0x7f800000u,0xff800000u,0x7fc0003au,0xff800001u,0x00000001u,0x807fffffu,0x00800000u
    };
    for (auto offset = 0u; offset < 0x50u; offset += 4u) {
        c.memory.put(cache + offset,random());
        c.memory.put(bounds + offset,inputs.at(random() % inputs.size()));
        c.memory.put(code + offset,inputs.at(random() % inputs.size()));
    }
    if (row % 4u == 0u) {
        for (auto offset = 0u; offset < 16u; offset += 4u)
            c.memory.put(code + offset,offset == 12u ? 0x47800000u : 0x3f000000u);
        for (auto offset = 0u; offset < 32u; offset += 4u)
            c.memory.put(bounds + offset,offset < 16u ? 0xbf000000u : 0x3f000000u + (offset << 15u));
    }
    const auto opcode = alias > 3u && row % 2u ? 0x30u : 0u;
    c.memory.put(code + 0x30u,opcode);
    c.memory.put(keys,key_buffer);
    c.memory.put(keys + 4u,0u);
    c.memory.put(keys + 8u,4u);
    for (auto offset = 0u; offset < 16u; offset += 4u)
        c.memory.put(key_buffer + offset,random());
    c.memory.put(0x309fb0u,0u,1u);
    c.memory.put(0x309fb0u + 0x30u,23u,1u);
    c.memory.put(0x309f30u,0x309e97u);
    c.memory.put(0x309f30u + 4u*23u,0x309eefu);
    return c;
}

Observation execute(FaultMemory &memory, const Case &c, bool raw, bool selected, unsigned store_fault, bool child_fault) {
    Observation result;
    result.fail_child = child_fault;
    const SourcePhysicsContactRecordObserver observe = [&](const auto &event) {
        result.stores.push_back(event);
        if (result.stores.size() == store_fault)
            throw std::runtime_error("Injected MOPP setup observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) {
        result.cursor = pc;
        const auto base = selected ? setup::first : setup::first - 0x3b00u;
        result.literals += pc >= base && pc <= base + setup::last - setup::first;
    };
    if (!census_mode && std::getenv("HALO2_REFERENCE_MOPP_QUANTIZED_AABB_30A1E0") == nullptr) {
        auto &cursor = services.instruction_cursor;
        cursor.position = &result.cursor;
        cursor.callback_type = &services.instruction.target_type();
        cursor.executor = std::this_thread::get_id();
        cursor.service_sites = halo2::native_census_detail::service_sites;
        cursor.literal_service_sites = true;
        cursor.observation_only_stores = true;
    }
    FpScope fp(*c.abi.floating_point);
    ObservedWorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    s.load(c.abi);
    s.selected_observation_pcs = selected;
    active = &result;
    try {
        if (raw)
            original::native_mopp_quantized_aabb_reference_3066e0_live(s);
        else
            original::run_3066e0(s);
    } catch (const std::exception &error) {
        result.fault = error.what();
    }
    active = nullptr;
    result.abi = s.save();
    return result;
}
} // namespace

namespace original {
static void run_305e40(ObservedWorldState &s) {
    active->traversals.push_back(s.save());
    auto &bank = static_cast<FaultMemory &>(s.memory);
    if (bank.get(s.gp[4]) != 0x30a30cu || bank.get(s.gp[4] + 8u) != code + 0x30u)
        throw std::logic_error("Wrong MOPP traversal return or code argument");
    if (active->fail_child) {
        s.gp[0] = 0xbadabc0du;
        throw std::runtime_error("Injected MOPP traversal fault");
    }
    fixture_305e40(s);
}
} // namespace original

namespace {
struct SetupTotals {unsigned comparisons{}, faults{}, accesses{}, stores{}, children{}, leaves{};};
std::pair<unsigned,unsigned> compare(const Case &c, unsigned row, const std::string &mode, SetupTotals &counts,
                                     unsigned access = 0u, unsigned store = 0u, bool child = false,
                                     bool stack_overflow = false, bool selected = true) {
    FaultMemory left(c.memory,access), right(c.memory,access);
    const auto ref = execute(left,c,true,selected,store,child);
    const auto actual = execute(right,c,false,selected,store,child);
    const bool injected = access || store || child || stack_overflow;
    const bool translated = !selected || mode == "reference";
    if (ref.fault.empty() == injected || ref.fault != actual.fault || !identical(ref.abi,actual.abi) ||
        !std::ranges::equal(ref.traversals,actual.traversals,identical) ||
        ref.stores != actual.stores || left.accesses != right.accesses || !left.same(right) ||
        (!census_mode && ref.cursor != actual.cursor) || bool(actual.literals) != translated || !ref.literals) {
        std::fprintf(stderr,"MOPP setup row %u mode %s access %u store %u child %u literals %u/%u "
                    "pc %x/%x fault '%s'/'%s'\n",row,mode.c_str(),access,store,child,actual.literals,ref.literals,
                    actual.cursor,ref.cursor,actual.fault.c_str(),ref.fault.c_str());
        for (auto reg = 0u; reg < 8u; ++reg)
            if (actual.abi.gp[reg] != ref.abi.gp[reg])
                std::fprintf(stderr,"GP %u native %x reference %x\n",reg,actual.abi.gp[reg],ref.abi.gp[reg]);
        throw std::runtime_error("World MOPP quantizer differs from pinned owner");
    }
    if (!injected) {
        if (actual.traversals.size() != 1u || actual.abi.gp[4] != c.abi.gp[4] + 16u)
            throw std::runtime_error("Wrong MOPP quantizer child count or return frame");
        const auto count = right.get(keys + 4u);
        if (count > 1u)
            throw std::runtime_error("Unexpected MOPP traversal leaf count");
        counts.leaves += count;
    }
    ++counts.comparisons;
    counts.faults += injected;
    counts.accesses += unsigned(left.accesses.size());
    counts.stores += unsigned(ref.stores.size());
    counts.children += unsigned(ref.traversals.size());
    return {unsigned(left.accesses.size()),unsigned(ref.stores.size())};
}
} // namespace

int main(int argc, char **argv) {
    if (argc != 3)
        return 2;
    try {
        const auto count = std::stoul(argv[1]);
        const std::string mode = argv[2];
        set_mode(mode,"HALO2_VERIFY_MOPP_QUANTIZED_AABB_30A1E0");
        if (mode == "reference")
            setenv("HALO2_REFERENCE_MOPP_QUANTIZED_AABB_30A1E0","1",1);
        std::mt19937 random(0x30a1e0u);
        SetupTotals totals, canonical;
        for (auto row = 0u; row < count; ++row)
            compare(fixture(random,row),row,mode,totals);
        for (const auto row : {5u,12u}) {
            const auto c = fixture(random,row);
            const auto [accesses,stores] = compare(c,row,mode,totals);
            for (auto cut = 1u; cut <= accesses; ++cut)
                compare(c,row,mode,totals,cut);
            for (auto cut = 1u; cut <= stores; ++cut)
                compare(c,row,mode,totals,0u,cut);
            compare(c,row,mode,totals,0u,0u,true);
        }
        compare(fixture(random,5u,7u),5u,mode,totals);
        compare(fixture(random,5u,8u),5u,mode,totals,0u,0u,false,true);
        compare(fixture(random,5u),5u,mode,canonical,0u,0u,false,false,false);
        const auto coverage = setup::coverage().load();
        if (coverage != (mode == "reference" ? 0u : 1u) || !totals.leaves)
            throw std::runtime_error("Missing MOPP quantizer native return or real traversal leaf");
        std::printf("{\"comparisons\":%u,\"faults\":%u,\"accesses\":%u,\"stores\":%u,"
                    "\"children\":%u,\"traversal_leaves\":%u,\"canonical_fallbacks\":%u,\"coverage\":%llu}\n",
                    totals.comparisons,totals.faults,totals.accesses,totals.stores,totals.children,totals.leaves,
                    canonical.comparisons,static_cast<unsigned long long>(coverage));
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr,"%s\n",error.what());
        return 1;
    }
}

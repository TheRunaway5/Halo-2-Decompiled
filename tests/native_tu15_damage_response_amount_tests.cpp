#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/source_difficulty_parameter_native_tu15_internal.hpp"
#include "gameplay/units/native_tu15_damage_response_amount.hpp"

using namespace frame_misc_harness;
namespace amount = halo2::native_damage_response_amount_detail;
using amount::Entry;
using amount::Next;

namespace original {
struct ObservedDamageState : WorldState {
    using WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t damage_observation_pc(std::uint32_t disc, std::uint32_t tu15, const ObservedDamageState &s) {
    return s.selected_observation_pcs && s.image == SourcePhysicsContactImage::TitleUpdate15 ? tu15 : disc;
}
static void run_139ff0(ObservedDamageState &);
static void run_1ed0b0(ObservedDamageState &);
static void run_a87e0(ObservedDamageState &);
#include "damage_response_amount_children.inc"
#include "damage_response_amount_reference.inc"
} // namespace original

namespace {
using original::ObservedDamageState;
constexpr std::array entries{Entry::counter,Entry::load,Entry::compare};
constexpr auto definition = 0x610000u;
constexpr auto parameters = 0x620000u;
constexpr auto globals = 0x630000u;
constexpr auto table = 0x640000u;
constexpr auto objects = 0x650000u;
constexpr auto object = 0x660000u;
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
            throw std::runtime_error("Injected damage amount backing fault");
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
    std::uint32_t target;
    SourcePhysicsWorldRuntimeAbi entry;
};
struct Observation {
    SourcePhysicsWorldRuntimeAbi abi{};
    Next next{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<Child> calls;
    std::uint32_t cursor{}, literals{};
    std::string fault;
};
thread_local Observation *active{};

bool identical(const SourcePhysicsWorldRuntimeAbi &a, const SourcePhysicsWorldRuntimeAbi &b) {
    return same_abi(a,b) && a.fs_base == b.fs_base && a.floating_point->mxcsr == b.floating_point->mxcsr;
}
bool identical(const std::vector<Child> &a, const std::vector<Child> &b) {
    return a.size() == b.size() && std::ranges::equal(a,b,[](const Child &left, const Child &right) {
        return left.target == right.target && identical(left.entry,right.entry);
    });
}
Next bridge(ObservedDamageState &s, Entry entry) {
    return amount::bridge(s,entry,original::native_damage_response_amount_reference_d73e0,original::run_139ff0,
        original::run_1ed0b0,original::run_a87e0,original::damage_mulss,original::damage_addss);
}
Observation execute(FaultMemory &memory, const Case &c, bool native, Entry entry, unsigned observer_fault = 0,
                    bool selected = true, SourcePhysicsContactImage image = SourcePhysicsContactImage::TitleUpdate15) {
    Observation observed;
    SourcePhysicsContactRecordObserver observer = [&](const auto &event) {
        observed.stores.push_back(event);
        if (observed.stores.size() == observer_fault)
            throw std::runtime_error("Injected damage amount observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) {
        observed.cursor = pc;
        observed.literals += pc >= std::uint32_t(entry) && pc <= 0xd8a05u;
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
    ObservedDamageState state{memory,observer,services,image};
    state.load(c.abi);
    state.selected_observation_pcs = selected;
    active = &observed;
    try {
        observed.next = native ? bridge(state,entry) : original::native_damage_response_amount_reference_d73e0_live(state,entry);
    } catch (const std::exception &error) {
        observed.fault = error.what();
    }
    active = nullptr;
    observed.abi = state.save();
    return observed;
}

Case fixture(std::mt19937 &random, unsigned row) {
    Case c;
    const auto mode = row / entries.size() % 12u;
    const auto sp = row % 17u == 9u ? 2u : 0x711ff0u + row % 19u;
    const auto bp = 0x720ff0u + row % 19u;
    random_abi(random,c,sp);
    c.abi.gp[5] = bp;
    c.abi.gp[6] = (c.abi.gp[6] & 0xffff0000u) | (mode == 2 ? 0x7fffu : 0x123u);
    c.abi.gp[7] = 0x8af80007u;
    c.abi.fs_base = std::uint32_t(random());
    for (unsigned offset = 0; offset <= 0x60u; offset += 4u) {
        c.memory.put(sp + offset,std::uint32_t(random()));
        c.memory.put(sp - offset,std::uint32_t(random()));
    }
    auto threshold = mode == 0 ? half : two;
    auto source = one;
    auto previous = half;
    auto factor = mode == 5 ? two : half;
    auto intrinsic = mode == 1 ? two : half;
    auto difficulty = one;
    if (row >= entries.size() * 12u && mode >= 7) {
        std::mt19937_64 edges{random()};
        threshold = special_float(edges);
        source = special_float(edges);
        previous = special_float(edges);
        factor = special_float(edges);
        intrinsic = special_float(edges);
        difficulty = special_float(edges);
    }
    c.abi.xmm[0][0] = source;
    c.abi.xmm[1][0] = intrinsic;
    c.abi.xmm[2][0] = threshold;
    c.memory.put(sp + 0x18u,definition);
    c.memory.put(sp + 0x1cu,threshold);
    c.memory.put(definition + 0x38u,threshold);
    c.memory.put(definition + 0xd4u,source);
    c.memory.put(bp + 0xecu,previous);
    c.memory.put(bp + 0x104u,std::uint32_t(random()),2);
    c.memory.put(bp + 0x106u,mode == 2 || mode == 3 ? 3u : 0u,2);
    c.memory.put(bp + 0x10bu,std::uint32_t(random()),1);
    c.memory.put(bp + 0xaau,mode == 6 || mode >= 10 ? 1u : 2u,1);
    c.memory.put(bp + 0x138u,std::uint32_t(random()),2);
    c.memory.put(0x51df54u,parameters);
    c.memory.put(parameters + 4u,factor);
    c.memory.put(0x4f3218u,globals);
    c.memory.put(globals + 8u,2u);
    c.memory.put(globals + 0xcu,mode == 0 || mode == 2 ? 4u : 0u,1);
    c.memory.put(0x4ecc1cu,0u);
    c.memory.put(0x46a0d0u,difficulty);
    c.memory.put(0x4ecbd0u,table);
    c.memory.put(table + 0x44u,objects);
    c.memory.put(objects + 12u*7u + 8u,object);
    c.memory.put(object + 0xd4u,0xffffffffu);
    return c;
}

struct Counts {unsigned comparisons{}, faults{}, calls{}, accesses{}, stores{};};
std::pair<unsigned,unsigned> compare(const Case &c, unsigned row, Entry entry, Counts &count,
                                     unsigned access = 0, unsigned store = 0) {
    FaultMemory ref_bank(c.memory,access), got_bank(c.memory,access);
    const auto ref = execute(ref_bank,c,false,entry,store);
    const auto got = execute(got_bank,c,true,entry,store);
    const bool fault = access || store;
    if (ref.fault.empty() == fault || ref.fault != got.fault || got.next != ref.next ||
        (!fault && (got.next == Next::retained || got.next != (entry == Entry::counter ? Next::continue_flags : Next::depletion))) ||
        !identical(ref.abi,got.abi) || !identical(ref.calls,got.calls) || ref.stores != got.stores ||
        ref_bank.accessed != got_bank.accessed || !ref_bank.same(got_bank) || got.literals || !ref.literals ||
        (!census_mode && ref.cursor != got.cursor)) {
        std::fprintf(stderr,"amount row %u entry %x access %u/store %u: cursor %x/%x, next %u/%u, fault '%s'/'%s'\n",
            row,unsigned(entry),access,store,got.cursor,ref.cursor,unsigned(got.next),unsigned(ref.next),
            got.fault.c_str(),ref.fault.c_str());
        std::fprintf(stderr,"flags %x/%x MXCSR %x/%x accesses %zu/%zu stores %zu/%zu calls %zu/%zu\n",
            got.abi.eflags,ref.abi.eflags,got.abi.floating_point->mxcsr,ref.abi.floating_point->mxcsr,
            got_bank.accessed.size(),ref_bank.accessed.size(),got.stores.size(),ref.stores.size(),got.calls.size(),ref.calls.size());
        for (unsigned i=0;i<8;++i)
            if (got.abi.gp[i]!=ref.abi.gp[i])
                std::fprintf(stderr,"GP %u %x/%x\n",i,got.abi.gp[i],ref.abi.gp[i]);
        for (std::size_t i=0;i<std::min(got.stores.size(),ref.stores.size());++i)
            if (got.stores[i]!=ref.stores[i]) {
                std::fprintf(stderr,"store %zu PC %x/%x\n",i,got.stores[i].instruction,ref.stores[i].instruction);
                break;
            }
        throw std::runtime_error("Selected damage response amount differs");
    }
    ++count.comparisons;
    count.faults += fault;
    count.calls += unsigned(ref.calls.size());
    count.stores += unsigned(ref.stores.size());
    count.accesses += unsigned(ref_bank.accessed.size());
    return {unsigned(ref_bank.accessed.size()),unsigned(ref.stores.size())};
}
} // namespace

namespace original {
void run_139ff0(ObservedDamageState &s) { active->calls.push_back({0x13b360u,s.save()}); fixture_139ff0(s); }
void run_1ed0b0(ObservedDamageState &s) { active->calls.push_back({0x1f19d0u,s.save()}); fixture_1ed0b0(s); }
void run_a87e0(ObservedDamageState &s) { active->calls.push_back({0xb1c30u,s.save()}); fixture_a87e0(s); }
} // namespace original

int main(int argc, char **argv) try {
    const auto rows = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000u;
    const std::string mode = argc > 2 ? argv[2] : "literal";
    set_mode(mode,"HALO2_VERIFY_DAMAGE_RESPONSE_AMOUNT");
    std::mt19937 random{0xd88f9u};
    const auto initial = fixture(random,1u);
    const auto refused = [&](bool selected, SourcePhysicsContactImage image) {
        for (auto entry : entries) {
            FaultMemory bank(initial.memory,0);
            const auto got = execute(bank,initial,true,entry,0,selected,image);
            if (got.next != Next::retained || !got.fault.empty() || got.cursor || !got.calls.empty() ||
                !bank.accessed.empty() || !identical(got.abi,initial.abi))
                throw std::runtime_error("Damage response amount touched an unsupported route");
        }
    };
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_DAMAGE_RESPONSE_AMOUNT","1",1);
        refused(true,SourcePhysicsContactImage::TitleUpdate15);
        std::puts("{\"forced_reference_refusals\":3}");
        return 0;
    }
    refused(false,SourcePhysicsContactImage::TitleUpdate15);
    refused(true,SourcePhysicsContactImage::MapPack11);
    Counts count;
    for (unsigned row = 0; row < rows; ++row) {
        const auto c = fixture(random,row);
        const auto entry = entries[row % entries.size()];
        const auto [accesses,stores] = compare(c,row,entry,count);
        if (row >= 36u)
            continue;
        for (unsigned step = 1; step <= accesses; ++step)
            compare(c,row,entry,count,step);
        for (unsigned step = 1; step <= stores; ++step)
            compare(c,row,entry,count,0,step);
    }
    const auto covered = amount::coverage().load();
    const auto replays = amount::summary().children.load();
    if (covered != 1023u || (mode == "verify" && replays == 0u))
        throw std::runtime_error("Incomplete damage amount branch or child coverage");
    std::printf("{\"mode\":\"%s\",\"comparisons\":%u,\"faults\":%u,\"accesses\":%u,\"stores\":%u,"
        "\"genuine_child_calls\":%u,\"verified_replays\":%llu,\"coverage\":%llu}\n",mode.c_str(),
        count.comparisons,count.faults,count.accesses,count.stores,count.calls,
        static_cast<unsigned long long>(replays),static_cast<unsigned long long>(covered));
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr,"%s\n",error.what());
    return 1;
}

#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/source_damage_region_eligibility_native_tu15_internal.hpp"
#include "gameplay/units/native_tu15_damage_due_response.hpp"
#include "halo2/native_tu15_damage_response.hpp"

using namespace frame_misc_harness;
namespace due = halo2::native_damage_due_response_detail;

namespace original {
struct ObservedDamageState : WorldState {
    using WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t damage_observation_pc(std::uint32_t disc, std::uint32_t tu15, const ObservedDamageState &s) {
    return s.selected_observation_pcs && s.image == SourcePhysicsContactImage::TitleUpdate15 ? tu15 : disc;
}
static void dispatch(ObservedDamageState &, std::uint32_t, std::uint32_t, bool);
static void run_db570(ObservedDamageState &);
static void run_db710(ObservedDamageState &);
static void run_dd280(ObservedDamageState &);
#include "damage_due_response_children.inc"
#include "damage_due_response_reference.inc"
} // namespace original

namespace {
using original::ObservedDamageState;
constexpr std::uint32_t info = 0x610000u, section = 0x620000u, response = 0x630000u,
    object_header = 0x640000u, object_table = 0x650000u, object = 0x660000u, globals = 0x670000u;

struct Access {
    std::uint32_t address;
    std::size_t width;
    bool write;
    bool operator==(const Access &) const = default;
};
struct FaultMemory : Memory {
    mutable std::vector<Access> accessed;
    unsigned fail_at{};
    FaultMemory(const Memory &source, unsigned fault) : fail_at(fault) { pages = source.clone().pages; }
    void access(std::uint32_t address, std::size_t width, bool write) const {
        accessed.push_back({address,width,write});
        if (accessed.size() == fail_at)
            throw std::runtime_error("Injected due-response backing fault");
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
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<Child> calls;
    std::uint32_t cursor{}, literals{};
    unsigned verified_required_calls{};
    bool admitted{};
    std::string fault;
};
thread_local Observation *active{};
thread_local unsigned variant{};

bool identical(const SourcePhysicsWorldRuntimeAbi &a, const SourcePhysicsWorldRuntimeAbi &b) {
    return same_abi(a,b) && a.fs_base == b.fs_base && a.floating_point->mxcsr == b.floating_point->mxcsr;
}
bool identical(const std::vector<Child> &a, const std::vector<Child> &b) {
    return a.size() == b.size() && std::ranges::equal(a,b,[](const Child &left, const Child &right) {
        return left.target == right.target && identical(left.entry,right.entry);
    });
}
bool bridge(ObservedDamageState &s) {
    return due::bridge(s,original::native_damage_due_response_reference_d73e0,
        original::run_db570,original::run_db710,original::run_dd280,original::dispatch);
}
Observation execute(FaultMemory &memory, const Case &c, bool native, unsigned row, unsigned observer_fault = 0,
                    bool selected = true, SourcePhysicsContactImage image = SourcePhysicsContactImage::TitleUpdate15) {
    Observation observed;
    SourcePhysicsContactRecordObserver observer = [&](const auto &event) {
        observed.stores.push_back(event);
        if (observed.stores.size() == observer_fault)
            throw std::runtime_error("Injected due-response observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) {
        observed.cursor = pc;
        observed.literals += pc >= due::first && pc <= due::last;
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
    if (row % 9u != 8u) {
        services.required_owner = [&](auto &bank, const auto &call, auto &packet, const auto &) {
            if (&bank != &memory || call.target != due::notify_target || call.instruction != due::notify_site || call.tail)
                throw std::runtime_error("Wrong due-response required route");
            ++observed.verified_required_calls;
            observed.calls.push_back({call.target,packet});
            services.instruction(call.target);
            const auto return_to = r(bank,packet.gp[4],4);
            if (return_to != due::continuation)
                throw std::runtime_error("Wrong due-response required return address");
            packet.gp[0] = return_to;
            packet.gp[1] ^= 0x42931456u;
            packet.gp[4] += 8u;
            packet.xmm[3][2] ^= 0xffc00123u;
        };
    }
    FpScope fp(*c.abi.floating_point);
    ObservedDamageState state{memory,observer,services,image};
    state.load(c.abi);
    state.selected_observation_pcs = selected;
    active = &observed;
    variant = row;
    try {
        if (native)
            observed.admitted = bridge(state);
        else
            original::native_damage_due_response_reference_d73e0_live(state);
    } catch (const std::exception &error) {
        observed.fault = error.what();
    }
    active = nullptr;
    observed.abi = state.save();
    return observed;
}

Case fixture(std::mt19937 &random, unsigned row) {
    Case c;
    const auto sp = row % 17u == 9u ? 2u : 0x701ff4u + row % 19u;
    random_abi(random,c,sp);
    c.abi.gp[0] &= ~7u;
    c.abi.gp[3] = 0;
    c.abi.fs_base = std::uint32_t(random());
    c.abi.eflags &= ~0x400u;
    if (row % 6u != 1u && row / 6u % 2u)
        c.abi.eflags |= 0x400u;
    for (unsigned offset = 0; offset <= 0xd0u; offset += 4u) {
        c.memory.put(sp + offset,std::uint32_t(random()));
        c.memory.put(sp - offset,std::uint32_t(random()));
    }
    c.memory.put(sp + 0x18u,info);
    c.memory.put(sp + 0x20u,84u);
    c.memory.put(sp + 0x84u,0x87650007u);
    c.memory.put(info + 0xc0u,section);
    c.memory.put(section + 0x10u,response);
    c.memory.put(response + 0x4u,0u);
    c.memory.put(response + 0x10u,0xffffffffu);
    c.memory.put(response + 0x22u,row % 6u == 0u ? 0u : 0xffffu,2);
    c.memory.put(response + 0x20u,3u,2);
    c.memory.put(response + 0x3cu,0u);
    c.memory.put(response + 0x40u,0u);
    c.memory.put(0x4ecbd0u,object_header);
    c.memory.put(object_header + 0x44u,object_table);
    c.memory.put(object_table + 84u + 8u,object);
    c.memory.put(object + 0xc2u,std::uint32_t(random()),2);
    c.memory.put(object + 0xc4u,std::uint32_t(random()));
    c.memory.put(object + 0xc8u,std::uint32_t(random()));
    c.memory.put(object + 0x118u,16u,2);
    c.memory.put(object + 0x11au,0x300u,2);
    c.memory.put(object + 0x122u,0x260u,2);
    c.memory.put(object + 0x260u,0u,2);
    c.memory.put(object + 0x303u,10u,1);
    c.memory.put(0x4f3218u,globals);
    c.memory.put(globals + 0xcu,4u,1);
    return c;
}

struct Counts {unsigned cases{}, faults{}, calls{}, stores{}, accesses{}, real_update_calls{}, required_calls{};};
std::pair<unsigned,unsigned> compare(const Case &c, unsigned row, Counts &count, unsigned access = 0, unsigned store = 0) {
    FaultMemory ref_bank(c.memory,access), got_bank(c.memory,access);
    const auto ref = execute(ref_bank,c,false,row,store);
    const auto got = execute(got_bank,c,true,row,store);
    const bool missing_owner = row % 9u == 8u && row % 6u != 0u && row % 6u != 1u && row % 2u == 0;
    const auto intended_fault = access || store || missing_owner;
    if (ref.fault.empty() == bool(intended_fault) || ref.fault != got.fault ||
        (got.fault.empty() && !got.admitted) || !identical(ref.abi,got.abi) || !identical(ref.calls,got.calls) ||
        ref.stores != got.stores || ref_bank.accessed != got_bank.accessed || !ref_bank.same(got_bank) ||
        got.literals || !ref.literals || (!census_mode && ref.cursor != got.cursor)) {
        std::fprintf(stderr,"due row %u access %u/store %u cursor %x/%x child count %zu/%zu fault '%s'/'%s'\n",
            row,access,store,got.cursor,ref.cursor,got.calls.size(),ref.calls.size(),got.fault.c_str(),ref.fault.c_str());
        throw std::runtime_error("Selected damage due response differs");
    }
    if (!access && !store) {
        std::vector<std::uint32_t> expected{due::check_target}, actual;
        if (row % 6u) {
            expected.push_back(due::apply_target);
            if (row % 6u >= 3u)
                expected.push_back(due::regions_target);
            if (row % 2u == 0u && !missing_owner)
                expected.push_back(due::notify_target);
        }
        for (const auto &call : got.calls)
            actual.push_back(call.target);
        if (actual != expected) {
            std::fprintf(stderr,"due row %u child targets:",row);
            for (const auto target : actual)
                std::fprintf(stderr," %x",target);
            std::fprintf(stderr," expected:");
            for (const auto target : expected)
                std::fprintf(stderr," %x",target);
            std::fprintf(stderr,"\n");
            throw std::runtime_error("Unexpected due-response original child sequence");
        }
        count.real_update_calls += row % 6u == 1u;
    }
    ++count.cases;
    count.faults += intended_fault;
    count.calls += unsigned(ref.calls.size());
    count.required_calls += got.verified_required_calls;
    count.stores += unsigned(ref.stores.size());
    count.accesses += unsigned(ref_bank.accessed.size());
    return {unsigned(ref_bank.accessed.size()),unsigned(ref.stores.size())};
}
} // namespace

namespace original {
void run_db570(ObservedDamageState &s) {
    active->calls.push_back({due::check_target,s.save()});
    fixture_db570(s);
}
void run_db710(ObservedDamageState &s) {
    active->calls.push_back({due::apply_target,s.save()});
    if (variant % 6u == 1u) {
        fixture_db710(s);
        return;
    }
    s.services.instruction(due::apply_target);
    s.gp[0] = s.pop();
    s.gp[4] += 20u;
    // Controlled child outputs exercise the independently optional parent
    // region and required-dispatch edges without extra world dependencies.
    const auto frames = variant % 6u;
    w(s.memory,s.observe,due::apply_target,s.gp[4] + 0x78u,2,frames == 3u || frames == 5u ? 1u : 0u);
    w(s.memory,s.observe,due::apply_target,s.gp[4] + 0x7au,2,frames >= 4u ? 2u : 0u);
    w(s.memory,s.observe,due::apply_target,s.gp[4] + 0x7cu,1,variant % 2u == 0u ? 4u : 0u);
    s.xmm[4] = _mm_setzero_ps();
}
void run_dd280(ObservedDamageState &s) {
    active->calls.push_back({due::regions_target,s.save()});
    s.services.instruction(due::regions_target);
    s.gp[0] = s.pop();
    s.gp[4] += 12u;
    s.flags.logic(s.gp[0],4);
}
void dispatch(ObservedDamageState &s, std::uint32_t target, std::uint32_t pc, bool tail) {
    if (target != due::notify_target)
        throw std::logic_error("Unqualified secondary due-response dispatch reached");
    s.required(pc,target,tail);
}
} // namespace original

int main(int argc, char **argv) try {
    const auto cases = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000u;
    const std::string mode = argc > 2 ? argv[2] : "literal";
    set_mode(mode,"HALO2_VERIFY_DAMAGE_DUE_RESPONSE");
    std::mt19937 random{0xd842cu};
    const auto initial = fixture(random,1u);
    const auto refuse = [&](bool selected, SourcePhysicsContactImage image) {
        FaultMemory bank(initial.memory,0);
        const auto got = execute(bank,initial,true,1u,0,selected,image);
        if (got.admitted || !got.fault.empty() || got.cursor || !got.calls.empty() || !bank.accessed.empty() ||
            !identical(got.abi,initial.abi))
            throw std::runtime_error("Due response touched an unsupported route");
    };
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_DAMAGE_DUE_RESPONSE","1",1);
        refuse(true,SourcePhysicsContactImage::TitleUpdate15);
        std::puts("{\"forced_reference_refusals\":1}");
        return 0;
    }
    refuse(false,SourcePhysicsContactImage::TitleUpdate15);
    refuse(true,SourcePhysicsContactImage::MapPack11);
    Counts count;
    for (unsigned row = 0; row < cases; ++row) {
        const auto c = fixture(random,row);
        const auto [accesses,stores] = compare(c,row,count);
        if (row >= 18u)
            continue;
        for (unsigned step = 1; step <= accesses; ++step)
            compare(c,row,count,step);
        for (unsigned step = 1; step <= stores; ++step)
            compare(c,row,count,0,step);
    }
    const auto mask = due::coverage().load();
    const auto replays = due::summary().children.load();
    if (mask != 255u || !count.real_update_calls || !count.required_calls || (mode == "verify" && replays < cases))
        throw std::runtime_error("Incomplete due-response fixture routes");
    std::printf("{\"mode\":\"%s\",\"comparisons\":%u,\"faults\":%u,\"accesses\":%u,\"stores\":%u,"
        "\"compared_child_calls\":%u,\"genuine_apply_successes\":%u,\"required_calls\":%u,"
        "\"verified_replays\":%llu,\"coverage\":%llu}\n",mode.c_str(),count.cases,count.faults,count.accesses,count.stores,
        count.calls,count.real_update_calls,count.required_calls,static_cast<unsigned long long>(replays),
        static_cast<unsigned long long>(mask));
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr,"%s\n",error.what());
    return 1;
}

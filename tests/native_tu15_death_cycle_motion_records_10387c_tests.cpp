#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_death_cycle_motion_records_10387c.hpp"

#include <set>

namespace {
using namespace frame_misc_harness;
using namespace halo2::native_object_tick_detail::boundary;
namespace records=halo2::native_death_cycle_motion_records;

constexpr std::uint32_t datum=0x80240003u, object=0x10002000u, record=0x10004000u;
constexpr std::uint32_t manager=0x10001000u, slots=manager+0x500u;

void dispatch(WorldState &state,std::uint32_t target,std::uint32_t site,bool tail) {
    const auto sp=state.gp[esp];
    if (tail || physics_step_detail::r(state.memory,sp)!=site+5u)
        throw std::runtime_error("Incorrect record-motion CALL frame");
    log_call(entry_hash(state,target));
    if (policy & 4u) throw std::runtime_error("Record-motion child rejected entry");
    (void)state.pop();
    switch (target) {
    case 0x107610u:
        state.set(eax,policy&1u,1,0);
        state.gp[esp]+=12u;
        return;
    case 0x10b330u:
        state.set(eax,(policy>>1u)&1u,1,0);
        state.gp[esp]+=8u;
        return;
    case 0x108d10u:
        state.gp[esp]+=4u;
        return;
    default:
        throw std::runtime_error("Unexpected record-motion dispatcher target");
    }
}

void run_108040(WorldState &state) {
    if (physics_step_detail::r(state.memory,state.gp[esp])!=0x1038f0u || state.gp[eax]!=datum)
        throw std::runtime_error("Incorrect direct record-motion CALL frame");
    log_call(entry_hash(state,0x108040u));
    if (policy & 4u) throw std::runtime_error("Record-motion application rejected entry");
    (void)state.pop();
}

#include "death_motion_records_reference.inc"

bool native(WorldState &state,records::Entry entry) {
    const auto result=records::run(state,[](auto &child,std::uint32_t site,std::uint32_t target) {
        if (site==0x1038ebu) run_108040(child);
        else dispatch(child,target,site,false);
    },entry);
    if (result==records::Continuation::reference) return false;
    state.instruction_literal(result==records::Continuation::apply_motion ? 0x103cd3u : 0x1040f0u);
    return true;
}

Case fixture(std::mt19937 &random,unsigned index) {
    Case test;
    const auto sp=(index%5u==0 ? 0x20011002u : 0x20010400u)+(index&3u);
    random_abi(random,test,sp);
    test.full_observation=true;
    test.abi.gp[ebp]=object;
    const auto offset=(index%3u)*12u;
    test.abi.gp[ebx]=offset;
    test.abi.gp[esi]=record;
    test.policy=(index>>5u)&3u;
    auto &memory=test.memory;
    for (auto at=sp-0x38u;at<=sp+0x60u;at+=4u) memory.put(at,random());
    memory.put(sp+0x14u,index&1u,1);
    memory.put(sp+0x24u,3u*12u);
    memory.put(sp+0x4cu,datum);
    memory.put(sp+0x30u,index&0x80u ? 0xffffffffu : 4u);
    memory.put(0x4ecbd0u,manager);
    memory.put(manager+0x44u,slots);
    memory.put(slots+3u*12u+8u,object);
    memory.put(record+8u,index&4u ? 0xffffu : 2u,2);
    memory.put(record+0xau,index&8u ? 0xffffu : 0xfffeu,2);
    memory.put(object+0x16eu,index&0x10u ? 0x40u : 0u,1);
    memory.put(object+0x210u+offset,(random()&~1u)|((index>>1u)&1u));
    memory.put(object+0x1a8u+2u*0x34u,random(),1);
    memory.put(object+0x1a8u-2u*0x34u,random(),1);
    return test;
}
}

int main() {
    std::mt19937 random{0x10387cu};
    Totals totals;
    std::set<std::uint32_t> reached,exits;
    unsigned n=0;
    for (const auto entry:{records::Entry::primary,records::Entry::alternate,records::Entry::contextual})
        for (unsigned index=0;index<256;++index) {
            auto test=fixture(random,index);
            const auto selected=[&](WorldState &state) {return native(state,entry);};
            const auto retained=[&](WorldState &state) {reference_motion_records(state,entry);};
            auto seed=test.memory.clone();
            const auto baseline=execute(seed,test,false,selected,retained);
            if (!baseline.fault.empty() || (baseline.cursor!=0x103cd3u && baseline.cursor!=0x1040f0u)) {
                std::fprintf(stderr,"Invalid record-motion fixture %u at %x: %s\n",n,baseline.cursor,baseline.fault.c_str());
                return 1;
            }
            exits.insert(baseline.cursor);
            if (!compare(n++,test,selected,retained,totals,false,{},true)) return 1;
            for (auto pc:baseline.instructions) if (pc!=baseline.cursor && reached.insert(pc).second) {
                auto signal=test;
                signal.reject_instruction=pc;
                if (!compare(n++,signal,selected,retained,totals,false,{},true)) return 1;
            }
            if (!baseline.stores.empty()) {
                const auto &store=baseline.stores[index%baseline.stores.size()];
                auto denied=test;
                denied.memory.denied_write=store.address;
                if (!compare(n++,denied,selected,retained,totals,false,{},true)) return 1;
                auto rejected=test;
                rejected.reject_after_store_pc=store.instruction;
                if (!compare(n++,rejected,selected,retained,totals,false,{},true)) return 1;
            }
            if (!baseline.calls.empty() && index%8u==3u) {
                test.policy|=4u;
                if (!compare(n++,test,selected,retained,totals,false,{},true)) return 1;
            }
        }
    report(n,totals,reached.size(),exits.size(),"observed");
    for (auto pc:reached) std::fprintf(stderr,"%x ",pc);
    return !totals.faults || reached.size()!=130 || exits!=std::set<std::uint32_t>{0x103cd3u,0x1040f0u};
}

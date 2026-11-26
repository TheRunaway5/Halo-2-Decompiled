#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/weapons/native_tu15_weapon_animation_selection.hpp"
#include <set>

namespace {
using namespace frame_misc_harness;
using namespace halo2::native_weapon_animation_selection;

constexpr std::uint32_t graph=0x10000100u, selected=0x10000300u, query_result=0x10000400u;
constexpr std::uint32_t indices=0x10000600u, records=0x10000700u, overrides=0x10000a00u;
constexpr std::uint32_t result=0x10000c00u, definitions=0x10000d00u;
constexpr std::uint32_t activity=0x0e000607u;
unsigned identify_calls{},query_calls{};

std::uint32_t return_address(WorldState &s) {
    return r(s.memory,s.gp[esp],4);
}
void return_from_child(WorldState &s,unsigned bytes) {
    (void)s.pop();
    s.gp[esp]+=bytes;
}
void identify(WorldState &s) {
    log_call(entry_hash(s,0x2a0100u));
    const auto mode=policy%11u;
    ++identify_calls;
    if(mode==2u) {
        w(s.memory,s.observe,0x2a0100u,s.gp[ebx],4,absent);
        s.gp[eax]=0u;
    } else {
        s.gp[eax]=query_result;
    }
    return_from_child(s,8);
}
void query(WorldState &s) {
    log_call(entry_hash(s,0x1e5810u));
    const auto caller=return_address(s);
    const auto mode=policy%11u;
    if(caller==0x1e34b7u) {
        const auto id=mode==3u&&identify_calls==1u?absent:0x01234000u;
        w(s.memory,s.observe,0x1e5810u,query_result+4u,4,id);
        s.gp[eax]=query_result;
    } else {
        ++query_calls;
        const bool missing=mode==4u ? (query_calls%4u==0u) : (mode==5u && query_calls%3u==1u);
        s.gp[eax]=missing ? 0u : selected;
    }
    return_from_child(s,12);
}
void load_graph(WorldState &s) {
    log_call(entry_hash(s,0x1e5c80u));
    s.gp[ebx]=graph;
    return_from_child(s,4);
}
void register_animation(WorldState &s) {
    log_call(entry_hash(s,0x126200u));
    w(s.memory,s.observe,0x126200u,result+0x10u,4,s.gp[eax]);
    return_from_child(s,0);
}

#include "weapon_animation_selection_reference.inc"

bool native(WorldState &s) {
    identify_calls=query_calls=0;
    return bridge(s,identify,query,load_graph,register_animation);
}
void reference(WorldState &s) {
    identify_calls=query_calls=0;
    run_1e3420(s);
}

Case fixture(std::mt19937 &random,unsigned index) {
    const auto mode=index%11u;
    Case test;
    random_abi(random,test,0x20010400u+(index&3u));
    test.policy=index;
    test.abi.gp[eax]=activity;
    const auto sp=test.abi.gp[esp];
    for(auto address=sp-0x100u;address<=sp+0x28u;address+=4u)test.memory.put(address,random());
    test.memory.put(sp,0x63000000u);
    test.memory.put(sp+4u,graph);
    test.memory.put(sp+8u,result);
    test.memory.put(sp+0xcu,graph);
    test.memory.put(sp+0x10u,mode==1u?absent:0x01230003u);
    test.memory.put(sp+0x14u,0x01450005u);
    test.memory.put(sp+0x18u,mode==8u?0u:result+4u);
    test.memory.put(sp+0x1cu,mode==8u?0u:result+8u);
    test.memory.put(sp+0x20u,mode==8u?0u:result+12u);

    test.memory.put(graph+override_count,mode==6u?0u:2u);
    test.memory.put(graph+override_records,overrides);
    test.memory.put(graph+graph_records,records);
    test.memory.put(overrides+4u,mode==7u?absent:0x01120000u);
    test.memory.put(overrides+override_stride+4u,0x01120001u);
    test.memory.put(0x4f0414u,definitions);
    test.memory.put(definitions+8u,graph);
    test.memory.put(definitions+0x18u,graph);
    test.memory.put(selected+selection_count,mode==9u?0xffffffffu:(mode==10u?0u:3u));
    test.memory.put(selected+selection_indices,indices);
    for(unsigned i=0;i<3u;++i) {
        test.memory.put(indices+i*4u,i);
        test.memory.put(records+i*20u+record_enabled,i!=1u,1);
    }
    // An absent override still runs the original lookup sequence. Its
    // zero graph can carry a default record in the authored test memory.
    test.memory.put(graph_records,records);
    return test;
}
}

int main(int argc,char **argv) {
    if(argc!=2)return 2;
    const auto count=std::stoul(argv[1]);
    std::mt19937 random{0x1e3420u};
    Totals totals;
    std::set<std::uint32_t> reached;
    for(unsigned index=0;index<count;++index) {
        auto test=fixture(random,index);
        test.full_observation=true;
        auto copy=test.memory.clone();
        const auto baseline=execute(copy,test,false,native,reference);
        if(!baseline.fault.empty()||baseline.stores.empty()) {
            std::fprintf(stderr,"invalid baseline case %u: %s\n",index,baseline.fault.c_str());
            return 1;
        }
        reached.insert(baseline.instructions.begin(),baseline.instructions.end());
        const auto &store=baseline.stores[index%baseline.stores.size()];
        switch((index/11u)%6u) {
            case 0: break;
            case 1: test.full_observation=false;break;
            case 2: test.full_observation=false;test.activate_trace_at_store=store.instruction;break;
            case 3: test.reject_after_store_pc=store.instruction;break;
            case 4: test.memory.denied_write=store.address;break;
            case 5: test.reject_instruction=baseline.instructions[index%baseline.instructions.size()];break;
        }
        if(!compare(index,test,native,reference,totals,false,{},true))return 1;
    }
    if(reached.size()!=1007u||!totals.faults||!totals.children) {
        std::fprintf(stderr,"incomplete selection coverage: %zu source sites\n",reached.size());
        for(const auto pc:reached)std::fprintf(stderr,"%x ",pc);
        return 1;
    }
    report(unsigned(count),totals,reached.size(),3,"quiet-and-observed");
}

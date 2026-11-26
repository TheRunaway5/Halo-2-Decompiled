#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_polygon_intersection.hpp"
#include <numbers>

namespace {
using namespace frame_misc_harness;

#include "polygon_intersection_reference.inc"

void axis(WorldState &state) {
    log_call(entry_hash(state,0x123210u));
    run_123210(state);
}
void contains(WorldState &state) {
    log_call(entry_hash(state,0x243510u));
    run_243510(state);
}
bool native(WorldState &state) {
    return halo2::native_polygon_intersection::bridge(state,axis,contains);
}

Case fixture(std::mt19937 &random,unsigned index) {
    using namespace halo2::native_polygon_intersection;
    constexpr std::uint32_t shape=0x10000200u,ray=0x10000400u,hit=0x10000600u,output=0x10000800u;
    Case test;
    random_abi(random,test,0x20010300u+(index&3u));
    test.abi.gp[ecx]=shape;
    const auto stack=test.abi.gp[esp];
    const auto mode=(index/6u)%12u;
    const auto axis=(index/2u)%3u;
    const auto count=3u+(index/72u)%6u;
    const auto result=mode==10u?ray:output;
    test.memory.put(stack,0x63000000u);
    test.memory.put(stack+4u,hit);
    test.memory.put(stack+8u,ray);
    test.memory.put(stack+12u,result);
    for(auto address=stack-0x100u;address<stack;address+=4u)test.memory.put(address,random());
    test.memory.put(shape+vertex_count,count);
    const auto put_float=[&](std::uint32_t address,float value) {
        test.memory.put(address,std::bit_cast<std::uint32_t>(value));
    };
    const auto point=[&](std::uint32_t address,float u,float v,float w) {
        put_float(address+4u*axis,w);
        put_float(address+4u*((axis+1u)%3u),u);
        put_float(address+4u*((axis+2u)%3u),v);
    };
    const auto radius=mode==5u?0.0f:(mode==6u?0.0002f:1.0f);
    for(unsigned i=0;i<count;++i) {
        const auto turn=(index&1u)?float(i):-float(i);
        const auto angle=turn*2.0f*std::numbers::pi_v<float>/float(count);
        point(shape+first_vertex+i*vertex_stride,std::cos(angle)*radius,std::sin(angle)*radius,0.0f);
    }
    const auto u=mode==1u?3.0f:(mode==9u?radius:0.0f);
    const auto start=mode==4u?0.0f:(mode==11u?-2.0f:2.0f);
    const auto stop=mode==4u?0.0f:(mode==3u?4.0f:(mode==11u?2.0f:-2.0f));
    point(ray,u,0.0f,start);
    point(ray+0x10u,u+(mode==4u?0.125f:0.0f),0.0f,stop);
    if(mode==7u)test.memory.put(shape+first_vertex+4u*axis,0x7fc22304u);
    if(mode==8u)test.memory.put(shape+first_vertex+vertex_stride+4u*axis,0xff800000u);
    put_float(result+0x14u,mode==2u?0.375f:1.0f);
    put_float(one,1.0f);
    put_float(negative_one,-1.0f);
    test.memory.put(minimum_normal,0x38d1b717u);
    constexpr std::array<unsigned,18> table{2,1,0, 1,2,0, 0,2,1, 2,0,1, 1,0,2, 0,1,2};
    for(unsigned i=0;i<table.size();++i)test.memory.put(axis_pairs+i*2u,table[i],2);
    return test;
}
}

int main(int argc,char **argv) {
    using namespace frame_misc_harness;
    if(argc!=2)return 2;
    const auto count=std::stoul(argv[1]);
    std::mt19937 random{0x1f6220u};
    Totals totals;
    unsigned accepted=0,rejected=0;
    std::vector<std::uint32_t> reached;
    for(unsigned index=0;index<count;++index) {
        auto test=fixture(random,index);
        test.full_observation=true;
        auto memory=test.memory.clone();
        const auto baseline=execute(memory,test,false,native,reference_polygon_intersection);
        if(!baseline.fault.empty()||baseline.stores.empty()) {
            std::fprintf(stderr,"case %u failed baseline: %s\n",index,baseline.fault.c_str());
            return 1;
        }
        accepted+=baseline.cursor==0x1f65efu;
        rejected+=baseline.cursor==0x1f6606u;
        for(const auto pc:baseline.instructions)
            if(pc>=0x1f6220u && pc<=0x1f6606u && std::ranges::find(reached,pc)==reached.end())reached.push_back(pc);
        const auto &store=baseline.stores[index%baseline.stores.size()];
        switch(index%5u) {
            case 0: test.full_observation=false;break;
            case 1: test.full_observation=false;test.activate_trace_at_store=store.instruction;break;
            case 2: test.reject_after_store_pc=store.instruction;break;
            case 3: test.memory.denied_write=store.address;break;
            case 4: break;
        }
        if(!compare(index,test,native,reference_polygon_intersection,totals,false,{},true))return 1;
    }
    if(!accepted||!rejected||reached.size()!=253u||!totals.faults||!totals.children) {
        std::fprintf(stderr,"incomplete polygon coverage: hit %u, reject %u, sites %zu\n",accepted,rejected,reached.size());
        return 1;
    }
    report(unsigned(count),totals,reached.size(),2,"quiet-and-observed");
}

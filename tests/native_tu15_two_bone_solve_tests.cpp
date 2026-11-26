#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_two_bone_solve.hpp"

using namespace frame_misc_harness;
namespace solve=halo2::native_two_bone_solve;
namespace original {
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
#include "normalize.inc"
static void run_30bd0(WorldState &s) {
    const auto returned=r(s.memory,s.gp[4]);
    if(returned!=0x122da2u && returned!=0x122f2cu && returned!=0x122f89u &&
       returned!=0x12306au && returned!=0x1230c5u)
        throw std::logic_error("Unexpected normalization return site");
    log_call(mix(entry_hash(s,0x30bd0u),returned));
    run_normalization_body(s);
}
#include "solve.inc"
}

namespace {
constexpr std::uint32_t stack=0x20010400u, parent=0x10000100u, grandparent=0x10001100u,
    goal=0x10002100u, node=0x10003100u;

void matrix(Memory &memory,std::uint32_t at,std::array<float,3> position) {
    for(std::uint32_t offset=0;offset<104u;offset+=4u)memory.put(at+offset-52u,0u);
    constexpr std::array<float,10> axes{1,1,0,0,0,1,0,0,0,1};
    for(unsigned i=0;i<axes.size();++i)memory.put(at+4u*i,std::bit_cast<std::uint32_t>(axes[i]));
    for(unsigned i=0;i<position.size();++i)memory.put(at+40u+4u*i,std::bit_cast<std::uint32_t>(position[i]));
}
Case authored(std::mt19937 &rng,unsigned index) {
    Case c;
    auto &memory=c.memory;
    random_abi(rng,c,stack);
    const unsigned shape=index%16u;
    if(shape!=15u)c.abi.x87_depth=0;
    c.abi.gp[0]=parent;
    c.abi.gp[1]=grandparent;
    for(auto at=stack-0x80u;at<=stack+0x30u;at+=4u)memory.put(at,std::uint32_t(rng()));
    memory.put(stack,0x1e14bbu);
    memory.put(stack+4u,goal);
    memory.put(stack+8u,shape==12u?goal:node);
    const auto perturb=float(int(rng()%101u)-50)/256.0f;
    matrix(memory,grandparent,{0,0,0});
    matrix(memory,parent,shape==0u?std::array<float,3>{0,0,0}:std::array<float,3>{1.0f,0.4f,perturb});
    matrix(memory,node,{2.5f,0.3f,1.0f});
    matrix(memory,goal,{shape==2u?10.0f:1.5f,0.1f,perturb+0.5f});
    if(shape==1u)matrix(memory,node,{1.0f,0.4f,perturb});
    if(shape==3u)matrix(memory,goal,{0,0,0});
    if(shape==4u)memory.put(goal+40u,0x7fc20055u);
    if(shape==5u)memory.put(parent+40u,0xff800000u);
    memory.put(0x46a0ecu,0x358637bdu);
    memory.put(0x46a0d0u,0x3f800000u);
    memory.put(0x46a11cu,0x40000000u);
    memory.put(0x46a1acu,0x3f7fbe77u);
    switch(shape) {
    case 6:memory.drop(grandparent);break;
    case 7:memory.denied_write=stack-0x2cu;break; // First x87 FSTP.
    case 8:memory.denied_write=stack-0x28u;memory.writes_before_denial=1u;break;
    case 9:memory.denied_write=parent+0x14u;break;
    case 10:memory.denied_write=node+0x18u;break;
    case 11:memory.denied_write=stack-0x34u;break;
    case 13:memory.denied_write=stack-0x40u;break;
    case 14:c.reject_after_store_pc=0x12313du;break;
    default:break;
    }
    c.full_observation=!census_mode && (index/16u)%2u!=0;
    c.census_child_range={0x30bd0u,0x30c49u};
    if(c.full_observation && (index/32u)%2u) {
        constexpr std::uint32_t stops[]{0x122ce1u,0x122d22u,0x122ed1u,0x123115u,0x123146u};
        c.reject_instruction=stops[(index/64u)%std::size(stops)];
    }
    if(!c.full_observation && !census_mode && (index/256u)%2u)
        c.activate_trace_at_store=0x122d9du;
    return c;
}
bool candidate(WorldState &s){return solve::bridge(s,original::run_30bd0);}
}

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):2000;
    const std::string mode=argc>2?argv[2]:"";
    census_mode=mode=="census";
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    std::mt19937 rng(0x122be0u);
    Totals totals;
    for(unsigned index=0;index<cases;++index)
        if(!compare(index,authored(rng,index),candidate,original::run_122be0,totals,false,{},true))return 1;
    report(cases,totals,0u,0u,mode);
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

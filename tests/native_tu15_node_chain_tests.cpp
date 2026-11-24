// Native TU1.5 node chain (1E1390) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children 144090 (the unit's dispatch route), 145560, 122BE0 and
// 170730 are genuine stand-ins whose effects are a hash of their complete
// entry ABI, their stack arguments and the return address: 144090 fills
// the destination matrix, all of them pop their arguments, return
// EAX/ECX/EDX, flags and XMM, write scratch, and occasionally clobber
// callee-saved registers or leave the stack unbalanced. Fixtures author
// model tags whose nodes have no parent, a parent only, or a parent and a
// grandparent (negative indices too), node matrices, weights of one, near
// one, partial, special floats and NaN payloads, and the full weight
// constant; one region removed injects a fault; an x87 stack without two
// free registers is refused. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_node_chain.hpp"
using namespace frame_misc_harness;
namespace chain=halo2::native_node_chain_detail;
namespace {
constexpr std::uint32_t instances=0x10000000u,tags=0x10100000u,nodes=0x10200000u,matrices=0x10300000u,
    scratch=0x10600000u;
bool nan_heavy=false;   // shape 4: NaNs often, so two NaN payloads meet in the blend
std::uint32_t special_float(std::mt19937_64 &rng) {
    static constexpr std::uint32_t values[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7f800000u,0xff800000u,0x00000001u,
        0x80000000u,0x00400000u};
    return values[rng()%8];
}
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=0;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    if(target==0x144090u) {
        for(std::uint32_t k=0;k<13;++k) {
            const auto value=rng()%(nan_heavy?2:9)==0?special_float(rng):std::bit_cast<std::uint32_t>(float(int(rng()%33)-16)*0.25f);
            w(s.memory,s.observe,target+2u,s.gp[0]+4u*k,4,value);
        }
    }
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==3?matrices+0x34u*std::uint32_t(rng()%4):which==6?std::uint32_t(rng()%8):
            which==7?matrices+0x34u*std::uint32_t(rng()%8):std::uint32_t(rng());
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*arguments+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x144090u)throw std::runtime_error("Unbound live support dispatch target");
    stub(s,0x144090u,0);
}
static void run_145560(WorldState &s){stub(s,0x145560u,1);}
static void run_122be0(WorldState &s){stub(s,0x122be0u,2);}
static void run_170730(WorldState &s){stub(s,0x170730u,5);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    std::mt19937_64 wide(rng());
    nan_heavy=shape==4;
    m.put(0x4f0414u,instances);
    for(std::uint32_t k=0;k<4;++k) {
        m.put(instances+16u*k,std::uint32_t(rng()));m.put(instances+16u*k+4u,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,tags+0x100u*k);m.put(instances+16u*k+12u,std::uint32_t(rng()));
        for(std::uint32_t at=tags+0x100u*k;at<tags+0x100u*(k+1u);at+=4)m.put(at,std::uint32_t(rng()));
        m.put(tags+0x100u*k+0x4cu,nodes+0x60u*2u);
    }
    // Nodes -2..9 (negative ones lie before the array start).
    for(std::uint32_t at=nodes;at<nodes+0x60u*12u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<12;++k) {
        const auto node=nodes+0x60u*k;
        std::uint32_t parent=pick(4)==0?0xffffu:pick(8);
        if(pick(9)==0)parent=0x10000u-1u-pick(2);
        if(parent==0xfffeu)parent=0xfffeu;
        m.put(node+4u,(std::uint32_t(rng())&0xffff0000u)|parent);
    }
    for(std::uint32_t at=matrices-0x34u*4u;at<matrices+0x34u*12u;at+=4)
        m.put(at,pick(nan_heavy?2:9)==0?special_float(wide):std::bit_cast<std::uint32_t>(float(int(pick(33))-16)*0.5f));
    m.put(0x46a0d0u,std::bit_cast<std::uint32_t>(1.0f));
    m.put(0x46a0ecu,pick(4)?std::bit_cast<std::uint32_t>(0.001f):special_float(wide));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x200u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1e0000u+pick(0x100));
    static constexpr float weights[]{1.0f,1.0005f,0.9999f,0.5f,0.0f,0.25f,1.5f,-0.5f};
    std::uint32_t weight=std::bit_cast<std::uint32_t>(weights[pick(8)]);
    if(shape==4 && pick(4)==0)weight=special_float(wide);
    const std::uint32_t args[]{scratch+0x40u,scratch+0x80u,weight,scratch+0xc0u};
    for(std::uint32_t k=0;k<4;++k)m.put(stack+4u+4u*k,args[k]);
    random_abi(rng,c,stack);
    if(c.abi.x87_depth>6u)c.abi.x87_depth=6u;
    if(shape==14)c.abi.x87_depth=7u;                                              // refused
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);c.abi.gp[3]=matrices;c.abi.gp[6]=pick(10);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{instances,tags,nodes,matrices,0x46a000u};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return chain::bridge(s,[](auto &v){original::native_frame_misc_reference_1e1390(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},
        [](auto &v){original::run_145560(v);},[](auto &v){original::run_122be0(v);},[](auto &v){original::run_170730(v);});
}
void reference(WorldState &s){original::run_1e1390(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_NODE_CHAIN");
    std::mt19937 rng(0x1e1390u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t,c.abi.x87_depth>6u))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,chain::coverage().load(),unsigned(chain::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

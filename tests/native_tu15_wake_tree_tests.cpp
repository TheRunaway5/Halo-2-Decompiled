// Native TU1.5 wake tree (BDA50) against the unmodified translated body of
// the scene device constructor unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children BCFB0 (returns another handle), B9390 and BE7E0 are genuine
// stand-ins whose effects are a hash of their complete entry ABI, their
// stack and the return address: they return EAX/ECX/EDX, flags and XMM,
// write scratch, and occasionally clobber callee-saved registers or leave
// the stack unbalanced. Fixtures author objects with every header flag (the
// indirect and active ones among them); one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_wake_tree.hpp"
using namespace frame_misc_harness;
namespace tree=halo2::native_wake_tree_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,scratch=0x10600000u,
    none=0xffffffffu;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=0;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=target==0xbcfb0u?(std::uint32_t(rng()%0x7fff)<<16)|std::uint32_t(rng()%8):std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==3?header_data+12u*std::uint32_t(rng()%4):which==7?header_data+12u*std::uint32_t(rng()%8):
            which==6?(rng()%2?objects+0x100u*std::uint32_t(rng()%8):std::uint32_t(rng()%8)):std::uint32_t(rng());
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
static void run_bcfb0(WorldState &s){stub(s,0xbcfb0u,0);}
static void run_b9390(WorldState &s){stub(s,0xb9390u,0);}
static void run_be7e0(WorldState &s){stub(s,0xbe7e0u,1);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    // A forest: each object's children have higher indices (no cycles).
    for(std::uint32_t k=0;k<8;++k) {
        const auto object=objects+0x100u*k;
        const std::uint32_t type=pick(3)==0?12u:pick(14);
        const std::uint32_t flags=std::uint32_t(rng())&0xffu;
        m.put(header_data+12u*k,(std::uint32_t(rng())&0xffffu)|(flags<<16)|(type<<24));
        m.put(header_data+12u*k+4u,std::uint32_t(rng()));m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x10u,none);m.put(object+0xcu,none);
    }
    for(std::uint32_t k=1;k<8;++k)if(pick(3)) {
        const auto parent=pick(k);const auto handle=(std::uint32_t(rng()%0x7fff)<<16)|k;
        m.put(objects+0x100u*k+0xcu,m.get(objects+0x100u*parent+0x10u));
        m.put(objects+0x100u*parent+0x10u,handle);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb0000u+pick(0x100));
    const auto handle=(std::uint32_t(rng()%0x7fff)<<16)|pick(8);
    m.put(stack+4u,handle);
    random_abi(rng,c,stack);
    c.abi.gp[0]=handle;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects};
        m.drop(regions[pick(3)]+(pick(3)==2?0x100u*pick(8):0u));
    }
    return c;
}
bool native(WorldState &s) {
    return tree::bridge_wake(s,[](auto &v){original::native_frame_misc_reference_bda50(v);},
        [](auto &v){original::run_bcfb0(v);},[](auto &v){original::run_b9390(v);},[](auto &v){original::run_be7e0(v);});
}
void reference(WorldState &s){original::run_bda50(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_WAKE_TREE");
    std::mt19937 rng(0xbda50u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,tree::wake_coverage().load(),unsigned(tree::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

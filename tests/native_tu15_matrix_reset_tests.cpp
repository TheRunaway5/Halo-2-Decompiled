// Native TU1.5 matrix reset (144CB0) against the unmodified translated body, extracted
// by tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The child 144910 (the unit's dispatch route) is a genuine stand-in whose
// effects are a hash of its complete entry ABI and return address: it
// writes the rotation, returns EAX/ECX/EDX (EDX sometimes another matrix),
// flags and XMM, and occasionally clobbers callee-saved registers or leaves
// the stack unbalanced. Fixtures author matrices and positions (special
// floats); one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_matrix_reset.hpp"
using namespace frame_misc_harness;
namespace reset=halo2::native_matrix_reset_detail;
namespace {
constexpr std::uint32_t matrices=0x10000000u,positions=0x10100000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=0;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<9;++k)w(s.memory,s.observe,target+2u,s.gp[0]+4u*k,4,std::uint32_t(rng()));
    const auto edx_in=s.gp[2];s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=rng()%6==0?matrices+0x40u*std::uint32_t(rng()%8):edx_in;
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==6?positions+0x10u*std::uint32_t(rng()%8):std::uint32_t(rng());
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
    if(target!=0x144910u)throw std::runtime_error("Unbound sandbox special dispatch target");
    stub(s,target,0);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=matrices;at<matrices+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=positions;at<positions+0x100u;at+=4)m.put(at,pick(6)?std::bit_cast<std::uint32_t>(float(int(pick(64))-32)):std::uint32_t(rng()));
    m.put(0x46a0d0u,std::bit_cast<std::uint32_t>(1.0f));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x140000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[2]=matrices+0x40u*pick(7);c.abi.gp[6]=positions+0x10u*pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{matrices,positions,0x46a000u};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return reset::bridge(s,[](auto &v){original::native_frame_misc_reference_144cb0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_144cb0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_MATRIX_RESET");
    std::mt19937 rng(0x144cb0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,reset::coverage().load(),unsigned(reset::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

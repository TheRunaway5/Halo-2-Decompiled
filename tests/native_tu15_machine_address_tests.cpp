// Native TU1.5 machine address check (69AD0) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 6C3F0 is a hash-driven stub of its complete entry ABI (and the network
// block it is handed): it answers a peer record or none, writes scratch; the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author networking off and on, modes, addresses equal to this
// machine's or differing in each word, both string directions (backward
// matches against the frame below the copy), and one region removed injects
// a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "runtime/native_tu15_machine_address.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_machine_address_detail;
namespace {
constexpr std::uint32_t network=0x10000000u,addresses=0x10010000u,peers=0x10020000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=rng()%3?peers+0x100u*std::uint32_t(rng()%2):0u;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=8u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_6c3f0(WorldState &s){stub(s,0x6c3f0u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4dc040u,pick(6)?1u+pick(255):0u,1);m.put(0x4dc04cu,network);
    for(std::uint32_t at=network;at<network+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(network+8u,pick(6)?1u+pick(4):0u);
    for(std::uint32_t at=addresses;at<addresses+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    // The given address: this machine's, or differing in one of its words.
    const auto given=addresses+0x10u+2u*pick(4);
    for(std::uint32_t k=0;k<6;++k)m.put(given+k,m.get(network+0xdu+k,1),1);
    if(pick(3))m.put(given+2u*pick(3),(m.get(given,2)+1u+pick(0xfffe))&0xffffu,2);
    for(std::uint32_t at=peers;at<peers+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=given;
    c.abi.eflags&=~0x400u;
    if(rng()%3==0) {
        // Backward: the words below the given one against the frame below the copy (its answer byte cleared), so a
        // backward match is authored half the time.
        c.abi.eflags|=0x400u;
        const auto backward=addresses+0x30u;
        c.abi.gp[0]=backward;
        m.put(backward,m.get(network+0xdu,2),2);
        if(pick(2)){m.put(backward-2u,m.get(stack-0xau,1),2);m.put(backward-4u,m.get(stack-0xcu,2),2);}
    }
    if(shape==15) {
        static constexpr std::uint32_t regions[]{network,addresses,peers};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_69ad0(v);},[](auto &v){original::run_6c3f0(v);});}
void reference(WorldState &s){original::run_69ad0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_MACHINE_ADDRESS");
    std::mt19937 rng(0x69ad0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

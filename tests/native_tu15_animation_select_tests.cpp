// Native TU1.5 animation select (1E5E40) against the unmodified translated
// body of the physics live support unit (the validator repeats it with the
// AI frame and AI actor units'), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1E5810 and 126200 are hash-driven stubs of their complete entry ABIs and
// arguments: 1E5810 answers an entry (or 0) and pops its three arguments,
// 126200 (EAX the item, CL the list) writes the item; both write scratch,
// the caller-saved registers, XMM, flags and (by policy) MXCSR are random,
// and by policy BL, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author every forced byte and flag, entries with lists of zero,
// negative and a few counts, and items in use or not; one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_animation_select.hpp"
using namespace frame_misc_harness;
namespace select_test=halo2::native_animation_select_detail;
namespace {
constexpr std::uint32_t graphs=0x10000000u,entries=0x10010000u,lists=0x10020000u,items=0x10030000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,target==0x126200u && rng()%2?s.gp[0]+4u*std::uint32_t(rng()%4):scratch+std::uint32_t(rng()%64)*4u,
      4,std::uint32_t(rng()));
    s.gp[0]=target==0x1e5810u?(rng()%6==0?0u:entries+0x40u*std::uint32_t(rng()%4)):std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=graphs+0x100u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=entries+0x40u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng()%3);
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
static void run_1e5810(WorldState &s){stub(s,0x1e5810u,3);}
static void run_126200(WorldState &s){stub(s,0x126200u,0);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t g=0;g<2;++g) {
        for(std::uint32_t at=graphs+0x100u*g;at<graphs+0x100u*g+0xc0u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(graphs+0x100u*g+0xb0u,items+0x400u*g);
        for(std::uint32_t i=0;i<8;++i) {
            for(std::uint32_t at=items+0x400u*g+20u*i;at<items+0x400u*g+20u*i+20u;at+=4)m.put(at,std::uint32_t(rng()));
            m.put(items+0x400u*g+20u*i+0x10u,pick(2)?0u:1u+pick(255),1);
        }
    }
    for(std::uint32_t e=0;e<4;++e) {
        const auto entry=entries+0x40u*e;
        for(std::uint32_t at=entry;at<entry+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        static constexpr std::uint32_t counts[]{0u,1u,2u,3u,0xffffffffu};
        m.put(entry+0x24u,counts[pick(5)]);m.put(entry+0x2cu,counts[pick(5)]);
        m.put(entry+0x28u,lists+0x100u*e);m.put(entry+0x30u,lists+0x100u*e+0x80u);
        for(std::uint32_t k=0;k<8;++k){m.put(lists+0x100u*e+4u*k,pick(8));m.put(lists+0x100u*e+0x80u+4u*k,pick(8));}
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1e0000u+pick(0x100));
    m.put(stack+4u,graphs+0x100u*pick(2));
    m.put(stack+0x14u,(std::uint32_t(rng())&~0xffu)|(pick(2)?0u:1u+pick(255)));
    random_abi(rng,c,stack);
    c.abi.gp[3]=(c.abi.gp[3]&~0xffu)|(pick(2)?0u:1u+pick(255));
    if(shape==15) {
        static constexpr std::uint32_t regions[]{graphs,entries,lists,items};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return select_test::bridge(s,[](auto &v){original::native_frame_misc_reference_1e5e40(v);},
        [](auto &v){original::run_1e5810(v);},[](auto &v){original::run_126200(v);});
}
void reference(WorldState &s){original::run_1e5e40(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ANIMATION_SELECT");
    std::mt19937 rng(0x1e5e40u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,select_test::coverage().load(),unsigned(select_test::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

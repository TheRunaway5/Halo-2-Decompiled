// Native TU1.5 tag iterator (125790) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The child 125810 (the unit's dispatch route) is a genuine stand-in whose
// effects are a hash of its complete entry ABI and return address: it
// returns in EAX one of several three-group blocks (with and without the
// wanted group), returns ECX/EDX, flags and XMM, writes scratch, and
// occasionally clobbers callee-saved registers (EBX's NONE among them) or
// leaves the stack unbalanced. Fixtures author the tag count, tag instances
// (empty, without a group or a handle, of the wanted group or not; a null
// instance address too) and iterators at the start, the middle and past the
// end, wanting every group or one; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_tag_iterator.hpp"
using namespace frame_misc_harness;
namespace iterator=halo2::native_tag_iterator_detail;
namespace {
constexpr std::uint32_t tags_block=0x10000000u,instances=0x10100000u,iterators=0x10200000u,groups=0x10300000u,
    scratch=0x10600000u,none=0xffffffffu;
constexpr std::uint32_t wanted=0x77656170u;   // 'weap'
void groups_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x125810u);
    h=mix(h,r(s.memory,s.gp[4]));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x125811u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=groups+16u*std::uint32_t(rng()%8);
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%5==0)s.gp[3]=rng()%2?wanted:0x62697064u;                 // EBX's NONE lost
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==3?(rng()%2?none:wanted):which==6?instances+16u*std::uint32_t(rng()%32):
            which==7?iterators+0x20u*std::uint32_t(rng()%4):std::uint32_t(rng());
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x125810u)throw std::runtime_error("Unbound live support dispatch target");
    groups_stub(s);
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
    const std::uint32_t count=shape==1?0u:pick(33);
    m.put(0x556d90u,tags_block);
    for(std::uint32_t at=tags_block;at<tags_block+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(tags_block+0x18u,count);
    m.put(0x4f0414u,shape==2?0u-16u*pick(4):instances);
    for(std::uint32_t k=0;k<40;++k) {
        const auto instance=instances+16u*k;
        const auto kind=pick(6);
        m.put(instance,kind==0?none:kind<3?wanted:0x62697064u+pick(4));
        m.put(instance+4u,kind==1?none:(std::uint32_t(rng()%0x7fff)<<16)|k);
        m.put(instance+8u,std::uint32_t(rng()));m.put(instance+12u,std::uint32_t(rng()));
    }
    for(std::uint32_t k=0;k<8;++k) {
        for(std::uint32_t i=0;i<4;++i)m.put(groups+16u*k+4u*i,pick(4)==0?wanted:pick(3)==0?none:0x62697064u+pick(4));
    }
    for(std::uint32_t k=0;k<4;++k) {
        const auto it=iterators+0x20u*k;
        for(std::uint32_t at=it;at<it+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(it+0xcu,pick(5)==0?count+pick(3):pick(5)==0?0u-1u-pick(2):pick(count+1u));
        m.put(it+0x10u,pick(3)==0?none:pick(4)==0?0x62697064u:wanted);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x120000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[7]=iterators+0x20u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{tags_block,instances,iterators,groups,0x556000u,0x4f0000u};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s) {
    return iterator::bridge(s,[](auto &v){original::native_frame_misc_reference_125790(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_125790(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_TAG_ITERATOR");
    std::mt19937 rng(0x125790u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,iterator::coverage().load(),unsigned(iterator::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

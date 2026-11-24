// Native TU1.5 effect event parts (17BEB0) against the unmodified translated
// body of the death cycle unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children 158290 (the owner's classification, the unit's dispatch
// route), 17BD40 (makes a part's element) and 17BB10 (creates the part) are
// genuine stand-ins whose effects are a hash of their complete entry ABI,
// stack arguments and return address: they return EAX (158290 often 0,
// 17BD40 an element or 0), ECX/EDX, flags and XMM, write scratch, and
// occasionally clobber callee-saved registers or leave the stack
// unbalanced. Fixtures author effects (tags, current events with no, one
// and many parts, negative event indices, owners NONE or not, the
// environment flag, sixteen slots naming some of the parts), parts (active
// or not, every environment and mode, types NONE or not), the local player
// count and the element array; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_effect_parts.hpp"
using namespace frame_misc_harness;
namespace parts=halo2::native_effect_parts_detail;
namespace {
constexpr std::uint32_t effect_array=0x10000000u,effect_data=0x10010000u,instances=0x10100000u,tags=0x10200000u,
    events=0x10300000u,part_data=0x10400000u,player_block=0x10500000u,element_array=0x10600000u,
    element_data=0x10610000u,scratch=0x10700000u,none=0xffffffffu;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=0;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    if(target==0x158290u)s.gp[0]=rng()%3==0?std::uint32_t(rng()):std::uint32_t(rng()%2);
    else if(target==0x17bd40u)s.gp[0]=rng()%4==0?0u:element_data+0x54u*std::uint32_t(rng()%16);
    else s.gp[0]=std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        s.gp[saved[rng()%4]]=rng()%2?part_data+0x38u*std::uint32_t(rng()%8):std::uint32_t(rng()%3);
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
    if(target!=0x158290u)throw std::runtime_error("Unbound death cycle dispatch target");
    stub(s,0x158290u,0);
}
static void run_17bd40(WorldState &s){stub(s,0x17bd40u,1);}
static void run_17bb10(WorldState &s){stub(s,0x17bb10u,5);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=effect_array;at<effect_array+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(0x4f720cu,effect_array);m.put(effect_array+0x44u,effect_data);
    m.put(0x4f0414u,instances);
    for(std::uint32_t at=instances;at<instances+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    // Four tags, each with four events of up to eight parts.
    for(std::uint32_t t=0;t<4;++t) {
        const auto tag=tags+0x40u*t;
        m.put(instances+16u*t+8u,tag);
        for(std::uint32_t at=tag;at<tag+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(tag+0x18u,events+0x38u*(18u*t+4u));
    }
    for(std::uint32_t at=events;at<events+0x1000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t e=0;e<0x1000u/0x38u;++e) {
        const auto event=events+0x38u*e;
        std::uint32_t count=pick(4)==0?0u:1u+pick(8);
        if(shape==5)count=0x80000000u|pick(4);
        m.put(event+0x30u,count);m.put(event+0x34u,part_data+0x38u*pick(8));
    }
    for(std::uint32_t at=part_data;at<part_data+0x38u*24u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t p=0;p<24;++p) {
        const auto part=part_data+0x38u*p;
        m.put(part+8u,pick(5)==0?none:std::uint32_t(rng()));
        m.put(part+0xcu,(std::uint32_t(rng())&0xffff0000u)|(pick(3)?(pick(2)?pick(4):pick(4)<<8):0u));
        m.put(part+0x10u,(pick(4)<<16)|pick(4));
    }
    for(std::uint32_t at=effect_data;at<effect_data+0x190u*8u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k) {
        const auto effect=effect_data+0x190u*k;
        m.put(effect,(std::uint32_t(rng())&0xff00ffffu)|((rng()%2?0x20u:0u)|(std::uint32_t(rng())&0xdfu))<<16);
        m.put(effect+4u,(std::uint32_t(rng())<<16)|pick(4));
        m.put(effect+0x58u,pick(3)==0?none:std::uint32_t(rng()));
        m.put(effect+0x5cu,(std::uint32_t(rng())<<16)|(pick(8)==0?0x10000u-1u-pick(2):pick(4)));
        for(std::uint32_t slot=0;slot<16;++slot) {
            m.put(effect+0x110u+8u*slot,pick(3)==0?part_data+0x38u*pick(24):std::uint32_t(rng()));
            m.put(effect+0x114u+8u*slot,(std::uint32_t(rng())<<16)|pick(16));
        }
    }
    m.put(0x4f54f0u,player_block);
    for(std::uint32_t at=player_block;at<player_block+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(player_block+8u,(std::uint32_t(rng())<<16)|(pick(3)?1u:pick(4)));
    m.put(0x51df74u,element_array);
    for(std::uint32_t at=element_array;at<element_array+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(element_array+0x44u,pick(9)==0?0u-0x54u*pick(16):element_data);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x17c000u+pick(0x100));
    m.put(stack+4u,(std::uint32_t(rng())<<16)|pick(8));
    m.put(stack+8u,std::bit_cast<std::uint32_t>(float(pick(100))*0.01f));m.put(stack+12u,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{effect_array,effect_data,instances,tags,events,part_data,player_block,
            element_array,0x4f7000u,0x4f5000u};
        m.drop(regions[pick(10)]);
    }
    return c;
}
bool native(WorldState &s) {
    return parts::bridge(s,[](auto &v){original::native_frame_misc_reference_17beb0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},
        [](auto &v){original::run_17bd40(v);},[](auto &v){original::run_17bb10(v);});
}
void reference(WorldState &s){original::run_17beb0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_EFFECT_PARTS");
    std::mt19937 rng(0x17beb0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,parts::coverage().load(),unsigned(parts::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

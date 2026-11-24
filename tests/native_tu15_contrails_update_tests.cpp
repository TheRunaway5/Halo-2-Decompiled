// Native TU1.5 contrails update (17EAD0) against the unmodified translated
// body of the contrail geometry unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 16EA60, BCB70, 17EF70, 17EED0, 17EE10, 17F710 and 16E8A0 (through the
// dispatch) and 17F3D0 (direct) are hash-driven stubs of their complete
// entry ABIs: each hashes what it reads (its pushed arguments), writes
// scratch and its outputs (BCB70 the scale, 17EE10 the frame time, 17F3D0
// the point slots), and 16EA60 answers the first index or NONE, BCB70 AL,
// 17EED0 a count; the caller-saved registers, flags and (by policy) MXCSR
// are random, the XMM registers random (17EE10 changes only the upper lanes
// of XMM1 to XMM5, which the original keeps across it, and lane 0 of the
// others), and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author arrays of contrails, active or not, with and
// without objects, points and definitions of every speed, rate and flag;
// the direction flag is set now and then; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_contrails_update.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_contrails_update_detail;
namespace {
constexpr std::uint32_t array_header=0x10000000u,contrail_data=0x10001000u,active_bits=0x10002000u,instances=0x10003000u,
    definitions=0x10004000u,scratch=0x10600000u;
constexpr std::uint32_t contrail_count=6u;
std::uint32_t bits(float f){std::uint32_t v;std::memcpy(&v,&f,4);return v;}
std::uint32_t grid(std::mt19937_64 &rng){return rng()%13?bits(float(int(rng()%9)-4)/2.0f):0x7fc00000u|std::uint32_t(rng()%0x100);}
std::uint32_t arguments(std::uint32_t target){switch(target){case 0xbcb70u:return 3u;case 0x17ef70u:case 0x17f3d0u:return 2u;case 0x17eed0u:return 1u;default:return 0u;}}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto args=arguments(target);
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto arg=[&](unsigned k){return r(s.memory,s.gp[4]+4u*k);};
    if(target==0xbcb70u && rng()%2)w(s.memory,s.observe,target+1u,arg(3),4,grid(rng));
    if(target==0x17ee10u && rng()%3)w(s.memory,s.observe,target+1u,s.gp[0]+0x28u,4,bits(float(rng()%2)/16.0f));
    if(target==0x17f3d0u && rng()%3==0) {
        const auto contrail=contrail_data+0x48u*(arg(1)&0xffffu)%(0x48u*contrail_count);
        w(s.memory,s.observe,target+1u,contrail+0x38u+4u*std::uint32_t(rng()%4),4,rng()%2?0xffffffffu:std::uint32_t(rng()));
    }
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x16ea60u)s.gp[0]=rng()%5?std::uint32_t(rng()%contrail_count):0xffffffffu;
    if(target==0xbcb70u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    if(target==0x17eed0u)s.gp[0]=std::uint32_t(rng()%4);
    if(target!=0x17ee10u) {
        if(policy&2u && rng()%13==0)s.gp[3]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%contrail_count);
        if(policy&2u && rng()%13==0)s.gp[5]=rng()%2?array_header:definitions+0x40u*std::uint32_t(rng()%4);
        if(policy&2u && rng()%13==0)s.gp[6]=contrail_data+0x48u*std::uint32_t(rng()%contrail_count);
        if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng()%contrail_count);
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    if(target==0x17ee10u) {
        // The loop's registers keep lane 0; their upper lanes and the other registers change.
        const unsigned n=unsigned(rng()%8);
        std::array<std::uint32_t,4> lanes;std::memcpy(lanes.data(),&s.xmm[n],16);
        if(n==0u||n>=6u)lanes[0]=grid(rng);
        // Now and then the rest shrinks, zero rises or the elapsed time changes: bounded, as the loop needs.
        if(rng()%6==0) {
            if(n==2u){float rest;std::memcpy(&rest,&lanes[0],4);lanes[0]=bits(rest/2.0f);}
            if(n==4u)lanes[0]=bits(float(rng()%5)/2.0f);
            if(n==5u)lanes[0]=grid(rng);
        }
        lanes[1+rng()%3]=std::uint32_t(rng());
        std::memcpy(&s.xmm[n],lanes.data(),16);
    } else {
        std::array<std::uint32_t,4> lanes{grid(rng),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
        std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    }
    std::uint32_t cleanup=0u;
    if(policy&2u && target!=0x17ee10u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*args+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_17f3d0(WorldState &s){stub(s,0x17f3d0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    static constexpr std::uint32_t known[]{0x16ea60u,0xbcb70u,0x17ef70u,0x17eed0u,0x17ee10u,0x17f710u,0x16e8a0u};
    bool ok=false;for(auto k:known)ok|=k==target;
    if(!ok)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    std::mt19937_64 floats(rng());
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x4f7214u,array_header);m.put(0x4f0414u,instances);m.put(0x46a0d0u,bits(1.0f));
    for(std::uint32_t at=array_header;at<array_header+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(array_header+0x24u,0x48u);m.put(array_header+0x38u,chance(6)?0u:contrail_count-pick(2));
    m.put(array_header+0x44u,contrail_data);m.put(array_header+0x48u,active_bits);
    m.put(active_bits,std::uint32_t(rng())&0x3fu);m.put(active_bits+4u,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<contrail_count;++k) {
        const auto e=contrail_data+0x48u*k;
        for(std::uint32_t at=e;at<e+0x48u;at+=4)m.put(at,grid(floats));
        m.put(e,std::uint32_t(rng()),2);
        m.put(e+2u,std::uint32_t(rng())&0xfeu|pick(2),1);
        m.put(e+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
        m.put(e+8u,chance(3)?0xffffffffu:std::uint32_t(rng()));
        m.put(e+0x10u,std::uint32_t(rng()));
        m.put(e+0x14u,chance(8)?grid(floats):bits(float(1+pick(4))/2.0f));
        m.put(e+0x28u,bits(float(pick(2))/16.0f));
        for(std::uint32_t slot=0;slot<4;++slot)m.put(e+0x38u+4u*slot,chance(2)?0xffffffffu:std::uint32_t(rng()));
    }
    for(std::uint32_t at=instances;at<instances+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(instances+16u*k+8u,definitions+0x40u*pick(4));
    for(std::uint32_t k=0;k<4;++k) {
        const auto d=definitions+0x40u*k;
        for(std::uint32_t at=d;at<d+0x40u;at+=4)m.put(at,grid(floats));
        m.put(d+2u,chance(2)?0x20u:0u,1);m.put(d+3u,pick(4),1);
        if(chance(3))m.put(d+0x2cu,bits(float(1+pick(4))));
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,grid(floats));
    m.put(stack,0x17f000u+pick(0x100));m.put(stack+4u,bits(float(pick(9))/4.0f));
    random_abi(rng,c,stack);
    for(auto &lane:c.abi.xmm)for(auto &b:lane)b=grid(floats);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{array_header,contrail_data,active_bits,instances,definitions};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_17ead0(v);},
        [](auto &v,std::uint32_t target){if(target!=0x17f3d0u)throw std::logic_error("Unbound contrails update child");original::run_17f3d0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_17ead0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CONTRAILS_UPDATE");
    std::mt19937 rng(0x17ead0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

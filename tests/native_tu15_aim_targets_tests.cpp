// Native TU1.5 aim target chain (1AE2B0) against the unmodified translated
// body of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children 1AE500 (is it a target) and 1AE7F0 (add it) are genuine
// stand-ins whose effects are a hash of their complete entry ABI, return
// address and argument words: 1AE500 returns AL (mostly a target) and may
// write its result byte through the slot pointer it gets; 1AE7F0 returns a
// count (one or two more, 64 and over, negative), may cut or redirect a
// target's attached chain and may switch the object header table (to a
// second table with other types and flags); both return EAX/ECX/EDX, flags,
// XMM and MXCSR sticky bits, write scratch, and occasionally clobber
// callee-saved registers or leave the stack unbalanced. The recursion runs
// through the bridge first, as the bound unit does. Fixtures author the
// header table (types 0, 1, 7, others, and types past 31 that alias them by
// the shift count), skip flags on headers and objects, sibling chains with
// attached chains up to three deep (a cycle in one shape, bounded by the
// count), unit values on both sides of the limit, spheres behind, beyond,
// outside and inside the cone, special floats with distinct NaN payloads
// and handles past the table; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_aim_targets.hpp"
#include <algorithm>
#include <functional>
#include <numeric>
using namespace frame_misc_harness;
namespace walk=halo2::native_aim_targets_detail;
namespace {
constexpr std::uint32_t headers=0x10100000u,headers_alt=0x10100100u,rows=0x10110000u,rows_alt=0x10120000u,
    objects=0x10200000u,vectors=0x10400000u,scratch=0x10600000u;
constexpr std::uint32_t object_count=64u,stride=0x300u,none=0xffffffffu;
std::uint32_t stack=0x20010000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const unsigned words=target==0x1ae500u?4u:7u;
    for(unsigned k=0;k<=words;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x1ae501u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    std::uint32_t result=std::uint32_t(rng());
    if(target==0x1ae500u) {
        if(rng()%4==0)w(s.memory,s.observe,0x1ae502u,r(s.memory,s.gp[4]+0x10u),1,std::uint32_t(rng()%3));
        result=(result&0xffffff00u)|(rng()%8<5?1u+std::uint32_t(rng()%255):0u);
    } else {
        const auto count=r(s.memory,s.gp[4]+0x18u);
        const auto roll=rng()%32;
        if(std::int32_t(count)<0 || std::int32_t(count)>=64)result=std::uint32_t(rng()%64);
        else if(roll==0)result=64u+std::uint32_t(rng()%4);
        else if(roll==1)result=0x80000000u|std::uint32_t(rng()%8);
        else if(roll<6)result=count+2u;
        else result=count+1u;
        const auto handle=r(s.memory,s.gp[4]+8u);
        if(rng()%6==0 && (handle&0xffffu)<object_count)
            w(s.memory,s.observe,0x1ae7f1u,objects+(handle&0xffffu)*stride+0x10u,4,
              rng()%2?none:(std::uint32_t(rng()%0x7fff)<<16)|std::uint32_t(rng()%object_count));
        if(rng()%9==0)w(s.memory,s.observe,0x1ae7f2u,0x4ecbd0u,4,rng()%2?headers:headers_alt);
    }
    s.gp[0]=result;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        s.gp[saved[rng()%4]]=rng()%3?std::uint32_t(rng()%64):0x80000000u|std::uint32_t(rng()%64);
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=4u*words;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%9);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
static void run_1ae500(WorldState &s){stub(s,0x1ae500u);}
static void run_1ae7f0(WorldState &s){stub(s,0x1ae7f0u);}
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t quiet_nan(std::mt19937 &rng) {
    static constexpr std::uint32_t payloads[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7fc12345u,0xffc54321u,0xff800003u};
    return payloads[rng()%6];
}
std::uint32_t special(std::mt19937 &rng,unsigned shape,float value) {
    if(shape==4 && rng()%7==0)return quiet_nan(rng);
    if(shape==3 && rng()%5==0) {
        static constexpr std::uint32_t values[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7f800000u,0xff800000u,0x00000001u,
            0x807fffffu,0x80000000u,0x00000000u};
        return values[rng()%9];
    }
    return std::bit_cast<std::uint32_t>(value);
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto real=[&](float low,float high){return low+(high-low)*float(pick(10001))/10000.0f;};
    const unsigned shape=n%16;
    stack=0x20010000u+pick(64)*4u;
    // The header tables: the live one and a second one a child may switch to.
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,rows);m.put(headers_alt+0x44u,rows_alt);
    const auto type=[&]()->std::uint32_t {
        static constexpr std::uint32_t candidates[]{0,1,7};
        if(pick(10)<7)return candidates[pick(3)];
        return pick(3)==0?32u+pick(16):pick(32);                 // past 31 the shift count wraps
    };
    for(std::uint32_t k=0;k<object_count+8u;++k)
        for(const auto table:{rows,rows_alt}) {
            const auto row=table+k*12u;
            m.put(row,std::uint32_t(rng()));
            const auto flags=std::uint32_t(rng())&0xffu;
            m.put(row+2u,pick(9)==0?flags|0x10u:flags&~0x10u,1);
            m.put(row+3u,type(),1);
            m.put(row+4u,std::uint32_t(rng()));
            m.put(row+8u,k<object_count?objects+k*stride:std::uint32_t(rng()));
        }
    // Objects: spheres around the aim ray, skip flags, unit values.
    std::vector<std::uint32_t> salt(object_count);
    for(auto &v:salt)v=std::uint32_t(rng()%0x7fff);
    const auto handle=[&](std::uint32_t k){return (salt[k]<<16)|k;};
    for(std::uint32_t k=0;k<object_count;++k) {
        const auto o=objects+k*stride;
        for(std::uint32_t at=o;at<o+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        const auto flags=std::uint32_t(rng())&0xffu;
        m.put(o+4u,pick(7)==0?flags|1u:flags&~1u,1);
        m.put(o+0xcu,none);m.put(o+0x10u,none);
        const float along=pick(8)==0?real(-12.0f,0.0f):real(-1.0f,70.0f);
        m.put(o+0x30u,special(rng,shape,along));
        m.put(o+0x34u,special(rng,shape,real(-1.0f,1.0f)*(1.0f+along*0.4f)));
        m.put(o+0x38u,special(rng,shape,real(-1.0f,1.0f)*(1.0f+along*0.3f)));
        m.put(o+0x3cu,special(rng,shape,real(0.05f,3.0f)));
        m.put(o+0x2b0u,special(rng,shape,real(0.0f,2.0f)));
    }
    // The chains: siblings, each with an attached chain up to three deep.
    std::vector<std::uint32_t> order(object_count);
    std::iota(order.begin(),order.end(),0u);
    std::shuffle(order.begin(),order.end(),rng);
    std::size_t used=0;
    std::uint32_t root_last=none;
    std::function<std::uint32_t(unsigned)> chain=[&](unsigned depth)->std::uint32_t {
        const unsigned length=1u+pick(depth?3:8);
        std::uint32_t first=none,previous=none;
        for(unsigned i=0;i<length && used<object_count;++i) {
            const auto k=order[used++];
            if(previous==none)first=handle(k);
            else m.put(objects+previous*stride+0xcu,handle(k));
            if(depth<3 && pick(3)==0)m.put(objects+k*stride+0x10u,chain(depth+1));
            previous=k;
        }
        if(!depth)root_last=previous;
        return first;
    };
    std::uint32_t first=chain(0);
    if(shape==12 && pick(2))first=(salt[0]<<16)|(object_count+pick(4));  // past the table
    // A cycle: the last root object's attached chain is the root chain.
    if(shape==13 && root_last!=none)m.put(objects+root_last*stride+0x10u,first);
    m.put(0x46a0d0u,special(rng,shape,real(0.5f,1.5f)));
    m.put(0x46a11cu,shape==4&&pick(3)==0?quiet_nan(rng):std::bit_cast<std::uint32_t>(real(0.5f,2.5f)));
    // The ray: from near the origin, mostly along +x.
    const auto point=vectors,direction=vectors+0x40u;
    for(unsigned k=0;k<3;++k)m.put(point+4u*k,special(rng,shape,real(-1.0f,1.0f)));
    m.put(direction,special(rng,shape,real(0.6f,1.0f)));
    m.put(direction+4u,special(rng,shape,real(-0.4f,0.4f)));m.put(direction+8u,special(rng,shape,real(-0.4f,0.4f)));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x6000u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1ae282u);
    m.put(stack+0x4u,std::uint32_t(rng()));                               // search
    m.put(stack+0x8u,first);
    m.put(stack+0xcu,point);m.put(stack+0x10u,direction);
    m.put(stack+0x14u,special(rng,shape,real(5.0f,60.0f)));               // max distance
    m.put(stack+0x18u,special(rng,shape,real(0.0f,4.0f)));                // width
    m.put(stack+0x1cu,special(rng,shape,real(0.2f,1.3f)));                // spread
    m.put(stack+0x20u,std::uint32_t(rng()));m.put(stack+0x24u,std::uint32_t(rng()));
    std::uint32_t count=pick(40);
    if(shape==9)count=60u+pick(8);
    if(shape==10)count=0x80000000u|pick(8);
    m.put(stack+0x28u,(std::uint32_t(rng())&0xffffff00u&(shape==11?~0u:0u))|count);
    m.put(stack+0x2cu,std::uint32_t(rng()));                              // add context
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,rows,objects+0x3000u,objects,vectors,0x46a000u,0x4ec000u,rows_alt};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s);
void chain(WorldState &s){if(!native(s))original::run_1ae2b0(s);}
bool native(WorldState &s) {
    return walk::bridge(s,[](auto &v){original::native_frame_misc_reference_1ae2b0(v);},
        [](auto &v){original::run_1ae500(v);},[](auto &v){original::run_1ae7f0(v);},[](auto &v){chain(v);});
}
void reference(WorldState &s){original::run_1ae2b0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_AIM_TARGETS");
    std::mt19937 rng(0x1ae2b0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,walk::coverage().load(),unsigned(walk::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

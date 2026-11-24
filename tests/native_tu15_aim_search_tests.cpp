// Native TU1.5 weapon targeting search (1AE090) against the unmodified
// translated body of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The stack probe 32A660 (the unit's dispatch route) moves ESP by EAX and
// reads the probed words; the children 14D200 (the cluster flood), BD460
// (the object query), 16EA60 (the player iterator) and 1AE2B0 (the target
// chain) are genuine stand-ins whose effects are a hash of their complete
// entry ABI, return address and argument words: the flood and the query
// write their lists and return AX counts (zero, negative, upper bits set),
// the query advances the stamp and marks what it found, the iterator steps
// through the players or ends, the chain returns a count (one more, 64 and
// over); all return EAX/ECX/EDX, flags, XMM and MXCSR sticky bits, write
// scratch, and occasionally clobber callee-saved registers or leave the
// stack unbalanced. Fixtures author the search's angle/range pairs in every
// order (equal, non-positive, special floats, NaN payloads, angles past the
// x87 trig range), the x87 control word, the player table (records without
// a unit, a null first record), object headers with parent chains, the
// query marks and stamp; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_aim_search.hpp"
using namespace frame_misc_harness;
namespace aim=halo2::native_aim_search_detail;
namespace {
constexpr std::uint32_t search=0x10000000u,players=0x10100000u,records=0x10110000u,headers=0x10200000u,
    rows=0x10210000u,objects=0x10300000u,scratch=0x10600000u;
constexpr std::uint32_t object_count=64u,object_stride=0x40u,record_stride=0x21cu,none=0xffffffffu;
std::uint32_t stack=0x20010000u,player_count=4u;
void stub(WorldState &s,std::uint32_t target,unsigned words) {
    std::uint64_t h=entry_hash(s,target);
    for(unsigned k=0;k<=words;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    std::uint32_t result=std::uint32_t(rng());
    const auto count16=[&](std::uint32_t n) {
        const auto roll=rng()%16;
        if(roll==0)n=0x8000u|std::uint32_t(rng()%4);                // negative as 16 bits
        result=(roll==1?result&0xffff0000u:std::uint32_t(rng()%2)<<16)|(n&0xffffu);
    };
    if(target==0x14d200u) {
        const auto output=r(s.memory,s.gp[4]+0x18u);
        const auto n=std::uint32_t(rng()%9);
        for(std::uint32_t k=0;k<n;++k)w(s.memory,s.observe,0x14d201u,output+2u*k,2,std::uint32_t(rng()%32));
        count16(n);
    } else if(target==0xbd460u) {
        const auto output=r(s.memory,s.gp[4]+0x14u);
        const auto stamp=r(s.memory,0x4eabccu)+1u;
        w(s.memory,s.observe,0xbd461u,0x4eabccu,4,stamp);
        const auto n=std::uint32_t(rng()%6);
        for(std::uint32_t k=0;k<n;++k) {
            const auto index=std::uint32_t(rng()%object_count);
            w(s.memory,s.observe,0xbd462u,output+4u*k,4,(std::uint32_t(rng()%0x7fff)<<16)|index);
            if(rng()%2)w(s.memory,s.observe,0xbd463u,0x4eabd0u+4u*index,4,stamp);
        }
        count16(n);
    } else if(target==0x16ea60u) {
        const auto start=s.gp[2];
        result=start<player_count && rng()%12?start+std::uint32_t(rng()%4==0):none;
        if(result!=none && result>=player_count)result=none;
    } else {
        const auto count=r(s.memory,s.gp[4]+0x28u);
        const auto roll=rng()%16;
        if(std::int32_t(count)<0 || std::int32_t(count)>=64)result=std::uint32_t(rng()%40);
        else if(roll==0)result=64u+std::uint32_t(rng()%3);
        else result=count+std::uint32_t(rng()%3);
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
// 32A660: ESP moves below the caller's by EAX; the probed words are read.
void probe(WorldState &s) {
    log_call(entry_hash(s,0x32a660u));
    const auto ret=r(s.memory,s.gp[4]),top=s.gp[4]+4u-s.gp[0];
    for(auto at=s.gp[4]+4u-0x1000u;at>top;at-=0x1000u)(void)r(s.memory,at);
    (void)r(s.memory,top);
    w(s.memory,s.observe,0x32a69fu,top-4u,4,ret);
    s.gp[4]=top;s.gp[0]=ret;
    s.flags.logic(ret&0x2410u,4);
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
long double native_fsin(long double x,std::uint16_t &status) {
    long double result;asm volatile("fsin\n\tfnstsw %1":"=t"(result),"=m"(status):"0"(x));return result;
}
long double native_fcos(long double x,std::uint16_t &status) {
    long double result;asm volatile("fcos\n\tfnstsw %1":"=t"(result),"=m"(status):"0"(x));return result;
}
std::uint32_t native_single_spill(long double x,std::uint16_t &status) {
    float result;std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1":"=m"(result),"=m"(actual):"m"(x):"st");
    status=(status&~0x200u)|(actual&0x200u);
    return std::bit_cast<std::uint32_t>(result);
}
static void run_14d200(WorldState &s){stub(s,0x14d200u,7);}
static void run_bd460(WorldState &s){stub(s,0xbd460u,5);}
static void run_16ea60(WorldState &s){stub(s,0x16ea60u,0);}
static void run_1ae2b0(WorldState &s){stub(s,0x1ae2b0u,11);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x32a660u)throw std::runtime_error("Unbound combat action dispatch target");
    probe(s);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t quiet_nan(std::mt19937 &rng) {
    static constexpr std::uint32_t payloads[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7fc12345u,0xffc54321u,0xff800003u};
    return payloads[rng()%6];
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto real=[&](float low,float high){return low+(high-low)*float(pick(10001))/10000.0f;};
    const unsigned shape=n%16;
    stack=0x20010000u+pick(64)*4u;
    // The search: angle/range pairs, sometimes equal, non-positive or special.
    std::uint32_t pair_values[8];
    for(unsigned k=0;k<8;++k) {
        const bool angle=k%2==0;
        float v=angle?real(0.02f,1.4f):real(0.5f,60.0f);
        if(pick(8)==0)v=-v;
        if(pick(10)==0)v=0.0f;
        pair_values[k]=std::bit_cast<std::uint32_t>(v);
        if(shape==6 && angle && pick(2))pair_values[k]=std::bit_cast<std::uint32_t>(real(1.0e19f,1.0e21f));  // past FSIN's range
        if(shape==4 && pick(5)==0)pair_values[k]=quiet_nan(rng);
        if(shape==3 && pick(4)==0) {
            static constexpr std::uint32_t special[]{0x7f800000u,0xff800000u,0x00000001u,0x807fffffu,0x80000000u,0x7f7fffffu};
            pair_values[k]=special[pick(6)];
        }
    }
    if(shape==5)for(unsigned k=1;k<8;k+=2)pair_values[k]=std::bit_cast<std::uint32_t>(-real(0.0f,5.0f));   // no reach
    if(shape==8)pair_values[3]=pair_values[7];                                                          // equal ranges
    if(shape==9)pair_values[2]=pair_values[6];                                                          // equal angles
    for(unsigned k=0;k<8;++k)m.put(search+4u*k,pair_values[k]);
    // Players and their units.
    player_count=pick(6);
    m.put(0x4f54f4u,players);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x24u,record_stride);m.put(players+0x44u,shape==7?0u:records);
    for(std::uint32_t k=0;k<6;++k) {
        const auto record=records+k*record_stride;
        m.put(record,std::uint32_t(rng()));
        m.put(record+0x2cu,pick(4)==0?none:(std::uint32_t(rng()%0x7fff)<<16)|pick(object_count));
    }
    // Object headers and parent chains (a parent has a lower index).
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,rows);
    for(std::uint32_t k=0;k<object_count;++k) {
        m.put(rows+k*12u,std::uint32_t(rng()));m.put(rows+k*12u+4u,std::uint32_t(rng()));
        m.put(rows+k*12u+8u,objects+k*object_stride);
        const auto o=objects+k*object_stride;
        for(std::uint32_t at=o;at<o+object_stride;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(o+0x14u,k && pick(3)==0?(std::uint32_t(rng()%0x7fff)<<16)|pick(k):none);
    }
    // The query stamp and marks: some roots already carry the next stamp.
    const auto stamp=std::uint32_t(rng());
    m.put(0x4eabc8u,std::uint32_t(rng()));m.put(0x4eabccu,stamp);
    for(std::uint32_t k=0;k<object_count;++k)m.put(0x4eabd0u+4u*k,pick(6)==0?stamp+1u:std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x3000u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x180000u+pick(0x100));
    m.put(stack+0x4u,search);
    for(std::uint32_t k=2;k<=7;++k)m.put(stack+4u*k,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{search,players,records,headers,rows,objects,0x4eabd0u,0x4f5000u};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return aim::bridge(s,[](auto &v){original::native_frame_misc_reference_1ae090(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},
        [](auto &v){original::run_14d200(v);},[](auto &v){original::run_bd460(v);},
        [](auto &v){original::run_16ea60(v);},[](auto &v){original::run_1ae2b0(v);});
}
void reference(WorldState &s){original::run_1ae090(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_AIM_SEARCH");
    std::mt19937 rng(0x1ae090u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,aim::coverage().load(),unsigned(aim::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

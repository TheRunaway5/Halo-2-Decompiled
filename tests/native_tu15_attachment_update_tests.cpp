// Native TU1.5 per-object attachment update (D7230) against the unmodified
// translated body of the model render unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Every child (the descriptor's +0x30 update callback, 41480 and the +0x34
// mode callback) is a genuine stand-in whose effects are a hash of its
// complete entry ABI, its return address and the argument words: a store,
// EAX/ECX/EDX results, occasional EBX/EBP/ESI/EDI clobbers, its own stack
// cleanup (sometimes unbalanced), flags, XMM and MXCSR sticky bits. Any entry
// difference therefore diverges. Fixtures author the object table, the
// attachment data array and type descriptors: empty and long acyclic chains,
// handle upper-bit garbage, negative and large type words, null callbacks,
// modes 0..3 with upper-bit garbage and both request flags, plus fault
// injection (one region removed).
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_attachment_update.hpp"
using namespace frame_misc_harness;
namespace update=halo2::native_attachment_update_detail;
namespace {
constexpr std::uint32_t objects=0x10000000u,object_data=0x10100000u,pool=0x10200000u,records=0x10300000u,
    descriptors=0x4739f8u,scratch=0x10600000u,stack=0x20010000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]));
    const unsigned words=target==0x41480u?1u:(target>=0x30000000u?2u:8u);
    for(unsigned k=1;k<=words;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x41481u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%23==0){static constexpr unsigned saved[]{3,5,6,7};s.gp[saved[rng()%4]]=std::uint32_t(rng());}
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    // RET n: its own argument cleanup, occasionally unbalanced.
    std::uint32_t cleanup=4u*words;
    if(policy&2u && rng()%31==0)cleanup=4u*std::uint32_t(rng()%4);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool){stub(s,target);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    std::mt19937_64 wide(rng());
    m.put(0x4ecbd0u,objects);m.put(objects+0x44u,objects+0x100u);
    m.put(0x4ecbf0u,pool);m.put(pool+0x44u,records);
    for(std::uint32_t at=objects;at<objects+0x44u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=pool;at<pool+0x44u;at+=4)m.put(at,std::uint32_t(rng()));
    const std::uint32_t object=pick(64);
    // Indices with bit 15 set (upper half of the 16-bit handle) use a
    // sparse record beyond the dense ones.
    const std::uint32_t index=shape==7?0x8000u|object:object;
    for(std::uint32_t k=0;k<64;++k) {
        m.put(objects+0x100u+k*12u,std::uint32_t(rng()));m.put(objects+0x104u+k*12u,std::uint32_t(rng()));
        m.put(objects+0x108u+k*12u,object_data+k*0x100u);
    }
    if(index!=object) {
        m.put(objects+0x100u+index*12u,std::uint32_t(rng()));m.put(objects+0x104u+index*12u,std::uint32_t(rng()));
        m.put(objects+0x108u+index*12u,object_data+object*0x100u);
    }
    for(std::uint32_t at=object_data;at<object_data+64u*0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    // Acyclic chain of distinct record indices.
    std::uint32_t length=pick(9);
    if(shape==0)length=0;
    if(shape==1)length=12+pick(20);
    std::vector<std::uint32_t> chain;
    for(std::uint32_t k=0;k<length;++k) {
        std::uint32_t index;
        do index=pick(200);while(std::find(chain.begin(),chain.end(),index)!=chain.end());
        chain.push_back(index);
    }
    const auto handle=[&](std::uint32_t index){return (shape==2?(std::uint32_t(rng())<<16):(pick(0x8000u)<<16))|index;};
    m.put(object_data+object*0x100u+0xdcu,chain.empty()?0xffffffffu:handle(chain[0]));
    const int low=shape==3?-6:0,high=shape==4?40:6;
    for(std::uint32_t k=0;k<200;++k) {
        const auto record=records+k*12u;
        m.put(record,std::uint32_t(rng()),2);
        m.put(record+2u,std::uint32_t(low+int(pick(std::uint32_t(high-low)))),2);
        m.put(record+4u,std::uint32_t(rng()));
        m.put(record+8u,std::uint32_t(rng()));
    }
    for(std::size_t k=0;k<chain.size();++k)
        m.put(records+chain[k]*12u+8u,k+1<chain.size()?handle(chain[k+1]):0xffffffffu);
    for(int type=low;type<high;++type) {
        const auto d=descriptors+std::uint32_t(type)*0x38u;
        for(std::uint32_t at=d;at<d+0x38u;at+=4)m.put(at,std::uint32_t(rng()));
        if(pick(4)==0)m.put(d+0x28u,0);
        if(pick(4)==0)m.put(d+0x34u,0);
        else m.put(d+0x34u,0x30000000u+pick(0x1000u));
        m.put(d+0x30u,0x20000000u+pick(0x1000u));
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,0);
    for(std::uint32_t at=stack-0x2000u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x41c40u+pick(0x40));
    std::uint32_t flags=std::uint32_t(rng());
    if(shape==5)flags&=~0x3000u;
    if(shape==6)flags|=0x3000u;
    m.put(stack+4u,flags);
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|index;
    static constexpr std::uint32_t modes[]{0,1,2,3,0x10001u,0x20002u,0xffff0002u,0x7u};
    c.abi.gp[3]=(std::uint32_t(rng())&0xffff0000u)|modes[pick(8)];
    if(shape<8)c.abi.gp[3]=(c.abi.gp[3]&0xffff0000u)|(1u+pick(2));
    if(shape==15) {
        static constexpr std::uint32_t regions[]{objects,object_data,pool,records,descriptors,scratch};
        const auto region=regions[pick(6)];
        m.drop(region+(region==object_data?object*0x100u:0u));
    }
    return c;
}
bool native(WorldState &s) {
    return update::bridge(s,[](auto &v){original::native_frame_misc_reference_d7230(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_d7230(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ATTACHMENT_UPDATE");
    std::mt19937 rng(0xd7230u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,update::coverage().load(),unsigned(update::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

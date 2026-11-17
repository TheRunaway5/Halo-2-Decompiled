// Native TU1.5 render-model attachment scheduling (41970) against the
// unmodified translated body of the model render unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Every child is a genuine stand-in whose effects are a hash of its complete
// entry ABI, its return address and its argument words: 32A660 allocates the
// frame as the original probe does (ESP = ESP + 4 - EAX); BAD60 returns a
// marker count in AX and writes marker records into the frame; C0700 returns
// XMM0; the others return EAX, clobber ECX/EDX (and sometimes EBX/EBP/ESI/
// EDI), clean up their own arguments (sometimes unbalanced), set flags, XMM
// and MXCSR sticky bits, and store into scratch and occasionally into the
// caller's frame, so every re-read after a call is exercised. Fixtures author
// the object table, model tags, attachment records ('lens', 'MGS2', 'tdtl'
// and other tags, -1 marker names), lens/MGS2 tag flags, the [474C70] vector,
// the lens limit byte and nonzero modes, plus fault injection.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_attachment_schedule.hpp"
using namespace frame_misc_harness;
namespace schedule=halo2::native_attachment_schedule_detail;
namespace {
bool many_markers{};
constexpr std::uint32_t objects=0x10000000u,object_data=0x10100000u,tags=0x10200000u,models=0x10300000u,
    attachments=0x10400000u,lens_tags=0x10500000u,vector=0x10600000u,scratch=0x10700000u,stack=0x20010000u;
unsigned cleanup_words(std::uint32_t target) {
    switch(target){case 0xbad60u:return 3;case 0x3e9b0u:return 8;case 0x2db80u:return 1;case 0x42840u:return 4;
        case 0x42990u:return 6;default:return 0;}
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]));
    const auto words=cleanup_words(target);
    for(unsigned k=1;k<=words;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x32a660u) {
        // The stack probe: the frame below the caller's return address.
        auto frame=s.gp[0];
        if(policy&2u && rng()%41==0)frame+=4u*std::uint32_t(rng()%3);
        s.gp[4]=s.gp[4]+4u-frame;
        s.flags.load(std::uint32_t(rng())&0x8d5u);
        return;
    }
    w(s.memory,s.observe,0x41481u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    const auto caller=s.gp[4]+4u+4u*words;
    if(target==0xbad60u) {
        // Marker records at EBX (0x70 bytes each) and the count in AX.
        std::int32_t count=std::int32_t(many_markers?11u+rng()%2u:rng()%5);
        if(!many_markers && rng()%9==0)count=-1-std::int32_t(rng()%3);
        for(std::uint32_t k=0;k<(many_markers?12u:4u);++k)for(std::uint32_t at=0;at<0x70u;at+=4)
            w(s.memory,s.observe,0xbad61u,s.gp[3]+k*0x70u+at,4,rng()%5==0?special_float(rng):std::uint32_t(rng()));
        s.gp[0]=(std::uint32_t(rng())<<16)|(std::uint32_t(count)&0xffffu);
    } else s.gp[0]=std::uint32_t(rng());
    if(!many_markers && rng()%7==0)w(s.memory,s.observe,0x41482u,caller+std::uint32_t(rng()%0x50u)*4u,4,std::uint32_t(rng()%6));
    // EBP is the caller's attachment record: occasionally retag it, so the
    // marker loop's per-marker tag re-read is exercised.
    if(!many_markers && rng()%13==0 && s.gp[5]>=attachments && s.gp[5]<attachments+0x4000u) {
        static constexpr std::uint32_t kinds[]{0x6c656e73u,0x4d475332u,0x7464746cu,0x6e6f6e65u};
        w(s.memory,s.observe,0x41483u,s.gp[5],4,kinds[rng()%4]);
    }
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0){static constexpr unsigned saved[]{3,5,6,7};s.gp[saved[rng()%4]]=std::uint32_t(rng());}
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[target==0xc0700u?0u:std::uint32_t(rng()%8)],lanes.data(),16);
    std::uint32_t cleanup=4u*words;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
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
    many_markers=shape==4;
    std::mt19937_64 wide(rng());
    m.put(0x4ecbd0u,objects);m.put(objects+0x44u,objects+0x100u);
    m.put(0x4f0414u,tags);m.put(0x474c70u,vector);
    m.put(0x46a0d0u,rng()%5==0?special_float(wide):std::uint32_t(rng()));
    m.put(0x56e194u,std::uint32_t(rng()));
    if(shape==3 || (shape==4 && pick(2)))m.put(0x56e197u,0,1);
    for(std::uint32_t at=vector;at<vector+12u;at+=4)m.put(at,std::uint32_t(rng()));
    const std::uint32_t object=pick(32);
    for(std::uint32_t k=0;k<32;++k) {
        m.put(objects+0x100u+k*12u,std::uint32_t(rng()));m.put(objects+0x104u+k*12u,std::uint32_t(rng()));
        m.put(objects+0x108u+k*12u,object_data+k*0x40u);
        m.put(object_data+k*0x40u,(std::uint32_t(rng())<<16)|pick(16));
    }
    // Model tags 0..15 (+0x94 count, +0x98 attachments); lens/MGS2 tags 16..31.
    for(std::uint32_t k=0;k<32;++k) {
        m.put(tags+k*16u,std::uint32_t(rng()));m.put(tags+k*16u+4u,std::uint32_t(rng()));
        m.put(tags+k*16u+8u,k<16?models+k*0x100u:lens_tags+(k-16)*0x40u);m.put(tags+k*16u+12u,std::uint32_t(rng()));
    }
    for(std::uint32_t k=0;k<16;++k) {
        const auto model=models+k*0x100u;
        for(std::uint32_t at=model;at<model+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        std::int32_t count=std::int32_t(pick(6));
        if(shape==1)count=0;
        if(shape==2)count=-1-std::int32_t(pick(3));
        if(shape==4)count=8;
        m.put(model+0x94u,std::uint32_t(count));
        m.put(model+0x98u,attachments+k*0x400u);
        const std::uint32_t family=pick(4);
        for(std::uint32_t a=0;a<8u;++a) {
            const auto record=attachments+k*0x400u+a*0x18u;
            static constexpr std::uint32_t kinds[]{0x6c656e73u,0x4d475332u,0x7464746cu,0x6e6f6e65u};
            m.put(record,shape==4?kinds[0]:kinds[(family+pick(3))%4]);
            m.put(record+4u,shape!=4 && pick(6)==0?0xffffffffu:((std::uint32_t(rng())<<16)|(16u+pick(16))));
            m.put(record+8u,std::uint32_t(rng()));
            m.put(record+0xcu,pick(3)==0?0u:std::uint32_t(rng()),2);
            m.put(record+0xeu,std::uint32_t(rng()),2);
            m.put(record+0x10u,std::uint32_t(rng()));m.put(record+0x14u,std::uint32_t(rng()));
        }
    }
    for(std::uint32_t k=0;k<16;++k) {
        const auto tag=lens_tags+k*0x40u;
        for(std::uint32_t at=tag;at<tag+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(tag+8u,pick(3)==0?0u:pick(3)==0?0x80000000u:1u+pick(4));
        m.put(tag+0xcu,tag+0x30u);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,0);
    for(std::uint32_t at=stack-0x2000u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x41c40u+pick(0x40));
    m.put(stack+4u,shape==0?1u+pick(0xffffu):0u,2);
    m.put(stack+8u,(std::uint32_t(rng())<<16)|object);
    // The lens count slot starts near the 0x40 limit in some cases.
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{objects,object_data,tags,models,attachments,lens_tags,vector,0x56e000u};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return schedule::bridge(s,[](auto &v){original::native_frame_misc_reference_41970(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_41970(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ATTACHMENT_SCHEDULE");
    std::mt19937 rng(0x41970u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,schedule::coverage().load(),unsigned(schedule::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

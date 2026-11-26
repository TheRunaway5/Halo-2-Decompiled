// Native TU1.5 physics groups (1DDBF0) against the unmodified translated body
// of the live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The stack probe 32A660 moves ESP below the caller's by EAX and reads the
// probed words, as the CRT's does. 212C70, 184920, 1DDEA0, 2E31E0 (dispatch),
// 1D8330 and 1DED80 are hash-driven stubs of their complete entry ABIs: each
// hashes what it reads (its pushed arguments), writes scratch and its outputs
// (212C70 writes the physics model pointer at view+0x48, 1DDEA0 writes the
// step status, visited bits and group entry), answers AL or EAX; the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and by
// policy EBX, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author headers with and without flag bit 0 and kinds including bit
// 7 (0x80), models with zero/negative/positive body and node counts (including
// >32 nodes to exercise bitset word shifts), all six rigid-body motion kinds,
// keyframed/fixed overrides with -1, negative and positive node indices,
// capacities with and without bit 31, and the original's jump table at
// 0x1DDE84; one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_physics_groups.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_physics_groups_detail;
namespace {
constexpr std::uint32_t objects=0x10000000u,headers=0x10001000u,state_a=0x10002000u,state_b=0x10002400u,
    model_mem=0x10003000u,bodies_mem=0x10004000u,nodes_page=0x10005000u,nodes_mem=0x10005100u,scratch=0x10600000u;
constexpr std::uint32_t kind_targets[6]{0x1ddcb1u,0x1ddcb8u,0x1ddcbfu,0x1ddcc6u,0x1ddccdu,0x1ddcd4u};
std::uint32_t arguments(std::uint32_t target) {
    switch(target) {
    case 0x184920u:return 2u;
    case 0x1ddea0u:return 8u;
    case 0x2e31e0u:return 3u;
    case 0x1d8330u:return 4u;
    default:return 0u;
    }
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto args=arguments(target);
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto arg=[&](unsigned k){return r(s.memory,s.gp[4]+4u*k);};
    std::uint32_t answer=std::uint32_t(rng());
    if(target==0x212c70u) {
        const bool ok=(rng()%5)!=0;
        const auto view=s.gp[6];
        if(ok) {
            for(std::uint32_t f=0;f<0x48u;f+=4)w(s.memory,s.observe,target+1u,view+f,4,std::uint32_t(rng()));
            w(s.memory,s.observe,target+2u,view+0x48u,4,model_mem);
            answer=(answer&0xffffff00u)|(1u+std::uint32_t(rng()%255));
        } else {
            answer&=0xffffff00u;
        }
    } else if(target==0x184920u) {
        const auto mat=s.gp[6];
        for(std::uint32_t f=0;f<0x20u;f+=4)w(s.memory,s.observe,target+1u,mat+f,4,std::uint32_t(rng()));
    } else if(target==0x1ddea0u) {
        const auto step_out=arg(5),visited_ptr=arg(6),slot=arg(8);
        const std::uint32_t step_val=rng()%3==0?0u-std::uint32_t(1u+(rng()%3)):std::uint32_t(rng()%5);
        w(s.memory,s.observe,target+1u,step_out,4,step_val);
        if(rng()%2) {
            const auto bit=std::uint32_t(rng()%40);
            const auto waddr=visited_ptr+4u*(bit>>5);
            w(s.memory,s.observe,target+2u,waddr,4,r(s.memory,waddr)|(1u<<(bit&31u)));
        }
        const bool created=(rng()%5)<3;
        if(created) {
            for(std::uint32_t f=0;f<0x4cu;f+=4)w(s.memory,s.observe,target+3u,slot+f,4,std::uint32_t(rng()));
            answer=(answer&0xffffff00u)|(1u+std::uint32_t(rng()%255));
        } else {
            answer&=0xffffff00u;
        }
    }
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=answer;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%17==0)s.gp[3]=rng()%2?state_a:state_b;
    if(policy&2u && target==0x2e31e0u && rng()%3==0)s.gp[7]=rng()%2?0u:0xffffffffu;
    if(policy&2u && target==0x1ddea0u && rng()%17==0)s.gp[6]=std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+(target==0x2e31e0u?0u:4u*args)+cleanup;
}
// 32A660: ESP moves below the caller's by EAX; the probed words are read.
void probe(WorldState &s) {
    log_call(entry_hash(s,0x32a660u));
    const auto ret=r(s.memory,s.gp[4]),top=s.gp[4]+4u-s.gp[0];
    for(auto at=s.gp[4]+4u-0x1000u;at>top;at-=0x1000u)(void)r(s.memory,at);
    (void)r(s.memory,top);
    w(s.memory,s.observe,0x32a69fu,top-4u,4,ret);
    s.gp[4]=top;s.gp[0]=ret;
    s.flags.logic(ret&0x4e84u,4);
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_212c70(WorldState &s){stub(s,0x212c70u);}
static void run_184920(WorldState &s){stub(s,0x184920u);}
static void run_1ddea0(WorldState &s){stub(s,0x1ddea0u);}
static void run_1d8330(WorldState &s){stub(s,0x1d8330u);}
static void run_1ded80(WorldState &s){stub(s,0x1ded80u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target==0x32a660u)probe(s);
    else if(target==0x2e31e0u)stub(s,target);
    else throw std::runtime_error("Unbound live support dispatch target");
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const auto fill=[&](std::uint32_t from,std::uint32_t size){for(std::uint32_t at=from;at<from+size;at+=4)m.put(at,std::uint32_t(rng()));};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    const auto top=stack+4u-0x4e84u;
    m.put(0x4ecbd0u,objects);
    for(std::uint32_t k=0;k<6;++k)m.put(0x1dde84u+4u*k,kind_targets[k]);
    fill(objects,0x50u);m.put(objects+0x44u,headers);
    const auto header_index=[&]{return chance(4)?0x8000u+pick(4):pick(8);};
    for(std::uint32_t k=0;k<8;++k) {
        const auto h=headers+12u*k;
        fill(h,12u);
        m.put(h+2u,chance(3)?(std::uint32_t(rng())|1u):(std::uint32_t(rng())&0xfeu),1);
        m.put(h+3u,chance(3)?(7u+32u*pick(4)):pick(32),1);
    }
    for(std::uint32_t k=0;k<4;++k) {
        const auto h=headers+12u*(0x8000u+k);
        fill(h,12u);
        m.put(h+2u,chance(3)?(std::uint32_t(rng())|1u):(std::uint32_t(rng())&0xfeu),1);
        m.put(h+3u,chance(3)?7u:pick(32),1);
    }
    for(const auto st:{state_a,state_b}) {
        fill(st,0x100u);
        m.put(st+8u,(std::uint32_t(rng())&0xffff0000u)|header_index());
        const auto cap=chance(2)?pick(3):3u+pick(12);
        m.put(st+0x78u,cap|(chance(2)?0x80000000u:0u));
    }
    fill(model_mem,0x100u);
    const std::uint32_t body_count=chance(5)?(chance(2)?0u:0xffffffffu):1u+pick(6);
    const std::uint32_t node_count=chance(5)?(chance(2)?0u:0xffffffffu):(chance(6)?33u+pick(3):1u+pick(6));
    m.put(model_mem+0x38u,body_count);
    m.put(model_mem+0x3cu,bodies_mem);
    m.put(model_mem+0xc8u,node_count);
    m.put(model_mem+0xccu,nodes_mem);
    fill(bodies_mem,6u*0x90u);
    for(std::uint32_t k=0;k<6;++k) {
        const auto b=bodies_mem+0x90u*k;
        const std::uint32_t node_word=chance(4)?0xffffu:(chance(6)?0xfffeu:pick(8));
        m.put(b,node_word,2);
        m.put(b+0x1au,pick(6),2);
    }
    fill(nodes_page,0x200u);
    for(std::int32_t k=-2;k<8;++k) {
        const auto nd=std::uint32_t(std::int32_t(nodes_mem)+12*k);
        m.put(nd+4u,chance(2)?(std::uint32_t(rng())|1u):(std::uint32_t(rng())&0xfeu),1);
    }
    fill(scratch,0x100u);
    // Map the probed stack pages and randomize the active frame regions.
    for(std::uint32_t k=0;k<=5;++k)(void)m.page(stack-0x1000u*k,true);
    fill(top-0x80u,0x380u);
    fill(top+0xa20u,0x80u);
    fill(stack-0x40u,0x80u);
    for(std::uint32_t k=1;k<=4;++k)m.put(stack+4u-0x1000u*k,std::uint32_t(rng()));
    m.put(stack,0x1dd000u+pick(0x100));
    m.put(stack+4u,state_a);
    m.put(stack+8u,chance(2)?0u:1u+pick(255));
    m.put(stack+12u,chance(2)?0u:1u+pick(255));
    m.put(stack+16u,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t areas[]{objects,headers,state_a,model_mem,bodies_mem,nodes_mem,0x1dde84u};
        m.drop(areas[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1ddbf0(v);},
        [](auto &v,std::uint32_t target) {
            switch(target) {
            case 0x212c70u:original::run_212c70(v);break;
            case 0x184920u:original::run_184920(v);break;
            case 0x1ddea0u:original::run_1ddea0(v);break;
            case 0x1d8330u:original::run_1d8330(v);break;
            case 0x1ded80u:original::run_1ded80(v);break;
            default:throw std::logic_error("Unbound physics groups child");
            }},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1ddbf0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PHYSICS_GROUPS");
    std::mt19937 rng(0x1ddbf0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

// Native TU1.5 departure remap (19AE70) against the unmodified translated body
// of the player departure unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 32A660 (direct stack probe) moves ESP below the caller's by EAX and reads
// the probed words; 19DD10 (direct, RET 4) is a hash-driven stub of its
// complete entry ABI. Fixtures author inactive and locked departure states,
// active player tables with varying limits, empty salts, matching/duplicate/
// new players, non-zero DF occasionally (with surrounding pages mapped),
// populated roster/stats/matrix/event tables, and dropped pages for fault
// injection. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_departure_remap.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_departure_remap_detail;
namespace {
constexpr std::uint32_t players_hdr=0x10000000u,players_data=0x10002000u,scratch=0x10600000u;
void probe(WorldState &s) {
    log_call(entry_hash(s,0x32a660u));
    const auto ret=r(s.memory,s.gp[4]),top=s.gp[4]+4u-s.gp[0];
    for(auto at=s.gp[4]+4u-0x1000u;at>top;at-=0x1000u)(void)r(s.memory,at);
    (void)r(s.memory,top);
    w(s.memory,s.observe,0x32a69fu,top-4u,4,ret);
    s.gp[4]=top;s.gp[0]=ret;
    s.flags.logic(ret&0x4568u,4);
}
void finalize_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x19dd10u);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x19dd14u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    s.gp[4]+=8u;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_32a660(WorldState &s){probe(s);}
static void run_19dd10(WorldState &s){finalize_stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    // Fast page allocation for stack (including below top for DF=1) and global departure tables.
    for(std::uint32_t p=0x20017000u;p<=0x20021000u;p+=0x1000u)(void)m.page(p,true);
    for(std::uint32_t p=0x51a000u;p<=0x52c000u;p+=0x1000u)(void)m.page(p,true);
    (void)m.page(0x4f5000u,true);
    (void)m.page(players_hdr,true);
    for(std::uint32_t p=players_data-0x1000u;p<players_data+0x3000u;p+=0x1000u)(void)m.page(p,true);
    (void)m.page(scratch,true);

    // About 3/4 of cases hit early exit (keeps 8000-case runtime fast while 2000+ full remaps exercise every path).
    const bool run_full=(shape<4);
    const std::uint32_t active_val=(!run_full && shape%2==0)?0u:1u;
    const std::uint32_t locked_val=(!run_full && shape%2==1)?1u:0u;
    m.put(0x51dfa0u,active_val,1);
    m.put(0x51dfb1u,locked_val,1);
    m.put(0x4f54f4u,players_hdr);
    const std::uint32_t limit=chance(3)?4u+pick(12):16u;
    const std::uint32_t stride=0x100u;
    m.put(players_hdr+0x24u,stride);
    m.put(players_hdr+0x38u,limit);
    m.put(players_hdr+0x44u,players_data);

    // Four reusable identities so players hit matches, duplicates, unmatched, and new players.
    std::uint32_t ids[6][3];
    for(auto &id:ids)for(auto &w:id)w=std::uint32_t(rng());
    if(run_full) {
        for(std::uint32_t k=0;k<16u;++k) {
            const auto raddr=0x51e334u+0xa4u*k;
            const bool used=pick(3)!=0;
            m.put(raddr,used?1u:0u,1);
            const auto id_idx=pick(5);
            for(std::uint32_t w=0;w<3u;++w)m.put(raddr+2u+4u*w,ids[id_idx][w]);
            m.put(raddr+0x10u,std::uint32_t(rng()));
        }
        for(std::uint32_t k=0;k<16u;++k) {
            const auto paddr=players_data+stride*k;
            const std::uint32_t salt=chance(4)?0u:(1u+pick(65535));
            m.put(paddr,salt,2);
            const auto id_idx=chance(4)?5u:pick(5);
            for(std::uint32_t w=0;w<3u;++w)m.put(paddr+4u+4u*w,ids[id_idx][w]);
            m.put(paddr+0x44u,std::uint32_t(rng()));
        }
        for(std::uint32_t k=0;k<16u;++k)m.put(0x51eef4u+0x36au*k,std::uint32_t(rng()));
        for(std::uint32_t k=0;k<16u;++k)m.put(0x522594u+68u*k,std::uint32_t(rng()));
        for(std::uint32_t k=0;k<16u;++k) {
            const auto ev=0x522f35u+0x24u*k;
            const std::uint32_t v0=chance(3)?0xffu:(chance(3)?(0x10u+pick(16)):(chance(3)?(0xf0u+pick(15)):pick(16)));
            const std::uint32_t v1=chance(3)?0xffu:pick(16);
            m.put(ev,v0,1);m.put(ev+1u,v1,1);
        }
        // Also seed tail event record to test full 1000-record loop count.
        m.put(0x522f35u+0x24u*999u,pick(16),1);
        m.put(0x522f35u+0x24u*999u+1u,0xfeu,1);
    }
    m.put(stack,0x19a000u+pick(0x100));
    random_abi(rng,c,stack);
    // Keep DF=1 occasional on full runs too (all downward pages are mapped).
    if(shape==15) {
        m.put(0x51dfa0u,1u,1);m.put(0x51dfb1u,0u,1);
        static constexpr std::uint32_t areas[]{0x51dfa0u,players_hdr,players_data,0x51e334u,0x51eef4u,0x522594u,0x522f35u};
        m.drop(areas[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_19ae70(v);},
        [](auto &v){original::run_32a660(v);},
        [](auto &v){original::run_19dd10(v);});
}
void reference(WorldState &s){original::run_19ae70(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_DEPARTURE_REMAP");
    std::mt19937 rng(0x19ae70u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

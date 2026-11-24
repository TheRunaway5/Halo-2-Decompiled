// Native TU1.5 byte timer step (1E15D0) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author timers with no limit, done, and
// counting up or down near their ends (negative bytes too); one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_byte_timer.hpp"
using namespace frame_misc_harness;
namespace timer=halo2::native_byte_timer_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t timers=0x10000000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=timers;at<timers+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto timer=timers+4u*pick(8)+pick(4);
    static constexpr std::uint32_t limits[]{0u,1u,2u,5u,0x7fu,0x80u,0xffu};
    const auto limit=pick(5)?limits[pick(7)]:pick(256);
    m.put(timer+1u,limit,1);
    m.put(timer+3u,(pick(256)&~3u)|pick(4),1);
    // Counts at and around both ends.
    const auto count=pick(3)==0?pick(256):pick(2)?(limit+std::uint32_t(int(pick(3))-2))&0xffu:pick(3);
    m.put(timer,count,1);
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x110000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[1]=timer;
    if(shape==15)m.drop(timers);
    return c;
}
bool native(WorldState &s){return timer::bridge(s,[](auto &v){original::native_frame_misc_reference_1e15d0(v);});}
void reference(WorldState &s){original::run_1e15d0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_BYTE_TIMER");
    std::mt19937 rng(0x1e15d0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,timer::coverage().load(),unsigned(timer::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

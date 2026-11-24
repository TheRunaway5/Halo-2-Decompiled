// Native TU1.5 record sanity check (14FBC0) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author sane records and spoil one or two
// fields (infinite or NaN floats, values past their ranges, kinds past
// their limits or equal, the flags byte, the pair zero or not, links NONE),
// or none;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_record_sanity.hpp"
using namespace frame_misc_harness;
namespace sanity=halo2::native_record_sanity_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t records=0x10000000u;
std::uint32_t f32(float v){return std::bit_cast<std::uint32_t>(v);}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x46a0d0u,f32(1.0f));m.put(0x46a0dcu,pick(4)?f32(-1.0f):f32(float(int(pick(200))-100)/100.0f));
    const auto record=records+0x100u*pick(4);
    for(std::uint32_t at=records;at<records+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto unit=[&]{return f32(float(pick(1001))/1000.0f);};
    // A sane record.
    m.put(record+4u,f32(1.5f));m.put(record+8u,f32(-2.0f));m.put(record+0x14u,unit());m.put(record+0x18u,f32(3.0f));
    m.put(record+0xcu,unit());m.put(record+0x10u,unit());
    m.put(record+0x20u,pick(3)?pick(4):0xffu,1);m.put(record+0x21u,0xffu,1);
    m.put(record+0x22u,pick(3)?pick(2):0xffffu,2);m.put(record+0x24u,pick(3)?pick(4):0xffffu,2);
    m.put(record+0x1du,pick(2),1);
    m.put(record+0x50u,pick(2)?0u:unit());m.put(record+0x54u,pick(2)?0u:unit());
    m.put(record+0x4cu,pick(2),1);m.put(record+0x34u,pick(4)?std::uint32_t(rng()):0xffffffffu);
    m.put(record+0x28u,pick(9),2);m.put(record+0x2cu,pick(4)?std::uint32_t(rng()):0xffffffffu);
    // Spoil some fields.
    static constexpr std::uint32_t offsets[]{4u,8u,0xcu,0x10u,0x14u,0x18u,0x50u,0x54u};
    static constexpr std::uint32_t bad[]{0x7f800000u,0x7fc00000u,0xff800000u,0x3fc00000u,0xbf000000u,0x80000000u,0x00000001u,0x7f7fffffu};
    for(unsigned k=pick(3);k;--k) {
        switch(pick(7)) {
        case 5:m.put(record+0x1du,2u+pick(254),1);break;
        case 0:case 1:m.put(record+offsets[pick(8)],bad[pick(8)]);break;
        case 2:m.put(record+0x20u+pick(2),pick(3)?pick(8):0x80u|pick(4),1);break;
        case 3:m.put(record+0x21u,m.get(record+0x20u,1),1);break;
        case 4:m.put(record+0x22u+2u*pick(2),pick(2)?pick(8):0x8000u,2);break;
        default:m.put(record+0x28u,pick(2)?9u+pick(4):0x8000u,2);break;
        }
    }
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x140000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[1]=pick(16)==0?0u:record;
    if(shape==15)m.drop(pick(2)?records:0x46a000u);
    return c;
}
bool native(WorldState &s){return sanity::bridge(s,[](auto &v){original::native_frame_misc_reference_14fbc0(v);});}
void reference(WorldState &s){original::run_14fbc0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RECORD_SANITY");
    std::mt19937 rng(0x14fbc0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,sanity::coverage().load(),unsigned(sanity::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

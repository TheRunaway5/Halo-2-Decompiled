// Native TU1.5 player action search (153BA0) against the unmodified
// translated body of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Every child (the sphere query BD080 and the tests 1540D0, 153F20, 154270,
// 1541B0) is a genuine stand-in whose effects are a hash of its complete
// entry ABI, its return address and its argument words: the sphere query
// writes found handles into the caller's list (its sixth argument) and
// returns a count in AX (zero, negative, within and beyond the list's 64);
// every child returns EAX/ECX/EDX, flags, XMM and MXCSR sticky bits, writes
// scratch, the action context or the frame's pass and found slots, and
// occasionally clobbers callee-saved registers or leaves the stack
// unbalanced. Fixtures author the player and object tables, unit objects
// with and without a parent, found objects of every type (and out-of-range
// types), positions in and out of reach (with special floats), attached
// child and sibling chains (including cycles that fill the list), NaN
// radii and margins with distinct payloads (operand order), the two
// margin constants and the type case table (mapped, unmapped and unexpected
// targets), plus fault injection (one region removed).
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_player_action.hpp"
using namespace frame_misc_harness;
namespace action=halo2::native_player_action_detail;
namespace {
constexpr std::uint32_t players=0x10000000u,player_records=0x10010000u,headers=0x10100000u,header_rows=0x10110000u,
    objects=0x10200000u,scratch=0x10600000u,context=0x10700000u,stack=0x20010000u;
constexpr std::uint32_t object_count=64u,object_stride=0x100u;
std::uint32_t found_list{},pass_slot{};
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]));
    const unsigned words=target==0xbd080u?7u:2u;
    for(unsigned k=1;k<=words;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x154271u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    std::uint32_t result=std::uint32_t(rng());
    if(target==0xbd080u) {
        found_list=r(s.memory,s.gp[4]+0x18u);pass_slot=found_list-0x1cu;
        const std::uint32_t written=std::uint32_t(rng()%9);
        for(std::uint32_t k=0;k<written;++k)
            w(s.memory,s.observe,0xbd081u,found_list+4u*k,4,(std::uint32_t(rng()%0x7fff)<<16)|std::uint32_t(rng()%object_count));
        std::uint32_t count=written;
        const auto roll=rng()%16;
        if(roll==0)count=0;
        else if(roll==1)count=0x8000u|std::uint32_t(rng()%4);
        else if(roll==2 && written)count=written-1u;
        else if(roll==3)count=0x10000u|written;                  // only AX counts
        result=(result&0xffff0000u)|(count&0xffffu);
        if(count&0x10000u)result=(result&0xfffe0000u)|count;
    } else if(rng()%3==0) {
        const auto roll=rng()%6;
        if(roll==0)w(s.memory,s.observe,0x154272u,r(s.memory,s.gp[4]+8u),4,std::uint32_t(rng()));
        else if(roll==1)w(s.memory,s.observe,0x154273u,r(s.memory,s.gp[4]+8u)+4u,4,
            (std::uint32_t(rng())<<16)|std::uint32_t(rng()%object_count));
        else if(roll==2 && found_list)w(s.memory,s.observe,0x154274u,found_list+4u*std::uint32_t(rng()%64),4,
            (std::uint32_t(rng()%0x7fff)<<16)|std::uint32_t(rng()%object_count));
        else if(roll==3 && pass_slot && rng()%4==0)
            w(s.memory,s.observe,0x154275u,pass_slot,4,rng()%2?0u:std::uint32_t(rng()%3)*4u);
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
constexpr std::uint32_t mapped[]{0x153d33u,0x153d4au,0x153d61u,0x153d78u,0x153d8fu,0x153d48u};
std::uint32_t branch_map(std::uint32_t p,SourcePhysicsContactImage image) {
    if(image==SourcePhysicsContactImage::MapPack11)return p;
    for(const auto v:mapped)if(v==p)return p;
    throw std::runtime_error("Unbound TU1.5 contact dispatch target");
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage image){return branch_map(p,image);}
static void run_bd080(WorldState &s){stub(s,0xbd080u);}
static void run_154270(WorldState &s){stub(s,0x154270u);}
static void run_1541b0(WorldState &s){stub(s,0x1541b0u);}
static void run_153f20(WorldState &s){stub(s,0x153f20u);}
static void run_1540d0(WorldState &s){stub(s,0x1540d0u);}
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t quiet_nan(std::mt19937 &rng) {
    static constexpr std::uint32_t payloads[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7fc12345u,0xffc54321u,0xff800003u};
    return payloads[rng()%6];
}
std::uint32_t coordinate(std::mt19937 &rng,unsigned shape) {
    if(shape==4 && rng()%2==0)return quiet_nan(rng);
    if(shape==3 && rng()%2==0) {
        static constexpr std::uint32_t special[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7f800000u,0xff800000u,0x00000001u,
            0x807fffffu,0x80000000u};
        return special[rng()%8];
    }
    return std::bit_cast<std::uint32_t>(float(int(rng()%2001)-1000)/400.0f);
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    found_list=0;pass_slot=0;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    m.put(0x4f54f4u,players);m.put(players+0x44u,player_records);
    m.put(0x4ecbd0u,headers);m.put(headers+0x44u,header_rows);
    for(std::uint32_t at=players;at<players+0x48u;at+=4)if(at!=players+0x44u)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=headers;at<headers+0x48u;at+=4)if(at!=headers+0x44u)m.put(at,std::uint32_t(rng()));
    m.put(0x46a240u,shape==3&&pick(2)==0?coordinate(rng,3):shape==4&&pick(2)?quiet_nan(rng):0x3ecccccdu);
    m.put(0x46a0e4u,shape==3&&pick(2)==0?coordinate(rng,3):shape==4?quiet_nan(rng):0x40400000u);
    // Objects: types (some out of range), children and sibling chains,
    // centers near the unit and radii.
    for(std::uint32_t k=0;k<object_count;++k) {
        m.put(header_rows+k*12u,std::uint32_t(rng()));m.put(header_rows+k*12u+4u,std::uint32_t(rng()));
        m.put(header_rows+k*12u+8u,objects+k*object_stride);
        const auto o=objects+k*object_stride;
        for(std::uint32_t at=0;at<object_stride;at+=4)m.put(o+at,std::uint32_t(rng()));
        std::uint32_t type=pick(13);
        if(pick(9)==0)type=pick(2)?0xffu:0x80u|pick(0x7f);
        if(pick(11)==0)type=13u+pick(20);
        m.put(o+0xaau,type,1);
        const auto handle=[&]{return pick(4)==0?0xffffffffu:((std::uint32_t(rng()%0x7fff)<<16)|pick(object_count));};
        m.put(o+0x10u,shape==7?handle():(pick(2)?0xffffffffu:handle()));
        m.put(o+0x0cu,pick(3)==0?0xffffffffu:handle());
        m.put(o+0x14u,pick(5)==0?std::uint32_t(rng()):0xffffffffu);
        for(std::uint32_t at=0x30u;at<0x3cu;at+=4)m.put(o+at,coordinate(rng,shape));
        m.put(o+0x3cu,shape==3&&pick(2)==0?coordinate(rng,3):shape==4&&pick(2)?quiet_nan(rng):
              std::bit_cast<std::uint32_t>(float(pick(400))/200.0f));
    }
    // The player and its unit.
    const std::uint32_t player=pick(16),unit=pick(object_count);
    for(std::uint32_t k=0;k<16;++k)for(std::uint32_t at=0;at<0x21cu;at+=4)m.put(player_records+k*0x21cu+at,std::uint32_t(rng()));
    m.put(player_records+player*0x21cu+0x2cu,shape==1?0xffffffffu:((std::uint32_t(rng()%0x7fff)<<16)|unit));
    m.put(objects+unit*object_stride+0x14u,shape==2?std::uint32_t(rng()):0xffffffffu);
    // The type case table: 0 biped, 1 vehicle, 2 weapon, 8 control.
    static constexpr std::uint32_t table[]{0x153d78u,0x153d61u,0x153d33u,0x153d8fu,0x153d8fu,0x153d8fu,0x153d8fu,0x153d8fu,
        0x153d4au};
    for(std::uint32_t k=0;k<9;++k)m.put(0x153de0u+k*4u,table[k]);
    if(shape==6) {
        const auto roll=pick(2);
        if(roll==0)m.put(0x153de0u+pick(9)*4u,0x153d48u);            // mapped, not a case
        else m.put(0x153de0u+pick(9)*4u,0x153d34u);                  // unmapped
    }
    for(std::uint32_t at=context-0x10u;at<context+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,0);
    for(std::uint32_t at=stack-0x3000u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x189000u+pick(0x40));
    m.put(stack+4u,(std::uint32_t(rng())<<16)|player);
    m.put(stack+8u,shape==5?objects+unit*object_stride+0x10u:context+4u*pick(3));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{players,player_records,headers,header_rows,objects,context,0x46a000u,0x153000u};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return action::bridge(s,[](auto &v){original::native_frame_misc_reference_153ba0(v);},
        [](std::uint32_t p,SourcePhysicsContactImage image){return original::canonical_branch(p,image);},
        [](auto &v){original::run_bd080(v);},[](auto &v){original::run_1540d0(v);},[](auto &v){original::run_153f20(v);},
        [](auto &v){original::run_154270(v);},[](auto &v){original::run_1541b0(v);});
}
void reference(WorldState &s){original::run_153ba0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLAYER_ACTION");
    std::mt19937 rng(0x153ba0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,action::coverage().load(),unsigned(action::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

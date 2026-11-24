// Native TU1.5 item pickup (1538B0) against the unmodified translated body
// of the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The fourteen children (direct, or through the dispatch) are hash-driven
// stubs of their complete entry ABIs: each hashes what it reads (its pushed
// arguments), writes scratch and its outputs (104D30 the count, CFC90 the
// taken byte), answers AL (BCDF0 the equipment or 0, CF060 a handle or
// NONE); the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author players with and without units, items on
// parents of every type, owned or not, near and far (and NaN), weapons in
// every slot from every current one, equipment of every powerup kind, and
// weapons; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_item_pickup.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_item_pickup_detail;
namespace {
constexpr std::uint32_t players=0x10000000u,player_data=0x10001000u,objects=0x10002000u,entries=0x10003000u,bodies=0x10010000u,
    game=0x10004000u,definitions=0x10005000u,definition_bodies=0x10006000u,equipment=0x10008000u,scratch=0x10600000u;
constexpr std::uint32_t object_count=8u,body_size=0x400u;
std::uint32_t arguments(std::uint32_t target) {
    switch(target) {
    case 0x104d30u:return 5u;
    case 0x197e9du:case 0x1549d0u:case 0xb3140u:return 2u;
    case 0xcfc90u:return 3u;
    case 0xcf080u:case 0x197e11u:case 0x161320u:case 0xbcdf0u:return 1u;
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
    if(target==0x104d30u && rng()%2)w(s.memory,s.observe,target+1u,arg(5),4,rng()%3?std::uint32_t(rng()%5):std::uint32_t(rng()));
    if(target==0xcfc90u && rng()%2)w(s.memory,s.observe,target+1u,arg(3),1,rng()%2?0u:1u+std::uint32_t(rng()%255));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%3?1u+std::uint32_t(rng()%255):0u);
    if(target==0xbcdf0u)s.gp[0]=rng()%4?equipment+0x10u*std::uint32_t(rng()%4):0u;
    if(target==0xcf060u)s.gp[0]=rng()%2?0xffffffffu:std::uint32_t(rng()%8);
    if(policy&2u && rng()%13==0)s.gp[3]=player_data+0x21cu*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[5]=rng()%2?bodies:equipment+0x10u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=rng()%2?0xffffffffu:std::uint32_t(rng()%object_count);
    if(policy&2u && rng()%13==0)s.gp[7]=rng()%2?std::uint32_t(rng()%6):(std::uint32_t(rng())&0xffff0000u)|0x8000u;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*args+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_104d30(WorldState &s){stub(s,0x104d30u);}
static void run_197e9d(WorldState &s){stub(s,0x197e9du);}
static void run_cf080(WorldState &s){stub(s,0xcf080u);}
static void run_197e11(WorldState &s){stub(s,0x197e11u);}
static void run_161320(WorldState &s){stub(s,0x161320u);}
static void run_cf060(WorldState &s){stub(s,0xcf060u);}
static void run_13b360(WorldState &s){stub(s,0x13b360u);}
static void run_153850(WorldState &s){stub(s,0x153850u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0xc6920u && target!=0xbcdf0u && target!=0x1549d0u && target!=0xcfc90u && target!=0xb3140u && target!=0xe8e50u)
        throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t bits(float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const auto real=[&](float lo,float hi){return lo+(hi-lo)*float(rng()%10000)/10000.0f;};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x4f54f4u,players);m.put(0x4ecbd0u,objects);m.put(0x4f3218u,game);m.put(0x4f0414u,definitions);
    m.put(0x456918u,bits(chance(8)?real(-1.0f,3.0f):0.25f));
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,player_data);
    for(std::uint32_t at=objects;at<objects+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(objects+0x44u,entries);
    // Objects 0-7: the unit is object 0, the item 1, the rest weapons and parents.
    static constexpr std::uint32_t types[]{0u,1u,2u,3u,6u,7u,0xbu,0xcu};
    for(std::uint32_t k=0;k<object_count;++k) {
        m.put(entries+12u*k,std::uint32_t(rng()));m.put(entries+12u*k+3u,k==1u&&chance(2)?2u:types[pick(8)],1);
        m.put(entries+12u*k+4u,std::uint32_t(rng()));m.put(entries+12u*k+8u,bodies+body_size*k);
        const auto b=bodies+body_size*k;
        for(std::uint32_t at=b;at<b+0x230u;at+=4)m.put(at,std::uint32_t(rng()));
    }
    // The unit: its position, radius, current weapon and weapons.
    const float ux=real(-20.0f,20.0f),uy=real(-20.0f,20.0f),uz=real(-5.0f,5.0f);
    m.put(bodies+0x30u,bits(ux));m.put(bodies+0x34u,bits(uy));m.put(bodies+0x38u,bits(uz));
    m.put(bodies+0x3cu,bits(chance(10)?std::numeric_limits<float>::quiet_NaN():real(0.1f,1.5f)));
    m.put(bodies+0x212u,chance(4)?0x80u+pick(0x80):pick(4),1);
    for(std::uint32_t k=0;k<4;++k)m.put(bodies+0x218u+4u*k,chance(3)?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|pick(object_count));
    // The item: its parent, owner, position and definition.
    const auto item=bodies+body_size;
    m.put(item,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    m.put(item+0x14u,chance(2)?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|pick(object_count));
    const auto unit_handle=(std::uint32_t(rng())&0xffff0000u);
    m.put(item+0x14cu,chance(8)?unit_handle:std::uint32_t(rng()));
    const float spread=chance(3)?10.0f:1.5f;
    m.put(item+0x30u,bits(ux+real(-spread,spread)));m.put(item+0x34u,bits(uy+real(-spread,spread)));m.put(item+0x38u,bits(uz+real(-1.0f,1.0f)));
    for(std::uint32_t at=game;at<game+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(game+8u,chance(2)?2u:pick(4));
    // Definitions 0-3 and the equipment BCDF0 may answer.
    for(std::uint32_t k=0;k<4;++k) {
        m.put(definitions+16u*k,std::uint32_t(rng()));m.put(definitions+16u*k+4u,std::uint32_t(rng()));
        m.put(definitions+16u*k+8u,definition_bodies+0x200u*k);m.put(definitions+16u*k+12u,std::uint32_t(rng()));
        for(std::uint32_t at=definition_bodies+0x200u*k;at<definition_bodies+0x200u*k+0x140u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(definition_bodies+0x200u*k+0x12cu,chance(3)?6u:chance(3)?0u:pick(8),2);
        m.put(equipment+0x10u*k,(std::uint32_t(rng())&0xffff0000u)|pick(4));
        for(std::uint32_t at=equipment+0x10u*k+4u;at<equipment+0x10u*(k+1u);at+=4)m.put(at,std::uint32_t(rng()));
    }
    // The players: their units and words.
    for(std::uint32_t k=0;k<4;++k) {
        const auto p=player_data+0x21cu*k;
        for(std::uint32_t at=p;at<p+0x21cu;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(p+0x2cu,chance(8)?0xffffffffu:unit_handle);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x153000u+pick(0x100));
    m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    m.put(stack+8u,(std::uint32_t(rng())&0xffff0000u)|1u);
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{players,player_data,objects,entries,bodies,game,definitions,definition_bodies};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1538b0(v);},
        [](auto &v,std::uint32_t target){
            switch(target) {
            case 0x104d30u:original::run_104d30(v);break;case 0x197e9du:original::run_197e9d(v);break;
            case 0xcf080u:original::run_cf080(v);break;case 0x197e11u:original::run_197e11(v);break;
            case 0x161320u:original::run_161320(v);break;case 0xcf060u:original::run_cf060(v);break;
            case 0x13b360u:original::run_13b360(v);break;case 0x153850u:original::run_153850(v);break;
            default:throw std::logic_error("Unbound item pickup child");
            }},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1538b0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ITEM_PICKUP");
    std::mt19937 rng(0x1538b0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

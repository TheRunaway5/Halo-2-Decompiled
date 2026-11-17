// Native TU1.5 cluster distance flood (131470) against the unmodified
// translated body of the campaign environment unit, extracted by
// tools/validate_native_cluster_distance_flood_tu15.py into flood.inc, plus
// the verification reference the production binder generates
// (flood_reference.inc). 14DD70 is a genuine stand-in: its XMM0 result
// (including NaN payloads, infinities, denormals and -0), registers, flags,
// stack balance and one store are a hash of its complete entry ABI (GP, flags,
// XMM, MXCSR) and the portal it measures, so any entry difference diverges.
// Seeded fixtures author complete cluster/portal graphs (negative, invalid
// and out-of-vector cluster words, empty and negative portal counts, special
// limits), both direction flags and every MXCSR rounding/DAZ/FTZ mode, plus
// fault injection. Each case compares ordered stores (pc, address, preimage,
// bytes), the child log, bank, registers, flags, XMM, x87 status, MXCSR and
// the cursor; faults compare the exception, cursor and store prefix.
// Inline modes (inline, inline-census, inline-verify) prove the inline
// 14DD70 route: the native closure runs the leaf kernel on its own state,
// while the translated body's dispatch reaches the unmodified translated
// 14DD70 (scene device unit, segment_body.inc) through a modeled required
// owner round trip (save, callee state, translated body, save, reload).
// Those fixtures author real portal segments (row table, origins, radii),
// the point, x87 stacks of depth 0..7 (7 keeps the dispatch route), every
// x87 precision/rounding control and the MXCSR modes; the x87 lanes,
// status, C1 and sticky flags are compared as well (MXCSR on completed cases:
// a fault inside 14DD70 stops the game, and the round trip's fault path
// reloads the caller's saved controls). The other modes keep the
// dispatch route (HALO2_REFERENCE_CLUSTER_DISTANCE_CHILD) and its stand-in.
// Modes: <cases> [census|verify|inline|inline-census|inline-verify].
// Prints a JSON summary with branch coverage.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_cluster_distance_flood.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <random>
#include <thread>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace flood=halo2::native_cluster_distance_detail;
namespace {
std::uint64_t mix(std::uint64_t h,std::uint64_t v){h^=v+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);return h*0xff51afd7ed558ccdull;}
constexpr std::uint32_t bsp=0x10000000u,clusters=0x10100000u,portals=0x10200000u,lists=0x10300000u,
    output=0x10400000u,point=0x10500000u,scratch=0x10600000u,holder=0x10700000u,rows=0x10780000u,stack=0x20010000u;
std::vector<std::uint64_t> *call_log{};std::uint32_t policy{};
bool inline_mode{};std::uint32_t *trip_cursor{};
std::uint32_t special(std::mt19937_64 &rng) {
    static constexpr std::uint32_t values[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7f800000u,0xff800000u,
        0x00000001u,0x807fffffu,0x80000000u,0x00000000u,0x3f800000u,0x447a0000u,0x00800000u};
    return values[rng()%12];
}
// 14DD70 stand-in: EAX bsp, ECX portal, EDX point; RET; result in XMM0.
void stub_14dd70(WorldState &s) {
    if(r(s.memory,s.gp[4])!=0x131562u)throw std::runtime_error("14DD70 entered without its 131470 return address");
    std::uint64_t h=0x14dd70u;
    for(const auto v:s.gp)h=mix(h,v);
    h=mix(h,s.flags.bits());h=mix(h,s.other_flags);h=mix(h,_mm_getcsr());
    for(unsigned i=0;i<8;++i){std::array<std::uint32_t,4> b;std::memcpy(b.data(),&s.xmm[i],16);for(auto v:b)h=mix(h,v);}
    h=mix(h,r(s.memory,s.gp[1]));h=mix(h,r(s.memory,s.gp[2]));
    std::mt19937_64 rng(h);
    if(call_log) {
        if(call_log->size()>=8192)throw std::runtime_error("Child call budget exceeded");
        call_log->push_back(h);
    }
    w(s.memory,s.observe,0x14dd71u,scratch+std::uint32_t(rng()%32)*4u,4,std::uint32_t(rng()));
    std::uint32_t bits=std::uint32_t(rng()%1000)*0x3a83126fu%0x44000000u;
    if(rng()%9==0)bits=special(rng);
    std::array<std::uint32_t,4> lanes{bits,std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[0],lanes.data(),16);
    // Exercise the comparison's sticky flags as a real x87/SSE child would.
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%31==0){static constexpr unsigned saved[]{3,5,6,7};s.gp[saved[rng()%4]]=std::uint32_t(rng()%3);}
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    s.gp[4]+=4u;
}
}
namespace original {
using halo2::physics_world_runtime_detail::WorldState;
std::uint32_t sign_extend(std::uint32_t value,unsigned count) {
    if(count==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    if(count==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    throw std::runtime_error("Unverified sign-extension width");
}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail) {
    if(target!=0x14dd70u)throw std::runtime_error("Unbound environment dispatch target");
    // The unit's default route for 14DD70 is the required owner.
    if(inline_mode)s.required(site,target,tail);
    else stub_14dd70(s);
}
#include "flood.inc"
#include "flood_reference.inc"
}
// The unmodified translated 14DD70 (scene device unit).
namespace segment {
using halo2::physics_world_runtime_detail::WorldState;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
std::uint32_t native_single_spill(long double x,std::uint16_t &status) {
    float result;std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1":"=m"(result),"=m"(actual):"m"(x):"st");
    status=(status&~0x200u)|(actual&0x200u);
    return std::bit_cast<std::uint32_t>(result);
}
#include "segment_body.inc"
}
namespace {
// The required owner round trip: the caller's saved ABI enters a callee
// state, the translated body runs with full instruction delivery (cursor),
// and its saved ABI returns to the caller.
void round_trip(SourcePhysicsContactMemory &m,const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &a,
                const SourcePhysicsContactRecordObserver &observe) {
    if(call.target!=0x14dd70u || call.instruction!=0x13155du || call.tail)throw std::runtime_error("Round trip at a wrong site");
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[](std::uint32_t pc){if(trip_cursor)*trip_cursor=pc;};
    WorldState callee{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};callee.load(a);
    segment::run_14dd70(callee);a=callee.save();
}
std::uint32_t small_float(std::mt19937 &rng){return std::bit_cast<std::uint32_t>(float(int(rng()%41)-20)/10.0f);}
struct Memory : SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::shared_ptr<std::array<std::byte,4096>>> pages;
    std::array<std::byte,4096> &page(std::uint32_t at,bool create) {
        auto found=pages.find(at>>12);
        if(found==pages.end()) {
            if(!create){char t[64];std::snprintf(t,sizeof t,"Access escaped the authored fixture at %x",at);throw std::runtime_error(t);}
            found=pages.emplace(at>>12,std::make_shared<std::array<std::byte,4096>>()).first;
        }
        return *found->second;
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        for(std::size_t n=0;n<out.size();++n)out[n]=const_cast<Memory*>(this)->page(at+std::uint32_t(n),false)[(at+n)&4095u];
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        for(std::size_t n=0;n<in.size();++n)page(at+std::uint32_t(n),false)[(at+n)&4095u]=in[n];
    }
    void put(std::uint32_t at,std::uint32_t value,unsigned width=4) {
        for(unsigned n=0;n<width;++n)page(at+n,true)[(at+n)&4095u]=std::byte(value>>(8*n));
    }
    Memory clone()const {
        Memory copy;for(const auto &[k,v]:pages)copy.pages.emplace(k,std::make_shared<std::array<std::byte,4096>>(*v));return copy;
    }
    bool same(const Memory &other)const {
        if(pages.size()!=other.pages.size())return false;
        for(const auto &[k,v]:pages){const auto at=other.pages.find(k);if(at==other.pages.end()||*at->second!=*v)return false;}
        return true;
    }
};
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t mxcsr{},policy{};};

Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    std::uint32_t count=1+pick(40);
    if(shape==3)count=1+pick(4);
    if(shape==5)count=400+pick(112);
    std::uint32_t portal_count=pick(count*2+4);
    if(shape==6)portal_count=0;
    m.put(0x4ecc18u,bsp);
    std::mt19937_64 wide(rng());
    m.put(0x46a110u,shape==8?special(wide):std::uint32_t(0x40000000u+pick(0x4000000u)));
    m.put(bsp+0xa0u,clusters);m.put(bsp+0x60u,portals);
    const auto cluster_word=[&]()->std::uint32_t {
        const auto roll=pick(40);
        if(roll==0)return 0xffffu;
        if(roll==1)return std::uint32_t(-2-int(pick(30)))&0xffffu;
        if(roll==2 && shape==11)return 512u+pick(600);
        return pick(count);
    };
    for(std::uint32_t p=0;p<portal_count;++p){m.put(portals+p*36u,cluster_word(),2);m.put(portals+p*36u+2u,cluster_word(),2);
        for(std::uint32_t k=4;k<36;k+=4)m.put(portals+p*36u+k,std::uint32_t(rng()));}
    std::uint32_t list=lists;
    // Records exist for every cluster word the walk can index (including
    // negative words and those beyond the frame vector).
    for(int k=-40;k<int(std::max(count,1u)+(shape==11?1200:0));++k) {
        const auto record=clusters+std::uint32_t(k)*0xb0u;
        std::uint32_t portals_here=portal_count?pick(6):0;
        if(pick(11)==0)portals_here=std::uint32_t(-int(pick(3)));
        m.put(record+0x8cu,portals_here);m.put(record+0x90u,list);
        for(std::uint32_t i=0;i<std::max<std::int32_t>(std::int32_t(portals_here),0);++i){m.put(list,portal_count?pick(portal_count):0,2);list+=2;}
        list+=2;
    }
    for(std::uint32_t at=point;at<point+16;at+=4)m.put(at,std::uint32_t(rng()));
    if(inline_mode) {
        // Real segments: row index +4 into the 16-byte rows [[bsp+0x18]+0xC],
        // origin +8, radius +0x14; small coordinates reach both sides of the
        // limit, with occasional special floats and an out-of-table row.
        m.put(bsp+0x18u,holder);m.put(holder+0xcu,rows);
        for(std::uint32_t k=0;k<16*4;++k)m.put(rows+k*4u,pick(13)==0?special(wide):small_float(rng));
        for(std::uint32_t p=0;p<portal_count;++p) {
            const auto record=portals+p*36u;
            m.put(record+4u,pick(97)==0?0x10000u+pick(16):pick(16));
            for(std::uint32_t k=8;k<0x18u;k+=4)m.put(record+k,pick(29)==0?special(wide):small_float(rng));
            m.put(record+0x14u,pick(29)==0?special(wide):std::bit_cast<std::uint32_t>(float(pick(11))/10.0f));
        }
        for(std::uint32_t at=point;at<point+12;at+=4)m.put(at,pick(31)==0?special(wide):small_float(rng));
    }
    for(std::uint32_t at=output;at<output+0x18u*(std::max(count,1u)+(shape==11?1200:0)+48);at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x80u;at+=4)m.put(at,0);
    for(std::uint32_t at=stack-0x1800u;at<stack+(shape==11?0x2400u:0x40u);at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x131b00u+pick(0x80));m.put(stack+4u,point);m.put(stack+8u,output);
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[0]=shape==9?(std::uint32_t(-2-int(pick(8)))):pick(std::max(count,1u));
    c.abi.gp[4]=stack;
    c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u)|(shape==10?0x400u:0u);
    for(auto &lane:c.abi.xmm)for(auto &b:lane)b=std::uint32_t(rng());
    static constexpr std::uint32_t modes[]{0x1f80u,0x3f80u,0x5f80u,0x7f80u,0x9fc0u,0x1fc0u,0x9f80u,0x1f81u};
    c.mxcsr=modes[pick(8)];c.policy=pick(4);
    if(inline_mode) {
        // Live x87 stack (depth 0..7; 7 leaves no room), condition bits, TOP
        // and any precision/rounding control with masked exceptions.
        c.abi.x87_depth=pick(8);
        for(unsigned i=0;i<c.abi.x87_depth;++i)c.abi.x87[i]=(long double)(int(rng()%2001)-1000)/7.0L;
        const unsigned top=(8u-c.abi.x87_depth+pick(8))&7u;
        c.abi.x87_status=std::uint16_t((top<<11)|(std::uint32_t(rng())&0x4700u));
        const std::uint16_t control=std::uint16_t(0x3fu|0x40u|(pick(4)<<8)|(pick(4)<<10));
        c.abi.floating_point=SourcePhysicsFactoryFloatingPoint{control,c.mxcsr};
    }
    if(shape==15) {
        static constexpr std::uint32_t regions[]{clusters,portals,lists,output,point};
        auto at=m.pages.lower_bound(regions[pick(5)]>>12);
        if(at!=m.pages.end())m.pages.erase(at);
    }
    return c;
}
struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;std::vector<std::uint64_t> calls;
    SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t cursor{},mxcsr{};bool admitted{};std::string fault;
};
bool census_mode{};
template<class V> bool native(V &s) {
    return flood::bridge(s,[](auto &v){original::native_cluster_distance_reference_131470(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
Result execute(Memory &memory,const Case &c,bool is_native) {
    Result result;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e){result.stores.push_back(e);};
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner=round_trip;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(is_native) {
        services.instruction=[](auto){throw std::runtime_error("Native cluster distance delivered a translated instruction");};
        if(!census_mode) {
            auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
            cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
            cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        }
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    const auto saved=_mm_getcsr();std::uint16_t control;asm volatile("fnstcw %0":"=m"(control));
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);
    _mm_setcsr(c.mxcsr);
    policy=c.policy;call_log=&result.calls;trip_cursor=&result.cursor;
    try {
        if(is_native)result.admitted=native(s);
        else original::run_131470(s);
    } catch(const std::exception &e){result.fault=e.what();}
    call_log=nullptr;trip_cursor=nullptr;result.mxcsr=_mm_getcsr();
    result.abi=s.save();
    // Fresh sticky x87 exception flags and the host controls for every run.
    asm volatile("fnclex\n\tfldcw %0"::"m"(control));_mm_setcsr(saved);
    return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    bool same=a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth;
    for(unsigned i=0;i<a.x87_depth && i<8;++i)same=same && !std::memcmp(&a.x87[i],&b.x87[i],10);
    if(a.floating_point && b.floating_point)same=same && a.floating_point->x87_control_word==b.floating_point->x87_control_word;
    return same;
}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    inline_mode=mode.starts_with("inline");
    const auto variant=inline_mode?mode.substr(std::min<std::size_t>(mode.size(),7)):mode;
    census_mode=variant=="census";
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(variant=="verify")setenv("HALO2_VERIFY_CLUSTER_DISTANCE","1",1);
    if(!inline_mode)setenv("HALO2_REFERENCE_CLUSTER_DISTANCE_CHILD","1",1);
    std::mt19937 rng(0x131470u+(census_mode?1u:0u)+(variant=="verify"?2u:0u)+(inline_mode?4u:0u));
    std::uint64_t stores{},compared{},faults{},children{},refused{};
    for(unsigned n=0;n<cases;++n) {
        auto c=author(rng,n);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native_result=execute(right,c,true);
        if(!native_result.admitted && native_result.fault.empty()) {
            // Unmasked MXCSR exceptions keep the translated route.
            if((c.mxcsr&0x1f80u)!=0x1f80u){++refused;continue;}
            std::fprintf(stderr,"case %u refused\n",n);return 1;
        }
        const auto reference=execute(left,c,false);
        ++compared;stores+=reference.stores.size();children+=reference.calls.size();
        if(!reference.fault.empty())++faults;
        if(native_result.fault!=reference.fault) {
            std::fprintf(stderr,"case %u fault mismatch: native '%s' translated '%s'\n",n,native_result.fault.c_str(),reference.fault.c_str());
            return 1;
        }
        if(native_result.stores!=reference.stores) {
            std::size_t at=0;while(at<native_result.stores.size()&&at<reference.stores.size()&&native_result.stores[at]==reference.stores[at])++at;
            std::fprintf(stderr,"case %u store mismatch at %zu of %zu/%zu",n,at,native_result.stores.size(),reference.stores.size());
            if(at<native_result.stores.size()&&at<reference.stores.size())
                std::fprintf(stderr," native pc=%x a=%x original pc=%x a=%x",native_result.stores[at].instruction,native_result.stores[at].address,
                    reference.stores[at].instruction,reference.stores[at].address);
            std::fprintf(stderr,"\n");return 1;
        }
        if(native_result.calls!=reference.calls){std::fprintf(stderr,"case %u child log mismatch\n",n);return 1;}
        if(!census_mode && native_result.cursor!=reference.cursor){std::fprintf(stderr,"case %u cursor %x vs %x\n",n,native_result.cursor,reference.cursor);return 1;}
        // A fault inside 14DD70 stops the game; its round trip's fault path
        // reloads the caller's saved controls, so the sticky flags the callee
        // raised before the fault are compared only on completed cases.
        if((!inline_mode || reference.fault.empty()) && native_result.mxcsr!=reference.mxcsr) {
            std::fprintf(stderr,"case %u MXCSR %x vs %x\n",n,native_result.mxcsr,reference.mxcsr);return 1;
        }
        if(reference.fault.empty() && !same_abi(native_result.abi,reference.abi)){std::fprintf(stderr,"case %u register/flag/XMM mismatch\n",n);return 1;}
        if(!left.same(right)){std::fprintf(stderr,"case %u bank mismatch\n",n);return 1;}
    }
    if(!census_mode) {
        auto c=author(rng,0);SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;
        WorldState a{c.memory,observe,services,SourcePhysicsContactImage::MapPack11};a.load(c.abi);
        if(native(a))throw std::runtime_error("MapPack11 admitted");
        WorldState b{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};b.load(c.abi);
        const auto saved=_mm_getcsr();_mm_setcsr(0x1f00u);
        const bool unmasked=native(b);_mm_setcsr(saved);
        if(unmasked)throw std::runtime_error("Unmasked MXCSR admitted");
        services.instruction=[](auto){};
        WorldState d{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};d.load(c.abi);
        if(native(d))throw std::runtime_error("Full instruction callbacks admitted");
    }
    std::printf("{\"cases\":%u,\"compared\":%llu,\"refused\":%llu,\"faults\":%llu,\"ordered_stores\":%llu,\"children\":%llu,"
        "\"coverage\":%llu,\"branches\":%u,\"mode\":\"%s\"}\n",cases,(unsigned long long)compared,(unsigned long long)refused,
        (unsigned long long)faults,(unsigned long long)stores,(unsigned long long)children,
        (unsigned long long)flood::coverage().load(),unsigned(flood::branch_count),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

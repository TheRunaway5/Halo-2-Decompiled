// Native TU1.5 object sphere query closure (BD080/BA970/BAA40) against the
// unmodified translated bodies of all three authenticated BD080 variants,
// extracted by tools/validate_native_object_sphere_query_tu15.py into
// sphere_live.inc (live support: direct callees), sphere_combat.inc (combat
// action/scene device: dispatched 14D0E0/BA970) and sphere_ai.inc (AI frame:
// dispatched BAA40). 14D0E0 is a genuine child in both routes: a seeded stub
// whose every effect (cluster list, count, registers, XMM, flags, stores)
// is a hash of its complete entry ABI, so any entry difference diverges.
// Seeded fixtures author complete reference tables/pools, dedupe stamps,
// stack frames, special floats, aliasing and MXCSR modes; every case
// compares the ordered store events (pc, address, preimage, bytes), final
// bank, registers, flags, XMM, x87 status, MXCSR and instruction cursor.
// Fault cases compare the exception, the cursor and the store prefix.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_object_sphere_query.hpp"
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace {
std::uint32_t sign_extend_value(std::uint32_t value,unsigned count) {
    if(count==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    if(count==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    throw std::runtime_error("Unverified sign-extension width");
}
std::uint64_t stub_calls{};
std::uint64_t mix(std::uint64_t h,std::uint64_t v){h^=v+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);return h*0xff51afd7ed558ccdull;}
constexpr std::uint32_t stub_config=0x30000000u;
// 14D0E0 stand-in: RET 0x14 child whose outcome hashes its whole entry ABI.
void stub_14d0e0(WorldState &s) {
    ++stub_calls;
    const auto sp=s.gp[4];
    if(r(s.memory,sp)!=0xbd119u)throw std::runtime_error("14D0E0 entered without its BD080 return address");
    std::uint64_t h=0x14d0e0u;
    for(const auto v:s.gp)h=mix(h,v);
    for(unsigned i=0;i<6;++i)h=mix(h,r(s.memory,sp+4u+i*4u));
    h=mix(h,s.flags.bits());h=mix(h,s.other_flags);h=mix(h,s.fp.status);
    for(unsigned i=0;i<8;++i){std::array<std::uint32_t,4> b;std::memcpy(b.data(),&s.xmm[i],16);for(auto v:b)h=mix(h,v);}
    const auto mode=r(s.memory,stub_config),clusters=r(s.memory,stub_config+4u);
    std::mt19937_64 rng(h^mode);
    const auto out=r(s.memory,sp+0x14u),limit=r(s.memory,sp+0x10u);
    // One visited-stamp store as the real child makes.
    w(s.memory,s.observe,0x14d12du,0x4f3ce8u+std::uint32_t(rng()%4)*4u,4,r(s.memory,0x4f3ce4u));
    std::uint32_t n=std::uint32_t(rng()%40);
    if(mode&1u)n=std::uint32_t(rng()%3)?0x200u+std::uint32_t(rng()%0x200):0x200u;
    const auto written=std::min<std::uint32_t>(n,limit);
    for(std::uint32_t i=0;i<written;++i) {
        auto c=std::uint32_t(rng()%std::max(clusters,1u));
        if(rng()%23==0)c=0xffffu;
        w(s.memory,s.observe,0x14d118u,out+i*2u,2,c);
    }
    std::uint32_t eax=n;
    if(mode&2u){if(rng()%2)eax=std::uint32_t(-std::int32_t(rng()%5));else eax=0;}
    if(rng()%3==0)eax|=std::uint32_t(rng())&0xffff0000u;
    s.gp[0]=eax;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(mode&4u){s.gp[3]=std::uint32_t(rng());s.gp[5]=std::uint32_t(rng());s.gp[6]=std::uint32_t(rng());s.gp[7]=std::uint32_t(rng());}
    if(mode&8u)for(unsigned i=0;i<8;++i){
        std::array<std::uint32_t,4> b;for(auto &v:b)v=std::uint32_t(rng());std::memcpy(&s.xmm[i],b.data(),16);
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    s.gp[4]=sp+4u+0x14u;
}
}
namespace live {
using halo2::physics_world_runtime_detail::WorldState;
std::uint32_t sign_extend(std::uint32_t v,unsigned n){return sign_extend_value(v,n);}
static void run_14d0e0(WorldState &s){stub_14d0e0(s);}
#include "sphere_live.inc"
}
namespace combat {
using halo2::physics_world_runtime_detail::WorldState;
std::uint32_t sign_extend(std::uint32_t v,unsigned n){return sign_extend_value(v,n);}
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target==0x14d0e0u){stub_14d0e0(s);return;}
    if(target==0xba970u){live::run_ba970(s);return;}
    throw std::runtime_error("Unbound combat dispatch target");
}
#include "sphere_combat.inc"
}
namespace ai {
using halo2::physics_world_runtime_detail::WorldState;
std::uint32_t sign_extend(std::uint32_t v,unsigned n){return sign_extend_value(v,n);}
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_14d0e0(WorldState &s){stub_14d0e0(s);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target==0xbaa40u){live::run_baa40(s);return;}
    throw std::runtime_error("Unbound AI frame dispatch target");
}
#include "sphere_ai.inc"
}
namespace {
namespace query=native_object_sphere_query_detail;
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

struct Case { Memory memory; SourcePhysicsWorldRuntimeAbi abi{}; unsigned variant{}; };

std::uint32_t special_float(std::mt19937 &rng) {
    switch(rng()%9) {
    case 0:return 0x7fc00000u|(std::uint32_t(rng())&0x3fffffu);   // quiet NaN payloads
    case 1:return 0x7f800001u|(std::uint32_t(rng())&0x3ffffeu);   // signaling NaN
    case 2:return 0xff800000u;case 3:return 0x7f800000u;
    case 4:return 0x80000000u;case 5:return std::uint32_t(rng())&0x007fffffu; // denormal
    case 6:return 0x80000000u|(std::uint32_t(rng())&0x007fffffu);
    default:return std::bit_cast<std::uint32_t>(float(int(rng()%2000)-1000)*0.01f);
    }
}

// One authored world. `shape` selects structural edge cases.
Case author(std::mt19937 &rng,unsigned shape,unsigned variant) {
    Case c;c.variant=variant;auto &m=c.memory;
    const auto pick=[&](std::uint32_t n){return std::uint32_t(rng()%n);};
    const auto real=[&](float lo,float hi){return std::bit_cast<std::uint32_t>(lo+(hi-lo)*float(rng()%100000)/100000.0f);};
    constexpr std::uint32_t stack=0x20010000u,location=0x11000000u,center=0x11100000u,output=0x11200000u,
        table1=0x12000000u,pool1=0x12100000u,data1=0x12200000u,table2=0x13000000u,pool2=0x13100000u,data2=0x13200000u;
    const std::uint32_t clusters=1+pick(shape%5==4?40u:12u);
    const std::uint32_t objects=1+pick(shape==8?6u:120u);
    const bool specials=shape==9;
    const auto coordinate=[&]{return specials && pick(4)==0?special_float(rng):real(-8.f,8.f);};
    // Globals.
    m.put(0x46a0e8u,pick(6)?0u:(pick(2)?real(-1.f,1.f):special_float(rng)));
    m.put(0x4f3ce4u,std::uint32_t(rng()));m.put(0x4f3ce1u,std::uint32_t(rng())&0xffu,1);
    for(unsigned i=0;i<4;++i)m.put(0x4f3ce8u+i*4u,std::uint32_t(rng()));
    const auto stamp=std::uint32_t(rng());
    m.put(0x4eabccu,stamp);m.put(0x4eabc8u,std::uint32_t(rng())&0xffu,1);
    m.put(0x4eabb0u,table1);m.put(0x4eabb4u,pool1);m.put(0x4eaba4u,table2);m.put(0x4eaba8u,pool2);
    for(std::uint32_t i=0;i<objects;++i)m.put(0x4eabd0u+i*4u,pick(5)==0?stamp+1u:std::uint32_t(rng()));
    m.put(stub_config,(shape==4?1u:0u)|(shape==5?2u:0u)|(shape==10?4u|8u:0u)|(pick(9)==0?8u:0u));
    m.put(stub_config+4u,clusters);
    // Two reference tables over their pools; lists share objects (dedupe).
    const auto tables=[&](std::uint32_t table,std::uint32_t pool,std::uint32_t data) {
        const std::uint32_t stride=0x20u+4u*pick(4);
        m.put(pool+0x24u,stride);m.put(pool+0x44u,data);
        std::uint32_t next=0;
        for(std::int32_t k=-1;k<std::int32_t(clusters);++k) {
            const auto length=pick(5)==0?0u:pick(shape==8?12u:6u);
            const auto slot=table+std::uint32_t(k)*4u;
            if(!length){m.put(slot,0xffffffffu);continue;}
            const auto head=next;
            for(std::uint32_t n=0;n<length;++n,++next) {
                const auto record=data+next*stride;
                auto link=n+1<length?next+1:0xffffffffu;
                if(link!=0xffffffffu && pick(4)==0)link|=std::uint32_t(rng())&0xffff0000u;
                m.put(record,std::uint32_t(rng()));
                auto object=pick(objects);if(pick(3)==0)object|=std::uint32_t(rng())&0xffff0000u;
                m.put(record+4u,object);m.put(record+8u,link);
                m.put(record+0xcu,pick(7)==0?std::uint32_t(rng()):pick(8),1);
                for(unsigned i=0;i<3;++i)m.put(record+0x10u+i*4u,coordinate());
                m.put(record+0x1cu,specials && pick(4)==0?special_float(rng):real(0.f,3.f));
                for(std::uint32_t pad=0x20u;pad<stride;pad+=4)m.put(record+pad,std::uint32_t(rng()));
            }
            auto first=head;if(pick(4)==0)first|=std::uint32_t(rng())&0xffff0000u;
            m.put(slot,first);
        }
    };
    tables(table1,pool1,data1);tables(table2,pool2,data2);
    // Query arguments and the caller frame.
    for(std::uint32_t at=stack-0x500u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    std::uint32_t lists=std::uint32_t(rng());
    if(shape==1 || pick(6)==0)lists=0;else if(pick(2))lists=pick(4);
    std::uint32_t types=pick(3)?(0xffu&std::uint32_t(rng()))|(std::uint32_t(rng())&0xffffff00u):0xffffffffu;
    if(shape==1 || pick(8)==0)types=0;
    m.put(location,std::uint32_t(rng()));
    std::uint32_t cluster=pick(clusters);if(shape==2)cluster=0xffffu;
    m.put(location+4u,cluster|(std::uint32_t(rng())&0xffff0000u));
    for(unsigned i=0;i<3;++i)m.put(center+i*4u,coordinate());
    std::uint32_t radius=real(0.5f,6.f);
    if(shape==3)radius=pick(2)?real(-3.f,0.f):(pick(2)?0u:special_float(rng));
    if(specials && pick(2))radius=special_float(rng);
    std::uint32_t capacity=pick(shape==6?4u:64u);
    if(pick(10)==0)capacity|=0x8000u|(std::uint32_t(rng())&0x7fffu);
    if(pick(7)==0)capacity|=std::uint32_t(rng())&0xffff0000u;
    std::uint32_t out=output;
    if(shape==7)out=pick(2)?stack-0x400u:0x4eabd0u+pick(objects)*4u;
    for(std::uint32_t i=0;i<0x200u;++i)m.put(output+i*4u,std::uint32_t(rng()));
    m.put(stack,0x12345678u);m.put(stack+4u,lists);m.put(stack+8u,types);m.put(stack+0xcu,location);
    m.put(stack+0x10u,pick(19)==0?location:center);m.put(stack+0x14u,radius);m.put(stack+0x18u,out);m.put(stack+0x1cu,capacity);
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[4]=stack;c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u);
    for(auto &lane:c.abi.xmm)for(auto &v:lane)v=std::uint32_t(rng());
    // Masked exceptions; any rounding mode, DAZ/FTZ.
    c.abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80u|(std::uint32_t(rng())&0xe040u)};
    if(shape==11) {
        // Fault injection: one authored region disappears.
        const std::uint32_t victims[]={center,data1,data2,output,location,table1,0x4eabd0u};
        const auto victim=victims[pick(7)];
        c.memory.pages.erase(victim>>12);
        if(victim==0x4eabd0u)c.memory.pages.erase((0x4eabd0u>>12)+1);
    }
    return c;
}

struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;SourcePhysicsWorldRuntimeAbi abi{};
    std::uint32_t cursor{};bool admitted{};std::string error;std::uint64_t callbacks{};
};
enum class Route { reference, native, census };
void original_bd080(unsigned variant,WorldState &s) {
    if(variant==0)live::run_bd080(s);else if(variant==1)combat::run_bd080(s);else ai::run_bd080(s);
}
bool native_bd080(unsigned variant,WorldState &s) {
    const auto reference=[variant](auto &v){original_bd080(variant,v);};
    if(variant==1)return query::bridge(s,reference,[](auto &v){combat::dispatch(v,0x14d0e0u,0xbd114u,false);});
    if(variant==0)return query::bridge(s,reference,[](auto &v){live::run_14d0e0(v);});
    return query::bridge(s,reference,[](auto &v){ai::run_14d0e0(v);});
}
Result execute(Memory &memory,const Case &c,Route route) {
    Result result;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e){result.stores.push_back(e);};
    SourcePhysicsWorldRuntimeServices services;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(route==Route::native) {
        services.instruction=[](auto){throw std::runtime_error("Native object sphere query delivered a translated instruction");};
        auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
        cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;++result.callbacks;};
    const auto host=_mm_getcsr();
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    try {
        s.load(c.abi);
        if(route==Route::reference)original_bd080(c.variant,s);
        else result.admitted=native_bd080(c.variant,s);
        result.abi=s.save();
    } catch(const std::exception &e) {result.error=e.what();}
    _mm_setcsr(host);
    return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    return a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth &&
        a.floating_point.has_value()==b.floating_point.has_value() &&
        (!a.floating_point || (a.floating_point->mxcsr==b.floating_point->mxcsr &&
            a.floating_point->x87_control_word==b.floating_point->x87_control_word));
}
}

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    if(mode=="census")setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(mode=="verify")setenv("HALO2_VERIFY_OBJECT_SPHERE_QUERY","1",1);
    std::mt19937 rng(0xbd080u+unsigned(mode.size()));
    std::uint64_t stores{},calls{},refused{},faults{},children{};
    for(unsigned n=0;n<cases;++n) {
        const auto variant=n%3;
        auto c=author(rng,(n/3)%12,variant);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto before_children=stub_calls;
        const auto native=execute(right,c,mode=="census"?Route::census:Route::native);
        if(native.error.empty() && !native.admitted){++refused;continue;}
        const auto reference=execute(left,c,Route::reference);
        children+=stub_calls-before_children;
        ++calls;stores+=reference.stores.size();
        if(native.stores!=reference.stores) {
            std::size_t at=0;while(at<native.stores.size()&&at<reference.stores.size()&&native.stores[at]==reference.stores[at])++at;
            std::fprintf(stderr,"case %u variant %u store mismatch at %zu of %zu/%zu",n,variant,at,native.stores.size(),reference.stores.size());
            if(at<native.stores.size()&&at<reference.stores.size())
                std::fprintf(stderr," native pc=%x a=%x original pc=%x a=%x",native.stores[at].instruction,native.stores[at].address,
                    reference.stores[at].instruction,reference.stores[at].address);
            std::fprintf(stderr," errors '%s' '%s'\n",native.error.c_str(),reference.error.c_str());return 1;
        }
        if(native.error!=reference.error) {
            std::fprintf(stderr,"case %u variant %u error mismatch '%s' vs '%s'\n",n,variant,native.error.c_str(),reference.error.c_str());return 1;
        }
        if(mode!="census" && native.cursor!=reference.cursor) {
            std::fprintf(stderr,"case %u variant %u cursor %x vs %x (%s)\n",n,variant,native.cursor,reference.cursor,reference.error.c_str());return 1;
        }
        if(!reference.error.empty()){++faults;continue;}
        if(!same_abi(native.abi,reference.abi)||!left.same(right)) {
            std::fprintf(stderr,"case %u variant %u register/flag/bank mismatch eax %x/%x ecx %x/%x edx %x/%x flags %x/%x\n",n,variant,
                native.abi.gp[0],reference.abi.gp[0],native.abi.gp[1],reference.abi.gp[1],native.abi.gp[2],reference.abi.gp[2],
                native.abi.eflags,reference.abi.eflags);return 1;
        }
        if(mode=="census" && native.callbacks)throw std::runtime_error("Census admission delivered closure instructions");
    }
    // Admission refusals: other image, MMX, unmasked SSE, full callbacks
    // without a cursor (outside census), traces, service site in the closure.
    if(mode.empty()) {
        auto c=author(rng,0,0);SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;
        const auto refuse=[&](WorldState &s,const char *what){if(native_bd080(0,s))throw std::runtime_error(std::string(what)+" admitted");};
        {WorldState s{c.memory,observe,services,SourcePhysicsContactImage::MapPack11};s.load(c.abi);refuse(s,"MapPack11");}
        {WorldState s{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);s.active_mmx=SourcePhysicsMmxAlias{};refuse(s,"Active MMX");}
        {
            const auto host=_mm_getcsr();
            WorldState s{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);
            _mm_setcsr(_mm_getcsr()&~0x0200u);refuse(s,"Unmasked SSE");_mm_setcsr(host);
        }
        services.instruction=[](auto){};
        {WorldState s{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);refuse(s,"Full instruction callbacks");}
        std::uint32_t position{};bool trace=true;
        constexpr std::array inside{0x38846eu,0x21f590u,0xba9c9u};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        auto &cursor=services.instruction_cursor;cursor.position=&position;cursor.service_sites=sites;
        cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=true;cursor.observation_only_stores=true;cursor.first_trace=&trace;
        {WorldState s{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);refuse(s,"Active trace");}
        cursor.first_trace=nullptr;cursor.observation_only_stores=false;
        SourcePhysicsContactRecordObserver arbitrary=[](const auto&){};
        {WorldState s{c.memory,arbitrary,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);refuse(s,"Arbitrary observer");}
        cursor.observation_only_stores=true;cursor.service_sites=inside;
        {WorldState s{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);
            if(s.literal_service_sites)throw std::runtime_error("Unexpected literal site set");refuse(s,"Service site in closure");}
    }
    std::printf("{\"cases\":%u,\"mode\":\"%s\",\"compared\":%llu,\"refused\":%llu,\"faults\":%llu,\"ordered_stores\":%llu,"
        "\"children\":%llu,\"coverage\":%llu,\"branches\":%u}\n",cases,mode.c_str(),
        (unsigned long long)calls,(unsigned long long)refused,(unsigned long long)faults,(unsigned long long)stores,
        (unsigned long long)children,(unsigned long long)query::coverage().load(),unsigned(query::branch_count));
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

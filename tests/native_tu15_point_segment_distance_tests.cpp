// Native TU1.5 point/segment distance leaf (14DD70, scene device) against
// the unmodified translated body, extracted by
// tools/validate_native_point_segment_distance_tu15.py into
// point_segment_body.inc. Seeded fixtures author the segment record, its
// row table, the point, the caller frame, a live x87 stack of depth 0..6,
// every x87 precision/rounding control, MXCSR rounding/DAZ/FTZ modes and
// special floats (NaN payloads, infinities, denormals, signed zeros). Every
// case compares the ordered store events (pc, address, preimage, bytes),
// final bank, registers, flags, XMM, x87 status (C1, TOP, sticky flags),
// x87 lanes, control word, MXCSR and the instruction cursor; fault cases
// compare the exception, the cursor and the store prefix.
// Modes: <cases> [verify]. Prints a JSON summary with path coverage.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include <cstdio>
#include <map>
#include <memory>
#include <random>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace original {
using halo2::physics_world_runtime_detail::WorldState;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
std::uint32_t native_single_spill(long double x,std::uint16_t &status) {
    float result;std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1":"=m"(result),"=m"(actual):"m"(x):"st");
    status=(status&~0x200u)|(actual&0x200u);
    return std::bit_cast<std::uint32_t>(result);
}
#include "point_segment_body.inc"
}
namespace {
namespace leaves=native_tu15_leaves;
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
struct Case { Memory memory; SourcePhysicsWorldRuntimeAbi abi{}; };

std::uint32_t special_float(std::mt19937 &rng) {
    switch(rng()%9) {
    case 0:return 0x7fc00000u|(std::uint32_t(rng())&0x3fffffu);
    case 1:return 0x7f800001u|(std::uint32_t(rng())&0x3ffffeu);
    case 2:return 0xff800000u;case 3:return 0x7f800000u;
    case 4:return 0x80000000u;case 5:return std::uint32_t(rng())&0x007fffffu;
    case 6:return 0xffc00000u|(std::uint32_t(rng())&0x3fffffu);
    default:return std::bit_cast<std::uint32_t>(float(int(rng()%2000)-1000)*0.013f);
    }
}
Case author(std::mt19937 &rng,unsigned shape) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t n){return std::uint32_t(rng()%n);};
    const auto real=[&](float lo,float hi){return std::bit_cast<std::uint32_t>(lo+(hi-lo)*float(rng()%1000003)/1000003.0f);};
    const bool specials=shape%5==4;
    const auto value=[&](float lo,float hi){return specials && pick(3)==0?special_float(rng):real(lo,hi);};
    constexpr std::uint32_t segment=0x11000000u,point=0x11100000u,object=0x11200000u,link=0x11300000u,rows=0x11400000u,stack=0x20010000u;
    const std::uint32_t row_count=1+pick(16);
    for(std::uint32_t i=0;i<row_count*4;++i)m.put(rows+i*4u,value(-1.f,1.f));
    std::uint32_t row=pick(row_count);
    if(pick(6)==0)row|=0x10000000u*pick(16);  // SHL 4 discards the top nibble
    m.put(segment,std::uint32_t(rng()));m.put(segment+4u,row);
    for(unsigned i=0;i<3;++i)m.put(segment+8u+i*4u,value(-20.f,20.f));
    // The radius decides the FABS or the FSQRT exit.
    m.put(segment+0x14u,shape%3==0?value(0.f,40.f):value(0.f,3.f));
    for(unsigned i=0;i<3;++i)m.put(point+i*4u,value(-20.f,20.f));
    m.put(object+0x18u,link);m.put(link+0xcu,rows);
    for(std::uint32_t at=stack-0x40u;at<stack+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x18d9b1u);
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[0]=object;c.abi.gp[1]=segment;c.abi.gp[2]=point;c.abi.gp[4]=stack;
    if(shape==7)c.abi.gp[2]=segment+8u;   // point aliases the origin
    c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u);
    for(auto &lane:c.abi.xmm)for(auto &v:lane)v=std::uint32_t(rng());
    // Live x87 stack (depth 0..6), status condition bits and TOP.
    c.abi.x87_depth=pick(7);
    for(unsigned i=0;i<c.abi.x87_depth;++i)c.abi.x87[i]=(long double)(int(rng()%2001)-1000)/7.0L;
    const unsigned top=(8u-c.abi.x87_depth+pick(8))&7u;
    c.abi.x87_status=std::uint16_t((top<<11)|(std::uint32_t(rng())&0x4700u));
    // Masked exceptions; any precision and rounding control.
    const std::uint16_t control=std::uint16_t(0x3fu|0x40u|(pick(4)<<8)|(pick(4)<<10));
    c.abi.floating_point=SourcePhysicsFactoryFloatingPoint{control,0x1f80u|(std::uint32_t(rng())&0xe040u)};
    if(shape==11) {
        const std::uint32_t victims[]={segment,point,object,link,rows};
        m.pages.erase(victims[pick(5)]>>12);
    }
    return c;
}
struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;SourcePhysicsWorldRuntimeAbi abi{};
    std::uint32_t cursor{};bool admitted{};std::string error;
};
bool native_route(WorldState &s) {
    return leaves::x87_room(s,2u) && leaves::run(s,[](auto &io,auto &m){leaves::kernel_14dd70(io,m);},
        [](auto &r){original::run_14dd70(r);},0x0u);
}
Result execute(Memory &memory,const Case &c,bool native) {
    Result result;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e){result.stores.push_back(e);};
    SourcePhysicsWorldRuntimeServices services;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(native) {
        services.instruction=[](auto){throw std::runtime_error("Native 14DD70 delivered a translated instruction");};
        auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
        cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    const auto host=_mm_getcsr();std::uint16_t host_control;asm volatile("fnstcw %0":"=m"(host_control));
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    try {
        // Fresh sticky x87 exception flags for every run.
        asm volatile("fnclex");
        s.load(c.abi);
        if(native)result.admitted=native_route(s);else original::run_14dd70(s);
        result.abi=s.save();
    } catch(const std::exception &e){result.error=e.what();}
    asm volatile("fnclex\n\tfldcw %0"::"m"(host_control));_mm_setcsr(host);
    return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    bool same=a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth &&
        a.floating_point.has_value() && b.floating_point.has_value() &&
        a.floating_point->mxcsr==b.floating_point->mxcsr && a.floating_point->x87_control_word==b.floating_point->x87_control_word;
    for(unsigned i=0;i<a.x87_depth && i<8;++i)same=same && !std::memcmp(&a.x87[i],&b.x87[i],10);
    return same;
}
}

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    if(mode=="verify")setenv("HALO2_VERIFY_MATRIX_LEAVES","1",1);
    std::mt19937 rng(0x14dd70u+unsigned(mode.size()));
    std::uint64_t stores{},calls{},refused{},faults{},near{},far{},c1{};
    for(unsigned n=0;n<cases;++n) {
        auto c=author(rng,n%12);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native=execute(right,c,true);
        if(native.error.empty() && !native.admitted){++refused;continue;}
        const auto reference=execute(left,c,false);
        // The shared leaf verify mode computes the kernel on a pure overlay
        // first, so a faulting operand stops before the live prefix stores.
        if(mode=="verify" && !reference.error.empty() && native.error==reference.error){++faults;continue;}
        ++calls;stores+=reference.stores.size();
        if(native.stores!=reference.stores || native.error!=reference.error) {
            std::fprintf(stderr,"case %u store/fault mismatch (%zu/%zu) '%s' '%s'\n",n,native.stores.size(),reference.stores.size(),
                native.error.c_str(),reference.error.c_str());return 1;
        }
        if(mode.empty() && native.cursor!=reference.cursor) {
            std::fprintf(stderr,"case %u cursor %x vs %x (%s)\n",n,native.cursor,reference.cursor,reference.error.c_str());return 1;
        }
        if(!reference.error.empty()){++faults;continue;}
        if(!same_abi(native.abi,reference.abi)||!left.same(right)) {
            std::fprintf(stderr,"case %u register/flag/FP/bank mismatch eflags %x/%x sw %x/%x mxcsr %x/%x xmm0 %x/%x\n",n,
                native.abi.eflags,reference.abi.eflags,native.abi.x87_status,reference.abi.x87_status,
                native.abi.floating_point->mxcsr,reference.abi.floating_point->mxcsr,native.abi.xmm[0][0],reference.abi.xmm[0][0]);return 1;
        }
        (reference.cursor==0x14de5au?near:far)++;
        if(reference.abi.x87_status&0x200u)++c1;
    }
    if(mode.empty()) {
        // Refusals: full x87 stack, MMX ownership, unmasked x87, other image.
        auto c=author(rng,0);SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;
        const auto refuse=[&](WorldState &s,const char *what){if(native_route(s))throw std::runtime_error(std::string(what)+" admitted");};
        std::uint16_t host_control;asm volatile("fnstcw %0":"=m"(host_control));const auto host=_mm_getcsr();
        {auto abi=c.abi;abi.x87_depth=7;abi.x87_status=std::uint16_t(1u<<11);WorldState s{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);refuse(s,"Depth 7");}
        {WorldState s{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);s.active_mmx=SourcePhysicsMmxAlias{};refuse(s,"Active MMX");}
        {WorldState s{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);
            const std::uint16_t unmasked=std::uint16_t(c.abi.floating_point->x87_control_word&~0x1u);asm volatile("fldcw %0"::"m"(unmasked));refuse(s,"Unmasked x87");}
        {WorldState s{c.memory,observe,services,SourcePhysicsContactImage::MapPack11};s.load(c.abi);refuse(s,"MapPack11");}
        asm volatile("fnclex\n\tfldcw %0"::"m"(host_control));_mm_setcsr(host);
    }
    std::printf("{\"cases\":%u,\"mode\":\"%s\",\"compared\":%llu,\"refused\":%llu,\"faults\":%llu,\"ordered_stores\":%llu,"
        "\"near\":%llu,\"far\":%llu,\"c1_set\":%llu}\n",cases,mode.c_str(),(unsigned long long)calls,(unsigned long long)refused,
        (unsigned long long)faults,(unsigned long long)stores,(unsigned long long)near,(unsigned long long)far,(unsigned long long)c1);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

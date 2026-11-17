// Native TU1.5 303EE0 (append a contact record to an hkArray: grow through
// 2E3250 when full, copy a local through 303BA0) against the unmodified
// translated canonical bodies run_300400 and run_300100, extracted by
// tools/validate_native_contact_array_tu15.py into array_*.inc with the
// unit's selected() operand cases (array_selected.inc) and the binder's
// verification reference (array_reference.inc: run_300400 with its 2E3250
// child routed to the recorded boundary).
//
// The 2E3250 child is a deterministic stand-in (both parents enter the same
// one with the same state): it reads its two stack arguments, moves the array
// to a fresh block, doubles the capacity, keeps the flag bit, and leaves
// scratch registers, flags and XMM1 changed, as a real child may.
//
// Seeded fixtures: arrays with room, full (growth) or over-full, with and
// without the capacity flag bit; record blocks at the top of the fixture;
// random stack contents (the copied local's other bytes); special floats in
// the records; every MXCSR mode; random register files; fault injection with
// one fixture page removed. Each case compares the ordered store events (pc,
// address, preimage, bytes), the final bank, registers, flags, XMM, MXCSR
// and the cursor; fault cases compare the exception, the cursor and the bank.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace leaves=halo2::native_world_leaves_detail;

namespace t_array {
struct ObservedWorldState : halo2::physics_world_runtime_detail::WorldState {
    using halo2::physics_world_runtime_detail::WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t world_observation_pc(std::uint32_t disc,std::uint32_t tu15,const ObservedWorldState &state) {
    return state.selected_observation_pcs && state.image==SourcePhysicsContactImage::TitleUpdate15?tu15:disc;
}
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image) {
    if(image==SourcePhysicsContactImage::MapPack11)return p;
    switch(p) {
#include "array_selected.inc"
    default:throw std::runtime_error("Unmapped selected() operand");
    }
}
inline constexpr std::uint32_t grown_block=0x10800000u;
// Stand-in for 2E3250 (cdecl: [ESP+4] array, [ESP+8] element size; RET).
static void run_2df780(ObservedWorldState &s) {
    const auto ret=r(s.memory,s.gp[4]),array=r(s.memory,s.gp[4]+4u),size=r(s.memory,s.gp[4]+8u);
    const auto capacity=r(s.memory,array+8u);
    w(s.memory,s.observe,0x2e3260u,array,4,grown_block);
    w(s.memory,s.observe,0x2e3270u,array+8u,4,(capacity&0x80000000u)|((capacity&0x7fffffffu)*2u+1u));
    // The results also depend on the entry EAX, ECX, EDX and flags, so the
    // parent's register state at the call reaches the comparison (XMM2 is
    // not written again before the parent returns).
    s.xmm[2]=_mm_castsi128_ps(_mm_set_epi32(int(s.gp[0]),int(s.gp[1]),int(s.gp[2]),int(s.flags.bits())));
    s.gp[0]=array^size^s.gp[0];s.gp[1]=capacity+s.gp[1];s.gp[2]=ret^s.gp[2];s.flags.load((s.flags.bits()^0x893u)&0x8d5u);
    s.xmm[1]=_mm_castsi128_ps(_mm_set_epi32(int(array),int(size),int(capacity),int(ret)));
    s.gp[4]+=4u;
}
#include "array_300100.inc"
#include "array_300400.inc"
#include "array_reference.inc"
} // namespace t_array
using t_array::ObservedWorldState;

namespace {
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
    void put(std::uint32_t at,std::uint32_t value){for(unsigned n=0;n<4;++n)page(at+n,true)[(at+n)&4095u]=std::byte(value>>(8*n));}
    Memory clone()const {
        Memory copy;for(const auto &[k,v]:pages)copy.pages.emplace(k,std::make_shared<std::array<std::byte,4096>>(*v));return copy;
    }
    bool same(const Memory &other)const {
        if(pages.size()!=other.pages.size())return false;
        for(const auto &[k,v]:pages){const auto at=other.pages.find(k);if(at==other.pages.end()||*at->second!=*v)return false;}
        return true;
    }
};
std::uint32_t special(std::mt19937 &rng) {
    static constexpr std::uint32_t values[]{0x3f800000u,0xbf800000u,0u,0x80000000u,0x7fc00000u,0xffc12345u,
        0x7f812345u,0x7f800000u,0xff800000u,0x00000001u,0x807fffffu,0x7f7fffffu};
    if(rng()%3==0)return values[rng()%std::size(values)];
    return std::uint32_t(rng());
}
constexpr std::uint32_t array=0x10600000u,records=0x10700000u,stack=0x20010000u;
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t mxcsr{0x1f80u};unsigned kind{};};
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=n%16;c.kind=kind;
    // The array: size below, at or above the capacity; flag bit sometimes set.
    std::uint32_t capacity=1u+pick(12),size=pick(capacity);
    if(kind%3==1)size=capacity;                  // full: grows
    if(kind==5)size=capacity+1u+pick(2);         // over-full: no growth
    const std::uint32_t flag=pick(2)?0x80000000u:0u;
    std::uint32_t data=records;
    if(kind==7)data=records+0x1000u-0x80u*(size+1u);  // the new record ends at a page boundary
    m.put(array,data);m.put(array+4u,size);m.put(array+8u,flag|capacity);m.put(array+12u,std::uint32_t(rng()));
    for(std::uint32_t at=records;at<records+0x2000u;at+=4)m.put(at,special(rng));
    for(std::uint32_t at=t_array::grown_block;at<t_array::grown_block+0x1000u;at+=4)m.put(at,special(rng));
    // The stack: the return address, then random contents below (the local).
    const auto entry=stack-4u*pick(4);
    for(std::uint32_t at=stack-0x200u;at<stack+0x40u;at+=4)m.put(at,special(rng));
    m.put(entry,0x304500u+pick(0x100));
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[1]=array;c.abi.gp[4]=entry;
    c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u);
    for(auto &lane:c.abi.xmm)for(auto &v:lane)v=special(rng);
    c.mxcsr=0x1f80u|(pick(4)<<13)|(pick(2)?0x40u:0u)|(pick(2)?0x8000u:0u)|(pick(3)?0u:pick(64));
    if(kind==15) {
        static constexpr std::uint32_t regions[]{array,records,records+0x1000u,t_array::grown_block};
        m.pages.erase(regions[pick(4)]>>12);
    }
    return c;
}

struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;
    SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t cursor{},mxcsr{};bool admitted{};std::string fault;
};
bool census_mode{},verify{};
Result execute(Memory &memory,const Case &c,bool is_native) {
    Result result;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e){result.stores.push_back(e);};
    SourcePhysicsWorldRuntimeServices services;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(is_native && census_mode) {
        services.instruction=[](auto){throw std::runtime_error("Native contact array delivered a translated instruction");};
    } else if(is_native) {
        services.instruction=[](auto){throw std::runtime_error("Native contact array delivered a translated instruction");};
        auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
        cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    ObservedWorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);
    s.selected_observation_pcs=true;
    const auto saved=_mm_getcsr();_mm_setcsr(c.mxcsr);
    try {
        if(is_native)result.admitted=leaves::bridge_303ee0(s,[](auto &v){t_array::native_world_leaves_reference_300400(v);},
            [](auto &v){t_array::run_2df780(v);});
        else t_array::run_300400(s);
    } catch(const std::exception &e){result.fault=e.what();}
    result.mxcsr=_mm_getcsr();_mm_setcsr(saved);
    result.abi=s.save();return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    return a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth;
}
} // namespace

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):5000;
    const std::string mode=argc>2?argv[2]:"";
    census_mode=mode=="census";verify=mode=="verify";
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(verify)setenv("HALO2_VERIFY_PHYSICS_WORLD_LEAVES","1",1);
    std::mt19937 rng(0x303ee0u+(census_mode?1u:0u)+(verify?2u:0u));
    std::uint64_t stores{},compared{},faults{},grown{};
    for(unsigned n=0;n<cases;++n) {
        auto c=author(rng,n);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native_result=execute(right,c,true);
        if(!native_result.admitted && native_result.fault.empty()){std::fprintf(stderr,"case %u refused\n",n);return 1;}
        const auto reference=execute(left,c,false);
        ++compared;stores+=reference.stores.size();
        for(const auto &e:reference.stores)grown+=e.instruction==0x2e3260u;
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
        if(!census_mode && native_result.cursor!=reference.cursor) {
            std::fprintf(stderr,"case %u cursor %x vs %x\n",n,native_result.cursor,reference.cursor);return 1;
        }
        if(reference.fault.empty() && (!same_abi(native_result.abi,reference.abi) || native_result.mxcsr!=reference.mxcsr || !left.same(right))) {
            std::fprintf(stderr,"case %u (kind %u) mismatch:%s%s%s",n,c.kind,native_result.mxcsr!=reference.mxcsr?" mxcsr":"",
                left.same(right)?"":" bank",native_result.abi.eflags!=reference.abi.eflags?" eflags":"");
            for(unsigned i=0;i<8;++i)if(native_result.abi.gp[i]!=reference.abi.gp[i])
                std::fprintf(stderr," gp%u %x/%x",i,native_result.abi.gp[i],reference.abi.gp[i]);
            for(unsigned i=0;i<8;++i)if(native_result.abi.xmm[i]!=reference.abi.xmm[i])std::fprintf(stderr," xmm%u",i);
            std::fprintf(stderr,"\n");return 1;
        }
        if(!reference.fault.empty() && !left.same(right)){std::fprintf(stderr,"case %u fault bank mismatch\n",n);return 1;}
    }
    std::printf("{\"cases\":%u,\"compared\":%llu,\"faults\":%llu,\"ordered_stores\":%llu,\"grown\":%llu,"
        "\"coverage\":%llu,\"mode\":\"%s\"}\n",
        cases,(unsigned long long)compared,(unsigned long long)faults,(unsigned long long)stores,(unsigned long long)grown,
        (unsigned long long)leaves::coverage().load(),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

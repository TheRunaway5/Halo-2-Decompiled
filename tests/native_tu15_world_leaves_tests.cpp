// Native TU1.5 physics world leaves (1F5FE0 collision surface polygon, 2E4550
// relative transform, 303BA0 record copy, 2E47B0 transposed rotation, 2E4760
// rotation into world) against
// the unmodified translated canonical bodies (packed SSE with the production
// binder's explicit destinations), extracted by
// tools/validate_native_world_leaves_tu15.py into world_<name>.inc with the
// unit's selected() operand cases (world_selected.inc). Seeded fixtures author
// complete BSP surfaces (surface table, 12-byte edge rings with both sides,
// vertex rows, optional scaled transforms) with special floats (NaN payloads,
// SNaN, infinities, denormals, signed zeros), every MXCSR rounding/DAZ/FTZ
// mode, negative and garbage surface indices, out-of-range vertex indices,
// aliased and misaligned transform/record operands and fault injection (one
// fixture page removed). Each case compares the ordered
// store events (pc, address, preimage, bytes), the final bank, registers,
// flags, XMM, MXCSR and the instruction cursor; fault cases compare the
// exception, the cursor and the store prefix.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "native_tu15_world_leaves_remap.hpp"
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

namespace t_world {
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
#include "world_selected.inc"
    default:throw std::runtime_error("Unmapped selected() operand");
    }
}
std::uint32_t sign_extend(std::uint32_t value,unsigned count) {
    if(count==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    if(count==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    throw std::runtime_error("Unverified sign-extension width");
}
#include "world_1f16f0.inc"
#include "world_2e0a60.inc"
#include "world_300100.inc"
#include "world_2e0cc0.inc"
#include "world_2e0c70.inc"
} // namespace t_world
using t_world::ObservedWorldState;

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

constexpr std::uint32_t shape=0x10000000u,bsp=0x10100000u,surfaces=0x10200800u,edges=0x10300000u,
    vertices=0x10400000u,transform=0x10500000u,stack=0x20010000u;

std::uint32_t special(std::mt19937 &rng) {
    static constexpr std::uint32_t values[]{0x3f800000u,0xbf800000u,0u,0x80000000u,0x7fc00000u,0xffc12345u,
        0x7f812345u,0x7f800000u,0xff800000u,0x00000001u,0x807fffffu,0x7f7fffffu,0x3f800001u,0x3f7fffffu,0x00800000u};
    if(rng()%5==0)return values[rng()%std::size(values)];
    const float value=float(int(rng()%20001)-10000)/float(1+rng()%997);
    return std::bit_cast<std::uint32_t>(value);
}

enum Owner : unsigned {k1f5fe0,k2e4550,k303ba0,k2e47b0,k2e4760,owners};
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t mxcsr{0x1f80u};unsigned shape_kind{};Owner owner{};};

constexpr std::uint32_t operands=0x10600000u;
// Transform/record owners: aligned operand blocks that may alias each other.
Case author_block(std::mt19937 &rng,unsigned n,Owner owner) {
    Case c;c.owner=owner;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=(n/owners)%16;c.shape_kind=kind;
    // Kind 13: NaNs with distinct payloads in most words, so that both
    // operands of a product or sum are NaNs (the first one's payload wins).
    const auto payload=[&]{return 0x7f800001u|(std::uint32_t(rng())&0x807fffffu);};
    for(std::uint32_t at=operands;at<operands+0x400u;at+=4)
        m.put(at,kind==13 && pick(4)?payload():pick(3)?special(rng):std::uint32_t(rng()));
    std::uint32_t a=operands,b=operands+0x100u,dst=operands+0x200u;
    if(kind==2)dst=b;                  // in place
    if(kind==4)dst=a;                  // over the transform
    if(kind==6)dst=b+0x10u;            // overlapping rows
    if(kind==8)dst+=4u+4u*pick(3);     // unaligned (the translated route has no alignment check)
    if(kind==10)b=a;                   // transform composed with itself
    if(kind==12) {                     // rows at the top: the last ADD EAX,0x10 carries
        b=0xffffffc0u;
        for(std::uint32_t at=b;at>=b && at<=0xfffffffcu;at+=4)m.put(at,pick(3)?special(rng):std::uint32_t(rng()));
    }
    const auto stack_base=stack-4u*pick(4);
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack_base,0x300000u+pick(0x100));m.put(stack_base+4u,a);m.put(stack_base+8u,b);
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[1]=dst;c.abi.gp[4]=stack_base;
    c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u);
    for(auto &lane:c.abi.xmm)for(auto &v:lane)v=pick(3)?std::uint32_t(rng()):special(rng);
    c.mxcsr=0x1f80u|(pick(4)<<13)|(pick(2)?0x40u:0u)|(pick(2)?0x8000u:0u)|(pick(3)?0u:pick(64));
    if(kind==15)m.pages.erase(operands>>12);
    if(kind==14)m.put(stack_base+pick(3)*4u,0x10700000u+pick(0x100)*16u); // escapes the fixture
    return c;
}

Case author(std::mt19937 &rng,unsigned n) {
    if(n%owners!=k1f5fe0)return author_block(rng,n,Owner(n%owners));
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=(n/owners)%16;c.shape_kind=kind;
    m.put(0x48b880u,special(rng));
    m.put(0x46a0d0u,pick(4)?0x3f800000u:special(rng));
    // Surface index: small, negative, or carrying upper garbage.
    std::uint32_t surface=pick(16);
    if(kind==3)surface=std::uint32_t(-1-int(pick(3)));
    if(kind==5)surface=(std::uint32_t(rng())&0xffff0000u)|pick(16);
    const bool garbage=kind==5 && (surface>>16)!=0 && (surface>>16)!=0xffffu;
    // Shape, BSP header and the tables.
    for(std::uint32_t at=shape;at<shape+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(bsp+0x2cu,surfaces);m.put(bsp+0x34u,edges);m.put(bsp+0x3cu,vertices);
    for(std::uint32_t at=bsp;at<bsp+0x40u;at+=4)if(at!=bsp+0x2cu&&at!=bsp+0x34u&&at!=bsp+0x3cu)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=surfaces-0x40u;at<surfaces+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=edges;at<edges+64u*12u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=vertices;at<vertices+64u*16u;at+=4)m.put(at,special(rng));
    // The edge ring: distinct edges, each with a side and a vertex.
    const unsigned length=kind==7?1u:1u+pick(kind==9?16u:8u);
    std::vector<std::uint32_t> ring;
    while(ring.size()<length) {
        const auto e=pick(64);bool used=false;for(auto v:ring)used|=v==e;
        if(!used)ring.push_back(e);
    }
    const auto s16=std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(surface))));
    for(unsigned k=0;k<length;++k) {
        const auto e=edges+ring[k]*12u;
        bool side=!garbage && s16==surface && pick(2);
        if(kind==11)side=!garbage && s16==surface;
        m.put(e+0xau,side?surface:(surface+1u+pick(0x7ffe)),2);
        std::uint32_t vertex=pick(64);
        if(kind==13 && pick(4)==0)vertex=0xffffu;
        m.put(e+(side?2u:0u),vertex,2);
        m.put(e+(side?6u:4u),ring[(k+1)%length],2);
    }
    m.put(surfaces+surface*8u+2u,ring[0],2);
    // Optional transform: scale (often exactly the 46A0D0 constant), rows.
    const bool transformed=kind!=1 && pick(4)!=0;
    for(std::uint32_t at=transform;at<transform+0x40u;at+=4)m.put(at,special(rng));
    if(pick(2))m.put(transform,0x3f800000u);
    // Stack: return address and five arguments; locals below.
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1f7c00u+pick(0x100));m.put(stack+4u,bsp);m.put(stack+0x10u,surface);
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[1]=shape;c.abi.gp[4]=stack;c.abi.gp[7]=transformed?transform:0u;
    c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u);
    for(auto &lane:c.abi.xmm)for(auto &b:lane)b=pick(3)?std::uint32_t(rng()):special(rng);
    // Every rounding mode, DAZ and FTZ, sticky flags set or clear; masked.
    c.mxcsr=0x1f80u|(pick(4)<<13)|(pick(2)?0x40u:0u)|(pick(2)?0x8000u:0u)|(pick(3)?0u:pick(64));
    // Fault injection: remove one authored page (not the stack).
    if(kind==15) {
        static constexpr std::uint32_t regions[]{vertices,edges,surfaces,transform,shape,0x48b000u,0x46a000u,bsp};
        m.pages.erase(regions[pick(8)]>>12);
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
    if(is_native && (census_mode || !leaves_remap::live_cursor)) {
        services.instruction=[](auto){throw std::runtime_error("Native world leaf delivered a translated instruction");};
    } else if(is_native) {
        services.instruction=[](auto){throw std::runtime_error("Native world leaf delivered a translated instruction");};
        auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
        cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    SourcePhysicsWorldRuntimeServices outer;leaves_remap::wrap(services,outer);
    ObservedWorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);
    s.selected_observation_pcs=!leaves_remap::enabled;
    const auto saved=_mm_getcsr();_mm_setcsr(c.mxcsr);
    try {
        if(is_native)switch(c.owner) {
        case k1f5fe0:result.admitted=leaves::bridge_1f5fe0(s,[](auto &v){t_world::run_1f16f0(v);});break;
        case k2e4550:result.admitted=leaves::bridge_2e4550(s,[](auto &v){t_world::run_2e0a60(v);});break;
        case k303ba0:result.admitted=leaves::bridge_303ba0(s,[](auto &v){t_world::run_300100(v);});break;
        case k2e47b0:result.admitted=leaves::bridge_2e47b0(s,[](auto &v){t_world::run_2e0cc0(v);});break;
        default:result.admitted=leaves::bridge_2e4760(s,[](auto &v){t_world::run_2e0c70(v);});break;
        } else switch(c.owner) {
        case k1f5fe0:t_world::run_1f16f0(s);break;
        case k2e4550:t_world::run_2e0a60(s);break;
        case k303ba0:t_world::run_300100(s);break;
        case k2e47b0:t_world::run_2e0cc0(s);break;
        default:t_world::run_2e0c70(s);break;
        }
    } catch(const std::exception &e){result.fault=e.what();}
    result.mxcsr=_mm_getcsr();_mm_setcsr(saved);
    result.abi=s.save();return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    return a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth;
}
bool refused(SourcePhysicsContactImage image,bool selected,bool callbacks,bool trace,bool arbitrary,std::uint32_t mxcsr) {
    std::mt19937 rng(7);auto c=author(rng,0);
    SourcePhysicsContactRecordObserver observe;if(arbitrary)observe=[](const auto&){};
    SourcePhysicsWorldRuntimeServices services;bool active=trace;std::uint32_t cursor{};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(callbacks)services.instruction=[](auto){};
    if(trace) {
        services.instruction_cursor.position=&cursor;services.instruction_cursor.service_sites=sites;
        services.instruction_cursor.callback_type=&services.instruction.target_type();
        services.instruction_cursor.executor=std::this_thread::get_id();services.instruction_cursor.literal_service_sites=true;
        services.instruction_cursor.first_trace=&active;services.instruction_cursor.observation_only_stores=true;
    }
    ObservedWorldState s{c.memory,observe,services,image};s.load(c.abi);s.selected_observation_pcs=selected;
    const auto saved=_mm_getcsr();_mm_setcsr(mxcsr);
    bool admitted=true;
    try {admitted=leaves::bridge_1f5fe0(s,[](auto &v){t_world::run_1f16f0(v);});} catch(...) {admitted=true;}
    _mm_setcsr(saved);
    return !admitted;
}
} // namespace

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):5000;
    const std::string mode=argc>2?argv[2]:"";
    const auto base=leaves_remap::parse(mode);census_mode=base=="census";verify=base=="verify";
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(verify)setenv("HALO2_VERIFY_PHYSICS_WORLD_LEAVES","1",1);
    std::mt19937 rng(0x1f5fe0u+(census_mode?1u:0u)+(verify?2u:0u)+(leaves_remap::enabled?4u:0u));
    std::uint64_t stores{},compared{},faults{};std::array<std::uint64_t,owners> per{};
    for(unsigned n=0;n<cases;++n) {
        auto c=author(rng,n);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native_result=execute(right,c,true);
        if(!native_result.admitted && native_result.fault.empty()){std::fprintf(stderr,"case %u refused\n",n);return 1;}
        const auto reference=execute(left,c,false);
        ++compared;++per[c.owner];stores+=reference.stores.size();
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
            std::fprintf(stderr,"case %u register/flag/MXCSR/bank mismatch\n",n);return 1;
        }
        if(!reference.fault.empty() && !left.same(right)){std::fprintf(stderr,"case %u fault bank mismatch\n",n);return 1;}
    }
    // Admission refusals: other image, canonical observation PCs, full
    // callbacks without a cursor, traces, arbitrary observers, unmasked MXCSR.
    if(!census_mode) {
        if(!refused(SourcePhysicsContactImage::MapPack11,true,false,false,false,0x1f80u))throw std::runtime_error("MapPack11 admitted");
        if(!refused(SourcePhysicsContactImage::TitleUpdate15,false,false,false,false,0x1f80u))throw std::runtime_error("Canonical observation PCs admitted");
        if(!refused(SourcePhysicsContactImage::TitleUpdate15,true,true,false,false,0x1f80u))throw std::runtime_error("Full instruction callbacks admitted");
        if(!refused(SourcePhysicsContactImage::TitleUpdate15,true,true,true,false,0x1f80u))throw std::runtime_error("Active trace admitted");
        if(!refused(SourcePhysicsContactImage::TitleUpdate15,true,false,false,false,0x1d80u))throw std::runtime_error("Unmasked MXCSR admitted");
    }
    // Under the composition's remap each owner refuses a table that does not
    // map its canonical instructions with one delta, and a remap without a
    // live cursor; with both it runs.
    if(leaves_remap::enabled && !census_mode) {
        constexpr std::uint32_t canonical[owners][2]{{0x1f16f0u,0x1f18b6u},{0x2e0a60u,0x2e0b55u},{0x300100u,0x30015eu},
            {0x2e0cc0u,0x2e0d2cu},{0x2e0c70u,0x2e0cb4u}};
        for(unsigned o=0;o<owners;++o) {
            const auto ran=[&] {
                std::mt19937 local(0x5eed0u+o);auto c=author(local,o);auto bank=c.memory.clone();
                const auto r=execute(bank,c,true);return r.admitted || !r.fault.empty();
            };
            if(!leaves_remap::refuses_bad_remaps(canonical[o][0],canonical[o][1],ran))
                throw std::runtime_error("A world leaf admitted a remap that does not cover it");
        }
    }
    std::printf("{\"cases\":%u,\"compared\":%llu,\"faults\":%llu,\"ordered_stores\":%llu,\"per_owner\":[%llu,%llu,%llu,%llu,%llu],"
        "\"coverage\":%llu,\"branches\":%u,\"mode\":\"%s\"}\n",
        cases,(unsigned long long)compared,(unsigned long long)faults,(unsigned long long)stores,
        (unsigned long long)per[0],(unsigned long long)per[1],(unsigned long long)per[2],(unsigned long long)per[3],(unsigned long long)per[4],
        (unsigned long long)leaves::coverage().load(),unsigned(leaves::branch_count),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

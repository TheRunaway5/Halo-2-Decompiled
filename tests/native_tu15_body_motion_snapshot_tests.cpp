// Native TU1.5 1D7260 body motion snapshot (a havok component's snapshot of
// one body's position and velocities) against the unmodified translated
// canonical body run_1d29a0, extracted by
// tools/validate_native_body_motion_snapshot_tu15.py into snapshot_bodies.inc
// with the unit's selected() operand cases (snapshot_selected.inc) and the
// binder's verification reference (snapshot_reference.inc).
//
// The translated child 1D8B20 (the position query) is a deterministic
// stand-in, entered by both routes with the same state. It appends its
// complete entry state (target, general registers, flags, x87 depth/status/
// TOP, an XMM digest) to a log in the fixture, reads the component and its
// slot, writes the three position words to EDI's address, and sometimes
// moves the component's slot array, changes the body's motionless byte or
// its motion; it keeps EBX, ESI, EDI and EBP and changes the scratch
// registers, flags and XMM registers (XMM0 included).
//
// Seeded fixtures: indices inside the slot array, below it and wrapping;
// slots whose body is moving or motionless (any non-zero byte), motions and
// zero vectors of random and special words (NaN payloads, SNaN, infinities,
// denormals, signed zeros); aliasing that makes the access order observable:
// the slot inside the frame's locals (kind 13), the zero vector on the
// locals (kind 12), the motion's velocities on the locals and the body's
// motion pointer in them (kind 11); a velocity that straddles a missing page
// (kind 14); random register files, x87 depths, an active MMX state, every
// MXCSR mode and fault injection with one fixture page removed. Each case
// compares the ordered store events (pc, address, preimage, bytes), the final
// bank, registers, flags, XMM, x87, MXCSR and the cursor; fault cases compare
// the exception, the cursor and the bank.
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

namespace t_snapshot {
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
#include "snapshot_selected.inc"
    default:throw std::runtime_error("Unmapped selected() operand");
    }
}
// Fixture addresses (the zero vector's pointer at its TU1.5 literal).
inline constexpr std::uint32_t globals_page=0x474000u,components=0x10100000u,slots_base=0x10110000u,moved_slots=0x10114000u,
    bodies=0x10120000u,motions=0x10130000u,zero_base=0x10140000u,log_base=0x10900000u,fs=0x10700000u;
inline constexpr std::uint32_t slot_count=16u;
inline void log_entry(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,std::uint32_t extra=0) {
    const auto count=r(s.memory,log_base);
    const auto at=log_base+0x10u+(count%250u)*0x48u;
    std::uint32_t xmm_digest=0;
    for(unsigned i=0;i<8;++i) {
        std::uint32_t lanes[4];std::memcpy(lanes,&s.xmm[i],16);
        for(const auto lane:lanes)xmm_digest=(xmm_digest*0x9e3779b1u)^lane;
    }
    const std::uint32_t words[]{target,site,s.gp[0],s.gp[1],s.gp[2],s.gp[3],s.gp[4],s.gp[5],s.gp[6],s.gp[7],
        s.flags.bits(),std::uint32_t(s.fp.count),std::uint32_t(s.fp.status),std::uint32_t(s.fp.top),xmm_digest,count,extra};
    for(unsigned i=0;i<17;++i)w(s.memory,s.observe,0x7a0000u,at+4u*i,4,words[i]);
    w(s.memory,s.observe,0x7a0004u,log_base,4,count+1u);
}
// 1D8B20 (canonical 1D4260; ECX = the component, EAX = the index, EDI = the
// output; RET).
static void run_1d4260(ObservedWorldState &s) {
    const auto component=s.gp[1],index=s.gp[0],out=s.gp[7];
    auto seed=(component*0x9e3779b1u)^(index*0x85ebca6bu)^r(s.memory,globals_page+0xf00u);
    const auto slots=r(s.memory,component+0x70u);
    seed=seed*0x2c1b3c6du^r(s.memory,slots+index*0x60u+0x40u);
    log_entry(s,0x1d8b20u,0,seed);
    for(unsigned i=0;i<3;++i)w(s.memory,s.observe,0x1d8b78u+2u*i,out+4u*i,4,seed*(i+3u)^0x7fc00000u*(seed>>30&1u));
    // Sometimes the slot array moves (the original re-reads it for the
    // body, and still writes the slot it located first), the body becomes
    // motionless or moving, or its motion changes.
    if(seed%5u==0u)w(s.memory,s.observe,0x1d8b90u,component+0x70u,4,slots==slots_base?moved_slots:slots_base);
    const auto body=r(s.memory,r(s.memory,component+0x70u)+index*0x60u+0x40u);
    if(seed%7u==1u)w(s.memory,s.observe,0x1d8b94u,body+0x40u,1,(seed>>9)&1u?0u:(seed>>10)|1u);
    if(seed%11u==2u)w(s.memory,s.observe,0x1d8b98u,body+0x3cu,4,motions+0x100u*((seed>>12)%8u));
    const auto keep_ebx=s.gp[3];
    s.gp[0]^=seed*0x2c1b3c6du;s.gp[1]^=seed*0x45d9f3bu;s.gp[2]=seed^0x5a5a5a5au;s.flags.load((seed*0x2c1b3c6du)&0x8d5u);
    for(unsigned i=0;i<8;i+=1+(seed>>i&1u))
        s.xmm[i]=_mm_castsi128_ps(_mm_set_epi32(int(seed*i),int(seed*3u+i),int(seed^0x80000000u),int(seed+7u*i)));
    s.gp[3]=keep_ebx;s.gp[4]+=4u;
}
static void dispatch(ObservedWorldState &,std::uint32_t target,std::uint32_t site,bool) {
    char text[96];std::snprintf(text,sizeof text,"Unbound test dispatch target %x at %x",target,site);
    throw std::runtime_error(text);
}
#include "snapshot_bodies.inc"
#include "snapshot_reference.inc"
} // namespace t_snapshot
using t_snapshot::ObservedWorldState;

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
    void put8(std::uint32_t at,std::uint32_t value){page(at,true)[at&4095u]=std::byte(value);}
    void pages_at(std::uint32_t at,unsigned count){for(unsigned n=0;n<count;++n)page(at+n*4096u,true);}
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
constexpr std::uint32_t stack=0x20010000u;
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t mxcsr{0x1f80u};unsigned kind{};bool no_counter{};};
// A random register file, x87 stack and MXCSR mode.
void registers(Case &c,std::mt19937 &rng,std::uint32_t entry,unsigned kind) {
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[4]=entry;c.abi.fs_base=t_snapshot::fs;
    c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u);
    for(auto &lane:c.abi.xmm)for(auto &v:lane)v=special(rng);
    c.abi.x87_depth=kind==8?6u+pick(2):pick(4);
    for(unsigned i=0;i<c.abi.x87_depth;++i)c.abi.x87[i]=static_cast<long double>(i)+0.5L;
    c.abi.x87_status=std::uint16_t((((8u-c.abi.x87_depth)&7u)<<11)|(pick(2)?0x4100u:0u)|(pick(2)?0x200u:0u));
    if(kind==9) {
        c.abi.x87_depth=0;c.abi.x87_status=0;
        SourcePhysicsMmxAlias alias{};alias.known_mask=std::uint8_t(pick(256));
        c.abi.active_mmx=alias;
    }
    c.mxcsr=0x1f80u|(pick(4)<<13)|(pick(2)?0x40u:0u)|(pick(2)?0x8000u:0u)|(pick(3)?0u:pick(64));
}
Case author_snapshot(std::mt19937 &rng,unsigned n) {
    using namespace t_snapshot;
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=n%16;c.kind=kind;
    m.pages_at(globals_page,1);m.pages_at(components,1);m.pages_at(slots_base,8);m.pages_at(bodies,1);m.pages_at(motions,1);
    m.pages_at(zero_base,1);m.pages_at(log_base,5);
    m.put(globals_page+0xf00u,std::uint32_t(rng()));  // the stand-in's seed
    const auto entry=stack-4u*pick(4),t=entry-0x34u;
    for(std::uint32_t at=stack-0x1000u;at<stack+0x40u;at+=4)m.put(at,special(rng));
    // The zero vector (random words, so that a wrong source shows); kind 12
    // puts it on the frame's locals (the linear velocity's or the angular's).
    for(std::uint32_t at=0;at<0x100u;at+=4)m.put(zero_base+at,pick(3)?0u:special(rng));
    const auto zero=kind==12?t+0x10u+4u*pick(6):zero_base+0x10u*pick(8);
    m.put(0x474d04u,zero);
    // Motions: random and special words.
    for(std::uint32_t at=0;at<0x1000u;at+=4)m.put(motions+at,special(rng));
    // Bodies: a motion each (kind 11 also on the frame: its velocities on the
    // locals), and a motionless byte (zero, or any other value).
    for(std::uint32_t b=0;b<32;++b) {
        const auto body=bodies+0x80u*b;
        for(std::uint32_t at=0;at<0x80u;at+=4)m.put(body+at,std::uint32_t(rng()));
        const auto motion=kind==11 && pick(2)?t-0x40u+4u*pick(12)-(pick(2)?0x10u:0u):motions+0x100u*pick(8)+4u*pick(4);
        m.put(body+0x3cu,motion);
        m.put(body+0x40u,(std::uint32_t(rng())&0xffffff00u)|(pick(2)?0u:1u+pick(255)));
    }
    // Two slot arrays (the stand-in may switch the component to the other).
    for(const auto array:{slots_base,moved_slots})
        for(std::uint32_t at=0;at<0x60u*slot_count+0x60u;at+=4)m.put(array-0x60u+at,std::uint32_t(rng()));
    for(const auto array:{slots_base,moved_slots}) {
        for(std::uint32_t i=0;i<=slot_count;++i)m.put(array-0x60u+0x60u*i+0x40u,bodies+0x80u*pick(32));
        // The wrapping indices' slots (index*60 wraps to 200, 260, ...).
        for(std::uint32_t i=0;i<4;++i)if(pick(2))m.put(array+0x240u+0x60u*i,bodies+0x80u*pick(32));
    }
    // Kind 11: a body whose motion pointer lies in the linear velocity's locals.
    // The component: its slot array, and the index (inside, below the array,
    // or wrapping out of the fixture).
    const auto component=components+0x100u*pick(15)+4u*pick(4);
    for(std::uint32_t at=0;at<0xa0u;at+=4)m.put(component+at,std::uint32_t(rng()));
    std::uint32_t index=pick(slot_count);
    if(pick(16)==0)index=0xffffffffu;  // the slot below the array
    if(pick(40)==0)index=0x2aaaaab0u+pick(4);  // index*60 wraps
    std::uint32_t array=slots_base;
    // Kind 13: the slot inside the frame (the snapshot overwrites locals
    // before they are read back); its body pointer above the frame.
    if(kind==13) {
        const auto slot=t-0x30u+4u*pick(24);
        array=slot-index*0x60u;
        m.put(slot+0x40u,bodies+0x80u*pick(32));
    }
    m.put(component+0x70u,array);
    // Kind 11: half the time the slot's body lies on the frame, its motion
    // pointer on a linear velocity local (the staging overwrites it before
    // the angular velocity re-reads it) and its motionless byte clear.
    if(kind==11 && pick(2) && (index<slot_count || index==0xffffffffu)) {
        const auto lane=pick(3),body=t+0x10u+4u*lane-0x3cu;
        for(const auto base:{slots_base,moved_slots})m.put(base+index*0x60u+0x40u,body);
        m.put(t+0x10u+4u*lane,motions+0x100u*pick(8));
        m.put8(t+0x14u+4u*lane,0u);
        for(std::uint32_t at=0;at<0x800u;at+=0x100u)if(pick(2))m.put(motions+at+0x40u+4u*lane,motions+0x100u*pick(8));
    }
    m.put(entry,0x281688u);m.put(entry+4u,component);
    m.put(t_snapshot::fs,0x2001ff00u);
    registers(c,rng,entry,kind);
    c.abi.gp[0]=index;
    if(kind==14) {
        // A motion whose velocities straddle a missing page.
        const auto motion=motions+0x1000u-0x40u-4u*(1u+pick(8));
        for(std::uint32_t b=0;b<32;++b)if(pick(2))m.put(bodies+0x80u*b+0x3cu,motion);
    }
    if(kind==15) {
        static constexpr std::uint32_t regions[]{globals_page,components,slots_base,bodies,motions,zero_base,log_base,stack-0x1000u};
        m.pages.erase(regions[pick(std::size(regions))]>>12);
    }
    return c;
}

struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;
    SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t cursor{},mxcsr{};bool admitted{};std::string fault;
};
bool census_mode{},verify{};
template<class Native,class Translated> Result execute(Memory &memory,const Case &c,bool is_native,Native native,Translated translated) {
    Result result;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e){result.stores.push_back(e);};
    SourcePhysicsWorldRuntimeServices services;
    std::uint64_t counter=0x123456789abcull;
    if(!c.no_counter)services.timestamp_counter=[&counter]{counter+=0x10203u;return counter;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(is_native) {
        services.instruction=[](auto){throw std::runtime_error("Native body motion snapshot delivered a translated instruction");};
        if(!census_mode && leaves_remap::live_cursor) {
            auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
            cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
            cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        }
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    SourcePhysicsWorldRuntimeServices outer;leaves_remap::wrap(services,outer);
    ObservedWorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    try{s.load(c.abi);}catch(const std::exception &e){result.fault=e.what();return result;}
    s.selected_observation_pcs=!leaves_remap::enabled;
    const auto saved=_mm_getcsr();_mm_setcsr(c.mxcsr);
    try {
        if(is_native)result.admitted=native(s);else translated(s);
    } catch(const std::exception &e){result.fault=e.what();}
    result.mxcsr=_mm_getcsr();_mm_setcsr(saved);
    result.abi=s.save();return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    bool same=a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth &&
        a.fs_base==b.fs_base && a.active_mmx.has_value()==b.active_mmx.has_value();
    for(unsigned i=0;i<a.x87_depth && i<8;++i)same=same && !std::memcmp(&a.x87[i],&b.x87[i],10);
    return same;
}
struct Totals {std::uint64_t compared{},stores{},faults{},children{};};
template<class Author,class Native,class Translated>
int compare(const char *owner,unsigned cases,std::uint32_t seed,Totals &totals,Author author,Native native,Translated translated) {
    std::mt19937 rng(seed);
    for(unsigned n=0;n<cases;++n) {
        auto c=author(rng,n);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native_result=execute(right,c,true,native,translated);
        if(!native_result.admitted && native_result.fault.empty()){std::fprintf(stderr,"%s case %u refused\n",owner,n);return 1;}
        const auto reference=execute(left,c,false,native,translated);
        ++totals.compared;totals.stores+=reference.stores.size();
        for(const auto &e:reference.stores)totals.children+=e.instruction==0x7a0004u;
        if(!reference.fault.empty())++totals.faults;
        if(native_result.fault!=reference.fault) {
            std::fprintf(stderr,"%s case %u (kind %u) fault mismatch: native '%s' translated '%s'\n",owner,n,c.kind,
                native_result.fault.c_str(),reference.fault.c_str());
            return 1;
        }
        if(native_result.stores!=reference.stores) {
            std::size_t at=0;while(at<native_result.stores.size()&&at<reference.stores.size()&&native_result.stores[at]==reference.stores[at])++at;
            std::fprintf(stderr,"%s case %u (kind %u) store mismatch at %zu of %zu/%zu",owner,n,c.kind,at,native_result.stores.size(),
                reference.stores.size());
            if(at<native_result.stores.size()&&at<reference.stores.size()) {
                const auto &a=native_result.stores[at],&b=reference.stores[at];
                std::fprintf(stderr," native pc=%x a=%x original pc=%x a=%x",a.instruction,a.address,b.instruction,b.address);
                if(a.instruction==b.instruction && a.address==b.address && a.written!=b.written) {
                    std::fprintf(stderr," bytes");
                    for(std::size_t i=0;i<a.written.size();++i)std::fprintf(stderr," %02x/%02x",unsigned(a.written[i]),unsigned(b.written[i]));
                }
            } else if(at<reference.stores.size())std::fprintf(stderr," original pc=%x",reference.stores[at].instruction);
            else if(at<native_result.stores.size())std::fprintf(stderr," native pc=%x",native_result.stores[at].instruction);
            std::fprintf(stderr," (%s)\n",reference.fault.c_str());return 1;
        }
        if(!census_mode && native_result.cursor!=reference.cursor) {
            std::fprintf(stderr,"%s case %u (kind %u) cursor %x vs %x (%s)\n",owner,n,c.kind,native_result.cursor,reference.cursor,
                reference.fault.c_str());
            return 1;
        }
        if(reference.fault.empty() && (!same_abi(native_result.abi,reference.abi) || native_result.mxcsr!=reference.mxcsr || !left.same(right))) {
            std::fprintf(stderr,"%s case %u (kind %u) mismatch:%s%s%s",owner,n,c.kind,native_result.mxcsr!=reference.mxcsr?" mxcsr":"",
                left.same(right)?"":" bank",native_result.abi.eflags!=reference.abi.eflags?" eflags":"");
            if(native_result.abi.eflags!=reference.abi.eflags)std::fprintf(stderr," %x/%x",native_result.abi.eflags,reference.abi.eflags);
            for(unsigned i=0;i<8;++i)if(native_result.abi.gp[i]!=reference.abi.gp[i])
                std::fprintf(stderr," gp%u %x/%x",i,native_result.abi.gp[i],reference.abi.gp[i]);
            for(unsigned i=0;i<8;++i)if(native_result.abi.xmm[i]!=reference.abi.xmm[i])std::fprintf(stderr," xmm%u",i);
            if(native_result.abi.x87_status!=reference.abi.x87_status || native_result.abi.x87_depth!=reference.abi.x87_depth)
                std::fprintf(stderr," x87 %x/%x depth %u/%u",native_result.abi.x87_status,reference.abi.x87_status,
                    native_result.abi.x87_depth,reference.abi.x87_depth);
            std::fprintf(stderr,"\n");return 1;
        }
        if(!reference.fault.empty() && !left.same(right)){std::fprintf(stderr,"%s case %u fault bank mismatch\n",owner,n);return 1;}
    }
    return 0;
}
} // namespace

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    const auto base=leaves_remap::parse(mode);census_mode=base=="census";verify=base=="verify";
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(verify)setenv("HALO2_VERIFY_PHYSICS_WORLD_LEAVES","1",1);
    const std::uint32_t variant=(census_mode?1u:0u)+(verify?2u:0u)+(leaves_remap::enabled?4u:0u);
    Totals snapshot;
    const auto native=[](ObservedWorldState &s){return leaves::bridge_1d7260(s,[](auto &v){t_snapshot::native_world_leaves_reference_1d29a0(v);},
            [](auto &v){t_snapshot::run_1d4260(v);});};
    const auto translated=[](ObservedWorldState &s){t_snapshot::run_1d29a0(s);};
    if(compare("1D7260",cases,0x1d7260u+variant,snapshot,author_snapshot,native,translated))return 1;
    // Under the composition's remap the bridge refuses a table that does not
    // map its canonical instructions with one delta, and a remap without a
    // live cursor; with both it runs.
    if(leaves_remap::enabled && !census_mode) {
        const auto ran=[&] {
            std::mt19937 local(0x5eed0u);auto c=author_snapshot(local,0);auto bank=c.memory.clone();
            const auto r=execute(bank,c,true,native,translated);return r.admitted || !r.fault.empty();
        };
        if(!leaves_remap::refuses_bad_remaps(0x1d29a0u,0x1d2a90u,ran))
            throw std::runtime_error("1D7260 admitted a remap that does not cover it");
    }
    std::printf("{\"cases\":%u,\"compared\":%llu,\"ordered_stores\":%llu,\"faults\":%llu,\"child_calls\":%llu,"
        "\"coverage\":%llu,\"coverage_high\":%llu,\"coverage_top\":%llu,\"mode\":\"%s\"}\n",
        cases,(unsigned long long)snapshot.compared,(unsigned long long)snapshot.stores,(unsigned long long)snapshot.faults,
        (unsigned long long)snapshot.children,(unsigned long long)leaves::coverage(0).load(),(unsigned long long)leaves::coverage(1).load(),
        (unsigned long long)leaves::coverage(2).load(),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

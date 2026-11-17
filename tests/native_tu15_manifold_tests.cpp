// Native TU1.5 manifold owners (the children of 304350): the 30CF40 manifold
// report and the 304130 contact weld against the unmodified translated
// canonical bodies run_309450 and run_300650,
// extracted by tools/validate_native_manifold_tu15.py into
// manifold_bodies.inc with the unit's selected() operand cases
// (manifold_selected.inc) and the binder's verification references
// (manifold_reference.inc).
//
// The translated child (2E31E0, hkArrayUtil::_reserve) and the virtual target
// (the contact manager's removal) are deterministic stand-ins, entered by both
// routes with the same state. Each appends its complete entry state (target,
// canonical site, general registers, flags, x87 depth/status/TOP, an XMM
// digest) to a log in the fixture and reads its stack arguments, so a wrong
// register, flag, x87 state or argument at a call reaches the store stream.
// They keep EBX, ESI, EDI and EBP, and change the scratch registers, flags and
// XMM registers. The reserve moves the points to a grown block; the removal
// sometimes changes the count, a cached point or the output's size, which the
// original re-reads.
//
// Seeded fixtures: 0-9 cached points whose two points coincide in world space
// (kept), or are moved along the normal (the tolerance and separation tests)
// or across it (the drift test), random and special floats (NaN payloads,
// SNaN, infinities, denormals, signed zeros), outputs with and without room
// (growth to twice the capacity or to what is needed, borrowed storage),
// negative and zero counts, random register files, x87 depths 0-7 (7
// overflows the x87 stack, as in the original), an active MMX state, every
// MXCSR mode, and fault injection with one fixture page removed. Each case
// compares the ordered store events (pc, address, preimage, bytes), the final
// bank, registers, flags, XMM, x87, MXCSR and the cursor; fault cases compare
// the exception, the cursor and the bank.
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

namespace t_manifold {
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
#include "manifold_selected.inc"
    default:throw std::runtime_error("Unmapped selected() operand");
    }
}
// Fixture addresses.
inline constexpr std::uint32_t body_a=0x10100000u,body_b=0x10101000u,motions=0x10110000u,input=0x10120000u,
    settings=0x10121000u,points=0x10130000u,count_block=0x10140000u,output=0x10150000u,output_array=0x10151000u,
    output_data=0x10160000u,grown=0x10180000u,manager=0x10190000u,manager_vtable=0x10191000u,log_base=0x10900000u,
    fs=0x10700000u,one_page=0x41c000u;
inline constexpr std::uint32_t remove_target=0x7c0008u;
inline std::uint32_t weld_entry{};  // the current 304130 case's entry ESP (its stand-in rewrites n)
inline constexpr std::uint32_t weld_agent=0x10400000u,weld_contact=0x10401000u,weld_records=0x10402000u,
    weld_data=0x10410000u,weld_alternate=0x10420000u,weld_verdicts=0x10430000u,weld_test_target=0x303800u;
inline void log_entry(ObservedWorldState &s,std::uint32_t target,std::uint32_t site) {
    const auto count=r(s.memory,log_base);
    const auto at=log_base+0x10u+(count%250u)*0x40u;
    std::uint32_t xmm_digest=0;
    for(unsigned i=0;i<8;++i) {
        std::uint32_t lanes[4];std::memcpy(lanes,&s.xmm[i],16);
        for(const auto lane:lanes)xmm_digest=(xmm_digest*0x9e3779b1u)^lane;
    }
    const std::uint32_t words[]{target,site,s.gp[0],s.gp[1],s.gp[2],s.gp[3],s.gp[4],s.gp[5],s.gp[6],s.gp[7],
        s.flags.bits(),std::uint32_t(s.fp.count),std::uint32_t(s.fp.status),std::uint32_t(s.fp.top),xmm_digest,count};
    for(unsigned i=0;i<16;++i)w(s.memory,s.observe,0x7a0000u,at+4u*i,4,words[i]);
    w(s.memory,s.observe,0x7a0004u,log_base,4,count+1u);
}
inline std::uint32_t argument(ObservedWorldState &s,unsigned n){return r(s.memory,s.gp[4]+4u*n);}
// Scratch registers, flags and XMM change, as a real callee may.
inline void scratch(ObservedWorldState &s,std::uint32_t seed) {
    s.gp[1]^=seed*0x45d9f3bu;s.gp[2]=seed^0x5a5a5a5au;s.flags.load((seed*0x2c1b3c6du)&0x8d5u);
    for(unsigned i=1;i<8;i+=1+(seed>>i&1u))
        s.xmm[i]=_mm_castsi128_ps(_mm_set_epi32(int(seed*i),int(seed*3u+i),int(seed^0x80000000u),int(seed+7u*i)));
}
// 2E31E0, hkArrayUtil::_reserve (cdecl; array, capacity, element size): the
// elements move to an owned block with the capacity asked for.
static void run_2df710(ObservedWorldState &s) {
    log_entry(s,0x2e31e0u,0);
    const auto array=argument(s,1),capacity=argument(s,2),size=argument(s,3);
    const auto data=r(s.memory,array),count=r(s.memory,array+4u);
    for(std::uint32_t at=0;at<count*size && at<0x1000u;at+=4)w(s.memory,s.observe,0x2e3260u,grown+at,4,r(s.memory,data+at));
    w(s.memory,s.observe,0x2e3264u,array,4,grown);
    w(s.memory,s.observe,0x2e3268u,array+8u,4,capacity);
    s.gp[0]=array^capacity;scratch(s,count*7u+capacity);s.gp[4]+=4u;
}
static void dispatch(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool) {
    log_entry(s,target,site);
    if(target==weld_test_target) {
        // agent->test(&verdict, contact, &record.second_contact, parameter):
        // thiscall, RET 10. Writes the verdict; sometimes answers through
        // another hkBool, or moves the records' data.
        const auto self=s.gp[1],result=argument(s,1),contact=argument(s,2),second=argument(s,3),parameter=argument(s,4);
        std::uint32_t seed=self^contact*31u^parameter*7u;
        for(unsigned i=0;i<8;++i)seed=seed*0x9e3779b1u^r(s.memory,second+4u*i);
        seed^=r(s.memory,contact)^r(s.memory,contact+4u);
        const auto verdict=(seed>>9)%3u==0u?0u:(seed>>12)&0xffu;
        std::uint32_t answer=result;
        if(seed%7u==0u){answer=weld_verdicts+(seed>>4)%64u;}
        w(s.memory,s.observe,0x7d0010u,answer,1,verdict);
        if(seed%17u==0u)w(s.memory,s.observe,0x7d0014u,weld_records,4,r(s.memory,weld_records)==weld_data?weld_alternate:weld_data);
        // Sometimes rewrites 304130's n (its argument slot), which it re-reads.
        if(weld_entry && seed%11u==0u) {
            const auto n=r(s.memory,weld_entry+16u);
            w(s.memory,s.observe,0x7d0018u,weld_entry+16u,4,n-1u-(seed>>20)%2u+(seed>>24)%3u);
        }
        s.gp[0]=answer;scratch(s,seed);s.gp[4]+=0x14u;return;
    }
    if(target==remove_target) {
        // manager->removeContactPoint(id): thiscall, RET 4. Sometimes changes
        // the count, a cached point's normal or the output's size.
        const auto self=s.gp[1],id=argument(s,1);
        const auto removed=r(s.memory,self+4u);
        w(s.memory,s.observe,0x7c0010u,self+8u+4u*(removed%64u),4,id);
        w(s.memory,s.observe,0x7c0014u,self+4u,4,removed+1u);
        const auto seed=id*0x9e3779b1u^removed;
        if(seed%13u==0u)w(s.memory,s.observe,0x7c0018u,count_block,4,r(s.memory,count_block)-1u);
        if(seed%7u==0u)w(s.memory,s.observe,0x7c001cu,points+0x20u+(seed>>8)%4u*0x30u,4,seed);
        if(seed%11u==0u)w(s.memory,s.observe,0x7c0020u,output_array+4u,4,r(s.memory,output_array+4u)+1u);
        s.gp[0]=seed;scratch(s,seed);s.gp[4]+=8u;return;
    }
    char text[96];std::snprintf(text,sizeof text,"Unbound test dispatch target %x at %x",target,site);
    throw std::runtime_error(text);
}
#include "manifold_bodies.inc"
#include "manifold_reference.inc"
} // namespace t_manifold
using t_manifold::ObservedWorldState;

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
    void putf(std::uint32_t at,float value){put(at,std::bit_cast<std::uint32_t>(value));}
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
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t mxcsr{0x1f80u};unsigned kind{};};
// A random register file, x87 stack and MXCSR mode; the stack's arguments.
void registers(Case &c,std::mt19937 &rng,std::uint32_t entry,unsigned kind) {
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[4]=entry;c.abi.fs_base=t_manifold::fs;
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

// ---- 30CF40
float small(std::mt19937 &rng,float scale){return (float(rng()%2001u)-1000.0f)/1000.0f*scale;}
Case author_report(std::mt19937 &rng,unsigned n) {
    using namespace t_manifold;
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=n%16;c.kind=kind;
    // Two motions: their transforms at +20 (three columns, the translation).
    // Body B often shares body A's motion, so the two points of a cached
    // point coincide in world space.
    const bool shared=kind!=3 && pick(3)!=0;
    for(unsigned k=0;k<2;++k) {
        const auto motion=motions+k*0x100u;
        for(std::uint32_t at=0;at<0x100u;at+=4)m.put(motion+at,special(rng));
        if(kind!=4) {
            for(unsigned col=0;col<3;++col)for(unsigned row=0;row<4;++row)
                m.putf(motion+0x20u+col*16u+row*4u,row==col?1.0f:pick(4)?0.0f:small(rng,0.5f));
            for(unsigned row=0;row<4;++row)m.putf(motion+0x50u+row*4u,small(rng,100.0f));
        }
    }
    m.put(body_a,0x10200000u);m.put(body_a+4u,std::uint32_t(rng()));m.put(body_a+8u,motions);m.put(body_a+12u,std::uint32_t(rng()));
    m.put(body_b,0x10200040u);m.put(body_b+4u,std::uint32_t(rng()));m.put(body_b+8u,shared?motions:motions+0x100u);
    m.put(body_b+12u,std::uint32_t(rng()));
    // The input: its tolerance and settings (the drift tolerance at +8).
    m.put(input,0x10200080u);m.put(input+4u,std::uint32_t(rng()));
    m.putf(input+8u,kind==5?0.0f:pick(5)?0.1f+small(rng,0.05f):std::bit_cast<float>(special(rng)));
    m.put(input+0xcu,settings);
    for(std::uint32_t at=0;at<0x20u;at+=4)m.put(settings+at,special(rng));
    m.putf(settings+8u,kind==6?0.0f:pick(5)?0.05f+small(rng,0.02f):std::bit_cast<float>(special(rng)));
    // The cached points.
    const std::int32_t count=kind==1?0:kind==2?-std::int32_t(pick(3)+1u):std::int32_t(pick(10));
    m.pages_at(points,1);
    for(std::int32_t i=0;i<10;++i) {
        const auto p=points+std::uint32_t(i)*0x30u;
        for(std::uint32_t at=0;at<0x30u;at+=4)m.put(p+at,special(rng));
        if(pick(4)==0)continue;  // random
        float a[3],normal[3]{0,0,0};const auto axis=pick(3);normal[axis]=pick(2)?1.0f:-1.0f;
        for(auto &v:a)v=small(rng,2.0f);
        float b[3]{a[0],a[1],a[2]};
        switch(pick(4)) {
        case 0:break;                                                  // coincide
        case 1:b[axis]-=normal[axis]*small(rng,0.2f);break;           // moved along the normal
        case 2:b[(axis+1)%3]+=small(rng,0.4f);break;                   // drifted across it
        default:b[axis]+=small(rng,0.001f);b[(axis+2)%3]+=small(rng,0.01f);break;
        }
        for(unsigned k=0;k<3;++k){m.putf(p+4u*k,a[k]);m.putf(p+0x10u+4u*k,b[k]);m.putf(p+0x20u+4u*k,normal[k]);}
        m.putf(p+0xcu,pick(4)?float(pick(5))/4.0f:std::bit_cast<float>(special(rng)));
        m.put(p+0x1cu,(std::uint32_t(rng())&0xffff0000u)|pick(0x10000));
    }
    m.put(count_block,std::uint32_t(count));
    // The output: its array of points (data, size, capacity and flags).
    m.put(output,output_array);m.put(output+4u,std::uint32_t(rng()));
    const auto size=pick(4);
    const auto capacity=kind==7?size+std::uint32_t(std::max(count,0))+pick(3):pick(3)==0?size:pick(12);
    m.put(output_array,output_data);m.put(output_array+4u,size);m.put(output_array+8u,capacity|(pick(4)==0?0x80000000u:0u));
    m.pages_at(output_data,1);m.pages_at(grown,1);
    // The contact manager and the constant one.
    m.put(manager,manager_vtable);m.put(manager+4u,0u);m.pages_at(manager,1);
    m.put(manager_vtable+8u,remove_target);
    m.pages_at(one_page,1);for(unsigned i=0;i<4;++i)m.putf(0x41c500u+4u*i,1.0f);
    m.pages_at(log_base,4);
    // The stack: the return address and eight arguments, random below.
    const auto entry=stack-4u*pick(4);
    for(std::uint32_t at=stack-0x1000u;at<stack+0x40u;at+=4)m.put(at,special(rng));
    const float separation=kind==10?std::bit_cast<float>(special(rng)):pick(4)?-0.05f-small(rng,0.04f):small(rng,0.1f);
    m.put(entry,0x304a11u);m.put(entry+4u,body_a);m.put(entry+8u,body_b);m.put(entry+12u,input);m.putf(entry+16u,separation);
    m.put(entry+20u,points);m.put(entry+24u,count_block);m.put(entry+28u,output);m.put(entry+32u,manager);
    m.put(fs,0x2001ff00u);
    registers(c,rng,entry,kind);
    if(kind==15) {
        static constexpr std::uint32_t regions[]{body_a,motions,input,settings,points,count_block,output,output_array,
            output_data,grown,manager,manager_vtable,one_page,log_base,stack-0x1000u};
        m.pages.erase(regions[pick(std::size(regions))]>>12);
    }
    return c;
}

// ---- 304130
Case author_weld(std::mt19937 &rng,unsigned n) {
    using namespace t_manifold;
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=n%16;c.kind=kind;
    // The records: n of them (and one more), state 1 mostly; floats at +20
    // and +24 with special values (an SNaN comes back quieted from the x87).
    const std::uint32_t count=kind==1?0u:kind==11?1u+pick(3):pick(9);
    const std::uint32_t data=kind==11?0x2000fe40u:weld_data;
    m.pages_at(weld_data,2);m.pages_at(weld_alternate,2);m.pages_at(weld_verdicts,1);
    for(const auto block:{weld_data,weld_alternate})
        for(std::uint32_t at=0;at<0x80u*10u;at+=4)m.put(block+at,special(rng));
    for(std::uint32_t i=0;i<10;++i)for(const auto block:{weld_data,weld_alternate}) {
        const auto record=block+i*0x80u;
        m.put(record+0x7cu,(pick(4)?1u:pick(3))|(std::uint32_t(rng())&0xffff0000u)|(pick(256)<<8));
        if(pick(3)==0)m.put(record+0x20u,0x7f812345u);
        if(pick(3)==0)m.put(record+0x24u,0xff800001u);
    }
    m.put(weld_records,data);m.put(weld_records+4u,count+pick(3));m.put(weld_records+8u,16u);
    for(std::uint32_t at=0;at<0x40u;at+=4)m.put(weld_contact+at,special(rng));
    m.put(weld_agent,0x10400800u);
    m.pages_at(log_base,4);
    // first: 0..n, sometimes past n or negative.
    const std::uint32_t first=kind==2?count+pick(3):kind==3?0xffffffffu-pick(2):count?pick(count):0u;
    const auto entry=stack-4u*pick(4);
    for(std::uint32_t at=stack-0x1000u;at<stack+0x40u;at+=4)m.put(at,special(rng));
    if(kind==11)for(std::uint32_t i=0;i<4;++i)m.put(data+i*0x80u+0x7cu,pick(4)?1u:0u);
    weld_entry=entry;
    m.put(entry,0x304884u);m.put(entry+4u,weld_contact);m.put(entry+8u,weld_records);m.put(entry+12u,first);
    m.put(entry+16u,count);m.put(entry+20u,std::uint32_t(rng()));
    m.put(t_manifold::fs,0x2001ff00u);
    registers(c,rng,entry,kind);
    c.abi.gp[1]=weld_agent;
    if(kind==15) {
        static constexpr std::uint32_t regions[]{weld_agent,weld_contact,weld_records,weld_data,weld_data+0x1000u,
            weld_verdicts,log_base,stack-0x1000u};
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
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(is_native) {
        services.instruction=[](auto){throw std::runtime_error("Native manifold owner delivered a translated instruction");};
        if(!census_mode) {
            auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
            cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
            cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        }
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    ObservedWorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    try{s.load(c.abi);}catch(const std::exception &e){result.fault=e.what();return result;}
    s.selected_observation_pcs=true;
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
    census_mode=mode=="census";verify=mode=="verify";
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(verify)setenv("HALO2_VERIFY_PHYSICS_WORLD_LEAVES","1",1);
    const std::uint32_t variant=(census_mode?1u:0u)+(verify?2u:0u);
    Totals report;
    if(compare("30CF40",cases,0x30cf40u+variant,report,author_report,
        [](ObservedWorldState &s){return leaves::bridge_30cf40(s,[](auto &v){t_manifold::native_world_leaves_reference_309450(v);},
            [](auto &v){t_manifold::run_2df710(v);},
            [](auto &v,std::uint32_t target,std::uint32_t site){t_manifold::dispatch(v,target,site,false);});},
        [](ObservedWorldState &s){t_manifold::run_309450(s);}))return 1;
    Totals weld;
    if(compare("304130",cases,0x304130u+variant,weld,author_weld,
        [](ObservedWorldState &s){return leaves::bridge_304130(s,[](auto &v){t_manifold::native_world_leaves_reference_300650(v);},
            [](auto &v,std::uint32_t target,std::uint32_t site){t_manifold::dispatch(v,target,site,false);});},
        [](ObservedWorldState &s){t_manifold::run_300650(s);}))return 1;
    const auto owner=[](const Totals &t){
        char text[160];std::snprintf(text,sizeof text,"{\"compared\":%llu,\"faults\":%llu,\"ordered_stores\":%llu,\"child_calls\":%llu}",
            (unsigned long long)t.compared,(unsigned long long)t.faults,(unsigned long long)t.stores,(unsigned long long)t.children);
        return std::string(text);
    };
    std::printf("{\"cases\":%u,\"report\":%s,\"weld\":%s,\"compared\":%llu,\"ordered_stores\":%llu,\"faults\":%llu,"
        "\"coverage\":%llu,\"mode\":\"%s\"}\n",
        cases,owner(report).c_str(),owner(weld).c_str(),(unsigned long long)(report.compared+weld.compared),
        (unsigned long long)(report.stores+weld.stores),(unsigned long long)(report.faults+weld.faults),
        (unsigned long long)leaves::coverage().load(),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}

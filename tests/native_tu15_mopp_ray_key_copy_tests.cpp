#include "physics/runtime/native_tu15_mopp_ray_key_copy.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage=native_mopp_ray_key_copy_detail;
using Route=stage::Continuation;
constexpr std::uint32_t base=0x62000000u, stack=base+0x180u, cache=base+0x580u, program=base+0x980u;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};

struct Fixture {
    std::uint32_t sp=stack, source=cache, code=program, key=0x8321fa10u;
    std::uint32_t deny{}, fault_pc{}, reenter_pc{};
    unsigned fault_half=1;
    bool observer{}, observer_fault{}, trace{}, authentic=true, detach_cache{}, move_program{}, move_frame{};
    bool offset{};
};
struct Access {
    std::uint32_t pc,address;
    std::size_t width;
    bool write;
    bool operator==(const Access &)const=default;
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;
    std::array<std::byte,8192> data{};
    std::vector<Access> accesses;
    std::vector<SourcePhysicsWorldRuntimeAbi> states;
    std::vector<SourcePhysicsContactRecordStore> observed;
    std::vector<std::uint32_t> instructions;
    std::uint32_t pc{};
    Route route=Route::original,tail=Route::original;
    std::string fault;
};
struct Bank final : SourcePhysicsContactMemory {
    const Fixture &f;
    Result &r;
    SourceDirectPageIndex index;
    std::function<SourcePhysicsWorldRuntimeAbi()> snapshot;
    std::function<void()> on_write;
    unsigned fault_count{};

    template<class T> void put(std::uint32_t address,const T &value) const {
        std::memcpy(r.data.data()+address-base,&value,sizeof(value));
    }
    Bank(const Fixture &fixture,Result &result):f(fixture),r(result) {
        direct_pages_=&index;
        auto &block=index.blocks[base>>22u];
        block=std::make_unique<SourceDirectPageIndex::Block>();
        (*block)[(base>>12u)&1023u]={r.data.data(),true};
        (*block)[((base>>12u)&1023u)+1u]={r.data.data()+4096u,true};
        if(f.detach_cache)(*block)[(f.source>>12u)&1023u]={};
        put(f.source,std::array{0x00000001u,0xffffffffu,0x7f812345u,0x80000000u,
                               0xf0210000u,0x3f800000u,0x22334455u,0xbc791afa});
        std::array<std::byte,4> bytes{};
        for(unsigned i=0;i<4u;++i)bytes[i]=std::byte(f.key>>((3u-i)*8u));
        put(f.code+1u,bytes);
    }
    void access(std::uint32_t address,std::size_t width,bool write)const {
        r.accesses.push_back({r.pc,address,width,write});
        r.states.push_back(snapshot());
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        access(address,out.size(),false);
        if(r.pc==f.fault_pc)throw std::runtime_error("Injected key-copy read fault");
        std::memcpy(out.data(),r.data.data()+address-base,out.size());
    }
    bool try_callback_free_write(std::uint32_t address,std::span<const std::byte> in)noexcept override {
        if(f.observer||r.pc==f.deny||in.size()!=4u||!direct_page(address,in.size()))return false;
        r.accesses.push_back({r.pc,address,in.size(),true});
        std::memcpy(r.data.data()+address-base,in.data(),in.size());
        return true;
    }
    void write(std::uint32_t address,std::span<const std::byte> in)override {
        if(try_callback_free_write(address,in))return;
        access(address,in.size(),true);
        if(r.pc==f.fault_pc&&++fault_count==f.fault_half)
            throw std::runtime_error("Injected key-copy store fault");
        std::memcpy(r.data.data()+address-base,in.data(),in.size());
        if(r.pc==f.reenter_pc)on_write();
    }
};

void exact_key_copy(WorldState &s,Route route,Route *tail) {
    switch(route) { case Route::original: goto L30b4c0; case Route::read_vector: goto L_read_vector; case Route::read_shift: goto L_read_shift; case Route::write_shift: goto L_write_shift; case Route::read_scale: goto L_read_scale; case Route::write_scale: goto L_write_scale; case Route::read_old_key: goto L_read_old_key; case Route::write_old_key: goto L_write_old_key; case Route::read_property: goto L_read_property; case Route::write_property: goto L_write_property;case Route::set_cache: goto L_set_cache; case Route::write_key: goto L30b50f; case Route::advance_key: goto L_advance_key; case Route::resume: return; }
L30b4c0: s.instruction_literal(0x30b4c0u); s.set(0,r(s.memory,(0x1u+s.gp[6]),1),4,0);
s.instruction_literal(0x30b4c4u); s.set(2,r(s.memory,(0x2u+s.gp[6]),1),4,0);
s.instruction_literal(0x30b4c8u); s.set(1,r(s.memory,(0x3u+s.gp[6]),1),4,0);
s.instruction_literal(0x30b4ccu); s.set(0,s.flags.shift(s.reg(0,4,0),0x8u,4),4,0);
s.instruction_literal(0x30b4cfu); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(2,4,0),4),4,0);
s.instruction_literal(0x30b4d1u); s.set(2,r(s.memory,(0x4u+s.gp[6]),1),4,0);
s.instruction_literal(0x30b4d5u); s.set(0,s.flags.shift(s.reg(0,4,0),0x8u,4),4,0);
s.instruction_literal(0x30b4d8u); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(1,4,0),4),4,0);
s.instruction_literal(0x30b4dau); s.set(0,s.flags.shift(s.reg(0,4,0),0x8u,4),4,0);
s.instruction_literal(0x30b4ddu); s.set(1,(0x40u+s.gp[4]),4,0);
s.instruction_literal(0x30b4e1u); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(2,4,0),4),4,0);
s.instruction_literal(0x30b4e3u); s.flags.minus(s.reg(3,4,0),s.reg(1,4,0),4);
s.instruction_literal(0x30b4e5u); if(s.flags.zf)goto L30b50f;
L_read_vector: s.instruction_literal(0x30b4e7u); s.xmm[0]=v(s.memory,(0x0u+s.gp[3]));
s.instruction_literal(0x30b4eau); vw(s.memory,s.observe,0x30b4eau,(0x40u+s.gp[4]),s.xmm[0]);
L_read_shift: s.instruction_literal(0x30b4efu); s.set(2,r(s.memory,(0x10u+s.gp[3]),4),4,0);
L_write_shift: s.instruction_literal(0x30b4f2u); w(s.memory,s.observe,0x30b4f2u,(0x50u+s.gp[4]),4,s.reg(2,4,0));
L_read_scale: s.instruction_literal(0x30b4f6u); s.set(1,r(s.memory,(0x14u+s.gp[3]),4),4,0);
L_write_scale: s.instruction_literal(0x30b4f9u); w(s.memory,s.observe,0x30b4f9u,(0x54u+s.gp[4]),4,s.reg(1,4,0));
L_read_old_key: s.instruction_literal(0x30b4fdu); s.set(2,r(s.memory,(0x18u+s.gp[3]),4),4,0);
L_write_old_key: s.instruction_literal(0x30b500u); w(s.memory,s.observe,0x30b500u,(0x58u+s.gp[4]),4,s.reg(2,4,0));
L_read_property: s.instruction_literal(0x30b504u); s.set(1,r(s.memory,(0x1cu+s.gp[3]),4),4,0);
L_write_property: s.instruction_literal(0x30b507u); w(s.memory,s.observe,0x30b507u,(0x5cu+s.gp[4]),4,s.reg(1,4,0));
L_set_cache: s.instruction_literal(0x30b50bu); s.set(3,(0x40u+s.gp[4]),4,0);
L30b50f:
    if(tail) {
        *tail=stage::key_copy<true>(s);
        if(*tail==Route::resume)return;
        if(*tail==Route::advance_key)goto L_advance_key;
    }
    s.instruction_literal(0x30b50fu); w(s.memory,s.observe,0x30b50fu,(0x58u+s.gp[4]),4,s.reg(0,4,0));
L_advance_key: s.instruction_literal(0x30b513u); s.set(6,s.flags.plus(s.reg(6,4,0),0x5u,4),4,0);
s.instruction_literal(0x30b516u); goto L30ae02;
L30ae02: return;
}

void exact_offset_copy(WorldState &s,Route route) {
    auto &g=s.gp;
    switch(route) {
    case Route::original: break;
    case Route::read_vector: goto read_vector;
    case Route::read_shift: goto read_shift;
    case Route::write_shift: goto write_shift;
    case Route::read_scale: goto read_scale;
    case Route::write_scale: goto write_scale;
    case Route::read_old_key: goto read_old_key;
    case Route::write_old_key: goto write_old_key;
    case Route::read_property: goto read_property;
    case Route::write_property: goto write_property;
    case Route::set_cache: goto set_cache;
    case Route::write_key: goto write_key;
    case Route::advance_key: goto advance_key;
    case Route::resume: return;
    }
    s.instruction_literal(0x30b477u);g[0]=r(s.memory,g[6]+1u,1u);
    s.instruction_literal(0x30b47bu);g[2]=r(s.memory,g[6]+2u,1u);
    s.instruction_literal(0x30b47fu);g[0]=s.flags.shift(g[0],8u,4u);
    s.instruction_literal(0x30b482u);g[1]=g[4]+0x40u;
    s.instruction_literal(0x30b486u);g[0]=s.flags.plus(g[0],g[2],4u);
    s.instruction_literal(0x30b488u);s.flags.minus(g[3],g[1],4u);
    s.instruction_literal(0x30b48au);if(s.flags.zf)goto write_key;
read_vector:
    s.instruction_literal(0x30b48cu);s.xmm[0]=v(s.memory,g[3]);
    s.instruction_literal(0x30b48fu);vw(s.memory,s.observe,0x30b48fu,g[4]+0x40u,s.xmm[0]);
read_shift:
    s.instruction_literal(0x30b494u);g[2]=r(s.memory,g[3]+0x10u,4u);
write_shift:
    s.instruction_literal(0x30b497u);w(s.memory,s.observe,0x30b497u,g[4]+0x50u,4u,g[2]);
read_scale:
    s.instruction_literal(0x30b49bu);g[1]=r(s.memory,g[3]+0x14u,4u);
write_scale:
    s.instruction_literal(0x30b49eu);w(s.memory,s.observe,0x30b49eu,g[4]+0x54u,4u,g[1]);
read_old_key:
    s.instruction_literal(0x30b4a2u);g[2]=r(s.memory,g[3]+0x18u,4u);
write_old_key:
    s.instruction_literal(0x30b4a5u);w(s.memory,s.observe,0x30b4a5u,g[4]+0x58u,4u,g[2]);
read_property:
    s.instruction_literal(0x30b4a9u);g[1]=r(s.memory,g[3]+0x1cu,4u);
write_property:
    s.instruction_literal(0x30b4acu);w(s.memory,s.observe,0x30b4acu,g[4]+0x5cu,4u,g[1]);
set_cache:
    s.instruction_literal(0x30b4b0u);g[3]=g[4]+0x40u;
write_key:
    s.instruction_literal(0x30b4b4u);
    w(s.memory,s.observe,0x30b4b4u,g[4]+0x58u,4u,s.flags.plus(r(s.memory,g[4]+0x58u),g[0],4u));
advance_key:
    s.instruction_literal(0x30b4b8u);g[6]=s.flags.plus(g[6],3u,4u);
    s.instruction_literal(0x30b4bbu);
}

Result run(const Fixture &f,bool native) {
    Result r;r.pc=0x30aaa7u;r.accesses.reserve(32);
    Bank bank{f,r};
    SourcePhysicsWorldRuntimeServices services;
    auto trace=f.trace;
    services.instruction=[&](std::uint32_t pc){r.pc=pc;r.instructions.push_back(pc);};
    services.instruction_cursor={&r.pc,&trace,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    if(!f.authentic)services.instruction_cursor={};
    SourcePhysicsContactRecordObserver observe;
    auto on_observe=[&](const auto &event) {
        r.observed.push_back(event);r.states.push_back(bank.snapshot());
        if(f.observer_fault)throw std::runtime_error("Injected key-copy observer fault");
    };
    if(f.observer)observe=on_observe;
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={0x24210000u,2u,1u,f.source,f.sp,0x2020u,f.code,0x6700u};
    input.eflags=0x256u;input.x87_depth=3;input.x87_status=0x3200u;
    input.x87[0]=0.5L;input.x87[1]=-1.25L;input.x87[2]=0.125L;
    WorldState state{bank,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
    bank.snapshot=[&]{return state.save();};
    bank.on_write=[&] {
        if(f.move_frame) {
            const auto frame=state.gp[4]-base;
            std::memcpy(r.data.data()+frame+0x100u,r.data.data()+frame,0x64u);
            state.gp[4]+=0x100u;
        } else if(f.move_program)state.gp[6]=0xfffffffdu;
        else {
            trace=true;observe=on_observe;bank.put(f.source+0x1cu,0x37135791u);
        }
    };
    try {
        if(f.offset) {
            if(native)r.route=stage::key_copy<false,true>(state);
            exact_offset_copy(state,r.route);
        } else {
            if(native)r.route=stage::key_copy(state);
            exact_key_copy(state,r.route,native?&r.tail:nullptr);
        }
    } catch(const std::runtime_error &error) {r.fault=error.what();}
    r.abi=state.save();
    return r;
}
bool same(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_status!=b.x87_status||
       a.x87_depth!=b.x87_depth||a.floating_point->x87_control_word!=b.floating_point->x87_control_word||
       a.floating_point->mxcsr!=b.floating_point->mxcsr)return false;
    for(unsigned i=0;i<a.x87_depth;++i)if(std::memcmp(&a.x87[i],&b.x87[i],10u))return false;
    return true;
}
void compare(const Fixture &f,Route expected,std::optional<Route> expected_tail={}) {
    if(stage::reference_key_copy()||(f.offset&&(stage::reference_offset_key_node()||
       (f.source!=f.sp+0x40u&&stage::reference_offset_key_copy()))))expected=Route::original;
    const auto a=run(f,false),b=run(f,true);
    if(a.pc!=b.pc||a.fault!=b.fault||a.data!=b.data||a.accesses!=b.accesses||
       a.observed!=b.observed||a.instructions!=b.instructions||!same(a.abi,b.abi)||
       a.states.size()!=b.states.size()||(b.fault.empty()&&b.route!=expected)||
       (expected_tail&&b.tail!=*expected_tail)) {
        std::cerr<<"Key copy fault "<<std::hex<<f.fault_pc<<'/'<<f.deny<<" PC "<<a.pc<<'/'<<b.pc
                 <<" route "<<int(expected)<<'/'<<int(b.route)<<" fault "<<a.fault<<'/'<<b.fault<<'\n';
        throw std::runtime_error("MOPP key copy continuation, provider or ABI differs");
    }
    for(unsigned i=0;i<a.states.size();++i)
        if(!same(a.states[i],b.states[i]))throw std::runtime_error("Provider saw a different MOPP key-copy frame");
}
unsigned prove() {
    unsigned compared{};
    auto check=[&](const Fixture &f,Route expected){compare(f,expected);++compared;};
    constexpr std::array stores{
        std::pair{0x30b4f2u,Route::write_shift},std::pair{0x30b4f9u,Route::write_scale},
        std::pair{0x30b500u,Route::write_old_key},std::pair{0x30b507u,Route::write_property},
        std::pair{0x30b50fu,Route::write_key}};
    constexpr std::array after{Route::read_scale,Route::read_old_key,Route::read_property,
        Route::set_cache,Route::advance_key};
    for(auto key:{0u,0xffffffffu,0x80000000u,0x7fffffffu,0x12345678u,0x10200030u}) {
        Fixture f;f.key=key;
        check(f,Route::resume);
        auto reuse=f;reuse.source=stack+0x40u;
        check(reuse,Route::resume);
        const bool rollback=stage::reference_key_provider();
        reuse.deny=0x30b50fu;check(reuse,rollback?Route::write_key:Route::resume);
        for(std::size_t i=0;i<stores.size();++i) {
            const auto [pc,next]=stores[i];
            const bool key_store=pc==0x30b50fu&&!rollback;
            const bool field_store=pc!=0x30b50fu&&!stage::reference_key_copy_provider();
            auto refused=f;refused.deny=pc;
            if(pc==0x30b50fu)check(refused,key_store?Route::resume:next);
            else {
                const auto tail=field_store||rollback||stage::reference_key_copy()?Route::original:Route::resume;
                compare(refused,field_store?Route::resume:next,tail);++compared;
            }
            refused.reenter_pc=pc;check(refused,key_store||field_store?after[i]:next);
            if(pc!=0x30b50fu) {
                refused.move_frame=true;check(refused,field_store?Route::resume:next);
                refused.move_frame=false;
            }
            if(pc==0x30b50fu) {
                refused.move_program=true;check(refused,key_store?Route::resume:next);
                refused.move_program=false;
            }
            refused.fault_pc=pc;check(refused,next);
        }
        for(const auto half:{1u,2u}) {
            auto fault=f;fault.fault_pc=0x30b4eau;fault.fault_half=half;
            check(fault,Route::original);
        }
        auto dynamic=f;dynamic.reenter_pc=0x30b4eau;
        check(dynamic,Route::read_shift);
        dynamic.observer_fault=true;check(dynamic,Route::original);
        auto observed=f;observed.observer=true;observed.observer_fault=true;
        check(observed,Route::original);
        auto traced=f;traced.trace=true;check(traced,Route::original);
        auto weak=f;weak.authentic=false;check(weak,Route::original);
        auto crossed=f;crossed.code=base+0xffdu;check(crossed,Route::original);
        crossed=f;crossed.source=base+0xff8u;check(crossed,Route::read_vector);
        crossed.fault_pc=0x30b4e7u;check(crossed,Route::read_vector);
        crossed=f;crossed.source=base+0xfeeu;crossed.fault_pc=0x30b4efu;
        check(crossed,Route::read_shift);
        auto detached=f;detached.source=base+0x1400u;detached.detach_cache=true;
        check(detached,Route::read_vector);
        for(unsigned alias:{0x30u,0x38u,0x44u,0x48u,0x50u}) {
            auto overlap=f;overlap.source=stack+alias;check(overlap,Route::resume);
        }
    }
    return compared;
}
unsigned prove_offset() {
    unsigned count{};
    for(const auto key:{0u,0xffffffffu,0xffff0000u,0x80000000u,0x00120000u}) {
        Fixture f;f.offset=true;f.source=stack+0x40u;f.key=key;
        compare(f,Route::resume);++count;
        auto guarded=f;guarded.deny=0x30b4b4u;
        compare(guarded,Route::resume);++count;
        guarded.reenter_pc=guarded.deny;guarded.move_program=true;
        compare(guarded,Route::resume);++count;
        guarded.move_program=false;guarded.move_frame=true;
        compare(guarded,Route::resume);++count;
        guarded.move_frame=false;
        compare(guarded,Route::advance_key);++count;
        guarded.fault_pc=guarded.deny;
        compare(guarded,Route::original);++count;
        auto uncached=f;uncached.source=cache;
        compare(uncached,Route::resume);++count;
        auto observed=f;observed.observer=true;observed.observer_fault=true;
        compare(observed,Route::original);++count;
        auto traced=f;traced.trace=true;
        compare(traced,Route::original);++count;
        auto split=f;split.code=base+0xffeu;
        compare(split,Route::original);++count;
    }
    return count;
}

unsigned prove_uncached_offset() {
    unsigned count{};
    const auto check=[&](const Fixture &f,Route expected){compare(f,expected);++count;};
    constexpr std::array stores{
        std::pair{0x30b497u,Route::write_shift},std::pair{0x30b49eu,Route::write_scale},
        std::pair{0x30b4a5u,Route::write_old_key},std::pair{0x30b4acu,Route::write_property}};
    constexpr std::array after{Route::read_scale,Route::read_old_key,Route::read_property,Route::set_cache};
    const bool field_provider=!stage::reference_key_copy_provider();
    for(const auto key:{0u,0xffff0000u,0x80000000u,0x9ab10000u}) {
        Fixture f;f.offset=true;f.key=key;
        for(unsigned offset:{0x30u,0x38u,0x44u,0x48u,0x50u}) {
            auto overlap=f;overlap.source=stack+offset;check(overlap,Route::resume);
        }
        for(std::size_t i=0;i<stores.size();++i) {
            const auto [pc,next]=stores[i];
            auto refused=f;refused.deny=pc;check(refused,field_provider?Route::resume:next);
            refused.reenter_pc=pc;check(refused,field_provider?after[i]:next);
            refused.move_frame=true;check(refused,field_provider?Route::resume:next);
            refused.fault_pc=pc;check(refused,next);
        }
        for(const auto half:{1u,2u}) {
            auto fault=f;fault.fault_pc=0x30b48fu;fault.fault_half=half;check(fault,Route::original);
        }
        auto dynamic=f;dynamic.reenter_pc=0x30b48fu;check(dynamic,Route::read_shift);
        dynamic.observer_fault=true;check(dynamic,Route::original);
        auto cross=f;cross.source=base+0xff8u;cross.fault_pc=0x30b48cu;check(cross,Route::read_vector);
        cross=f;cross.source=base+0xfeeu;cross.fault_pc=0x30b494u;check(cross,Route::read_shift);
        auto detached=f;detached.source=base+0x1400u;detached.detach_cache=true;check(detached,Route::read_vector);
    }
    return count;
}
}
int main() try {
    std::cout<<"MOPP key nodes: "<<prove()+prove_offset()+prove_uncached_offset()<<" exact continuation and provider comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}

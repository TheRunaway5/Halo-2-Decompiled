#include "physics/runtime/native_tu15_mopp_ray_plane_split.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage=native_mopp_ray_plane_split_detail;
using Route=stage::Continuation;
constexpr std::uint32_t base=0x62000000u, default_stack=base+0x180u;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};

enum class Exit { geometry, resume };
struct Fixture {
    std::uint32_t stack=default_stack, start=0x40000000u, end=0x3f800000u, delta=17u;
    unsigned depth=3, precision=3, rounding{};
    long double top=3.0L;
    std::uint32_t fault_pc{}, dynamic_pc{};
    bool observer{}, trace{}, authentic=true, device{};
    SourcePhysicsContactImage image=SourcePhysicsContactImage::TitleUpdate15;
};
struct Access {
    std::uint32_t pc,address;
    std::size_t width;
    SourcePhysicsWorldRuntimeAbi state;
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;
    std::vector<Access> accesses;
    std::vector<std::uint32_t> instruction_sites;
    std::uint32_t pc{};
    unsigned observers{};
    Route route=Route::original;
    Exit exit=Exit::geometry;
    std::string fault;
};

struct Bank final : SourcePhysicsContactMemory {
    const Fixture &f;
    Result &r;
    SourceDirectPageIndex index;
    mutable std::array<std::byte,8192> data{};
    std::function<SourcePhysicsWorldRuntimeAbi()> snapshot;
    std::function<void()> on_read;

    void seed(std::uint32_t address,std::uint32_t value) const {
        std::memcpy(data.data()+address-base,&value,4u);
    }
    Bank(const Fixture &fixture,Result &result):f(fixture),r(result) {
        if(!f.device) {
            direct_pages_=&index;
            auto &block=index.blocks[base>>22u];
            block=std::make_unique<SourceDirectPageIndex::Block>();
            (*block)[(base>>12u)&1023u]={data.data(),true};
            (*block)[((base>>12u)&1023u)+1u]={data.data()+4096u,true};
        }
        seed(f.stack+0x14u,f.delta);seed(f.stack+0x18u,f.start);seed(f.stack+0x20u,f.end);
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        r.accesses.push_back({r.pc,address,out.size(),snapshot()});
        if(r.pc==f.fault_pc)throw std::runtime_error("Injected MOPP plane read fault");
        std::memcpy(out.data(),data.data()+address-base,out.size());
        if(r.pc==f.dynamic_pc)on_read();
    }
    void write(std::uint32_t,std::span<const std::byte>)override {
        throw std::runtime_error("Unexpected MOPP plane split write");
    }
};

Exit exact_split(WorldState &s,Route route) {
    switch(route) { case Route::original: goto L30ae55; case Route::read_start: goto L_read_start; case Route::geometry: return Exit::geometry; case Route::resume: return Exit::resume; }
L30ae55: s.instruction_literal(0x30ae55u); s.fp.comparison(ext(f(r(s.memory,(0x20u+s.gp[4]),4))),0);
s.instruction_literal(0x30ae59u); s.set(2,r(s.memory,(0x14u+s.gp[4]),4),4,0);
s.instruction_literal(0x30ae5du); s.set(6,s.flags.plus(s.reg(6,4,0),s.reg(2,4,0),4),4,0);
s.instruction_literal(0x30ae5fu); s.set(0,s.fp.word(),2,0);
s.instruction_literal(0x30ae61u); s.flags.logic((s.reg(0,1,8))&(0x41u),1);
s.instruction_literal(0x30ae64u); if(!s.flags.zf)goto L30ae75;
L_read_start: s.instruction_literal(0x30ae66u); s.fp.push(ext(f(r(s.memory,(0x18u+s.gp[4]),4))));
s.instruction_literal(0x30ae6au); s.fp.comparison(ext(f(r(s.memory,(0x20u+s.gp[4]),4))),1);
s.instruction_literal(0x30ae6eu); s.set(0,s.fp.word(),2,0);
s.instruction_literal(0x30ae70u); s.flags.logic((s.reg(0,1,8))&(0x41u),1);
s.instruction_literal(0x30ae73u); if(s.flags.zf)goto L30ae00;
L30ae75: return Exit::geometry;
L30ae00: return Exit::resume;
}


Result run(const Fixture &f,bool native) {
    Result r;r.pc=0x30adfcu;Bank bank{f,r};
    SourcePhysicsWorldRuntimeServices services;
    auto trace=f.trace;
    services.instruction=[&](std::uint32_t pc){r.pc=pc;r.instruction_sites.push_back(pc);};
    services.instruction_cursor={&r.pc,&trace,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    if(!f.authentic)services.instruction_cursor={};
    SourcePhysicsContactRecordObserver observe;
    if(f.observer)observe=[&](const auto &){++r.observers;};
    bank.on_read=[&] {trace=true;bank.seed(f.stack+0x18u,0u);};
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={0x25300000u,2u,1u,0x5000u,f.stack,0x2020u,0xfffffff0u,0x5300u};
    input.eflags=0x256u;input.x87_depth=f.depth;input.x87_status=0x3200u;
    input.x87[0]=f.top;
    for(unsigned i=1;i<f.depth;++i)input.x87[i]=0.125L*static_cast<long double>(i)+0.25L;
    const auto cw=std::uint16_t(0x7fu|(f.precision<<8u)|(f.rounding<<10u));
    input.floating_point=SourcePhysicsFactoryFloatingPoint{cw,0x1f80u};
    asm volatile("fnclex; fldcw %0"::"m"(cw));
    WorldState state{bank,observe,services,f.image};state.load(input);
    bank.snapshot=[&]{return state.save();};
    try {
        if(native)r.route=stage::plane_split(state);
        r.exit=exact_split(state,r.route);
    } catch(const std::runtime_error &e) {r.fault=e.what();}
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
void compare(const Fixture &f,Route expected) {
    if(stage::reference_plane_split())expected=Route::original;
    const auto a=run(f,false),b=run(f,true);
    if(a.pc!=b.pc||a.fault!=b.fault||a.exit!=b.exit||a.accesses.size()!=b.accesses.size()||
       a.observers!=0u||b.observers!=0u||a.instruction_sites!=b.instruction_sites||
       !same(a.abi,b.abi)||b.route!=expected) {
        std::cerr<<"Plane split case "<<std::hex<<f.start<<'/'<<f.end<<" PC "<<a.pc<<'/'<<b.pc
                 <<" route "<<int(expected)<<'/'<<int(b.route)<<" fault "<<a.fault<<'/'<<b.fault<<'\n';
        throw std::runtime_error("MOPP plane split continuation, provider or ABI differs");
    }
    for(unsigned i=0;i<a.accesses.size();++i) {
        const auto &x=a.accesses[i],&y=b.accesses[i];
        if(x.pc!=y.pc||x.address!=y.address||x.width!=y.width||!same(x.state,y.state))
            throw std::runtime_error("Provider saw a different MOPP plane split access");
    }
}

unsigned prove() {
    unsigned compared{};
    auto check=[&](const Fixture &f,Route expected){compare(f,expected);++compared;};
    for(const auto precision:{0u,2u,3u})for(unsigned rc=0;rc<4u;++rc) {
        Fixture f;f.precision=precision;f.rounding=rc;
        check(f,Route::resume);
        f.observer=true;check(f,Route::resume);
        auto first=f;first.top=-0.25L;check(first,Route::geometry);
        auto second=f;second.start=0u;check(second,Route::geometry);
        for(const auto bits:{0u,0x80000000u,0x7f800000u,0xff800000u,0x7fc21098u,0x7f800abcu,
                             1u,0x80000001u}) {
            auto edge=f;edge.end=bits;
            check(edge,bits==0x7f800000u||bits==0x7fc21098u||bits==0x7f800abcu?
                Route::geometry:Route::resume);
            edge=f;edge.start=bits;
            check(edge,bits==0x7f800000u?Route::resume:Route::geometry);
        }
        auto traced=f;traced.trace=true;check(traced,Route::original);
        auto callback=f;callback.authentic=false;check(callback,Route::original);
        auto full=f;full.depth=8;check(full,Route::read_start);
        auto empty=f;empty.depth=0;check(empty,Route::original);
        auto device=f;device.device=true;device.dynamic_pc=0x30ae55u;
        check(device,Route::original);
        for(const auto [offset,pc]:std::array{std::pair{0xfdeu,0x30ae55u},
             std::pair{0xfeau,0x30ae59u},std::pair{0xfe6u,0x30ae66u},std::pair{0xfdeu,0x30ae6au}}) {
            auto crossed=f;crossed.stack=base+offset;crossed.fault_pc=pc;
            check(crossed,Route::original);
            crossed.fault_pc=0;
            check(crossed,Route::original);
        }
    }
    return compared;
}
}

int main() try {
    std::cout<<"30AE55 plane split: "<<prove()<<" exact continuation and provider comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}

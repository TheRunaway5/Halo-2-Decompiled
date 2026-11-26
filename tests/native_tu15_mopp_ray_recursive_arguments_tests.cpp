#include "physics/runtime/native_tu15_mopp_ray_recursive_arguments.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage=native_mopp_ray_recursive_arguments_detail;
using Route=stage::Continuation;
constexpr std::uint32_t base=0x62000000u,sp=base+0x180u;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
struct Fixture {
    std::uint32_t deny_store{},fault_pc{},stack=sp,cache_word=0xbc41352u;
    std::array<std::uint32_t,3> offsets{0x541f0000u,0xc6210000u,0x402000u};
    bool trace{},authentic=true,observe{},observer_fault{},reenter{},sparse{},deny_cache{},move_frame{};
};
struct Access {
    std::uint32_t pc,address;
    std::size_t width;
    bool write;
    bool operator==(const Access &)const=default;
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;
    std::array<std::byte,8192> bytes{};
    std::vector<Access> accesses;
    std::vector<SourcePhysicsWorldRuntimeAbi> states;
    std::vector<SourcePhysicsContactRecordStore> observed;
    std::vector<std::uint32_t> instructions;
    std::uint32_t pc{};
    Route route=Route::original;
    bool child{};
    std::string fault;
};

struct Bank final : SourcePhysicsContactMemory {
    const Fixture &f;
    Result &r;
    SourceDirectPageIndex index;
    std::function<SourcePhysicsWorldRuntimeAbi()> snapshot;
    std::function<void()> reenter;
    template<class T>void put(std::uint32_t p,const T &value) {
        std::memcpy(r.bytes.data()+p-base,&value,sizeof(value));
    }
    Bank(const Fixture &f,Result &r):f(f),r(r) {
        if(!f.sparse) {
            direct_pages_=&index;
            auto &block=index.blocks[base>>22u];
            block=std::make_unique<SourceDirectPageIndex::Block>();
            (*block)[(base>>12u)&1023u]={r.bytes.data(),true};
            (*block)[((base>>12u)&1023u)+1u]={r.bytes.data()+4096u,true};
        }
        put(f.stack+0x28u,f.cache_word);
    }
    std::optional<std::span<const std::byte>> try_read_view(
        std::uint32_t p,std::size_t n)const noexcept override {
        if(!f.sparse || (p==f.stack+0x28u&&f.deny_cache) || p<base ||
           p-base>r.bytes.size() || n>r.bytes.size()-(p-base))return std::nullopt;
        return std::span<const std::byte>{r.bytes}.subspan(p-base,n);
    }
    void access(std::uint32_t p,std::size_t n,bool write)const {
        r.accesses.push_back({r.pc,p,n,write});r.states.push_back(snapshot());
        if(r.pc==f.fault_pc)throw std::runtime_error("Injected recursive argument provider fault");
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        if(!try_read_view(p,out.size()))access(p,out.size(),false);
        std::memcpy(out.data(),r.bytes.data()+p-base,out.size());
    }
    bool try_callback_free_write(std::uint32_t p,std::span<const std::byte> in)noexcept override {
        if(f.observe||r.pc==f.deny_store||(!f.sparse&&!direct_page(p,in.size())))return false;
        r.accesses.push_back({r.pc,p,in.size(),true});
        std::memcpy(r.bytes.data()+p-base,in.data(),in.size());
        return true;
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        if(try_callback_free_write(p,in))return;
        access(p,in.size(),true);
        std::memcpy(r.bytes.data()+p-base,in.data(),in.size());
        if(f.reenter||f.move_frame)reenter();
    }
};

template<bool Increasing> void retained(WorldState &,Route);
template<> void retained<true>(WorldState &s,Route route) {
    switch(route) { case Route::original: break; case Route::first: goto L_first; case Route::advance_code: goto L_advance_code; case Route::second: goto L_second; case Route::read_cache: goto L_read_cache; case Route::third: goto L_third; case Route::return_address: goto L_return_address; case Route::invoke: return; default: throw std::logic_error("Wrong recursive argument test route"); }
s.instruction_literal(0x30af16u); s.set(1,s.flags.minus(s.reg(1,4,0),s.reg(2,4,0),4),4,0);
s.instruction_literal(0x30af18u); s.set(0,(0x80u+s.gp[4]),4,0);
L_first: s.instruction_literal(0x30af1fu); s.push(0x30af1fu,s.reg(0,4,0));
L_advance_code: s.instruction_literal(0x30af20u); s.set(1,s.flags.plus(s.reg(1,4,0),s.reg(6,4,0),4),4,0);
L_second: s.instruction_literal(0x30af22u); s.push(0x30af22u,s.reg(1,4,0));
L_read_cache: s.instruction_literal(0x30af23u); s.set(1,r(s.memory,(0x30u+s.gp[4]),4),4,0);
L_third: s.instruction_literal(0x30af27u); s.push(0x30af27u,s.reg(3,4,0));
L_return_address: s.instruction_literal(0x30af28u); s.push(0x30af28u,0x30af2du);
}
template<> void retained<false>(WorldState &s,Route route) {
    switch(route) { case Route::original: break; case Route::first: goto L_first; case Route::second: goto L_second; case Route::third: goto L_third; case Route::return_address: goto L_return_address; case Route::invoke: return; default: throw std::logic_error("Wrong recursive argument test route"); }
s.instruction_literal(0x30b06fu); s.set(1,r(s.memory,(0x28u+s.gp[4]),4),4,0);
s.instruction_literal(0x30b073u); s.set(2,(0x80u+s.gp[4]),4,0);
L_first: s.instruction_literal(0x30b07au); s.push(0x30b07au,s.reg(2,4,0));
L_second: s.instruction_literal(0x30b07bu); s.push(0x30b07bu,s.reg(6,4,0));
L_third: s.instruction_literal(0x30b07cu); s.push(0x30b07cu,s.reg(3,4,0));
L_return_address: s.instruction_literal(0x30b07du); s.push(0x30b07du,0x30b082u);
}


template<bool Increasing>
Result run(const Fixture &f,bool native) {
    Result r;r.pc=0x30aaa7u;r.accesses.reserve(24);Bank bank{f,r};
    SourcePhysicsWorldRuntimeServices services;
    auto trace=f.trace;
    services.instruction=[&](std::uint32_t pc){r.pc=pc;r.instructions.push_back(pc);};
    services.instruction_cursor={&r.pc,&trace,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    if(!f.authentic)services.instruction_cursor={};
    SourcePhysicsContactRecordObserver observe;
    auto on_store=[&](const auto &event) {
        r.observed.push_back(event);r.states.push_back(bank.snapshot());
        if(f.observer_fault)throw std::runtime_error("Injected recursive argument observer fault");
    };
    if(f.observe)observe=on_store;
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={0x43200000u,f.offsets[0],f.offsets[1],0xeca1442cu,f.stack,f.stack+0x200u,f.offsets[2],0x76543210u};
    input.eflags=0xa57u;input.x87_depth=3;input.x87_status=0x3225u;
    input.x87[0]=0.125L;input.x87[1]=-1.25L;input.x87[2]=0.5L;
    WorldState state{bank,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
    bank.snapshot=[&]{return state.save();};
    bank.reenter=[&] {
        if(f.move_frame) {
            state.gp[4]+=0x100u;
            state.gp[6]^=0x11223344u;
        } else {
            trace=true;observe=on_store;bank.put(f.stack+0x28u,0xaf22u);
        }
    };
    try {
        if(native)r.route=stage::publish_arguments<Increasing>(state);
        retained<Increasing>(state,r.route);
        r.child=true;
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
template<bool Increasing>
void compare(const Fixture &f,Route expected) {
    if(stage::reference_recursive_arguments())expected=Route::original;
    const auto a=run<Increasing>(f,false),b=run<Increasing>(f,true);
    if(a.pc!=b.pc||a.fault!=b.fault||a.bytes!=b.bytes||a.accesses!=b.accesses||a.child!=b.child||
       a.observed!=b.observed||a.instructions!=b.instructions||!same(a.abi,b.abi)||
       a.states.size()!=b.states.size()||(b.fault.empty()&&b.route!=expected)) {
        std::cerr<<"Recursive arguments "<<Increasing<<" PC "<<std::hex<<a.pc<<'/'<<b.pc
                 <<" route "<<int(expected)<<'/'<<int(b.route)<<" fault "<<a.fault<<'/'<<b.fault<<'\n';
        throw std::runtime_error("MOPP recursive child argument or provider state differs");
    }
    for(unsigned i=0;i<a.states.size();++i)
        if(!same(a.states[i],b.states[i]))throw std::runtime_error("Different recursive PUSH fault/observer frame");
}
template<bool Increasing>
unsigned prove() {
    unsigned compared{};
    auto check=[&](const Fixture &f,Route route){compare<Increasing>(f,route);++compared;};
    constexpr auto pcs=Increasing?std::array{0x30af1fu,0x30af22u,0x30af27u,0x30af28u}:
                                   std::array{0x30b07au,0x30b07bu,0x30b07cu,0x30b07du};
    constexpr std::array next{Route::first,Route::second,Route::third,Route::return_address};
    constexpr auto after=Increasing?std::array{Route::advance_code,Route::read_cache,Route::return_address,Route::invoke}:
                                    std::array{Route::second,Route::third,Route::return_address,Route::invoke};
    const bool rollback=Increasing?stage::reference_increasing_arguments_provider():
        stage::reference_recursive_arguments_provider();
    for(bool sparse:{false,true}) {
        Fixture f;f.sparse=sparse;
        check(f,Route::invoke);
        auto overflow=f;overflow.offsets={0x80000000u,1u,0x80000000u};
        check(overflow,Route::invoke);
        auto wrapped=f;wrapped.offsets={0u,0xffffffffu,0xffffffffu};
        check(wrapped,Route::invoke);
        for(unsigned i=0;i<pcs.size();++i) {
            auto denied=f;denied.deny_store=pcs[i];
            check(denied,rollback?next[i]:Route::invoke);
            denied.reenter=true;check(denied,rollback?next[i]:after[i]);
            denied.reenter=false;
            denied.move_frame=true;check(denied,rollback?next[i]:Route::invoke);
            denied.move_frame=false;
            denied.fault_pc=pcs[i];check(denied,next[i]);
        }
        auto lease=f;lease.sparse=true;lease.deny_cache=true;
        const auto cache_route=Increasing?Route::read_cache:Route::original;
        check(lease,cache_route);
        lease.fault_pc=Increasing?0x30af23u:0x30b06fu;check(lease,cache_route);
        auto traced=f;traced.trace=true;check(traced,Route::original);
        auto weak=f;weak.authentic=false;check(weak,Route::original);
        auto observed=f;observed.observe=true;observed.observer_fault=true;check(observed,Route::original);
    }
    Fixture crossed;crossed.stack=base+0x1002u;
    check(crossed,rollback?Route::first:Route::invoke);
    return compared;
}
}
int main() try {
    const auto count=prove<false>()+prove<true>();
    std::cout<<"MOPP recursive arguments: "<<count<<" exact child entry and fault comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}

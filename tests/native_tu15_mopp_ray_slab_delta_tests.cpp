#include "physics/runtime/native_tu15_mopp_ray_slab_delta.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage = native_mopp_ray_slab_delta_detail;
using Route = stage::Continuation;
using RatioExit = stage::RatioExit;
constexpr std::uint32_t base=0x62000000u,stack=base+0x180u,rays=base+0x480u,epsilon=0x46a0e8u,ones=0x41c500u;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};

std::uint32_t native_single_spill(long double x,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    float result;std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1"
        :"=m"(result),"=m"(actual):"m"(x):"st");
    status=(status&~0x200u)|(actual&0x200u);
    return std::bit_cast<std::uint32_t>(result);
#else
    throw std::runtime_error("Required TU1.5 FSTP32 backend unavailable");
#endif
}

Route retained_0(WorldState &s,Route route) {
    switch(route) {case Route::original:break;case Route::read_end:goto L_reload;case Route::ratio:case Route::skip_ratio:return route;}
s.instruction_literal(0x30b264u); s.fp.push(ext(f(r(s.memory,(0x24u+s.gp[4]),4))));
s.instruction_literal(0x30b268u); s.xmm[1]=v(s.memory,(0x0u+s.gp[7]));
s.instruction_literal(0x30b26bu); s.xmm[2]=v(s.memory,(0x10u+s.gp[7]));
s.instruction_literal(0x30b26fu); s.fp.st(0)=sub(s.fp.st(0),s.fp.st(1));
s.instruction_literal(0x30b271u); w(s.memory,s.observe,0x30b271u,(0x14u+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
L_reload: s.instruction_literal(0x30b275u); s.fp.st(0)=sub(ext(f(r(s.memory,(0x18u+s.gp[4]),4))),s.fp.st(0));
s.instruction_literal(0x30b279u); s.fp.push(ext(f(r(s.memory,(0x14u+s.gp[4]),4))));
s.instruction_literal(0x30b27du); s.fp.st(0)=mul(s.fp.st(0),s.fp.st(1));
s.instruction_literal(0x30b27fu); s.fp.comparison(ext(f(r(s.memory,(0x46a0e8u),4))),1);
s.instruction_literal(0x30b285u); s.set(0,s.fp.word(),2,0);
s.instruction_literal(0x30b287u); s.flags.logic((s.reg(0,1,8))&(0x5u),1);
s.instruction_literal(0x30b28au); if(s.flags.pf)return Route::skip_ratio;
return Route::ratio;
}

Route retained_1(WorldState &s,Route route) {
    switch(route) {case Route::original:break;case Route::read_end:goto L_reload;case Route::ratio:case Route::skip_ratio:return route;}
s.instruction_literal(0x30b2c7u); s.fp.st(0)=s.fp.st(0);s.fp.status&=~0x200u;s.fp.pop();
s.instruction_literal(0x30b2c9u); s.fp.push(ext(f(r(s.memory,(0x24u+s.gp[4]),4))));
s.instruction_literal(0x30b2cdu); s.fp.st(0)=sub(s.fp.st(0),s.fp.st(1));
s.instruction_literal(0x30b2cfu); w(s.memory,s.observe,0x30b2cfu,(0x14u+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
L_reload: s.instruction_literal(0x30b2d3u); s.fp.st(0)=sub(ext(f(r(s.memory,(0x18u+s.gp[4]),4))),s.fp.st(0));
s.instruction_literal(0x30b2d7u); s.fp.push(ext(f(r(s.memory,(0x14u+s.gp[4]),4))));
s.instruction_literal(0x30b2dbu); s.fp.st(0)=mul(s.fp.st(0),s.fp.st(1));
s.instruction_literal(0x30b2ddu); s.fp.comparison(ext(f(r(s.memory,(0x46a0e8u),4))),1);
s.instruction_literal(0x30b2e3u); s.set(0,s.fp.word(),2,0);
s.instruction_literal(0x30b2e5u); s.flags.logic((s.reg(0,1,8))&(0x5u),1);
s.instruction_literal(0x30b2e8u); if(s.flags.pf)return Route::skip_ratio;
return Route::ratio;
}

template<bool Second>
void retained_ratio(WorldState &s,RatioExit route) {
    using namespace native_tu15_leaves;
    if(route==RatioExit::complete)return;
    auto &g=s.gp;auto &x=s.xmm;
    if constexpr(Second) {
        if(route==RatioExit::branch)goto branch;
        if(route==RatioExit::original) {
            s.instruction_literal(0x30b2eeu);s.fp.push(ext(f(r(s.memory,g[4]+0x14u))));
            s.instruction_literal(0x30b2f2u);g[1]=s.flags.shift(g[1],4u,4);
            s.instruction_literal(0x30b2f5u);s.fp.st(0)=sub(s.fp.st(0),s.fp.st(1));
            s.instruction_literal(0x30b2f7u);s.fp.st(0)=divide(ext(f(r(s.memory,g[4]+0x14u))),s.fp.st(0));
            s.instruction_literal(0x30b2fbu);w(s.memory,s.observe,0x30b2fbu,g[4]+0x14u,4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
        }
        s.instruction_literal(0x30b2ffu);x[0]=_mm_set_ss(f(r(s.memory,g[4]+0x14u)));
        s.instruction_literal(0x30b305u);x[0]=_mm_shuffle_ps(x[0],x[0],0);
        s.instruction_literal(0x30b309u);x[3]=x[0];
        s.instruction_literal(0x30b30cu);x[3]=ordered_mul_ps(x[3],x[2]);
        s.instruction_literal(0x30b30fu);x[2]=v(s.memory,ones);
        s.instruction_literal(0x30b316u);x[2]=_mm_sub_ps(x[2],x[0]);
        s.instruction_literal(0x30b319u);x[2]=ordered_mul_ps(x[2],x[1]);
        s.instruction_literal(0x30b31cu);x[2]=ordered_add_ps(x[2],x[3]);
        s.instruction_literal(0x30b31fu);vw(s.memory,s.observe,0x30b31fu,g[1]+g[7],x[2]);
branch: s.instruction_literal(0x30b323u);
    } else {
        if(route==RatioExit::original) {
            s.instruction_literal(0x30b28cu);s.fp.push(ext(f(r(s.memory,g[4]+0x14u))));
            s.instruction_literal(0x30b290u);x[4]=v(s.memory,ones);
            s.instruction_literal(0x30b297u);s.fp.st(0)=sub(s.fp.st(0),s.fp.st(1));
            s.instruction_literal(0x30b299u);g[0]=g[1];
            s.instruction_literal(0x30b29bu);g[0]=s.flags.shift(g[0],4u,4);
            s.instruction_literal(0x30b29eu);g[2]=g[7];
            s.instruction_literal(0x30b2a0u);s.fp.st(0)=divide(ext(f(r(s.memory,g[4]+0x14u))),s.fp.st(0));
            s.instruction_literal(0x30b2a4u);g[2]=s.flags.minus(g[2],g[0],4);
            s.instruction_literal(0x30b2a6u);w(s.memory,s.observe,0x30b2a6u,g[4]+0x14u,4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
        }
        s.instruction_literal(0x30b2aau);x[0]=_mm_set_ss(f(r(s.memory,g[4]+0x14u)));
        s.instruction_literal(0x30b2b0u);x[0]=_mm_shuffle_ps(x[0],x[0],0);
        s.instruction_literal(0x30b2b4u);x[4]=_mm_sub_ps(x[4],x[0]);
        s.instruction_literal(0x30b2b7u);x[3]=x[0];
        s.instruction_literal(0x30b2bau);x[3]=ordered_mul_ps(x[3],x[2]);
        s.instruction_literal(0x30b2bdu);x[4]=ordered_mul_ps(x[4],x[1]);
        s.instruction_literal(0x30b2c0u);x[4]=ordered_add_ps(x[4],x[3]);
        s.instruction_literal(0x30b2c3u);vw(s.memory,s.observe,0x30b2c3u,g[2]+0x10u,x[4]);
    }
}

struct Fixture {
    std::uint32_t sp{stack},ray{rays},start{0x40000000u},end{0x3fa00000u};
    std::uint32_t fault_pc{},deny_store{},epsilon_bits{};
    unsigned precision{3},rounding{},depth{3};
    bool trace{},observer{},observer_fault{},rebind{},partial_constants{},borrowed_epsilon{};
    bool move_frame{},sparse_after_write{},borrowed_stack{},change_control{};
    bool observe_after_write{};
};
struct Access {
    std::uint32_t pc,address;std::size_t width;bool write;
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
    Route stage{Route::original},exit{Route::original};
    RatioExit ratio{RatioExit::original};
    std::string fault;
};
struct Bank final : SourcePhysicsContactMemory {
    const Fixture &f;
    Result &r;
    SourceDirectPageIndex index;
    std::array<std::byte,4096> constants{};
    std::array<std::byte,4096> vectors{};
    std::function<SourcePhysicsWorldRuntimeAbi()> snapshot;
    std::function<void()> on_write;
    bool stack_sparse{};

    Bank(const Fixture &fixture,Result &result):f(fixture),r(result) {
        direct_pages_=&index;
        auto &block=index.blocks[base>>22u];
        block=std::make_unique<SourceDirectPageIndex::Block>();
        for(unsigned i=0;i<2;++i)
            (*block)[((base>>12u)&1023u)+i]={r.data.data()+i*4096u,true};
        if(!f.partial_constants) {
            auto &constant_block=index.blocks[epsilon>>22u];
            constant_block=std::make_unique<SourceDirectPageIndex::Block>();
            (*constant_block)[(epsilon>>12u)&1023u]={constants.data(),false};
            auto &vector_block=index.blocks[ones>>22u];
            if(!vector_block)vector_block=std::make_unique<SourceDirectPageIndex::Block>();
            (*vector_block)[(ones>>12u)&1023u]={vectors.data(),false};
        }
        std::memcpy(constants.data()+(epsilon&4095u),&f.epsilon_bits,4u);
        const auto unit=_mm_set1_ps(1.0f);
        std::memcpy(vectors.data()+(ones&4095u),&unit,16u);
        const auto ray_data=_mm_set_ps(-1.0f,4.0f,0.5f,-7.0f);
        std::memcpy(r.data.data()+f.ray-base,&ray_data,16u);
        std::memcpy(r.data.data()+f.ray-base+16u,&ray_data,16u);
        std::memcpy(r.data.data()+f.sp-base+0x18u,&f.end,4u);
        std::memcpy(r.data.data()+f.sp-base+0x24u,&f.start,4u);
        std::memcpy(r.data.data()+f.sp-base+0x14u,&f.start,4u);
    }
    void access(std::uint32_t address,std::size_t width,bool write)const {
        r.accesses.push_back({r.pc,address,width,write});
        r.states.push_back(snapshot());
        if(r.pc==f.fault_pc)throw std::runtime_error("Injected slab provider fault");
    }
    std::optional<std::span<const std::byte>> try_read_view(
        std::uint32_t address,std::size_t width) const noexcept override {
        if(stack_sparse && f.borrowed_stack && address>=base &&
           std::uint64_t(address-base)+width<=r.data.size())
            return std::span<const std::byte>{r.data}.subspan(address-base,width);
        if(f.borrowed_epsilon && address==ones && width==16u)
            return std::span<const std::byte>{vectors}.subspan(ones&4095u,width);
        if(!f.borrowed_epsilon || address!=epsilon || width!=4u)return std::nullopt;
        return std::span<const std::byte>{constants}.subspan(epsilon&4095u,width);
    }
    void read(std::uint32_t address,std::span<std::byte> data)const override {
        if(const auto view=try_read_view(address,data.size())) {
            std::memcpy(data.data(),view->data(),data.size());
            return;
        }
        access(address,data.size(),false);
        const auto *source=address==epsilon?constants.data()+(epsilon&4095u):
            address>=ones&&address<ones+16u?vectors.data()+(address&4095u):r.data.data()+address-base;
        std::memcpy(data.data(),source,data.size());
    }
    void read_vector(std::uint32_t address,std::span<std::byte,16> data)const override {
        if(const auto borrowed=try_read_view(address,data.size()))
            std::memcpy(data.data(),borrowed->data(),data.size());
        else
            SourcePhysicsContactMemory::read_vector(address,data);
    }
    bool try_callback_free_write(std::uint32_t address,std::span<const std::byte> bytes)noexcept override {
        if(f.observer||r.pc==f.deny_store||!direct_page(address,bytes.size()))return false;
        r.accesses.push_back({r.pc,address,bytes.size(),true});
        std::memcpy(r.data.data()+address-base,bytes.data(),bytes.size());
        return true;
    }
    void write(std::uint32_t address,std::span<const std::byte> bytes)override {
        if(try_callback_free_write(address,bytes))return;
        access(address,bytes.size(),true);
        std::memcpy(r.data.data()+address-base,bytes.data(),bytes.size());
        on_write();
    }
};

template<bool Second,bool Ratio=false>
Result run(const Fixture &f,bool native) {
    Result r;r.accesses.reserve(24);
    r.pc=Second?0x30b28au:0x30b25fu;
    Bank bank{f,r};
    SourcePhysicsWorldRuntimeServices services;
    auto trace=f.trace;
    services.instruction=[&](std::uint32_t pc){r.pc=pc;r.instructions.push_back(pc);};
    services.instruction_cursor={&r.pc,&trace,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    SourcePhysicsContactRecordObserver observe;
    const auto record=[&](const auto &event) {
        r.observed.push_back(event);r.states.push_back(bank.snapshot());
        if(f.observer_fault)throw std::runtime_error("Injected slab observer fault");
    };
    if(f.observer)observe=record;
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={0xd1a34ff5u,2u,1u,3u,f.sp,0x2020u,4u,f.ray};
    input.eflags=0x256u;input.x87_depth=f.depth;input.x87_status=0x3200u;
    input.x87[0]=0.5L;input.x87[1]=0.5L;input.x87[2]=-1.25L;
    if constexpr(Ratio) {
        input.xmm[1]={0x3f800000u,0xc0800000u,f.end,0x80000000u};
        input.xmm[2]={0x40000000u,0x40c00000u,0x3f000000u,f.end};
    }
    const auto cw=std::uint16_t(0x7fu|f.precision<<8u|f.rounding<<10u);
    input.floating_point=SourcePhysicsFactoryFloatingPoint{cw,0x1f80u};
    asm volatile("fnclex; fldcw %0"::"m"(cw));
    WorldState state{bank,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
    bank.snapshot=[&]{return state.save();};
    bank.on_write=[&] {
        if(f.rebind)trace=true;
        if(f.observe_after_write)observe=record;
        if(f.rebind || f.move_frame) {
            auto &frame=state.gp[4];
            if(f.move_frame)frame+=0x80u;
            const std::array values{std::bit_cast<std::uint32_t>(1.5f),std::bit_cast<std::uint32_t>(-1.5f)};
            std::memcpy(r.data.data()+frame-base+0x14u,values.data(),sizeof(values));
        }
        if(f.sparse_after_write) {
            bank.stack_sparse=true;
            bank.index.blocks[base>>22u]->at((f.sp>>12u)&1023u).bytes=nullptr;
        }
        if(f.change_control) {
            const auto changed=std::uint16_t(cw^0x0400u);
            asm volatile("fldcw %0"::"m"(changed));
        }
    };
    try {
        if constexpr(Ratio) {
            if(native)r.ratio=stage::slab_ratio<Second>(state,native_single_spill);
            retained_ratio<Second>(state,r.ratio);
        } else {
            if(native)r.stage=stage::slab_delta<Second>(state,native_single_spill);
            if constexpr(Second)r.exit=retained_1(state,r.stage);
            else r.exit=retained_0(state,r.stage);
        }
    } catch(const std::runtime_error &e) {
        r.fault=e.what();
    }
    r.abi=state.save();
    return r;
}
bool same(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_status!=b.x87_status||a.x87_depth!=b.x87_depth||
       a.floating_point->x87_control_word!=b.floating_point->x87_control_word||
       a.floating_point->mxcsr!=b.floating_point->mxcsr)return false;
    for(unsigned i=0;i<a.x87_depth;++i)if(std::memcmp(&a.x87[i],&b.x87[i],10u))return false;
    return true;
}
template<bool Second>
void compare(const Fixture &f,Route route) {
    if(stage::reference_slab_delta_provider() && f.deny_store && route!=Route::original)
        route=Route::read_end;
    if(stage::reference_slab_delta())route=Route::original;
    const auto a=run<Second>(f,false),b=run<Second>(f,true);
    if((b.fault.empty()&&b.stage!=route)||a.exit!=b.exit||a.pc!=b.pc||a.fault!=b.fault||a.data!=b.data||
       a.accesses!=b.accesses||a.observed!=b.observed||a.instructions!=b.instructions||!same(a.abi,b.abi)||
       a.states.size()!=b.states.size())
        {
            std::cerr<<"side "<<Second<<" start/end "<<std::hex<<f.start<<'/'<<f.end<<" route "
                <<int(route)<<'/'<<int(b.stage)<<" PC "<<a.pc<<'/'<<b.pc<<" fault "<<f.fault_pc
                <<" state "<<same(a.abi,b.abi)<<'\n';
            throw std::runtime_error("Slab delta continuation, store prefix or source state differs");
        }
    for(unsigned i=0;i<a.states.size();++i)
        if(!same(a.states[i],b.states[i]))
            throw std::runtime_error("Slab delta provider observed a different live x87 frame");
}

template<bool Second>
unsigned prove() {
    unsigned compared{};
    for(const auto p:{0u,2u,3u})for(unsigned rc=0;rc<4u;++rc) {
        Fixture f;f.precision=p;f.rounding=rc;
        compare<Second>(f,Route::skip_ratio);++compared;
        auto ratio=f;ratio.end=0xbf800000u;
        compare<Second>(ratio,Route::ratio);++compared;
        for(const auto value:{0x7fc12234u,0x7f812345u,0x80000001u,0x80000000u}) {
            auto exceptional=f;exceptional.start=value;
            compare<Second>(exceptional,(value&0x7f800000u)==0x7f800000u?Route::skip_ratio:Route::ratio);++compared;
        }
        auto denied=f;denied.deny_store=Second?0x30b2cfu:0x30b271u;
        compare<Second>(denied,Route::skip_ratio);++compared;
        auto moved=denied;moved.move_frame=true;
        compare<Second>(moved,Route::ratio);++compared;
        moved.sparse_after_write=true;
        compare<Second>(moved,Route::read_end);++compared;
        moved.borrowed_stack=true;
        compare<Second>(moved,Route::ratio);++compared;
        auto changed=denied;changed.change_control=true;
        compare<Second>(changed,Route::read_end);++compared;
        denied.rebind=true;
        compare<Second>(denied,Route::read_end);++compared;
        denied.fault_pc=denied.deny_store;
        compare<Second>(denied,Route::read_end);++compared;
        auto observed=f;observed.observer=true;observed.observer_fault=true;
        compare<Second>(observed,Route::original);++compared;
        auto crossed=f;crossed.sp=base+0xffeu-0x24u;
        crossed.fault_pc=Second?0x30b2c9u:0x30b264u;
        compare<Second>(crossed,Route::original);++compared;
        auto partial=f;partial.partial_constants=true;
        compare<Second>(partial,Route::read_end);++compared;
        partial.fault_pc=Second?0x30b2ddu:0x30b27fu;
        compare<Second>(partial,Route::read_end);++compared;
        partial.borrowed_epsilon=true;
        compare<Second>(partial,Route::skip_ratio);++compared;
        partial.epsilon_bits=0x7f800000u;
        compare<Second>(partial,Route::ratio);++compared;
        partial.deny_store=Second?0x30b2cfu:0x30b271u;partial.rebind=true;
        compare<Second>(partial,Route::read_end);++compared;
        auto traced=f;traced.trace=true;
        compare<Second>(traced,Route::original);++compared;
        if constexpr(!Second) {
            auto ray=f;ray.ray=base+0xffeu;ray.fault_pc=0x30b268u;
            compare<Second>(ray,Route::original);++compared;
        }
    }
    return compared;
}

template<bool Second>
void compare_ratio(const Fixture &f,RatioExit expected) {
    if(stage::reference_slab_ratio())expected=RatioExit::original;
    const auto a=run<Second,true>(f,false),b=run<Second,true>(f,true);
    if((b.fault.empty()&&b.ratio!=expected)||a.pc!=b.pc||a.fault!=b.fault||a.data!=b.data||
       a.accesses!=b.accesses||a.observed!=b.observed||a.instructions!=b.instructions||
       !same(a.abi,b.abi)||a.states.size()!=b.states.size()) {
        std::cerr<<"ratio "<<Second<<" PC "<<std::hex<<a.pc<<'/'<<b.pc
                 <<" route "<<int(expected)<<'/'<<int(b.ratio)<<'\n';
        throw std::runtime_error("Slab ratio output or continuation differs");
    }
    for(unsigned i=0;i<a.states.size();++i)
        if(!same(a.states[i],b.states[i]))
            throw std::runtime_error("Slab ratio provider observed different live state");
}

template<bool Second>
unsigned prove_ratio() {
    constexpr auto spill=Second?0x30b2fbu:0x30b2a6u;
    constexpr auto vector=Second?0x30b31fu:0x30b2c3u;
    unsigned count{};
    auto check=[&](const Fixture &f,RatioExit route) {compare_ratio<Second>(f,route);++count;};
    for(unsigned precision:{0u,2u,3u})for(unsigned rounding=0;rounding<4u;++rounding) {
        Fixture f;f.precision=precision;f.rounding=rounding;
        check(f,RatioExit::complete);
        for(auto value:{0u,0x80000000u,0x00000001u,0x7f800000u,0x7fc21334u,0x7f812345u}) {
            auto unusual=f;unusual.start=value;
            check(unusual,RatioExit::complete);
            unusual.end=value;
            check(unusual,RatioExit::complete);
        }
        auto guarded=f;guarded.deny_store=spill;
        check(guarded,RatioExit::complete);
        guarded.move_frame=true;
        check(guarded,RatioExit::complete);
        guarded.sparse_after_write=true;
        check(guarded,RatioExit::reload);
        guarded.borrowed_stack=true;
        check(guarded,RatioExit::complete);
        guarded.rebind=true;
        check(guarded,RatioExit::reload);
        guarded.fault_pc=spill;
        check(guarded,RatioExit::original);
        auto device=f;device.partial_constants=true;
        check(device,Second?RatioExit::reload:RatioExit::original);
        device.fault_pc=Second?0x30b30fu:0x30b290u;
        check(device,RatioExit::original);
        device.fault_pc=0;device.borrowed_epsilon=true;
        check(device,RatioExit::complete);
        auto output=f;output.deny_store=vector;output.rebind=true;output.observe_after_write=true;
        check(output,Second?RatioExit::branch:RatioExit::complete);
        output.observer_fault=true;
        check(output,RatioExit::original);
        output.fault_pc=vector;
        check(output,RatioExit::original);
        auto traced=f;traced.trace=true;
        check(traced,RatioExit::original);
        auto full=f;full.depth=8;
        check(full,RatioExit::original);
    }
    return count;
}
}

int main() try {
    std::cout<<"MOPP slab delta and ratio: "<<prove<false>()+prove<true>()+prove_ratio<false>()+prove_ratio<true>()
             <<" exact continuation and provider-fault comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}

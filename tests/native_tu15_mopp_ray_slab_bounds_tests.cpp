#include "physics/runtime/native_tu15_mopp_ray_slab_bounds.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage = native_mopp_ray_slab_bounds_detail;
using Route = stage::Continuation;
constexpr std::uint32_t base=0x62000000u,stack=base+0x180u,program=base+0x600u;
constexpr std::uint32_t cache=base+0x800u,vars=base+0xa00u;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};


void retained_0(WorldState &s,Route route) {
    switch(route) {case Route::original: goto L_original; case Route::store_low: goto L_store_low; case Route::short_adjust_axis: goto L_short_adjust_axis; case Route::short_read_lower: goto L_short_read_lower; case Route::store_high: goto L_store_high; case Route::read_upper: goto L_read_upper; default:return;}
L_original: s.instruction_literal(0x30b1dbu); s.set(1,r(s.memory,(0x1u+s.gp[6]),1),4,0);
s.instruction_literal(0x30b1dfu); s.set(2,r(s.memory,(0x2u+s.gp[6]),1),4,0);
L_store_low: s.instruction_literal(0x30b1e3u); w(s.memory,s.observe,0x30b1e3u,(0x14u+s.gp[4]),4,s.reg(1,4,0));
L_short_adjust_axis: s.instruction_literal(0x30b1e7u); s.set(0,s.flags.plus(s.reg(0,4,0),0xffffffdau,4),4,0);
s.instruction_literal(0x30b1eau); s.set(6,s.flags.plus(s.reg(6,4,0),0x3u,4),4,0);
L_short_read_lower: s.instruction_literal(0x30b1edu); s.fp.push(static_cast<long double>(si(r(s.memory,(0x14u+s.gp[4]),4))));
L_store_high: s.instruction_literal(0x30b1f1u); w(s.memory,s.observe,0x30b1f1u,(0x14u+s.gp[4]),4,s.reg(2,4,0));
L_read_upper: s.instruction_literal(0x30b1f5u); s.fp.push(static_cast<long double>(si(r(s.memory,(0x14u+s.gp[4]),4))));
}
void retained_1(WorldState &s,Route route) {
    switch(route) {case Route::original: goto L_original; case Route::store_low: goto L_store_low; case Route::wide_cache_low: goto L_wide_cache_low; case Route::wide_read_lower: goto L_wide_read_lower; case Route::wide_scale_cache_low: goto L_wide_scale_cache_low; case Route::wide_read_left_code: goto L_wide_read_left_code; case Route::wide_read_right_code: goto L_wide_read_right_code; case Route::wide_scale_vars_low: goto L_wide_scale_vars_low; case Route::store_high: goto L_store_high; case Route::wide_cache_high: goto L_wide_cache_high; case Route::wide_translate_low: goto L_wide_translate_low; case Route::read_upper: goto L_read_upper; case Route::wide_scale_cache_high: goto L_wide_scale_cache_high; case Route::wide_scale_vars_high: goto L_wide_scale_vars_high; case Route::wide_translate_high: goto L_wide_translate_high; default:return;}
L_original: s.instruction_literal(0x30b17du); s.set(1,r(s.memory,(0x1u+s.gp[6]),1),4,0);
s.instruction_literal(0x30b181u); s.set(2,r(s.memory,(0x2u+s.gp[6]),1),4,0);
s.instruction_literal(0x30b185u); s.set(1,s.flags.shift(s.reg(1,4,0),0x8u,4),4,0);
s.instruction_literal(0x30b188u); s.set(1,s.flags.plus(s.reg(1,4,0),s.reg(2,4,0),4),4,0);
s.instruction_literal(0x30b18au); s.set(2,r(s.memory,(0x3u+s.gp[6]),1),4,0);
s.instruction_literal(0x30b18eu); s.set(1,s.flags.shift(s.reg(1,4,0),0x8u,4),4,0);
s.instruction_literal(0x30b191u); s.set(1,s.flags.plus(s.reg(1,4,0),s.reg(2,4,0),4),4,0);
s.instruction_literal(0x30b193u); s.set(2,r(s.memory,(0x5u+s.gp[6]),1),4,0);
L_store_low: s.instruction_literal(0x30b197u); w(s.memory,s.observe,0x30b197u,(0x14u+s.gp[4]),4,s.reg(1,4,0));
L_wide_cache_low: s.instruction_literal(0x30b19bu); s.set(1,r(s.memory,(0x28u+s.gp[4]),4),4,0);
s.instruction_literal(0x30b19fu); s.set(0,s.flags.plus(s.reg(0,4,0),0xffffffd7u,4),4,0);
L_wide_read_lower: s.instruction_literal(0x30b1a2u); s.fp.push(static_cast<long double>(si(r(s.memory,(0x14u+s.gp[4]),4))));
L_wide_scale_cache_low: s.instruction_literal(0x30b1a6u); stage::wide_arithmetic<true>(s,r(s.memory,(0x14u+s.gp[1]),4));
L_wide_read_left_code: s.instruction_literal(0x30b1a9u); s.set(1,r(s.memory,(0x4u+s.gp[6]),1),4,0);
s.instruction_literal(0x30b1adu); s.set(1,s.flags.shift(s.reg(1,4,0),0x8u,4),4,0);
s.instruction_literal(0x30b1b0u); s.set(2,s.flags.plus(s.reg(2,4,0),s.reg(1,4,0),4),4,0);
L_wide_read_right_code: s.instruction_literal(0x30b1b2u); s.set(1,r(s.memory,(0x6u+s.gp[6]),1),4,0);
s.instruction_literal(0x30b1b6u); s.set(2,s.flags.shift(s.reg(2,4,0),0x8u,4),4,0);
s.instruction_literal(0x30b1b9u); s.set(2,s.flags.plus(s.reg(2,4,0),s.reg(1,4,0),4),4,0);
L_wide_scale_vars_low: s.instruction_literal(0x30b1bbu); stage::wide_arithmetic<true>(s,r(s.memory,(0x14u+s.gp[3]),4));
L_store_high: s.instruction_literal(0x30b1beu); w(s.memory,s.observe,0x30b1beu,(0x14u+s.gp[4]),4,s.reg(2,4,0));
L_wide_cache_high: s.instruction_literal(0x30b1c2u); s.set(2,r(s.memory,(0x28u+s.gp[4]),4),4,0);
L_wide_translate_low: s.instruction_literal(0x30b1c6u); stage::wide_arithmetic<false>(s,r(s.memory,(0x0u+s.gp[3]+s.gp[0]*4u),4));
s.instruction_literal(0x30b1c9u); s.set(6,s.flags.plus(s.reg(6,4,0),0x7u,4),4,0);
L_read_upper: s.instruction_literal(0x30b1ccu); s.fp.push(static_cast<long double>(si(r(s.memory,(0x14u+s.gp[4]),4))));
L_wide_scale_cache_high: s.instruction_literal(0x30b1d0u); stage::wide_arithmetic<true>(s,r(s.memory,(0x14u+s.gp[2]),4));
L_wide_scale_vars_high: s.instruction_literal(0x30b1d3u); stage::wide_arithmetic<true>(s,r(s.memory,(0x14u+s.gp[3]),4));
L_wide_translate_high: s.instruction_literal(0x30b1d6u); stage::wide_arithmetic<false>(s,r(s.memory,(0x0u+s.gp[3]+s.gp[0]*4u),4));
s.instruction_literal(0x30b1d9u);
}

struct Fixture {
    std::uint32_t cache_base{cache},vars_base{vars},code{program},sp{stack},deny{},fault{};
    std::uint32_t cache_scale{0x3f355802u},var_scale{0x3f4157ffu},translate{0x3f17ffffu};
    unsigned precision{3},rounding{},depth{3};
    bool trace{},observer{},observer_fault{},deny_second_page{},enable_trace{},move_frame{},move_code{},detach_code{};
};
struct Access {
    std::uint32_t pc,address;std::size_t width;bool write;
    bool operator==(const Access &)const=default;
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;
    std::array<std::byte,8192> bytes{};
    std::vector<Access> events;
    std::vector<std::uint32_t> instructions;
    std::vector<SourcePhysicsContactRecordStore> observed;
    std::vector<SourcePhysicsWorldRuntimeAbi> callback_abi;
    std::uint32_t pc{0x30b179u};
    Route route{Route::original};
    std::string fault;
};
struct Bank final : SourcePhysicsContactMemory {
    const Fixture &f;
    Result &r;
    SourceDirectPageIndex index;
    std::function<SourcePhysicsWorldRuntimeAbi()> snapshot;
    std::function<void()> on_write;

    Bank(const Fixture &fixture,Result &result):f(fixture),r(result) {
        direct_pages_=&index;
        auto &block=index.blocks[base>>22u];
        block=std::make_unique<SourceDirectPageIndex::Block>();
        (*block)[(base>>12u)&1023u]={r.bytes.data(),true};
        if(!f.deny_second_page)(*block)[((base>>12u)&1023u)+1u]={r.bytes.data()+4096u,true};
        constexpr std::array<std::uint8_t,7> encoded{0x2a,255,129,64,155,87,157};
        std::memcpy(r.bytes.data()+f.code-base,encoded.data(),encoded.size());
        std::memcpy(r.bytes.data()+f.sp-base+0x28u,&f.cache_base,4u);
        std::memcpy(r.bytes.data()+f.cache_base-base+0x14u,&f.cache_scale,4u);
        std::memcpy(r.bytes.data()+f.vars_base-base+0x14u,&f.var_scale,4u);
        std::memcpy(r.bytes.data()+f.vars_base-base+4u,&f.translate,4u);
    }
    void access(std::uint32_t address,std::size_t width,bool write)const {
        r.events.push_back({r.pc,address,width,write});
        r.callback_abi.push_back(snapshot());
        if(r.pc==f.fault)throw std::runtime_error("Injected slab bounds provider fault");
    }
    void read(std::uint32_t address,std::span<std::byte> data)const override {
        access(address,data.size(),false);
        std::memcpy(data.data(),r.bytes.data()+address-base,data.size());
    }
    bool try_callback_free_write(std::uint32_t address,std::span<const std::byte> bytes)noexcept override {
        if(f.observer||r.pc==f.deny||!direct_page(address,bytes.size()))return false;
        r.events.push_back({r.pc,address,bytes.size(),true});
        std::memcpy(r.bytes.data()+address-base,bytes.data(),bytes.size());
        return true;
    }
    void write(std::uint32_t address,std::span<const std::byte> bytes)override {
        if(try_callback_free_write(address,bytes))return;
        access(address,bytes.size(),true);
        std::memcpy(r.bytes.data()+address-base,bytes.data(),bytes.size());
        on_write();
    }
};
template<bool Wide>
Result run(const Fixture &f,bool native) {
    Result r;r.events.reserve(32);
    Bank bank{f,r};
    SourcePhysicsWorldRuntimeServices services;
    auto trace=f.trace;
    services.instruction=[&](std::uint32_t pc){r.pc=pc;r.instructions.push_back(pc);};
    services.instruction_cursor={&r.pc,&trace,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    SourcePhysicsContactRecordObserver observe;
    if(f.observer)observe=[&](const auto &event) {
        r.observed.push_back(event);r.callback_abi.push_back(bank.snapshot());
        if(f.observer_fault)throw std::runtime_error("Injected slab bounds observer fault");
    };
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={Wide?0x2au:0x27u,2u,1u,f.vars_base,f.sp,0x2020u,f.code,0x4300u};
    input.eflags=0x256u;input.x87_depth=f.depth;input.x87_status=0x3200u;
    input.x87[0]=0.5L;input.x87[1]=0.5L;input.x87[2]=-1.25L;
    const auto cw=std::uint16_t(0x7fu|f.precision<<8u|f.rounding<<10u);
    input.floating_point=SourcePhysicsFactoryFloatingPoint{cw,0x1f80u};
    asm volatile("fnclex; fldcw %0"::"m"(cw));
    WorldState state{bank,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
    bank.snapshot=[&]{return state.save();};
    bank.on_write=[&] {
        if(f.enable_trace)trace=true;
        if(f.move_frame) {
            state.gp[4]+=0x100u;
            const std::uint32_t replacement=17u,cache_pointer=f.cache_base;
            const auto frame=state.gp[4]-base;
            std::memcpy(r.bytes.data()+frame+0x14u,&replacement,4u);
            std::memcpy(r.bytes.data()+frame+0x28u,&cache_pointer,4u);
        }
        if(f.move_code) {
            state.gp[6]=base+0x1400u;
            constexpr std::array<std::uint8_t,7> moved{0x2a,17,23,31,63,85,122};
            std::memcpy(r.bytes.data()+state.gp[6]-base,moved.data(),moved.size());
        }
        if(f.detach_code)(*bank.index.blocks[base>>22u])[((base>>12u)&1023u)+1u]={};
    };
    try {
        if(native)r.route=stage::slab_bounds<Wide>(state);
        if constexpr(Wide)retained_1(state,r.route);
        else retained_0(state,r.route);
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
template<bool Wide>
void compare(const Fixture &f,Route route) {
    if(stage::reference_slab_bounds())route=Route::original;
    const auto a=run<Wide>(f,false),b=run<Wide>(f,true);
    if((b.fault.empty()&&b.route!=route)||a.pc!=b.pc||a.fault!=b.fault||a.bytes!=b.bytes||a.events!=b.events||
       a.observed!=b.observed||a.instructions!=b.instructions||!same(a.abi,b.abi)||
       a.callback_abi.size()!=b.callback_abi.size()) {
        std::cerr<<"slab bounds "<<Wide<<" deny/fault "<<std::hex<<f.deny<<'/'<<f.fault
                 <<" end "<<a.pc<<'/'<<b.pc<<" route "<<int(route)<<'/'<<int(b.route)<<'\n';
        throw std::runtime_error("Slab bounds continuation, provider or ABI differs");
    }
    for(unsigned i=0;i<a.callback_abi.size();++i)
        if(!same(a.callback_abi[i],b.callback_abi[i]))
            throw std::runtime_error("Slab bounds provider observed different live x87 or ABI");
}

template<bool Wide>
unsigned prove() {
    unsigned compared{};
    for(const auto p:{0u,2u,3u})for(unsigned rc=0;rc<4u;++rc) {
        Fixture f;f.precision=p;f.rounding=rc;
        compare<Wide>(f,Route::endpoint);++compared;
        for(const auto &[pc,next]:std::array{std::pair{Wide?0x30b197u:0x30b1e3u,Route::store_low},
                                            std::pair{Wide?0x30b1beu:0x30b1f1u,Route::store_high}}) {
            auto cold=f;cold.deny=pc;
            const bool rollback=Wide?stage::reference_wide_slab_provider():stage::reference_slab_bounds_provider();
            const auto advanced=rollback?next:Route::endpoint;
            const auto traced_next=rollback?next:Wide?
                (pc==0x30b197u?Route::wide_cache_low:Route::wide_cache_high):
                (pc==0x30b1e3u?Route::short_adjust_axis:Route::read_upper);
            compare<Wide>(cold,advanced);++compared;
            cold.move_frame=true;cold.move_code=Wide;
            compare<Wide>(cold,advanced);++compared;
            cold.move_frame=false;cold.move_code=false;
            cold.enable_trace=true;
            compare<Wide>(cold,traced_next);++compared;
            cold.fault=pc;
            compare<Wide>(cold,next);++compared;
        }
        auto observed=f;observed.observer=true;observed.observer_fault=true;
        compare<Wide>(observed,Route::original);++compared;
        auto traced=f;traced.trace=true;
        compare<Wide>(traced,Route::original);++compared;
        auto crossing=f;crossing.code=base+0xffdu;crossing.deny_second_page=true;
        if constexpr(!Wide)crossing.code++;
        crossing.fault=Wide?0x30b18au:0x30b1dfu;
        compare<Wide>(crossing,Route::original);++compared;
        auto scratch_alias=f;scratch_alias.code=stack+0x14u-4u;
        compare<Wide>(scratch_alias,Route::endpoint);++compared;
        if constexpr(Wide) {
            auto remapped=f;remapped.deny=0x30b197u;remapped.move_code=true;
            remapped.detach_code=true;remapped.fault=0x30b1a9u;
            compare<Wide>(remapped,Route::wide_read_left_code);++compared;
            for(const auto bits:{0x80000000u,0x80000001u,0x7fc34342u,0x7f812345u,0x7f800000u}) {
                auto special=f;special.cache_scale=bits;special.var_scale=bits^0x80000000u;
                compare<Wide>(special,Route::endpoint);++compared;
            }
            auto aliased=f;aliased.vars_base=stack;
            compare<Wide>(aliased,Route::endpoint);++compared;
            auto cache_fault=f;cache_fault.deny_second_page=true;cache_fault.cache_base=base+0x1400u;
            cache_fault.fault=0x30b1a6u;
            compare<Wide>(cache_fault,Route::wide_scale_cache_low);++compared;
            auto scale_fault=f;scale_fault.deny_second_page=true;scale_fault.vars_base=base+0x1600u;
            scale_fault.fault=0x30b1bbu;
            compare<Wide>(scale_fault,Route::wide_scale_vars_low);++compared;
            auto translation=f;translation.vars_base=base+0xffeu-4u;
            translation.fault=0x30b1c6u;
            compare<Wide>(translation,Route::wide_translate_low);++compared;
            auto pointer=f;pointer.sp=base+0xffeu-0x28u;pointer.fault=0x30b19bu;
            compare<Wide>(pointer,Route::wide_cache_low);++compared;
        }
        f.depth=7;
        compare<Wide>(f,Route::original);++compared;
    }
    return compared;
}
}

int main() try {
    std::cout<<"30B17D/30B1DB slab bounds: "<<prove<false>()+prove<true>()
             <<" exact continuation and provider comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}

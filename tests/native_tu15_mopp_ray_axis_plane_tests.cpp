#include "physics/runtime/native_tu15_mopp_ray_axis_plane.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage = native_mopp_ray_axis_plane_detail;
using Route = stage::Continuation;
constexpr std::uint32_t base=0x62000000u,stack=base+0x180u,rays=base+0x500u,program=base+0x1200u;
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

void retained_0(WorldState &s,Route route) {
    switch(route) {case Route::original: goto L_original; case Route::store_lower: goto L_store_lower; case Route::adjust_axis: goto L_adjust_axis; case Route::store_axis: goto L_store_axis; case Route::read_lower: goto L_read_lower; case Route::spill_lower: goto L_spill_lower; case Route::read_upper: goto L_read_upper; case Route::read_start: goto L_read_start; case Route::read_end: goto L_read_end; default:return;}
L_original: s.instruction_literal(0x30ad9bu); s.set(1,r(s.memory,(0x1u+s.gp[6]),1),4,0);
s.instruction_literal(0x30ad9fu); s.set(2,r(s.memory,(0x2u+s.gp[6]),1),4,0);
L_store_lower: s.instruction_literal(0x30ada3u); w(s.memory,s.observe,0x30ada3u,(0x14u+s.gp[4]),4,s.reg(1,4,0));
L_adjust_axis: s.instruction_literal(0x30ada7u); s.set(0,s.flags.plus(s.reg(0,4,0),0xfffffff0u,4),4,0);
L_store_axis: s.instruction_literal(0x30adaau); w(s.memory,s.observe,0x30adaau,(0x3cu+s.gp[4]),4,s.reg(0,4,0));
L_read_lower: s.instruction_literal(0x30adaeu); s.fp.push(static_cast<long double>(si(r(s.memory,(0x14u+s.gp[4]),4))));
s.instruction_literal(0x30adb2u); w(s.memory,s.observe,0x30adb2u,(0x14u+s.gp[4]),4,s.reg(2,4,0));
L_spill_lower: s.instruction_literal(0x30adb6u); w(s.memory,s.observe,0x30adb6u,(0x20u+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
L_read_upper: s.instruction_literal(0x30adbau); s.fp.push(static_cast<long double>(si(r(s.memory,(0x14u+s.gp[4]),4))));
s.instruction_literal(0x30adbeu); w(s.memory,s.observe,0x30adbeu,(0x1cu+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
L_read_start: s.instruction_literal(0x30adc2u); s.fp.push(ext(f(r(s.memory,(0x0u+s.gp[7]+s.gp[0]*4u),4))));
L_read_end: s.instruction_literal(0x30adc5u); s.fp.push(ext(f(r(s.memory,(0x10u+s.gp[7]+s.gp[0]*4u),4))));
}

void retained_1(WorldState &s,Route route) {
    switch(route) {case Route::original: goto L_original; case Route::clear_left: goto L_clear_left; case Route::read_left: goto L_read_left; case Route::store_right: goto L_store_right; default:return;}
L_original: s.instruction_literal(0x30adc9u); s.set(0,r(s.memory,(0x3u+s.gp[6]),1),4,0);
s.instruction_literal(0x30adcdu); s.set(6,s.flags.plus(s.reg(6,4,0),0x4u,4),4,0);
L_clear_left: s.instruction_literal(0x30add0u); w(s.memory,s.observe,0x30add0u,(0x24u+s.gp[4]),4,0x0u);
L_read_left: s.instruction_literal(0x30add8u); s.set(1,r(s.memory,(0x24u+s.gp[4]),4),4,0);
L_store_right: s.instruction_literal(0x30addcu); w(s.memory,s.observe,0x30addcu,(0x14u+s.gp[4]),4,s.reg(0,4,0));
}

struct Fixture {
    std::uint32_t ray{rays}, program_base{program}, stack_base{stack}, deny_store{}, fault_pc{};
    unsigned precision{3},rounding{},depth{3};
    bool trace{},observer{},observer_fault{},callback_activates_trace{},device_program{},move_frame{};
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
    Route stage{Route::original};
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
        (*block)[(base>>12u)&1023u]={r.data.data(),true};
        if(!f.device_program)(*block)[((base>>12u)&1023u)+1u]={r.data.data()+4096u,true};
        const std::array<std::byte,4> code{std::byte{0x12},std::byte{255},std::byte{129},std::byte{64}};
        std::memcpy(r.data.data()+f.program_base-base,code.data(),4u);
        const std::uint32_t start=0x80000000u,end=0x7fc32012u;
        std::memcpy(r.data.data()+f.ray-base+8u,&start,4u);
        std::memcpy(r.data.data()+f.ray-base+0x18u,&end,4u);
    }
    void access(std::uint32_t address,std::size_t width,bool write)const {
        r.accesses.push_back({r.pc,address,width,write});
        r.states.push_back(snapshot());
        if(r.pc==f.fault_pc)throw std::runtime_error("Injected axis-plane provider fault");
    }
    void read(std::uint32_t address,std::span<std::byte> data)const override {
        access(address,data.size(),false);
        std::memcpy(data.data(),r.data.data()+address-base,data.size());
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

template<bool Offsets, bool Shared = false>
Result run(const Fixture &f,bool native) {
    Result r;r.accesses.reserve(24);
    r.pc=Shared ? 0x30add0u : Offsets ? 0x30adc5u : 0x30ad99u;
    Bank bank{f,r};
    SourcePhysicsWorldRuntimeServices services;
    auto trace=f.trace;
    services.instruction=[&](std::uint32_t pc){r.pc=pc;r.instructions.push_back(pc);};
    services.instruction_cursor={&r.pc,&trace,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    SourcePhysicsContactRecordObserver observe;
    if(f.observer)observe=[&](const auto &event) {
        r.observed.push_back(event);r.states.push_back(bank.snapshot());
        if(f.observer_fault)throw std::runtime_error("Injected axis-plane observer fault");
    };
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={0x12u,2u,1u,3u,f.stack_base,0x2020u,f.program_base,f.ray};
    input.eflags=0x256u;input.x87_depth=f.depth;input.x87_status=0x3200u;
    input.x87[0]=0.5L;input.x87[1]=0.5L;input.x87[2]=-1.25L;
    const auto cw=std::uint16_t(0x7fu|f.precision<<8u|f.rounding<<10u);
    input.floating_point=SourcePhysicsFactoryFloatingPoint{cw,0x1f80u};
    asm volatile("fnclex; fldcw %0"::"m"(cw));
    WorldState state{bank,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
    bank.snapshot=[&]{return state.save();};
    bank.on_write=[&] {
        if(f.callback_activates_trace)trace=true;
        if(f.move_frame) {
            state.gp[4]+=0x100u;
            if(r.pc==0x30adb2u)state.fp.st(0)=19.0L;
        }
        if(f.move_frame||f.callback_activates_trace) {
            const std::uint32_t replacement=7u;
            std::memcpy(r.data.data()+state.gp[4]-base+0x14u,&replacement,4u);
        }
    };
    try {
        if(native) {
            if constexpr(Offsets)r.stage=stage::plane_offsets(state, Shared);
            else r.stage=stage::axis_plane(state,native_single_spill);
        }
        if constexpr(Offsets)retained_1(state, Shared && r.stage==Route::original ? Route::read_left : r.stage);
        else retained_0(state,r.stage);
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
template<bool Offsets, bool Shared = false>
void compare(const Fixture &f,Route route) {
    if(stage::reference_axis_plane())route=Route::original;
    const auto a=run<Offsets, Shared>(f,false),b=run<Offsets, Shared>(f,true);
    if((b.fault.empty()&&b.stage!=route)||a.pc!=b.pc||a.fault!=b.fault||a.data!=b.data||a.accesses!=b.accesses||
       a.observed!=b.observed||a.instructions!=b.instructions||!same(a.abi,b.abi)||a.states.size()!=b.states.size()) {
        std::cerr<<"axis plane "<<Offsets<<" deny/fault "<<std::hex<<f.deny_store<<'/'<<f.fault_pc
                 <<" end "<<a.pc<<'/'<<b.pc<<" route "<<int(route)<<'/'<<int(b.stage)<<'\n';
        throw std::runtime_error("Axis plane continuation, provider or ABI differs");
    }
    for(unsigned i=0;i<a.states.size();++i)
        if(!same(a.states[i],b.states[i]))throw std::runtime_error("Axis plane provider saw a different live x87 frame");
}

template<bool Offsets, bool Shared = false>
unsigned prove() {
    [[maybe_unused]] constexpr std::array axis_stores{std::pair{0x30ada3u,Route::store_lower},
        std::pair{0x30adaau,Route::store_axis},std::pair{0x30adb2u,Route::read_lower},
        std::pair{0x30adb6u,Route::read_upper},std::pair{0x30adbeu,Route::read_start}};
    [[maybe_unused]] constexpr std::array offset_stores{std::pair{0x30add0u,Route::clear_left},
        std::pair{0x30addcu,Route::store_right}};
    constexpr std::array after_axis{Route::adjust_axis,Route::read_lower,Route::spill_lower,
        Route::read_upper,Route::read_start};
    constexpr std::array after_offsets{Route::read_left,Route::compare};
    unsigned compared{};
    for(const auto p:{0u,2u,3u})for(unsigned rc=0;rc<4u;++rc) {
        Fixture f;f.precision=p;f.rounding=rc;
        compare<Offsets, Shared>(f,Offsets?Route::compare:Route::offsets);++compared;
        const auto stores=Offsets?std::span<const std::pair<std::uint32_t,Route>>(offset_stores).subspan(Shared ? 1u : 0u):
            std::span<const std::pair<std::uint32_t,Route>>(axis_stores);
        for(std::size_t i=0;i<stores.size();++i) {
            const auto [pc,next]=stores[i];
            auto denied=f;denied.deny_store=pc;
            const bool rollback=stage::reference_axis_plane_provider();
            const auto advanced=rollback?next:(Offsets?Route::compare:Route::offsets);
            const auto traced=rollback?next:(Offsets?after_offsets[i+unsigned(Shared)]:after_axis[i]);
            compare<Offsets, Shared>(denied,advanced);++compared;
            denied.move_frame=true;
            compare<Offsets, Shared>(denied,advanced);++compared;
            denied.move_frame=false;
            denied.callback_activates_trace=true;
            compare<Offsets, Shared>(denied,traced);++compared;
            denied.fault_pc=pc;
            compare<Offsets, Shared>(denied,next);++compared;
        }
        auto observed=f;observed.observer=true;observed.observer_fault=true;
        compare<Offsets, Shared>(observed,Route::original);++compared;
        auto device=f;device.device_program=true;
        if constexpr(Shared) {
            device.stack_base=base+0xffeu-0x24u;
            device.fault_pc=0x30add8u;
            compare<Offsets, Shared>(device,Route::read_left);
        } else {
            device.fault_pc=Offsets?0x30adc9u:0x30ad9fu;
            compare<Offsets, Shared>(device,Route::original);
        }
        ++compared;
        auto traced=f;traced.trace=true;
        compare<Offsets, Shared>(traced,Route::original);++compared;
        if constexpr(!Offsets) {
            auto alias=f;alias.ray=stack+0x14u-8u;
            compare<Offsets, Shared>(alias,Route::offsets);++compared;
            for(const bool second:{false,true}) {
                auto crossing=f;crossing.ray=base+0xffeu-8u-(second?0x10u:0u);
                crossing.fault_pc=second?0x30adc5u:0x30adc2u;
                compare<Offsets, Shared>(crossing,second?Route::read_end:Route::read_start);++compared;
            }
            f.depth=7;
            compare<Offsets, Shared>(f,Route::original);++compared;
        }
    }
    return compared;
}
}

int main() try {
    std::cout<<"30AD9B/30ADC9/30ADD8 axis plane: "<<prove<false>()+prove<true>()+prove<true, true>()
             <<" exact continuation and provider comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}

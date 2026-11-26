#include "physics/runtime/native_tu15_mopp_ray_wide_plane.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage = native_mopp_ray_wide_plane_detail;
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
void retained(WorldState &s,Route route) {
    switch(route) {case Route::original: goto L_original; case Route::save_low: goto L_save_low; case Route::read_left_code: goto L_read_left_code; case Route::read_lower: goto L_read_lower; case Route::save_high: goto L_save_high; case Route::read_right_code: goto L_read_right_code; case Route::read_upper: goto L_read_upper; case Route::save_axis: goto L_save_axis; case Route::advance_program: goto L_advance_program; case Route::read_start: goto L_read_start; case Route::read_end: goto L_read_end; case Route::read_left_high: goto L_read_left_high; case Route::save_left: goto L_save_left; case Route::read_right_high: goto L_read_right_high; case Route::offsets:return;}
L_original: s.instruction_literal(0x30ad4au); s.set(1,r(s.memory,(0x1u+s.gp[6]),1),4,0);
s.instruction_literal(0x30ad4eu); s.set(2,r(s.memory,(0x2u+s.gp[6]),1),4,0);
L_save_low: s.instruction_literal(0x30ad52u); w(s.memory,s.observe,0x30ad52u,(0x14u+s.gp[4]),4,s.reg(1,4,0));
L_read_left_code: s.instruction_literal(0x30ad56u); s.set(1,r(s.memory,(0x4u+s.gp[6]),1),4,0);
L_read_lower: s.instruction_literal(0x30ad5au); s.fp.push(static_cast<long double>(si(r(s.memory,(0x14u+s.gp[4]),4))));
s.instruction_literal(0x30ad5eu); s.set(0,s.flags.plus(s.reg(0,4,0),0xffffffddu,4),4,0);
L_save_high: s.instruction_literal(0x30ad61u); w(s.memory,s.observe,0x30ad61u,(0x14u+s.gp[4]),4,s.reg(2,4,0));
L_read_right_code: s.instruction_literal(0x30ad65u); s.set(2,r(s.memory,(0x6u+s.gp[6]),1),4,0);
s.instruction_literal(0x30ad69u); w(s.memory,s.observe,0x30ad69u,(0x20u+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
L_read_upper: s.instruction_literal(0x30ad6du); s.fp.push(static_cast<long double>(si(r(s.memory,(0x14u+s.gp[4]),4))));
L_save_axis: s.instruction_literal(0x30ad71u); w(s.memory,s.observe,0x30ad71u,(0x3cu+s.gp[4]),4,s.reg(0,4,0));
L_advance_program: s.instruction_literal(0x30ad75u); s.set(6,s.flags.plus(s.reg(6,4,0),0x7u,4),4,0);
s.instruction_literal(0x30ad78u); w(s.memory,s.observe,0x30ad78u,(0x1cu+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
L_read_start: s.instruction_literal(0x30ad7cu); s.fp.push(ext(f(r(s.memory,(0x0u+s.gp[7]+s.gp[0]*4u),4))));
L_read_end: s.instruction_literal(0x30ad7fu); s.fp.push(ext(f(r(s.memory,(0x10u+s.gp[7]+s.gp[0]*4u),4))));
L_read_left_high: s.instruction_literal(0x30ad83u); s.set(0,r(s.memory,(0xfffffffcu+s.gp[6]),1),4,0);
s.instruction_literal(0x30ad87u); s.set(0,s.flags.shift(s.reg(0,4,0),0x8u,4),4,0);
s.instruction_literal(0x30ad8au); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(1,4,0),4),4,0);
L_save_left: s.instruction_literal(0x30ad8cu); w(s.memory,s.observe,0x30ad8cu,(0x24u+s.gp[4]),4,s.reg(0,4,0));
L_read_right_high: s.instruction_literal(0x30ad90u); s.set(0,r(s.memory,(0xfffffffeu+s.gp[6]),1),4,0);
s.instruction_literal(0x30ad94u); s.set(0,s.flags.shift(s.reg(0,4,0),0x8u,4),4,0);
s.instruction_literal(0x30ad97u); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(2,4,0),4),4,0);
s.instruction_literal(0x30ad99u);
}

struct Fixture {
    std::uint32_t ray{rays}, program_base{program}, deny_store{}, fault_pc{};
    unsigned precision{3},rounding{},depth{3};
    bool trace{},observer{},observer_fault{},callback_activates_trace{},device_program{};
    bool move_frame{},device_after_store{};
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
        const std::array<std::byte,7> code{std::byte{0x25},std::byte{255},std::byte{129},std::byte{64},
            std::byte{89},std::byte{250},std::byte{201}};
        std::memcpy(r.data.data()+f.program_base-base,code.data(),code.size());
        const std::uint32_t start=0x80000000u,end=0x7fc32012u;
        std::memcpy(r.data.data()+f.ray-base+8u,&start,4u);
        std::memcpy(r.data.data()+f.ray-base+0x18u,&end,4u);
    }
    void access(std::uint32_t address,std::size_t width,bool write)const {
        r.accesses.push_back({r.pc,address,width,write});
        r.states.push_back(snapshot());
        if(r.pc==f.fault_pc)throw std::runtime_error("Injected wide-plane provider fault");
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

Result run(const Fixture &f,bool native) {
    Result r;r.accesses.reserve(24);
    r.pc=0x30ad48u;
    Bank bank{f,r};
    SourcePhysicsWorldRuntimeServices services;
    auto trace=f.trace;
    services.instruction=[&](std::uint32_t pc){r.pc=pc;r.instructions.push_back(pc);};
    services.instruction_cursor={&r.pc,&trace,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    SourcePhysicsContactRecordObserver observe;
    if(f.observer)observe=[&](const auto &event) {
        r.observed.push_back(event);r.states.push_back(bank.snapshot());
        if(f.observer_fault)throw std::runtime_error("Injected wide-plane observer fault");
    };
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={0x25u,2u,1u,3u,stack,0x2020u,f.program_base,f.ray};
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
            const auto moved_program=base+0x1600u;
            constexpr std::array<std::byte,7> bytes{std::byte{0x25},std::byte{1},
                std::byte{2},std::byte{8},std::byte{32},std::byte{64},std::byte{128}};
            std::memcpy(r.data.data()+moved_program-base,bytes.data(),bytes.size());
            state.gp[6]=moved_program+(r.pc==0x30ad78u||r.pc==0x30ad8cu?7u:0u);
            state.gp[4]+=0x100u;
            if(r.pc==0x30ad61u||r.pc==0x30ad71u)state.fp.st(0)=-19.125L;
        }
        if(f.device_after_store)
            (*bank.index.blocks[base>>22u])[((base>>12u)&1023u)+1u]={};
        if(f.move_frame||f.callback_activates_trace) {
            const std::uint32_t replacement=7u;
            std::memcpy(r.data.data()+state.gp[4]-base+0x14u,&replacement,4u);
        }
    };
    try {
        if(native)r.stage=stage::wide_plane(state,native_single_spill);
        retained(state,r.stage);
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
void compare(const Fixture &f,Route route) {
    if(stage::reference_wide_plane())route=Route::original;
    const auto a=run(f,false),b=run(f,true);
    if((b.fault.empty()&&b.stage!=route)||a.pc!=b.pc||a.fault!=b.fault||a.data!=b.data||a.accesses!=b.accesses||
       a.observed!=b.observed||a.instructions!=b.instructions||!same(a.abi,b.abi)||a.states.size()!=b.states.size()) {
        std::cerr<<"wide plane deny/fault "<<std::hex<<f.deny_store<<'/'<<f.fault_pc
                 <<" end "<<a.pc<<'/'<<b.pc<<" route "<<int(route)<<'/'<<int(b.stage)<<'\n';
        throw std::runtime_error("Wide plane continuation, provider or ABI differs");
    }
    for(unsigned i=0;i<a.states.size();++i)
        if(!same(a.states[i],b.states[i]))throw std::runtime_error("Wide plane provider saw a different live x87 frame");
}

unsigned prove() {
    constexpr std::array stores{std::pair{0x30ad52u,Route::save_low},std::pair{0x30ad61u,Route::save_high},
        std::pair{0x30ad69u,Route::read_upper},std::pair{0x30ad71u,Route::save_axis},
        std::pair{0x30ad78u,Route::read_start},std::pair{0x30ad8cu,Route::save_left}};
    constexpr std::array after_stores{Route::read_left_code,Route::read_right_code,
        Route::read_upper,Route::advance_program,Route::read_start,Route::read_right_high};
    unsigned compared{};
    for(const auto p:{0u,2u,3u})for(unsigned rc=0;rc<4u;++rc) {
        Fixture f;f.precision=p;f.rounding=rc;
        compare(f,Route::offsets);++compared;
        for(std::size_t i=0;i<stores.size();++i) {
            const auto [pc,next]=stores[i];
            auto denied=f;denied.deny_store=pc;
            const bool rollback=stage::reference_wide_plane_provider();
            compare(denied,rollback?next:Route::offsets);++compared;
            denied.move_frame=true;
            compare(denied,rollback?next:Route::offsets);++compared;
            denied.move_frame=false;
            denied.callback_activates_trace=true;
            compare(denied,rollback?next:after_stores[i]);++compared;
            denied.fault_pc=pc;
            compare(denied,next);++compared;
        }
        auto remapped=f;remapped.deny_store=0x30ad61u;remapped.device_after_store=true;
        remapped.fault_pc=0x30ad65u;
        compare(remapped,Route::read_right_code);++compared;
        auto observed=f;observed.observer=true;observed.observer_fault=true;
        compare(observed,Route::original);++compared;
        auto device=f;device.device_program=true;device.fault_pc=0x30ad4eu;
        compare(device,Route::original);++compared;
        auto traced=f;traced.trace=true;
        compare(traced,Route::original);++compared;
        auto alias=f;alias.ray=stack+0x14u-8u;
        compare(alias,Route::offsets);++compared;
        for(const auto pc:{0x14u,0x24u}) {
            auto coded=f;coded.program_base=stack+pc-(pc==0x14u?4u:5u);
            compare(coded,Route::offsets);++compared;
        }
        for(const bool second:{false,true}) {
            auto crossing=f;crossing.ray=base+0xffeu-8u-(second?0x10u:0u);
            crossing.fault_pc=second?0x30ad7fu:0x30ad7cu;
            compare(crossing,second?Route::read_end:Route::read_start);++compared;
        }
        f.depth=7;
        compare(f,Route::original);++compared;
    }
    return compared;
}
}

int main() try {
    std::cout<<"30AD4A wide plane: "<<prove()<<" exact continuation and provider comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}

#include "physics/runtime/native_tu15_mopp_ray_geometry_prep.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage=native_mopp_ray_geometry_prep_detail;
using Route=stage::Continuation;
constexpr std::uint32_t base=0x62000000u, stack=base+0x180u, rays=base+0x500u;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
enum class Exit { decreasing, numeric };

struct Fixture {
    std::uint32_t sp=stack, ray=rays, end=0x3f800000u, start=0x40000000u, bound=0xc1800000u;
    std::uint32_t deny{}, fault_pc{}, reenter_pc{};
    unsigned depth=3, precision=3, rounding{}, fault_half=1;
    long double older=3.0L;
    bool observer{}, observer_fault{}, trace{}, authentic=true, detach_stack{}, move_frame{}, drop_lanes{};
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
    Route route=Route::original;
    Exit exit=Exit::numeric;
    std::string fault;
};

struct Bank final : SourcePhysicsContactMemory {
    const Fixture &f;
    Result &r;
    SourceDirectPageIndex index;
    std::function<SourcePhysicsWorldRuntimeAbi()> snapshot;
    std::function<void()> on_write;
    unsigned faults{};

    template<class T> void put(std::uint32_t address,const T &value) const {
        std::memcpy(r.data.data()+address-base,&value,sizeof(value));
    }
    Bank(const Fixture &fixture,Result &result):f(fixture),r(result) {
        direct_pages_=&index;
        auto &block=index.blocks[base>>22u];
        block=std::make_unique<SourceDirectPageIndex::Block>();
        (*block)[(base>>12u)&1023u]={r.data.data(),true};
        (*block)[((base>>12u)&1023u)+1u]={r.data.data()+4096u,true};
        if(f.detach_stack)(*block)[(f.sp>>12u)&1023u]={};
        put(f.ray,std::array{0xc0800000u,0x41900000u,0x7fc42017u,0x80000000u});
        put(f.ray+0x10u,std::array{0x00000001u,0x41400000u,0x7f813403u,0u});
        put(f.sp+0x18u,f.start);put(f.sp+0x1cu,f.bound);put(f.sp+0x20u,f.end);
    }
    void access(std::uint32_t address,std::size_t width,bool write)const {
        r.accesses.push_back({r.pc,address,width,write});r.states.push_back(snapshot());
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        access(address,out.size(),false);
        if(r.pc==f.fault_pc)throw std::runtime_error("Injected MOPP geometry read fault");
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
        if(r.pc==f.fault_pc&&++faults==f.fault_half)throw std::runtime_error("Injected MOPP geometry store fault");
        std::memcpy(r.data.data()+address-base,in.data(),in.size());
        if(r.pc==f.reenter_pc)on_write();
    }
};

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
Exit exact_geometry(WorldState &s,Route route) {
    switch(route) { case Route::original: goto L30ae75; case Route::read_first_vector: goto L_read_first_vector; case Route::read_second_vector: goto L_read_second_vector; case Route::store_second_vector: goto L_store_second_vector; case Route::spill_first: goto L_spill_first; case Route::subtract_first: goto L_subtract_first; case Route::load_second: goto L_load_second; case Route::load_third: goto L_load_third; case Route::load_comparison: goto L_load_comparison;case Route::decreasing: return Exit::decreasing; case Route::numeric: return Exit::numeric; }
L30ae75: s.instruction_literal(0x30ae75u); s.fp.push(ext(f(r(s.memory,(0x20u+s.gp[4]),4))));
L_read_first_vector: s.instruction_literal(0x30ae79u); s.xmm[1]=v(s.memory,(0x0u+s.gp[7]));
s.instruction_literal(0x30ae7cu); halo2::native_mopp_ray_geometry_prep_detail::reverse_subtract(s);
L_read_second_vector: s.instruction_literal(0x30ae7eu); s.xmm[2]=v(s.memory,(0x10u+s.gp[7]));
s.instruction_literal(0x30ae82u); vw(s.memory,s.observe,0x30ae82u,(0x80u+s.gp[4]),s.xmm[1]);
L_store_second_vector: s.instruction_literal(0x30ae8au); vw(s.memory,s.observe,0x30ae8au,(0x90u+s.gp[4]),s.xmm[2]);
L_spill_first: s.instruction_literal(0x30ae92u); w(s.memory,s.observe,0x30ae92u,(0x34u+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
L_subtract_first: s.instruction_literal(0x30ae96u); halo2::native_mopp_ray_slab_bounds_detail::wide_arithmetic<false>(s,r(s.memory,(0x1cu+s.gp[4]),4));
s.instruction_literal(0x30ae9au); w(s.memory,s.observe,0x30ae9au,(0x2cu+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
L_load_second: s.instruction_literal(0x30ae9eu); s.fp.push(ext(f(r(s.memory,(0x18u+s.gp[4]),4))));
s.instruction_literal(0x30aea2u); halo2::native_mopp_ray_slab_bounds_detail::wide_arithmetic<false>(s,r(s.memory,(0x20u+s.gp[4]),4));
s.instruction_literal(0x30aea6u); w(s.memory,s.observe,0x30aea6u,(0x30u+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
L_load_third: s.instruction_literal(0x30aeaau); s.fp.push(ext(f(r(s.memory,(0x18u+s.gp[4]),4))));
s.instruction_literal(0x30aeaeu); halo2::native_mopp_ray_slab_bounds_detail::wide_arithmetic<false>(s,r(s.memory,(0x1cu+s.gp[4]),4));
s.instruction_literal(0x30aeb2u); w(s.memory,s.observe,0x30aeb2u,(0x18u+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
L_load_comparison: s.instruction_literal(0x30aeb6u); s.fp.push(ext(f(r(s.memory,(0x34u+s.gp[4]),4))));
s.instruction_literal(0x30aebau); s.fp.comparison(ext(f(r(s.memory,(0x30u+s.gp[4]),4))),1);
s.instruction_literal(0x30aebeu); s.set(0,s.fp.word(),2,0);
s.instruction_literal(0x30aec0u); s.flags.logic((s.reg(0,1,8))&(0x5u),1);
s.instruction_literal(0x30aec3u); if(s.flags.pf)goto L30b022;
return Exit::numeric;
L30b022: return Exit::decreasing;
}


Result run(const Fixture &f,bool native) {
    Result r;r.pc=0x30ae73u;r.accesses.reserve(30);Bank bank{f,r};
    SourcePhysicsWorldRuntimeServices services;
    auto trace=f.trace;
    services.instruction=[&](std::uint32_t pc){r.pc=pc;r.instructions.push_back(pc);};
    services.instruction_cursor={&r.pc,&trace,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    if(!f.authentic)services.instruction_cursor={};
    SourcePhysicsContactRecordObserver observe;
    auto on_observe=[&](const auto &event) {
        r.observed.push_back(event);r.states.push_back(bank.snapshot());
        if(f.observer_fault)throw std::runtime_error("Injected MOPP geometry observer fault");
    };
    if(f.observer)observe=on_observe;
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={0x24310000u,2u,1u,0x7100u,f.sp,0x2020u,0x7700u,f.ray};
    input.eflags=0x256u;input.x87_depth=f.depth;input.x87_status=0x3200u;
    input.x87[0]=f.older;
    for(unsigned i=1;i<f.depth;++i)input.x87[i]=0.125L*static_cast<long double>(i)+0.25L;
    const auto cw=std::uint16_t(0x7fu|(f.precision<<8u)|(f.rounding<<10u));
    input.floating_point=SourcePhysicsFactoryFloatingPoint{cw,0x1f80u};
    asm volatile("fnclex; fldcw %0"::"m"(cw));
    WorldState state{bank,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
    bank.snapshot=[&]{return state.save();};
    bank.on_write=[&] {
        if(f.drop_lanes) {
            while(state.fp.count>1u)state.fp.pop();
        } else if(f.move_frame) {
            const auto frame=state.gp[4]-base;
            std::memcpy(r.data.data()+frame+0x100u,r.data.data()+frame,0xa0u);
            state.gp[4]+=0x100u;
            bank.put(state.gp[4]+0x1cu,0x3f900000u);
        } else {
            trace=true;
            observe=on_observe;
            bank.put(state.gp[4]+0x1cu,0x7f804217u);
        }
    };
    try {
        if(native)r.route=stage::geometry_prep(state,native_single_spill);
        r.exit=exact_geometry(state,r.route);
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
    if(stage::reference_geometry_prep())expected=Route::original;
    const auto a=run(f,false),b=run(f,true);
    if(a.pc!=b.pc||a.fault!=b.fault||a.exit!=b.exit||a.data!=b.data||a.accesses!=b.accesses||
       a.observed!=b.observed||a.instructions!=b.instructions||!same(a.abi,b.abi)||
       a.states.size()!=b.states.size()||b.route!=expected) {
        std::cerr<<"Geometry prep fault "<<std::hex<<f.fault_pc<<'/'<<f.deny<<" PC "<<a.pc<<'/'<<b.pc
                 <<" route "<<int(expected)<<'/'<<int(b.route)<<" state "<<a.abi.x87_status<<'/'<<b.abi.x87_status
                 <<" fault "<<a.fault<<'/'<<b.fault<<'\n';
        throw std::runtime_error("MOPP geometry prep continuation, provider or ABI differs");
    }
    for(unsigned i=0;i<a.states.size();++i)
        if(!same(a.states[i],b.states[i]))throw std::runtime_error("Provider saw a different MOPP geometry frame");
}

unsigned prove() {
    unsigned compared{};
    auto check=[&](const Fixture &f,Route expected){compare(f,expected);++compared;};
    static constexpr std::array scalar_stores{
        std::pair{0x30ae92u,Route::subtract_first},std::pair{0x30ae9au,Route::load_second},
        std::pair{0x30aea6u,Route::load_third},std::pair{0x30aeb2u,Route::load_comparison}};
    static constexpr std::array vector_stores{
        std::pair{0x30ae82u,Route::store_second_vector},std::pair{0x30ae8au,Route::spill_first}};
    for(const auto precision:{0u,2u,3u})for(unsigned rc=0;rc<4u;++rc) {
        Fixture f;f.precision=precision;f.rounding=rc;
        check(f,Route::decreasing);
        auto second=f;second.older=0.25L;check(second,Route::numeric);
        auto narrow=f;narrow.depth=1;check(narrow,Route::decreasing);
        for(const auto bits:{0u,0x80000000u,0x7fc12345u,0x7f821242u,1u,0x80000001u,0x7f800000u,0xff800000u}) {
            auto edge=f;edge.end=bits;check(edge,Route::decreasing);
            edge=f;edge.start=bits;check(edge,bits==0x7f800000u?Route::numeric:Route::decreasing);
        }
        for(const auto &[pc,next]:scalar_stores) {
            auto denied=f;denied.deny=pc;
            const bool rollback=stage::reference_geometry_prep_provider();
            check(denied,rollback?next:Route::decreasing);
            denied.reenter_pc=pc;check(denied,next);
            denied.move_frame=true;
            const auto expected=run(denied,false).exit==Exit::decreasing?Route::decreasing:Route::numeric;
            check(denied,rollback?next:expected);
            denied.move_frame=false;
            denied.fault_pc=pc;check(denied,Route::original);
        }
        for(const auto &[pc,next]:vector_stores) {
            for(const auto half:{1u,2u}) {
                auto fault=f;fault.fault_pc=pc;fault.fault_half=half;
                check(fault,Route::original);
            }
            auto dynamic=f;dynamic.reenter_pc=pc;check(dynamic,next);
            dynamic.observer_fault=true;check(dynamic,Route::original);
        }
        auto observed=f;observed.observer=true;observed.observer_fault=true;check(observed,Route::original);
        auto traced=f;traced.trace=true;check(traced,Route::original);
        auto weak=f;weak.authentic=false;check(weak,Route::original);
        auto full=f;full.depth=8;check(full,Route::original);
        auto short_stack=f;short_stack.reenter_pc=0x30ae8au;short_stack.drop_lanes=true;
        check(short_stack,Route::subtract_first);
        auto detached=f;detached.detach_stack=true;detached.fault_pc=0x30ae75u;check(detached,Route::original);
        auto crossed=f;crossed.ray=base+0xff8u;check(crossed,Route::read_first_vector);
        crossed=f;crossed.ray=base+0xfe8u;check(crossed,Route::read_second_vector);
        crossed=f;crossed.sp=base+0xfe2u;check(crossed,Route::subtract_first);
        crossed.fault_pc=0x30ae96u;check(crossed,Route::subtract_first);
        auto alias=f;alias.ray=stack+0x78u;check(alias,Route::decreasing);
    }
    return compared;
}
}
int main() try {
    std::cout<<"30AE75 geometry prep: "<<prove()<<" exact continuation and provider comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}

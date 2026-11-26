#include "physics/runtime/native_tu15_mopp_ray_slab_interval.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage = native_mopp_ray_slab_interval_detail;
using Route = stage::Continuation;
constexpr std::uint32_t base=0x62000000u;
constexpr std::array services_sites{0x38846eu, 0x21f590u, 0x33b595u};

template<bool Ascending>
Route retained(WorldState &s) {
    const auto sp=s.gp[4];
    s.instruction_literal(Ascending?0x30b23du:0x30b217u);
    s.fp.push(ext(f(r(s.memory,sp+(Ascending?0x24u:0x18u)))));
    s.instruction_literal(Ascending?0x30b241u:0x30b21bu);
    s.fp.comparison(s.fp.st(2),1);
    s.instruction_literal(Ascending?0x30b243u:0x30b21du);
    s.set(0,s.fp.word(),2,0);
    s.instruction_literal(Ascending?0x30b245u:0x30b21fu);
    s.flags.logic(s.reg(0,1,8)&5u,1);
    s.instruction_literal(Ascending?0x30b248u:0x30b222u);
    if(!s.flags.pf)return Route::reject;
    s.instruction_literal(Ascending?0x30b24eu:0x30b228u);
    s.fp.push(ext(f(r(s.memory,sp+(Ascending?0x18u:0x24u)))));
    s.instruction_literal(Ascending?0x30b252u:0x30b22cu);
    s.fp.comparison(s.fp.st(1),1);
    s.instruction_literal(Ascending?0x30b254u:0x30b22eu);
    s.set(0,s.fp.word(),2,0);
    s.instruction_literal(Ascending?0x30b256u:0x30b230u);
    s.flags.logic(s.reg(0,1,8)&0x41u,1);
    s.instruction_literal(Ascending?0x30b259u:0x30b233u);
    if(s.flags.zf)return Route::reject;
    if constexpr(Ascending) {
        s.instruction_literal(0x30b25fu);
        s.gp[1]=1u;
    } else {
        s.instruction_literal(0x30b239u);
        s.gp[1]=s.flags.logic(0u,4);
        s.instruction_literal(0x30b23bu);
    }
    return Route::accept;
}

struct Fixture {
    std::uint32_t sp=base+0x120u, fault_pc{};
    long double finish=4.0L,origin=0.5L;
    unsigned depth{3},rounding{},precision{3};
    bool active_trace{},observer{};
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> provider;
    std::vector<std::uint32_t> instructions;
    std::uint32_t pc{0x30b216u};
    Route admitted{Route::original},exit{Route::original};
    std::string fault;
};
struct Bank final : SourcePhysicsContactMemory {
    std::array<std::byte,8192> data{};
    SourceDirectPageIndex index;
    const Fixture &fixture;
    Result &result;

    Bank(const Fixture &input,Result &output, std::uint32_t first,std::uint32_t second)
        :fixture(input),result(output) {
        direct_pages_=&index;
        auto &block=index.blocks[base>>22u];
        block=std::make_unique<SourceDirectPageIndex::Block>();
        for(unsigned i=0;i<2;++i)
            (*block)[((base>>12u)&1023u)+i]={data.data()+i*4096u,true};
        const auto first_bits=std::bit_cast<std::uint32_t>(1.0f);
        const auto second_bits=std::bit_cast<std::uint32_t>(3.0f);
        std::memcpy(data.data()+fixture.sp-base+first,&first_bits,4u);
        std::memcpy(data.data()+fixture.sp-base+second,&second_bits,4u);
    }
    void read(std::uint32_t address,std::span<std::byte> output)const override {
        if(output.size()!=4u)throw std::runtime_error("Unexpected interval read width");
        result.provider.emplace_back(result.pc,address);
        if(result.pc==fixture.fault_pc)throw std::runtime_error("Injected slab interval provider fault");
        std::memcpy(output.data(),data.data()+address-base,4u);
    }
    void write(std::uint32_t,std::span<const std::byte>)override {
        throw std::logic_error("Read-only interval wrote to guest memory");
    }
};

template<bool Ascending>
Result run(const Fixture &fixture,bool native) {
    Result result;
    Bank memory{fixture,result,Ascending?0x24u:0x18u,Ascending?0x18u:0x24u};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){result.pc=pc;result.instructions.push_back(pc);};
    services.instruction_cursor={&result.pc,&fixture.active_trace,nullptr,services_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    SourcePhysicsContactRecordObserver observe;
    if(fixture.observer)observe=[](const auto &){throw std::logic_error("Read-only interval notified a store observer");};
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={0xf114a239u,93u,2u,3u,fixture.sp,12u,9u,7u};
    input.eflags=0x2d7u;
    input.x87_status=0x3200u;
    input.x87_depth=fixture.depth;
    input.x87[0]=fixture.finish;
    input.x87[1]=fixture.origin;
    input.x87[2]=-1.25L;
    const auto cw=std::uint16_t(0x7fu|fixture.precision<<8u|fixture.rounding<<10u);
    input.floating_point=SourcePhysicsFactoryFloatingPoint{cw,0x1f80u};
    asm volatile("fnclex; fldcw %0"::"m"(cw));
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(input);
    try {
        if(native)result.admitted=stage::slab_interval<Ascending>(state);
        result.exit=result.admitted==Route::original?retained<Ascending>(state):result.admitted;
    } catch(const std::runtime_error &error) {
        result.fault=error.what();
    }
    result.abi=state.save();
    return result;
}

template<bool Ascending>
void compare(const Fixture &f,Route path) {
    const auto old=run<Ascending>(f,false),native=run<Ascending>(f,true);
    const bool fallback=stage::reference_slab_interval();
    const auto &a=old.abi,&b=native.abi;
    if(native.admitted!=(fallback?Route::original:path)||native.exit!=old.exit||native.pc!=old.pc||
        native.provider!=old.provider||native.instructions!=old.instructions||native.fault!=old.fault||
        a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_depth!=b.x87_depth||a.x87_status!=b.x87_status||
        a.floating_point->x87_control_word!=b.floating_point->x87_control_word||
        a.floating_point->mxcsr!=b.floating_point->mxcsr)
        throw std::runtime_error("Slab interval route, ABI or provider behavior differs");
    for(unsigned n=0;n<a.x87_depth;++n)
        if(std::memcmp(&a.x87[n],&b.x87[n],10u))
            throw std::runtime_error("Slab interval modified a live x87 lane");
}

template<bool Ascending>
unsigned cases() {
    unsigned compared{};
    for(const auto precision:{0u,2u,3u})for(unsigned rc=0;rc<4u;++rc) {
        Fixture input;input.precision=precision;input.rounding=rc;
        compare<Ascending>(input,Route::accept);++compared;
        auto first=input;first.origin=2.0L;
        compare<Ascending>(first,Route::reject);++compared;
        auto second=input;second.finish=2.0L;
        compare<Ascending>(second,Route::reject);++compared;
        for(const auto offset:{0x18u,0x24u}) {
            auto crossing=input;crossing.sp=base+0xffeu-offset;
            compare<Ascending>(crossing,Route::original);++compared;
            crossing.fault_pc=offset==(Ascending?0x24u:0x18u)?(Ascending?0x30b23du:0x30b217u):
                (Ascending?0x30b24eu:0x30b228u);
            compare<Ascending>(crossing,Route::original);++compared;
        }
        auto full=input;full.depth=8;
        compare<Ascending>(full,Route::original);++compared;
        auto traced=input;traced.active_trace=true;
        compare<Ascending>(traced,Route::original);++compared;
        auto observed=input;observed.observer=true;
        compare<Ascending>(observed,Route::original);++compared;
    }
    return compared;
}
}

int main() try {
    std::cout<<"30B217/30B23D slab interval: "<<cases<true>()+cases<false>()
             <<" exact continuation and provider comparisons\n";
    return 0;
} catch(const std::exception &error) {
    std::cerr<<error.what()<<'\n';
    return 1;
}

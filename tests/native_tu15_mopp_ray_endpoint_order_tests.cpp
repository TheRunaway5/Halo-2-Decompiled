#include "physics/runtime/native_tu15_mopp_ray_endpoint_order.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage = native_mopp_ray_endpoint_order_detail;
using Route = stage::Continuation;
constexpr std::uint32_t base=0x62000000u,stack=base+0x280u,ray=base+0x400u;
constexpr std::array services_sites{0x38846eu,0x21f590u,0x33b595u};

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

Route retained(WorldState &s,Route route,Route *stack_admission) {
    switch(route) { case Route::original: goto L_original; case Route::read_end: goto L_read_end; case Route::read_scratch: goto L_read_scratch; case Route::ascending: case Route::descending: return route; }
L_original: s.instruction_literal(0x30b1f9u); s.fp.push(ext(f(r(s.memory,(0x0u+s.gp[7]+s.gp[0]*4u),4))));
s.instruction_literal(0x30b1fcu); w(s.memory,s.observe,0x30b1fcu,(0x24u+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
L_read_end: s.instruction_literal(0x30b200u); s.fp.push(ext(f(r(s.memory,(0x10u+s.gp[7]+s.gp[0]*4u),4))));
s.instruction_literal(0x30b204u); w(s.memory,s.observe,0x30b204u,(0x18u+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
L_read_scratch:
    if(stack_admission) {
        *stack_admission=stage::compare_endpoints(s);
        if(*stack_admission!=Route::read_scratch)return *stack_admission;
    }
    s.instruction_literal(0x30b208u); s.fp.push(ext(f(r(s.memory,(0x24u+s.gp[4]),4))));
s.instruction_literal(0x30b20cu); s.fp.comparison(ext(f(r(s.memory,(0x18u+s.gp[4]),4))),1);
s.instruction_literal(0x30b210u); s.set(0,s.fp.word(),2,0);
s.instruction_literal(0x30b212u); s.flags.logic((s.reg(0,1,8))&(0x5u),1);
s.instruction_literal(0x30b215u); if(s.flags.pf)return Route::ascending;
return Route::descending;
}

struct Fixture {
    std::uint32_t start=0x3f950000u,end=0xbf238000u,stack_base=stack,ray_base=ray;
    std::uint32_t deny_store{}, fault_pc{};
    unsigned rounding{},precision{3},depth{3};
    bool active_trace{},observer{},observer_fault{},ambient_c1{};
    bool borrowed_stack{},trace_after_store{},move_frame{};
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
    std::vector<SourcePhysicsWorldRuntimeAbi> callbacks;
    std::string fault;
    std::uint32_t pc{0x30b1f5u};
    Route admission{Route::original},stack_admission{Route::read_scratch},exit{Route::original};
};
struct Bank final : SourcePhysicsContactMemory {
    const Fixture &fixture;
    Result &result;
    SourceDirectPageIndex index;
    std::function<SourcePhysicsWorldRuntimeAbi()> snapshot;
    std::function<void()> after_write;

    Bank(const Fixture &f,Result &r):fixture(f),result(r) {
        direct_pages_=&index;
        auto &block=index.blocks[base>>22u];
        block=std::make_unique<SourceDirectPageIndex::Block>();
        for(unsigned i=0;i<2;++i)
            (*block)[((base>>12u)&1023u)+i]={r.bytes.data()+i*4096u,true};
        std::memcpy(r.bytes.data()+f.ray_base-base+8u,&f.start,4u);
        std::memcpy(r.bytes.data()+f.ray_base-base+0x18u,&f.end,4u);
    }
    void access(std::uint32_t address,std::size_t width,bool write)const {
        result.events.push_back({result.pc,address,width,write});
        result.callbacks.push_back(snapshot());
        if(result.pc==fixture.fault_pc)throw std::runtime_error("Injected endpoint provider fault");
    }
    void read(std::uint32_t address,std::span<std::byte> output)const override {
        if(!try_read_view(address,output.size()))access(address,output.size(),false);
        std::memcpy(output.data(),result.bytes.data()+address-base,output.size());
    }
    std::optional<std::span<const std::byte>> try_read_view(
        std::uint32_t address,std::size_t width)const noexcept override {
        const auto first=fixture.stack_base+0x18u;
        if(!fixture.borrowed_stack || address<first || std::uint64_t(address-first)+width>16u)
            return std::nullopt;
        return std::span<const std::byte>{result.bytes}.subspan(address-base,width);
    }
    bool try_callback_free_write(std::uint32_t address,std::span<const std::byte> data)noexcept override {
        if(fixture.observer||result.pc==fixture.deny_store||!direct_page(address,data.size()))return false;
        result.events.push_back({result.pc,address,data.size(),true});
        std::memcpy(result.bytes.data()+address-base,data.data(),data.size());
        return true;
    }
    void write(std::uint32_t address,std::span<const std::byte> data)override {
        if(try_callback_free_write(address,data))return;
        access(address,data.size(),true);
        std::memcpy(result.bytes.data()+address-base,data.data(),data.size());
        if(after_write)after_write();
    }
};

void host_c1(bool set) {
    std::array<std::uint32_t,7> env;
    asm volatile("fnstenv %0":"=m"(env));
    env[1]=(env[1]&~0x200u)|(set?0x200u:0);
    asm volatile("fldenv %0"::"m"(env));
}

Result run(const Fixture &fixture,bool native) {
    Result result;result.events.reserve(16);
    Bank bank{fixture,result};
    SourcePhysicsWorldRuntimeServices services;
    auto tracing=fixture.active_trace;
    services.instruction=[&](std::uint32_t pc){result.pc=pc;result.instructions.push_back(pc);};
    services.instruction_cursor={&result.pc,&tracing,nullptr,services_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    SourcePhysicsContactRecordObserver observer;
    if(fixture.observer)observer=[&](const auto &store) {
        result.observed.push_back(store);
        result.callbacks.push_back(bank.snapshot());
        if(fixture.observer_fault)throw std::runtime_error("Injected endpoint observer fault");
    };
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={2,93u,1u,0x2000u,fixture.stack_base,0x2020u,3u,fixture.ray_base};
    input.eflags=0x2d7u;input.x87_depth=fixture.depth;input.x87_status=0x3200u;
    input.x87[0]=7.1L;input.x87[1]=-1.25L;input.x87[2]=0.125L;
    const auto cw=std::uint16_t(0x7fu|fixture.precision<<8u|fixture.rounding<<10u);
    input.floating_point=SourcePhysicsFactoryFloatingPoint{cw,0x1f80u};
    asm volatile("fnclex; fldcw %0"::"m"(cw));
    WorldState state{bank,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
    bank.snapshot=[&]{return state.save();};
    bank.after_write=[&] {
        if(fixture.trace_after_store)tracing=true;
        if(fixture.move_frame) {
            state.gp[4]+=0x80u;
            const auto frame=state.gp[4]-base;
            const auto start=std::bit_cast<std::uint32_t>(-1.5f);
            const auto end=std::bit_cast<std::uint32_t>(+1.5f);
            std::memcpy(result.bytes.data()+frame+0x24u,&start,4u);
            std::memcpy(result.bytes.data()+frame+0x18u,&end,4u);
        }
    };
    host_c1(fixture.ambient_c1);
    try {
        if(native)result.admission=stage::endpoint_order(state,native_single_spill);
        result.exit=retained(state,result.admission,native?&result.stack_admission:nullptr);
    } catch(const std::runtime_error &error) {
        result.fault=error.what();
    }
    result.abi=state.save();
    return result;
}
bool same(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_status!=b.x87_status||a.x87_depth!=b.x87_depth||
       a.floating_point->x87_control_word!=b.floating_point->x87_control_word||
       a.floating_point->mxcsr!=b.floating_point->mxcsr)return false;
    for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10u))return false;
    return true;
}
void compare(Fixture fixture,Route route) {
    if(stage::reference_endpoint_order())route=Route::original;
    const auto a=run(fixture,false),b=run(fixture,true);
    const auto scratch=(fixture.stack_base+0x18u)&4095u;
    const bool reached_stack=route!=Route::ascending && route!=Route::descending && a.fault.empty();
    const bool ordinary_stack=scratch+16u<=4096u || fixture.borrowed_stack;
    const bool admit_stack=reached_stack && ordinary_stack && !fixture.observer && !fixture.active_trace &&
        !fixture.trace_after_store && !stage::reference_endpoint_order() && !stage::reference_endpoint_compare();
    const auto expected_stack=admit_stack?a.exit:Route::read_scratch;
    if((b.fault.empty()&&b.admission!=route)||a.exit!=b.exit||a.pc!=b.pc||a.events!=b.events||a.fault!=b.fault||
       a.observed!=b.observed||a.instructions!=b.instructions||a.bytes!=b.bytes||
       b.stack_admission!=expected_stack||!same(a.abi,b.abi)||a.callbacks.size()!=b.callbacks.size()) {
        std::cerr<<"endpoint fixture fault "<<std::hex<<fixture.fault_pc<<" and deny "<<fixture.deny_store
                 <<" pc "<<a.pc<<"/"<<b.pc<<" route "<<int(route)<<"/"<<int(b.admission)<<'\n';
        throw std::runtime_error("Endpoint state, store or retained continuation differs");
    }
    for(unsigned i=0;i<a.callbacks.size();++i)
        if(!same(a.callbacks[i],b.callbacks[i]))
            throw std::runtime_error("Endpoint provider observed a different live x87 or ABI state");
}
}

int main() try {
    unsigned compared{};
    for(const auto pc:{0u,2u,3u})for(unsigned rc=0;rc<4u;++rc) {
        Fixture f;f.precision=pc;f.rounding=rc;
        compare(f,Route::ascending);++compared;
        auto reverse=f;std::swap(reverse.start,reverse.end);
        compare(reverse,Route::descending);++compared;
        auto zeros=f;zeros.start=0;zeros.end=0x80000000u;
        compare(zeros,Route::ascending);++compared;
        auto alias=f;alias.ray_base=stack+0x0cu;
        compare(alias,Route::ascending);++compared;
        auto crossed=f;crossed.stack_base=base+0xfe4u;
        compare(crossed,Route::read_scratch);++compared;
        crossed.borrowed_stack=true;
        compare(crossed,Route::read_scratch);++compared;
        for(const auto special:{0x00000001u,0xff800000u,0x7fc21234u,0x7f812345u}) {
            auto first=f;first.start=special;
            compare(first,Route::original);++compared;
            auto second=f;second.end=special;
            compare(second,Route::read_end);++compared;
        }
        for(const auto deny:{0x30b1fcu,0x30b204u}) {
            auto cold=f;cold.deny_store=deny;
            const auto path=deny==0x30b1fcu?Route::original:Route::read_end;
            const bool rollback=stage::reference_endpoint_provider();
            const bool comparison_rollback=stage::reference_endpoint_compare();
            const auto admitted=rollback?path:comparison_rollback?Route::read_scratch:Route::ascending;
            compare(cold,admitted);++compared;
            cold.trace_after_store=true;
            const auto traced=deny==0x30b1fcu?Route::read_end:Route::read_scratch;
            compare(cold,rollback?path:traced);++compared;
            cold.trace_after_store=false;
            cold.move_frame=true;
            compare(cold,rollback||comparison_rollback?admitted:Route::descending);++compared;
            cold.move_frame=false;
            cold.fault_pc=deny;
            compare(cold,path);++compared;
        }
        for(const auto read:{0x30b1f9u,0x30b200u}) {
            auto device=f;device.ray_base=base+0xffeu-(read==0x30b1f9u?8u:0x18u);
            device.fault_pc=read;
            compare(device,read==0x30b1f9u?Route::original:Route::read_end);++compared;
        }
        auto observed=f;observed.observer=true;observed.observer_fault=true;
        compare(observed,Route::original);++compared;
        f.active_trace=true;
        compare(f,Route::original);++compared;
        f.active_trace=false;f.ambient_c1=true;
        compare(f,Route::original);++compared;
        f.ambient_c1=false;f.depth=8;
        compare(f,Route::original);++compared;
    }
    std::cout<<"30B1F9 endpoints: "<<compared<<" exact continuation, observer and fault comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}

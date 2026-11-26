#include "physics/runtime/native_tu15_mopp_ray_collector.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage=native_mopp_ray_collector_detail;
using Part=stage::Part;
using Route=stage::Continuation;
enum class Exit { normal, alternate };
constexpr std::uint32_t base=0x62000000u,sp=base+0x180u,cache=base+0x500u,vars=base+0x600u,
    collection=base+0x700u,shape=base+0x800u,parent=base+0x900u,destination=base+0xa00u,table=base+0xd00u;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
struct EarlyUnwind {};

struct Fixture {
    Part part=Part::select;
    std::uint32_t deny_load{},deny_store{},fault_pc{},parent_pointer=parent;
    bool trace{},authentic=true,observe{},observer_fault{},reenter{},filter{},direct{},sparse{};
    bool move_frame{},replace_target{},child_unwind{};
};
struct Access {
    std::uint32_t pc,address;
    std::size_t width;
    bool write;
    bool operator==(const Access &)const=default;
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;
    std::array<std::byte,4096> bytes{};
    std::vector<Access> accesses;
    std::vector<SourcePhysicsWorldRuntimeAbi> states;
    std::vector<SourcePhysicsContactRecordStore> observed;
    std::vector<std::uint32_t> instructions;
    std::vector<std::array<std::uint32_t,4>> calls;
    std::uint32_t pc{};
    Route route=Route::original;
    Exit exit=Exit::normal;
    bool native_dispatch{},early_unwind{};
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
        }
        put(sp+0x28u,cache);put(vars+0x18u,0xffffffffu);
        put(cache+0x44u,f.filter?base+0xb00u:0u);put(cache+0x64u,collection);
        put(collection,table);put(shape,table);
        put(table+0x34u,0x123456u);put(table+0x24u,0x198765u);
        put(cache+0x58u,f.direct?base+0xc00u:0u);
        put(cache+0x60u,f.parent_pointer);put(f.parent_pointer+8u,0xc0ffee01u);
        put(cache+0x5cu,destination);put(destination+4u,0x3f123456u);
    }
    std::optional<std::span<const std::byte>> try_read_view(
        std::uint32_t p,std::size_t n)const noexcept override {
        if(!f.sparse||p==f.deny_load||p<base||p-base>r.bytes.size()||n>r.bytes.size()-(p-base))
            return std::nullopt;
        return std::span<const std::byte>{r.bytes}.subspan(p-base,n);
    }
    void access(std::uint32_t p,std::size_t n,bool write)const {
        r.accesses.push_back({r.pc,p,n,write});r.states.push_back(snapshot());
        if(r.pc==f.fault_pc)throw std::runtime_error("Injected collector provider fault");
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        if(!try_read_view(p,out.size()))access(p,out.size(),false);
        std::memcpy(out.data(),r.bytes.data()+p-base,out.size());
    }
    bool try_callback_free_write(std::uint32_t p,std::span<const std::byte> in)noexcept override {
        if(f.observe||r.pc==f.deny_store)return false;
        r.accesses.push_back({r.pc,p,in.size(),true});
        std::memcpy(r.bytes.data()+p-base,in.data(),in.size());
        return true;
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        if(try_callback_free_write(p,in))return;
        access(p,in.size(),true);
        std::memcpy(r.bytes.data()+p-base,in.data(),in.size());
        if(f.reenter||f.move_frame||f.replace_target)reenter();
    }
};

template<Part part> Exit retained(WorldState &,Route);
template<> Exit retained<Part::select>(WorldState &s,Route route) {
    switch(route) { case Route::original: break; case Route::read_key: goto L_read_key; case Route::read_filter: goto L_read_filter; case Route::done: return Exit::normal; case Route::alternate: return Exit::alternate; default: throw std::logic_error("Wrong collector test route"); }
s.instruction_literal(0x30b5f5u); s.set(0,r(s.memory,(0x28u+s.gp[4]),4),4,0);
L_read_key: s.instruction_literal(0x30b5f9u); s.set(6,r(s.memory,(0x18u+s.gp[3]),4),4,0);
L_read_filter: s.instruction_literal(0x30b5fcu); s.set(1,r(s.memory,(0x44u+s.gp[0]),4),4,0);
s.instruction_literal(0x30b5ffu); s.set(7,s.flags.plus(s.reg(7,4,0),s.reg(6,4,0),4),4,0);
s.instruction_literal(0x30b601u); s.flags.logic((s.reg(1,4,0))&(s.reg(1,4,0)),4);
s.instruction_literal(0x30b603u); if(s.flags.zf)return Exit::normal;
return Exit::alternate;
}
template<> Exit retained<Part::shape_call>(WorldState &s,Route route) {
    switch(route) { case Route::original: break; case Route::read_collection_table: goto L_read_collection_table; case Route::push_shape_output: goto L_push_shape_output; case Route::push_shape_key: goto L_push_shape_key; case Route::set_shape_receiver: goto L_set_shape_receiver; case Route::done: return Exit::normal; case Route::alternate: return Exit::alternate; default: throw std::logic_error("Wrong collector test route"); }
s.instruction_literal(0x30b624u); s.set(0,r(s.memory,(0x64u+s.gp[0]),4),4,0);
L_read_collection_table: s.instruction_literal(0x30b627u); s.set(2,r(s.memory,(0x0u+s.gp[0]),4),4,0);
s.instruction_literal(0x30b629u); s.set(1,(0xa0u+s.gp[4]),4,0);
L_push_shape_output: s.instruction_literal(0x30b630u); s.push(0x30b630u,s.reg(1,4,0));
L_push_shape_key: s.instruction_literal(0x30b631u); s.push(0x30b631u,s.reg(7,4,0));
L_set_shape_receiver: s.instruction_literal(0x30b632u); s.set(1,s.reg(0,4,0),4,0);
return Exit::normal;
}
template<> Exit retained<Part::shape_result>(WorldState &s,Route route) {
    switch(route) { case Route::original: break; case Route::read_output: goto L_read_output; case Route::read_shape_table: goto L_read_shape_table; case Route::done: return Exit::normal; case Route::alternate: return Exit::alternate; default: throw std::logic_error("Wrong collector test route"); }
s.instruction_literal(0x30b637u); s.set(6,r(s.memory,(0x28u+s.gp[4]),4),4,0);
L_read_output: s.instruction_literal(0x30b63bu); s.set(1,r(s.memory,(0x58u+s.gp[6]),4),4,0);
s.instruction_literal(0x30b63eu); s.flags.logic((s.reg(1,4,0))&(s.reg(1,4,0)),4);
L_read_shape_table: s.instruction_literal(0x30b640u); s.set(2,r(s.memory,(0x0u+s.gp[0]),4),4,0);
s.instruction_literal(0x30b642u); if(s.flags.zf)return Exit::normal;
return Exit::alternate;
}
template<> Exit retained<Part::collector_call>(WorldState &s,Route route) {
    switch(route) { case Route::original: break; case Route::save_parent: goto L_save_parent; case Route::read_parent_value: goto L_read_parent_value; case Route::save_parent_value: goto L_save_parent_value; case Route::read_collector: goto L_read_collector; case Route::push_collector: goto L_push_collector; case Route::resolve_query: goto L_resolve_query; case Route::push_query: goto L_push_query; case Route::resolve_ray: goto L_resolve_ray; case Route::push_ray: goto L_push_ray; case Route::set_collector_receiver: goto L_set_collector_receiver; case Route::save_shape: goto L_save_shape; case Route::save_key: goto L_save_key; case Route::done: return Exit::normal; case Route::alternate: return Exit::alternate; default: throw std::logic_error("Wrong collector test route"); }
s.instruction_literal(0x30b671u); s.set(1,r(s.memory,(0x60u+s.gp[6]),4),4,0);
L_save_parent: s.instruction_literal(0x30b674u); w(s.memory,s.observe,0x30b674u,(0x6cu+s.gp[4]),4,s.reg(1,4,0));
L_read_parent_value: s.instruction_literal(0x30b678u); s.set(1,r(s.memory,(0x8u+s.gp[1]),4),4,0);
L_save_parent_value: s.instruction_literal(0x30b67bu); w(s.memory,s.observe,0x30b67bu,(0x68u+s.gp[4]),4,s.reg(1,4,0));
L_read_collector: s.instruction_literal(0x30b67fu); s.set(1,r(s.memory,(0x5cu+s.gp[6]),4),4,0);
L_push_collector: s.instruction_literal(0x30b682u); s.push(0x30b682u,s.reg(1,4,0));
L_resolve_query: s.instruction_literal(0x30b683u); s.set(1,(0x64u+s.gp[4]),4,0);
L_push_query: s.instruction_literal(0x30b687u); s.push(0x30b687u,s.reg(1,4,0));
L_resolve_ray: s.instruction_literal(0x30b688u); s.set(1,(0x20u+s.gp[6]),4,0);
L_push_ray: s.instruction_literal(0x30b68bu); s.push(0x30b68bu,s.reg(1,4,0));
L_set_collector_receiver: s.instruction_literal(0x30b68cu); s.set(1,s.reg(0,4,0),4,0);
L_save_shape: s.instruction_literal(0x30b68eu); w(s.memory,s.observe,0x30b68eu,(0x6cu+s.gp[4]),4,s.reg(0,4,0));
L_save_key: s.instruction_literal(0x30b692u); w(s.memory,s.observe,0x30b692u,(0x70u+s.gp[4]),4,s.reg(7,4,0));
return Exit::normal;
}
template<> Exit retained<Part::collector_result>(WorldState &s,Route route) {
    switch(route) { case Route::original: break; case Route::read_fraction: goto L_read_fraction; case Route::done: return Exit::normal; case Route::alternate: return Exit::alternate; default: throw std::logic_error("Wrong collector test route"); }
s.instruction_literal(0x30b699u); s.set(2,r(s.memory,(0x5cu+s.gp[6]),4),4,0);
L_read_fraction: s.instruction_literal(0x30b69cu); s.set(0,r(s.memory,(0x4u+s.gp[2]),4),4,0);
s.instruction_literal(0x30b69fu); w(s.memory,s.observe,0x30b69fu,(0x54u+s.gp[6]),4,s.reg(0,4,0));
return Exit::normal;
}


template<Part part>
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
        if(f.observer_fault)throw std::runtime_error("Injected collector observer fault");
    };
    if(f.observe)observe=on_store;
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={part==Part::shape_call?cache:shape,2u,table,vars,sp,sp+0x200u,cache,0x76543210u};
    input.eflags=0xa57u;input.x87_depth=2;input.x87_status=0x3204u;
    input.x87[0]=0.125L;input.x87[1]=-1.25L;
    WorldState state{bank,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
    bank.snapshot=[&]{return state.save();};
    bank.reenter=[&] {
        if(f.reenter){trace=true;observe=on_store;}
        if(f.move_frame)state.gp[4]+=0x100u;
        if(f.replace_target) {
            bank.put(table+0x34u,0x011111u);bank.put(table+0x24u,0x022222u);
        }
    };
    try {
        if(native)r.route=stage::collector<part>(state);
        r.exit=retained<part>(state,r.route);
        if constexpr(part==Part::shape_call||part==Part::collector_call) {
            constexpr bool shape_call=part==Part::shape_call;
            constexpr auto pc=shape_call?0x30b634u:0x30b696u;
            constexpr auto returned=shape_call?0x30b637u:0x30b699u;
            constexpr auto offset=shape_call?0x34u:0x24u;
            const auto child=[&](WorldState &called,std::uint32_t target,std::uint32_t site,bool tail) {
                r.calls.push_back({target,site,std::uint32_t(tail),called.gp[4]});
                r.states.push_back(called.save());
                if(f.child_unwind)throw EarlyUnwind{};
                if(called.pop()!=returned)throw std::runtime_error("Wrong collector return address");
                called.gp[0]=shape;
            };
            if(native)r.native_dispatch=stage::dispatch_call<shape_call>(state,child);
            if(!r.native_dispatch) {
                state.instruction_literal(pc);
                const auto target=physics_step_detail::r(state.memory,state.gp[2]+offset);
                state.push(pc,returned);
                child(state,target,pc,false);
            }
        }
    } catch(const EarlyUnwind &) {
        r.early_unwind=true;
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
template<Part part>
void compare(const Fixture &f,Route expected,std::optional<bool> expected_dispatch={}) {
    if(stage::reference_collector())expected=Route::original;
    const auto a=run<part>(f,false),b=run<part>(f,true);
    if(a.pc!=b.pc||a.fault!=b.fault||a.bytes!=b.bytes||a.accesses!=b.accesses||a.exit!=b.exit||
       a.observed!=b.observed||a.instructions!=b.instructions||!same(a.abi,b.abi)||
       a.states.size()!=b.states.size()||a.early_unwind!=b.early_unwind||a.calls!=b.calls||
       (b.fault.empty()&&b.route!=expected)||(expected_dispatch&&b.native_dispatch!=*expected_dispatch)) {
        std::cerr<<"Collector "<<int(part)<<" PC "<<std::hex<<a.pc<<'/'<<b.pc
                 <<" route "<<int(expected)<<'/'<<int(b.route)<<" fault "<<a.fault<<'/'<<b.fault<<'\n';
        throw std::runtime_error("MOPP collector continuation, provider or ABI differs");
    }
    for(unsigned i=0;i<a.states.size();++i)
        if(!same(a.states[i],b.states[i]))throw std::runtime_error("Different collector provider frame");
}
Route after_store(std::uint32_t pc) {
    switch(pc) {
    case 0x30b630u:return Route::push_shape_key;
    case 0x30b631u:return Route::set_shape_receiver;
    case 0x30b674u:return Route::read_parent_value;
    case 0x30b67bu:return Route::read_collector;
    case 0x30b682u:return Route::resolve_query;
    case 0x30b687u:return Route::resolve_ray;
    case 0x30b68bu:return Route::set_collector_receiver;
    case 0x30b68eu:return Route::save_key;
    default:return Route::done;
    }
}
struct Control {std::uint32_t pc,address;Route next;};
template<Part part,size_t stores,size_t loads>
unsigned prove(const std::array<Control,stores> &writes,const std::array<Control,loads> &reads) {
    unsigned compared{};
    auto check=[&](const Fixture &f,Route route){compare<part>(f,route);++compared;};
    for(bool sparse:{false,true}) {
        Fixture f;f.sparse=sparse;
        check(f,Route::done);
        auto alias=f;alias.parent_pointer=sp+0x64u;check(alias,Route::done);
        if constexpr(part==Part::select||part==Part::shape_result) {
            auto alternate=f;alternate.filter=alternate.direct=true;check(alternate,Route::alternate);
        }
        for(const auto &w:writes) {
            const bool rollback=stage::reference_collector_provider();
            auto denied=f;denied.deny_store=w.pc;check(denied,rollback?w.next:Route::done);
            denied.move_frame=true;check(denied,rollback?w.next:Route::done);
            denied.move_frame=false;denied.reenter=true;
            check(denied,rollback?w.next:after_store(w.pc));
            denied.fault_pc=w.pc;check(denied,part==Part::collector_result?Route::original:w.next);
        }
        for(const auto &r:reads) {
            auto denied=f;denied.sparse=true;denied.deny_load=r.address;
            check(denied,r.next);
            denied.fault_pc=r.pc;check(denied,r.next);
        }
        auto traced=f;traced.trace=true;check(traced,Route::original);
        auto weak=f;weak.authentic=false;check(weak,Route::original);
        auto observed=f;observed.observe=true;observed.observer_fault=true;
        check(observed,part==Part::select||part==Part::shape_result?Route::done:Route::original);
        if constexpr(part==Part::shape_call||part==Part::collector_call) {
            constexpr bool shape_call=part==Part::shape_call;
            constexpr auto pc=shape_call?0x30b634u:0x30b696u;
            constexpr auto offset=shape_call?0x34u:0x24u;
            const bool admit=!stage::reference_collector()&&!stage::reference_collector_provider();
            compare<part>(f,Route::done,admit);++compared;
            auto guarded=f;guarded.deny_store=pc;guarded.replace_target=true;
            compare<part>(guarded,Route::done,admit);++compared;
            guarded.child_unwind=true;check(guarded,Route::done);
            guarded.child_unwind=false;guarded.fault_pc=pc;check(guarded,Route::done);
            auto cold=f;cold.sparse=true;cold.deny_load=table+offset;
            compare<part>(cold,Route::done,false);++compared;
            cold.fault_pc=pc;check(cold,Route::done);
        }
    }
    return compared;
}
}
int main() try {
    using Row=Control;
    constexpr std::array<Row,0> none{};
    const auto cases=prove<Part::select>(none,std::array{
        Row{0x30b5f5u,sp+0x28u,Route::original},Row{0x30b5f9u,vars+0x18u,Route::read_key},
        Row{0x30b5fcu,cache+0x44u,Route::read_filter}})
        +prove<Part::shape_call>(std::array{
            Row{0x30b630u,0,Route::push_shape_output},Row{0x30b631u,0,Route::push_shape_key}},std::array{
            Row{0x30b624u,cache+0x64u,Route::original},Row{0x30b627u,collection,Route::read_collection_table}})
        +prove<Part::shape_result>(none,std::array{
            Row{0x30b637u,sp+0x28u,Route::original},Row{0x30b63bu,cache+0x58u,Route::read_output},
            Row{0x30b640u,shape,Route::read_shape_table}})
        +prove<Part::collector_call>(std::array{
            Row{0x30b674u,0,Route::save_parent},Row{0x30b67bu,0,Route::save_parent_value},
            Row{0x30b682u,0,Route::push_collector},Row{0x30b687u,0,Route::push_query},
            Row{0x30b68bu,0,Route::push_ray},Row{0x30b68eu,0,Route::save_shape},Row{0x30b692u,0,Route::save_key}},std::array{
            Row{0x30b671u,cache+0x60u,Route::original},Row{0x30b678u,parent+8u,Route::read_parent_value},
            Row{0x30b67fu,cache+0x5cu,Route::read_collector}})
        +prove<Part::collector_result>(std::array{Row{0x30b69fu,0,Route::done}},std::array{
            Row{0x30b699u,cache+0x5cu,Route::original},Row{0x30b69cu,destination+4u,Route::read_fraction}});
    std::cout<<"MOPP collector stages: "<<cases<<" exact continuation and provider comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}

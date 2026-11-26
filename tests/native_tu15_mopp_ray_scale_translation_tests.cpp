#include "physics/runtime/native_tu15_mopp_ray_scale_translation.hpp"
#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage=native_mopp_ray_scale_translation_detail;

using Abi=SourcePhysicsWorldRuntimeAbi;
constexpr std::uint32_t base=0x62000000u;
constexpr std::array store_sites{0x30b381u,0x30b38au,0x30b38eu,0x30b39au,0x30b3a2u,0x30b3aau};

struct Failure {};
struct Fixture {
    std::uint32_t denied{};
    bool throw_write{},install_observer{},observe_at_entry{},direct{true};
    unsigned rc{};
    std::array<std::uint32_t,3> components{};
    bool trace_after_store{},move_frame{};
};
struct Store {
    std::uint32_t pc,address,value;
    bool operator==(const Store&)const=default;
};
struct Snapshot {
    std::uint32_t pc;
    Abi abi;
    std::uint16_t status;
};
bool same(const Abi&a,const Abi&b) {
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_depth!=b.x87_depth||
       a.x87_status!=b.x87_status||a.floating_point->mxcsr!=b.floating_point->mxcsr||
       a.floating_point->x87_control_word!=b.floating_point->x87_control_word)return false;
    for(unsigned n=0;n<a.x87_depth;++n)
        if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;
    return true;
}

struct Memory : SourcePhysicsContactMemory {
    const Fixture &fixture;
    std::uint32_t &pc;
    SourceDirectPageIndex index;
    std::array<std::byte,4096> bytes{};
    std::array<Store,6> stores{};
    unsigned used{};
    mutable std::vector<Snapshot> callbacks;
    std::function<Snapshot()> snapshot;
    std::function<void()> install;
    const SourcePhysicsContactRecordObserver& observer;

    Memory(const Fixture&f,std::uint32_t&p,const SourcePhysicsContactRecordObserver&o) :
            fixture(f),pc(p),observer(o) {
        if(f.direct) {
            direct_pages_=&index;
            auto &block=index.blocks[base>>22u];
            block=std::make_unique<SourceDirectPageIndex::Block>();
            (*block)[(base>>12u)&1023u]={bytes.data(),true};
        }
    }
    void read(std::uint32_t address,std::span<std::byte> output)const override {
        callbacks.push_back(snapshot());
        std::memcpy(output.data(),bytes.data()+(address-base),output.size());
    }
    void put(std::uint32_t address,std::span<const std::byte> input)noexcept {
        std::uint32_t word{};
        std::memcpy(&word,input.data(),4);
        stores[used++]={pc,address,word};
        std::memcpy(bytes.data()+(address-base),input.data(),input.size());
    }
    bool try_callback_free_write(std::uint32_t address,std::span<const std::byte> input)noexcept override {
        if(!fixture.direct||observer||pc==fixture.denied)return false;
        put(address,input);
        return true;
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        // The retained provider has the same internal warm policy as stock.
        if(try_callback_free_write(address,input))return;
        callbacks.push_back(snapshot());
        if(pc==fixture.denied&&fixture.throw_write)throw Failure{};
        put(address,input);
        if(pc==fixture.denied)install();
    }
};

std::uint32_t retained_single_spill(long double value,std::uint16_t &status) {
    float result;
    std::uint16_t actual;
    asm volatile("fldt %2; fstps %0; fnstsw %1"
        :"=m"(result),"=m"(actual):"m"(value):"st");
    status=(status&~0x200u)|(actual&0x200u);
    return std::bit_cast<std::uint32_t>(result);
}

void retained(WorldState&s,stage::Continuation start) {
    switch(start) {
    case stage::Continuation::original: goto read_x;
    case stage::Continuation::constant_one: goto constant_one;
    case stage::Continuation::zero_lane: goto zero_lane;
    case stage::Continuation::read_y: goto read_y;
    case stage::Continuation::scale_shift: goto scale_shift;
    case stage::Continuation::read_z: goto read_z;
    case stage::Continuation::first_vector: return;
    }
read_x:
    s.instruction_literal(0x30b37du);s.fp.push(static_cast<long double>(si(r(s.memory,s.gp[4]+0x14u,4))));
    s.instruction_literal(0x30b381u);w(s.memory,s.observe,0x30b381u,s.gp[4]+0x14u,4,s.gp[2]);
constant_one:
    s.instruction_literal(0x30b385u);s.gp[2]=1u;
    s.instruction_literal(0x30b38au);w(s.memory,s.observe,0x30b38au,s.gp[4]+0x60u,4,retained_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
zero_lane:
    s.instruction_literal(0x30b38eu);w(s.memory,s.observe,0x30b38eu,s.gp[4]+0x6cu,4,0u);
read_y:
    s.instruction_literal(0x30b396u);s.fp.push(static_cast<long double>(si(r(s.memory,s.gp[4]+0x14u,4))));
    s.instruction_literal(0x30b39au);w(s.memory,s.observe,0x30b39au,s.gp[4]+0x14u,4,s.gp[1]);
scale_shift:
    s.instruction_literal(0x30b39eu);s.gp[1]=s.gp[0];
    s.instruction_literal(0x30b3a0u);s.gp[2]=s.flags.shift(s.gp[2],s.reg(1,1,0),4);
    s.instruction_literal(0x30b3a2u);w(s.memory,s.observe,0x30b3a2u,s.gp[4]+0x64u,4,retained_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
read_z:
    s.instruction_literal(0x30b3a6u);s.fp.push(static_cast<long double>(si(r(s.memory,s.gp[4]+0x14u,4))));
    s.instruction_literal(0x30b3aau);w(s.memory,s.observe,0x30b3aau,s.gp[4]+0x68u,4,retained_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
}

struct Outcome {
    bool fault{};
    Snapshot end{};
    std::array<std::byte,4096> bytes{};
    std::vector<Store> stores;
    std::vector<Snapshot> callbacks;
    std::vector<SourcePhysicsContactRecordStore> observations;
    std::vector<std::uint32_t> instructions;
    stage::Continuation route{};
};

Outcome run(const Fixture&f,bool native) {
    std::uint32_t pc=0x30b379u;
    Outcome outcome;
    SourcePhysicsContactRecordObserver observer;
    Memory memory(f,pc,observer);
    auto capture_observer=[&](const auto &event) {
        outcome.observations.push_back(event);
        memory.callbacks.push_back(memory.snapshot());
    };
    if(f.observe_at_entry)observer=capture_observer;
    SourcePhysicsContactRuntimeServices services;
    static constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    bool trace{};
    services.instruction=[&](std::uint32_t instruction){
        pc=instruction;
        outcome.instructions.push_back(instruction);
    };
    auto&descriptor=services.instruction_cursor;
    descriptor.position=&pc;descriptor.callback_type=&services.instruction.target_type();
    descriptor.first_trace=&trace;
    descriptor.service_sites=sites;descriptor.executor=std::this_thread::get_id();
    descriptor.literal_service_sites=true;descriptor.observation_only_stores=true;
    Abi input{};
    input.gp={1,f.components[2],f.components[1],4,base+0x180u,6,7,8};
    input.eflags=0xad7u;input.x87_depth=2;input.x87_status=0x7d04u;
    input.x87[0]=1.25L;input.x87[1]=-3.25L;
    input.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x27fu|(f.rc<<10u)),0x1f81u};
    FpScope env(*input.floating_point);
    WorldState s{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    s.load(input);
    std::memcpy(memory.bytes.data()+0x194u,&f.components[0],4);
    memory.install=[&] {
        if(f.install_observer)observer=capture_observer;
        if(f.trace_after_store)trace=true;
        if(f.move_frame) {
            s.gp[4]+=0x100u;
            const auto replacement=std::uint32_t{117};
            std::memcpy(memory.bytes.data()+s.gp[4]-base+0x14u,&replacement,4);
        }
    };
    memory.snapshot=[&] {
        std::uint16_t status;
        asm volatile("fnstsw %0":"=am"(status)::"memory");
        return Snapshot{pc,s.save(),status};
    };
    try {
        outcome.route=native?stage::prepare(s,retained_single_spill):stage::Continuation::original;
        retained(s,outcome.route);
    } catch(const Failure&) {outcome.fault=true;}
    outcome.end=memory.snapshot();
    outcome.bytes=memory.bytes;
    outcome.stores.assign(memory.stores.begin(),memory.stores.begin()+memory.used);
    outcome.callbacks=std::move(memory.callbacks);
    return outcome;
}

void compare(const Fixture&f) {
    const auto retained=run(f,false),native=run(f,true);
    if(!f.throw_write) {
        auto expected=stage::Continuation::original;
        const bool byte_values=std::all_of(f.components.begin(),f.components.end(),[](auto v){return v<=255u;});
        if(byte_values&&f.direct&&!f.observe_at_entry&&!stage::reference_scale_translation()&&
           !native_mopp_ray_traversal_detail::reference()) {
            constexpr std::array reentries{stage::Continuation::constant_one,stage::Continuation::zero_lane,
                stage::Continuation::read_y,stage::Continuation::scale_shift,stage::Continuation::read_z,
                stage::Continuation::first_vector};
            const auto match=std::find(store_sites.begin(),store_sites.end(),f.denied);
            const bool resume=!(f.install_observer||f.trace_after_store||
                native_mopp_ray_scale_tail_detail::reference_scale_provider());
            expected=resume||match==store_sites.end()?stage::Continuation::first_vector:reentries[match-store_sites.begin()];
        }
        if(native.route!=expected)throw std::runtime_error("Wrong native transfer after provider refusal");
    }
    if(retained.fault!=native.fault||retained.end.pc!=native.end.pc||!same(retained.end.abi,native.end.abi)||
       retained.bytes!=native.bytes||retained.stores!=native.stores||
       retained.observations!=native.observations||retained.instructions!=native.instructions||
       retained.callbacks.size()!=native.callbacks.size()||
       ((retained.end.status^native.end.status)&0x023fu)) {
        std::cerr<<"denied "<<std::hex<<f.denied<<" rc "<<f.rc<<" first "<<f.components[0]
                 <<" thrown "<<retained.fault<<'/'<<native.fault<<" flag "<<retained.end.abi.eflags<<'/'<<native.end.abi.eflags
                 <<" prefix "<<retained.end.pc<<'/'<<native.end.pc
                 <<" ambient "<<retained.end.status<<'/'<<native.end.status
                 <<" source "<<retained.end.abi.x87_status<<'/'<<native.end.abi.x87_status
                 <<" depth "<<retained.end.abi.x87_depth<<'/'<<native.end.abi.x87_depth
                 <<" memory "<<(retained.bytes==native.bytes)<<" stores "<<(retained.stores==native.stores)
                 <<" obs "<<retained.observations.size()<<'/'<<native.observations.size()
                 <<" callback "<<retained.callbacks.size()<<'/'<<native.callbacks.size()<<std::dec<<'\n';
        throw std::runtime_error("Stage outcome or exact ordered memory changed");
    }
    for(unsigned n=0;n<retained.callbacks.size();++n) {
        const auto &a=retained.callbacks[n],&b=native.callbacks[n];
        if(a.pc!=b.pc||!same(a.abi,b.abi)||((a.status^b.status)&0x023fu)) {
            std::cerr<<"mode "<<f.rc<<" fault "<<f.throw_write<<" at "<<std::hex<<a.pc<<'/'<<b.pc
                     <<" logical "<<a.abi.x87_status<<'/'<<b.abi.x87_status<<" physical "
                     <<a.status<<'/'<<b.status<<std::dec<<'\n';
            throw std::runtime_error("A provider or observer saw different source FP/ABI");
        }
    }
}

}
int main()try {
    unsigned checks{};
    for(unsigned rc=0;rc<4;++rc)
        for(const auto values:{std::array{17u,31u,57u},std::array{0u,255u,127u},std::array{128u,1u,254u},std::array{0x7fffffffu,0x80000003u,0xfffefdfbu},
                              std::array{0x01000003u,0xff000001u,0x12345679u}}) {
            compare({0,false,false,false,true,rc,values});
            compare({0,false,false,true,true,rc,values});
            compare({0,false,false,false,false,rc,values});
            checks+=3;
            for(auto pc:store_sites) {
                compare({pc,true,false,false,true,rc,values});
                compare({pc,false,true,false,true,rc,values});
                auto provided=Fixture{pc,false,false,false,true,rc,values};
                compare(provided);
                provided.trace_after_store=true;
                compare(provided);
                provided.trace_after_store=false;
                provided.move_frame=true;
                compare(provided);
                checks+=5;
            }
        }
    std::cout<<"PASS "<<checks<<" scale translation boundary comparisons\n";
} catch(const std::exception&e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}

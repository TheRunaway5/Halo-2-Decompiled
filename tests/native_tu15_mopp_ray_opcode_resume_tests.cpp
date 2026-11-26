#include "physics/runtime/native_tu15_mopp_ray_opcode_resume.hpp"

#include <iostream>
#include <memory>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage = native_mopp_ray_opcode_resume_detail;

constexpr auto base=0x62000000u;
constexpr auto stack=base+0x200u;
constexpr auto sentinel=stack+0x3cu;
constexpr auto previous_pc=0x30b432u;
struct Failure {};

enum class Action { none, fail_read, fail_write, fail_observer, change_flags, enable_trace };
struct Fixture {
    std::uint8_t opcode;
    bool alias{}, resident{true}, warm{true}, observed{}, trace{};
    Action action{};
    bool pop_first{};
};
using Abi=SourcePhysicsWorldRuntimeAbi;
bool same(const Abi&a,const Abi&b) {
    return a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm &&
        a.x87_depth==b.x87_depth && a.x87_status==b.x87_status &&
        std::memcmp(a.x87.data(),b.x87.data(),10)==0 &&
        a.floating_point->x87_control_word==b.floating_point->x87_control_word &&
        a.floating_point->mxcsr==b.floating_point->mxcsr;
}
struct Event {
    unsigned kind;
    std::uint32_t pc, address;
    std::size_t width;
    Abi state;
    bool operator==(const Event&b)const {
        return kind==b.kind && pc==b.pc && address==b.address && width==b.width && same(state,b.state);
    }
};
struct Outcome {
    std::vector<Event> callbacks;
    std::vector<std::uint32_t> instructions;
    std::vector<SourcePhysicsContactRecordStore> observers;
    Abi abi;
    std::uint32_t pc{};
    std::uint32_t final_bytes{};
    unsigned writes{};
    stage::Continuation route{};
    bool fault{},classify{},bad_attempt_pc{};
};

struct Memory : SourcePhysicsContactMemory {
    const Fixture &fixture;
    Outcome &out;
    std::array<std::byte,4096> bytes{};
    SourceDirectPageIndex pages;
    std::function<Abi()> sample;
    std::function<void()> change;

    Memory(const Fixture &f,Outcome &o):fixture(f),out(o) {
        if(f.resident) {
            direct_pages_=&pages;
            auto &block=pages.blocks[base>>22u];
            block=std::make_unique<SourceDirectPageIndex::Block>();
            (*block)[(base>>12u)&1023u]={bytes.data(),true};
        }
    }
    void log(unsigned kind,std::uint32_t address,std::size_t width)const {
        out.callbacks.push_back({kind,out.pc,address,width,sample()});
    }
    void read(std::uint32_t address,std::span<std::byte> output)const override {
        log(1,address,output.size());
        if(fixture.action==Action::fail_read)throw Failure{};
        std::memcpy(output.data(),bytes.data()+(address-base),output.size());
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        if(try_callback_free_write(address,input))return;
        log(2,address,input.size());
        if(fixture.action==Action::fail_write)throw Failure{};
        std::memcpy(bytes.data()+(address-base),input.data(),input.size());
        ++out.writes;
        if(fixture.action==Action::change_flags || fixture.action==Action::enable_trace)change();
    }
    bool try_callback_free_write(std::uint32_t address,std::span<const std::byte> input)noexcept override {
        out.bad_attempt_pc|=out.pc!=0x30ae08u;
        if(!fixture.warm || fixture.observed || fixture.action==Action::fail_write ||
           fixture.action==Action::change_flags || fixture.action==Action::enable_trace)return false;
        std::memcpy(bytes.data()+(address-base),input.data(),input.size());
        ++out.writes;
        return true;
    }
};

bool retained(WorldState&s,stage::Continuation route) {
    if(route==stage::Continuation::classify)return true;
    if(route==stage::Continuation::diagnostic)return false;
    if(route==stage::Continuation::original || route==stage::Continuation::read_opcode) {
        s.instruction_literal(0x30ae02u);s.set(0,r(s.memory,s.gp[6],1),4,0);
        s.instruction_literal(0x30ae05u);s.flags.minus(s.reg(0,4,0),0x6bu,4);
    }
    if(route!=stage::Continuation::branch_default) {
        s.instruction_literal(0x30ae08u);w(s.memory,s.observe,0x30ae08u,s.gp[4]+0x3cu,4,999u);
    }
    s.instruction_literal(0x30ae10u);return s.flags.cf||s.flags.zf;
}

Outcome run(const Fixture&fixture,bool native) {
    Outcome out{};
    out.pc=previous_pc;
    Memory memory(fixture,out);
    const auto program=fixture.alias?sentinel:base+0x600u;
    std::fill_n(memory.bytes.begin()+(sentinel-base),4,std::byte{0xa5});
    memory.bytes[program-base]=std::byte{fixture.opcode};
    SourcePhysicsContactRecordObserver observer;
    if(fixture.observed)observer=[&](const auto &event){
        out.observers.push_back(event);
        memory.log(3,event.address,event.written.size());
        if(fixture.action==Action::fail_observer)throw Failure{};
    };

    SourcePhysicsContactRuntimeServices services;
    static constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    services.instruction=[&](unsigned instruction){out.pc=instruction;out.instructions.push_back(instruction);};
    auto &cursor=services.instruction_cursor;
    cursor.position=&out.pc;cursor.callback_type=&services.instruction.target_type();
    cursor.service_sites=sites;cursor.executor=std::this_thread::get_id();
    bool tracing=fixture.trace;
    cursor.literal_service_sites=true;cursor.first_trace=&tracing;
    cursor.observation_only_stores=true;
    Abi input{};
    input.gp={0xfffffff0u,2,3,4,stack,6,program,8};
    input.eflags=0xad7u;
    input.x87_status=0x7f04u;input.x87_depth=1;input.x87[0]=1.25L;
    input.xmm[3]={0x11111111u,0x22222222u,0x33333333u,0x44444444u};
    input.floating_point=SourcePhysicsFactoryFloatingPoint{0x27fu,0x1f81u};
    FpScope environment(*input.floating_point);
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(input);
    memory.sample=[&]{return state.save();};
    memory.change=[&] {
        if(fixture.action==Action::enable_trace)tracing=true;
        else {state.flags.cf=false;state.flags.zf=false;}
    };
    try {
        if(native)out.route=fixture.pop_first?stage::resume<true>(state):stage::resume(state);
        if(native && out.route==stage::Continuation::original &&
           (out.pc!=previous_pc || !same(state.save(),input) || out.writes))
            throw std::runtime_error("Whole-block refusal changed live source state");
        if(native && out.route==stage::Continuation::store_default &&
           (out.pc!=0x30ae05u || out.writes!=0u || state.gp[0]!=fixture.opcode))
            throw std::runtime_error("Store refusal lost the prior completed opcode load and comparison");
        if(fixture.pop_first && out.route==stage::Continuation::original) {
            state.instruction_literal(0x30ae00u);
            state.fp.status&=~0x200u;
            state.fp.pop();
        }
        out.classify=retained(state,out.route);
    } catch(const Failure&) {out.fault=true;}
    out.abi=state.save();
    std::memcpy(&out.final_bytes,memory.bytes.data()+(sentinel-base),4);
    return out;
}

void compare(const Fixture&fixture) {
    const auto expected=run(fixture,false),actual=run(fixture,true);
    if(actual.bad_attempt_pc || expected.bad_attempt_pc || actual.callbacks!=expected.callbacks||actual.instructions!=expected.instructions||
       actual.observers!=expected.observers||!same(actual.abi,expected.abi)||actual.pc!=expected.pc||
       actual.final_bytes!=expected.final_bytes||actual.writes!=expected.writes||
       actual.fault!=expected.fault||actual.classify!=expected.classify)
        throw std::runtime_error("Resume changed cursor, source accounting, GP/flags or observable memory");
    auto wanted=stage::Continuation::original;
    if(!stage::reference_opcode_resume() && !fixture.observed && !fixture.trace) {
        if(!fixture.resident) {
            if(fixture.pop_first)wanted=stage::Continuation::read_opcode;
        } else if(stage::reference_opcode_provider() &&
                  (!fixture.warm || fixture.action==Action::fail_write || fixture.action==Action::change_flags ||
                   fixture.action==Action::enable_trace))wanted=stage::Continuation::store_default;
        else if(fixture.action==Action::enable_trace)wanted=stage::Continuation::branch_default;
        else if(fixture.opcode<=0x6bu && fixture.action!=Action::change_flags)wanted=stage::Continuation::classify;
        else wanted=stage::Continuation::diagnostic;
    }
    if(!actual.fault && actual.route!=wanted)throw std::runtime_error("Unexpected particular resume continuation");
}
} // namespace

int main() try {
    unsigned checks{};
    for(std::uint8_t op:{0,1,0x6b,0x6c,0xff})
        for(bool alias:{false,true}) for(bool pop_first:{false,true}) {
            compare({op,alias,true,true,false,false,Action::none,pop_first});
            compare({op,alias,false,true,false,false,Action::none,pop_first});
            compare({op,alias,true,false,false,false,Action::none,pop_first});
            compare({op,alias,true,false,false,false,Action::fail_write,pop_first});
            compare({op,alias,true,false,false,false,Action::change_flags,pop_first});
            compare({op,alias,true,false,false,false,Action::enable_trace,pop_first});
            compare({op,alias,true,true,true,false,Action::none,pop_first});
            compare({op,alias,true,true,true,false,Action::fail_observer,pop_first});
            compare({op,alias,true,true,false,true,Action::none,pop_first});
            compare({op,alias,false,false,false,false,Action::fail_read,pop_first});
            checks+=10;
        }
    std::cout<<"PASS "<<checks<<" opcode resume comparisons\n";
} catch(const std::exception&e) {
    std::cerr<<e.what()<<'\n';return 1;
}

#include "physics/runtime/native_tu15_mopp_ray_dispatch.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace dispatch = native_mopp_ray_dispatch_detail;

void check(bool ok,const char *reason) {
    if(!ok)throw std::runtime_error(reason);
}

struct Memory : SourcePhysicsContactMemory {
    SourceDirectPageIndex index;
    std::array<std::array<std::byte,4096>,2> data{};
    mutable unsigned provider_reads{};

    explicit Memory(std::uint32_t base) {
        direct_pages_=&index;
        for(unsigned n=0;n<data.size();++n) {
            const auto address=base+n*4096u;
            auto &block=index.blocks[address>>22u];
            if(!block)block=std::make_unique<SourceDirectPageIndex::Block>();
            (*block)[(address>>12u)&1023u]={data[n].data(),false};
        }
    }

    void read(std::uint32_t address,std::span<std::byte> output) const override {
        ++provider_reads;
        for(auto &value:output) {
            const auto *page=direct_page(address,1u);
            check(page!=nullptr,"Fixture fell outside ordinary pages");
            value=page->bytes[address&4095u];
            ++address;
        }
    }
    void write(std::uint32_t,std::span<const std::byte>) override {
        throw std::runtime_error("Read-only branch wrote source memory");
    }
    void put(std::uint32_t address,std::span<const std::byte> bytes) {
        for(auto value:bytes) {
            direct_page(address,1)->bytes[address&4095u]=value;
            ++address;
        }
    }
    void put_word(std::uint32_t address,std::uint32_t value) {
        put(address,std::as_bytes(std::span{&value,1}));
    }
};

struct Cursor {
    std::uint32_t pc = 0x30aaa7u;
    bool tracing{};
    unsigned callbacks{};
    SourcePhysicsContactRuntimeServices services;
    SourcePhysicsContactRecordObserver observe;
    explicit Cursor(bool traced) : tracing(traced) {
        static constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction=[this](std::uint32_t seen){pc=seen;++callbacks;};
        auto &cursor=services.instruction_cursor;
        cursor.position=&pc;
        cursor.first_trace=&tracing;
        cursor.service_sites=sites;
        cursor.callback_type=&services.instruction.target_type();
        cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=true;
    }
};

using Abi=SourcePhysicsWorldRuntimeAbi;
Abi incoming() {
    Abi input{};
    input.gp={0x10203040u,0x11223344u,0x89abcdefu,0x76543210u,0x620001b0u,0x620001f0u,0u,0x66778899u};
    input.eflags=0xad7u;
    input.x87_depth=2;
    input.x87_status=0x7d04u;
    input.x87[0]=1.25L;
    input.x87[1]=-3.25L;
    input.xmm[7]={8,13,21,34};
    input.floating_point=SourcePhysicsFactoryFloatingPoint{0x27fu,0x1f81u};
    return input;
}

bool same(const Abi &a,const Abi &b) {
    return a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm &&
        a.x87_depth==b.x87_depth && a.x87_status==b.x87_status &&
        std::memcmp(&a.x87[0],&b.x87[0],10)==0 && std::memcmp(&a.x87[1],&b.x87[1],10)==0 &&
        a.floating_point->mxcsr==b.floating_point->mxcsr &&
        a.floating_point->x87_control_word==b.floating_point->x87_control_word;
}

// Retained TU1.5 register writes and terminal arithmetic. Preserve the
// explicit source steps so the accepted batched result has an independent
// reference for the flags and partially overwritten registers.
template<unsigned Count> void retained_jump(WorldState &s) {
    auto &g=s.gp;
    if constexpr(Count==1) {
        s.instruction_literal(0x30b328u);g[0]=r(s.memory,g[6]+1u,1);
        s.instruction_literal(0x30b32cu);g[6]=2u+g[6]+g[0];
        s.instruction_literal(0x30b330u);
    } else if constexpr(Count==2) {
        s.instruction_literal(0x30b335u);g[1]=r(s.memory,g[6]+1u,1);
        s.instruction_literal(0x30b339u);g[2]=r(s.memory,g[6]+2u,1);
        s.instruction_literal(0x30b33du);g[1]=s.flags.shift(g[1],8,4);
        s.instruction_literal(0x30b340u);g[2]=s.flags.plus(g[2],g[6],4);
        s.instruction_literal(0x30b342u);g[6]=3u+g[2]+g[1];
        s.instruction_literal(0x30b346u);
    } else {
        s.instruction_literal(0x30b34bu);g[0]=r(s.memory,g[6]+1u,1);
        s.instruction_literal(0x30b34fu);g[1]=r(s.memory,g[6]+2u,1);
        s.instruction_literal(0x30b353u);g[2]=r(s.memory,g[6]+3u,1);
        s.instruction_literal(0x30b357u);g[0]=s.flags.shift(g[0],8,4);
        s.instruction_literal(0x30b35au);g[0]=s.flags.plus(g[0],g[1],4);
        s.instruction_literal(0x30b35cu);g[0]=s.flags.shift(g[0],8,4);
        s.instruction_literal(0x30b35fu);g[2]=s.flags.plus(g[2],g[6],4);
        s.instruction_literal(0x30b361u);g[6]=4u+g[2]+g[0];
        s.instruction_literal(0x30b365u);
    }
}

void retained_empty(WorldState &s) {
    s.instruction_literal(0x30b6afu);s.gp[7]=s.pop();
    s.instruction_literal(0x30b6b0u);s.gp[6]=s.pop();
    s.instruction_literal(0x30b6b1u);s.gp[3]=s.pop();
    s.instruction_literal(0x30b6b2u);s.gp[4]=s.gp[5];
    s.instruction_literal(0x30b6b4u);s.gp[5]=s.pop();
    s.instruction_literal(0x30b6b5u);s.pop();s.gp[4]+=12u;
}

bool environment(const char *name) {
    const auto *value=std::getenv(name);
    return value&&*value;
}

template<class Candidate,class Reference>
void compare(Memory &memory,const Abi &input,bool trace,bool native,Candidate candidate,Reference reference) {
    constexpr auto dispatch_pc=0x30aaa7u;
    Cursor cursor(trace);
    FpScope fp(*input.floating_point);
    WorldState state{memory,cursor.observe,cursor.services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(input);
    reference(state);
    const auto expected=state.save();
    const auto expected_pc=cursor.pc;
    state.load(input);
    cursor.pc=dispatch_pc;
    cursor.callbacks=0;
    memory.provider_reads=0;
    check(candidate(state)==native,"Unexpected dispatch admission");
    check(cursor.callbacks==0 && memory.provider_reads==0,"Dispatch candidate called a provider or source hook");
    if(!native) {
        check(same(state.save(),input)&&cursor.pc==dispatch_pc,"Refusal retired source effects");
        reference(state);
    }
    check(same(state.save(),expected)&&cursor.pc==expected_pc,"Branch result or final cursor differs");
}

template<unsigned Count>
void jump(std::uint32_t program,bool trace,bool native) {
    const auto operand=program+1u;
    Memory memory(operand&~4095u);
    constexpr std::array largest{std::byte{0xe7},std::byte{0xef},std::byte{0xff}};
    memory.put(operand,std::span<const std::byte,3>(largest).first<Count>());
    auto input=incoming();
    input.gp[6]=program;
    compare(memory,input,trace,native,dispatch::jump_case<Count,WorldState>,retained_jump<Count>);
}

void empty(std::uint32_t registers,std::uint32_t frame,bool trace,bool native) {
    Memory memory(0x62000000u);
    memory.put_word(registers,0x12345678u);
    memory.put_word(registers+4u,0x9abcdef0u);
    memory.put_word(registers+8u,0x89abcdefu);
    memory.put_word(frame,0xabcdef01u);
    memory.put_word(frame+4u,0x76543210u);
    auto input=incoming();
    input.gp[4]=registers;
    input.gp[5]=frame;
    compare(memory,input,trace,native,dispatch::empty_case<WorldState>,retained_empty);
}
} // namespace

int main() try {
    const bool disabled=environment("HALO2_REFERENCE_MOPP_RAY_TRAVERSAL")||
        environment("HALO2_REFERENCE_MOPP_RAY_DISPATCH")||environment("HALO2_REFERENCE_SCOPED_ACCESS");
    for(auto program:{0x62000000u,0x7fffffc0u,0xffffffc0u,0xffffffffu}) {
        jump<1>(program,false,!disabled);
        jump<2>(program,false,!disabled);
        jump<3>(program,false,!disabled);
    }
    jump<2>(0x62000ffeu,false,false);
    jump<3>(0x62000ffeu,false,false);
    jump<3>(0x62000010u,true,false);
    const bool scalar=disabled||environment("HALO2_REFERENCE_SCALAR_STACK_POPS");
    empty(0x620001b0u,0x620001f0u,false,!scalar);
    empty(0x620001b0u,0x620001b4u,false,!scalar);
    empty(0x62000ff8u,0x620001f0u,false,false);
    empty(0x620001b0u,0x62000ffcu,false,false);
    empty(0x620001b0u,0x620001f0u,true,false);
    std::cout<<"PASS 20 table-selected branch cases\n";
} catch(const std::exception &error) {
    std::cerr<<error.what()<<'\n';
    return 1;
}

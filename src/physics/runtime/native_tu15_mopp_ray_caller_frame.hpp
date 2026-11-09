#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <array>
#include <cstdint>
#include <utility>

namespace halo2::native_mopp_ray_caller_detail {
struct Entry {
    std::uint32_t cache{},program{},opcode{},variables{},bounds{};
    bool valid_program_class{};
};
struct DirectIo {
    template<class Callback>void operator()(Callback&&callback)const{std::forward<Callback>(callback)();}
};

// Private TU1.5 caller-frame foundation. It owns physical caller storage and
// observable ABI publication, not geometry decisions or instruction execution.
// No production entry uses it yet; admission and the complete ray controller
// must be qualified together before installing a default prefix.
template<class State,class Before,class Io=DirectIo>class Frame {
protected:
    State&s_;Before&before_;Io io_;
    template<class Callback>void io(Callback&&callback){io_(std::forward<Callback>(callback));}
    void at(std::uint32_t pc){
        if(s_.instruction_cursor&&s_.instruction_cursor->position)
            *s_.instruction_cursor->position=pc;
        io([&]{before_(s_,pc);});
    }
    std::uint32_t word(std::uint32_t address,unsigned width=4){
        std::array<std::byte,4>bytes{};
        io([&]{s_.memory.read(address,std::span(bytes).first(width));});
        std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
        return value;
    }
    template<class Commit>
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,Commit commit,bool positioned=false){
        if(!positioned)at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(4);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};
        for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
        io([&]{s_.memory.write(address,bytes);});commit();
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());io([&]{s_.observe(event);});}
    }
    void push(std::uint32_t pc,std::uint32_t value){
        const auto address=s_.gp[4]-4u;
        store(pc,address,value,[&]{s_.gp[4]=address;});
    }
    std::uint32_t pop(std::uint32_t pc){
        at(pc);const auto value=word(s_.gp[4]);s_.gp[4]+=4u;return value;
    }
public:
    Frame(State&s,Before&before,Io owner={}):s_(s),before_(before),io_(std::move(owner)){}
    Entry begin(){
        auto&g=s_.gp;
        push(0x30aa70u,g[5]);g[5]=g[4];
        g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);
        g[4]=s_.flags.minus(g[4],0x194u,4);
        push(0x30aa7cu,g[3]);push(0x30aa7du,g[6]);
        at(0x30aa7eu);g[6]=word(g[5]+12u);
        at(0x30aa81u);g[0]=word(g[6],1);
        s_.flags.minus(g[0],0x6bu,4);
        Entry entry{};entry.program=g[6];entry.opcode=g[0];
        push(0x30aa87u,g[7]);
        entry.cache=g[1];store(0x30aa88u,g[4]+0x28u,g[1],[]{});
        store(0x30aa8cu,g[4]+0x3cu,999u,[]{});
        entry.valid_program_class=entry.opcode<=0x6bu;
        if(entry.valid_program_class){
            at(0x30aa9au);g[7]=word(g[5]+16u);entry.bounds=g[7];
            at(0x30aa9du);g[3]=word(g[5]+8u);entry.variables=g[3];
        }
        return entry;
    }
    // The typed DATA classifier owns this value. The original's two immutable
    // code-table fetches are replaced by that compiled format classification.
    void publish_program_class(std::uint32_t value){s_.gp[1]=value;}
    void finish(){
        auto&g=s_.gp;
        g[7]=pop(0x30b6afu);g[6]=pop(0x30b6b0u);g[3]=pop(0x30b6b1u);
        g[4]=g[5];g[5]=pop(0x30b6b4u);pop(0x30b6b5u);g[4]+=12u;
    }
};
} // namespace halo2::native_mopp_ray_caller_detail

#pragma once
#include "halo2/native_tu15_mopp_integer_program_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_mopp_aabb_traversal.hpp"
#include <cstring>
#include <type_traits>
#include <utility>

namespace halo2::native_mopp_ray_caller_detail {
struct AlignedLoadFault {
    std::uint32_t instruction{},address{},execution_eflags{},saved_eflags{};
};
template<class State,class Before,class Io=DirectIo>class IntegerFrame:public Frame<State,Before,Io> {
protected:
    using Base=Frame<State,Before,Io>;
    using Progress=native_mopp_integer_program_detail::Progress;
    using Family=native_mopp_integer_program_detail::Family;
    using Vector=std::remove_cvref_t<decltype(std::declval<State&>().xmm[0])>;
    using Base::s_;using Base::at;using Base::word;using Base::store;using Base::io;
    std::uint32_t load(unsigned reg,std::uint32_t pc,std::uint32_t address,unsigned width=4){
        at(pc);return s_.gp[reg]=word(address,width);
    }
    void combine(unsigned destination,unsigned low){
        auto&g=s_.gp;
        g[destination]=native_mopp_aabb_abi_detail::shifted_immediate<8>(s_,g[destination]);
        g[destination]=s_.flags.plus(g[destination],g[low],4);
    }
    Vector vector(std::uint32_t pc,std::uint32_t address){
        at(pc);
        // These authenticated source loads are MOVAPS, not unaligned loads.
        // Preserve their fault boundary before asking the DATA provider.
        if(address&15u){
            const auto flags=s_.other_flags|s_.flags.bits();
            // Intel SDM vol.3B, RF flag: a fault-class saved EFLAGS image has
            // RF set. Keep that exception image distinct from execution state.
            throw AlignedLoadFault{pc,address,flags,flags|0x10000u};
        }
        std::array<std::byte,16>bytes{};io([&]{s_.memory.read_vector(address,bytes);});
        Vector value;std::memcpy(&value,bytes.data(),16);return value;
    }
    void vector_store(std::uint32_t pc,std::uint32_t address,Vector value){
        at(pc);
        if(address&15u){
            const auto flags=s_.other_flags|s_.flags.bits();
            throw AlignedLoadFault{pc,address,flags,flags|0x10000u};
        }
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(16);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&value,16);io([&]{s_.memory.write(address,bytes);});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());io([&]{s_.observe(event);});}
    }
    void copy_variables(std::uint32_t vector_read,std::uint32_t vector_write,
                        std::span<const std::uint32_t>reads,std::span<const std::uint32_t>writes,
                        std::span<const unsigned>registers){
        auto&g=s_.gp;s_.xmm[0]=vector(vector_read,g[3]);vector_store(vector_write,g[4]+0x40u,s_.xmm[0]);
        for(unsigned n=0;n<reads.size();++n){
            const auto value=load(registers[n],reads[n],g[3]+0x10u+n*4u);
            store(writes[n],g[4]+0x50u+n*4u,value,[]{});
        }
        g[3]=g[4]+0x40u;
    }
public:
    IntegerFrame(State&s,Before&before,Io owner={}):Base{s,before,std::move(owner)}{}
    std::uint32_t encoded_byte(const Progress&d){
        const auto n=d.index,address=d.program+n+1u;
        if(d.family==Family::jump){
            if(d.count==1u)return load(0,0x30b328u,address,1);
            if(d.count==2u)return load(n?2:1,n?0x30b339u:0x30b335u,address,1);
            return load(n==0?0:n==1?1:2,std::array{0x30b34bu,0x30b34fu,0x30b353u}[n],address,1);
        }
        if(d.family==Family::key_base){
            if(d.count==1u)return load(0,0x30b437u,address,1);
            if(d.count==2u)return load(n?2:0,n?0x30b47bu:0x30b477u,address,1);
            if(n==3u)combine(0,2);
            return load(n==0?0:n==2?1:2,std::array{0x30b4c0u,0x30b4c4u,0x30b4c8u,0x30b4d1u}[n],address,1);
        }
        if(d.count==1u)return load(1,0x30b51bu,address,1);
        if(d.count==2u)return load(n?1:2,n?0x30b52fu:0x30b52bu,address,1);
        if(n>=2u)combine(2,1);
        return load(n?1:2,std::array{0x30b544u,0x30b548u,0x30b551u,0x30b55au}[n],address,1);
    }
    void decoded(const Progress&d){
        if(d.family==Family::key_base){
            if(d.count==2u)combine(0,2);
            else if(d.count==4u){combine(0,1);combine(0,2);}
        }else if(d.family==Family::property&&d.count!=1u)combine(2,1);
    }
    void ensure_variables(const Progress&d){
        auto&g=s_.gp;g[1]=g[4]+0x40u;s_.flags.minus(g[3],g[1],4);
        if(g[3]==g[4]+0x40u)return;
        constexpr std::array registers{2u,1u,2u,1u};
        if(d.count==1u){
            constexpr std::array reads{0x30b44bu,0x30b452u,0x30b459u,0x30b460u},writes{0x30b44eu,0x30b455u,0x30b45cu,0x30b463u};
            copy_variables(0x30b443u,0x30b446u,reads,writes,registers);
        }else if(d.count==2u){
            constexpr std::array reads{0x30b494u,0x30b49bu,0x30b4a2u,0x30b4a9u},writes{0x30b497u,0x30b49eu,0x30b4a5u,0x30b4acu};
            copy_variables(0x30b48cu,0x30b48fu,reads,writes,registers);
        }else{
            constexpr std::array reads{0x30b4efu,0x30b4f6u,0x30b4fdu,0x30b504u},writes{0x30b4f2u,0x30b4f9u,0x30b500u,0x30b507u};
            copy_variables(0x30b4e7u,0x30b4eau,reads,writes,registers);
        }
    }
    std::uint32_t key_base(const Progress&d){
        at(d.count==1u?0x30b46bu:0x30b4b4u);return word(s_.gp[4]+0x58u);
    }
    void store_key_base(const Progress&d,std::uint32_t value){
        const auto pc=d.count==1u?0x30b46bu:d.count==2u?0x30b4b4u:0x30b50fu;
        if(d.count==4u){store(pc,s_.gp[4]+0x58u,value,[]{});return;}
        auto committed_flags=s_.flags;committed_flags.plus(d.base,d.encoded,4);
        store(pc,s_.gp[4]+0x58u,value,[&]{s_.flags=committed_flags;},true);
    }
    void store_property(const Progress&d,unsigned slot,std::uint32_t value){
        const auto pc=d.count==1u?0x30b522u:d.count==2u?0x30b538u:0x30b563u;
        store(pc,s_.gp[4]+0x5cu+slot*4u,value,[]{});
    }
    void finish_property(const Progress&){
        auto&g=s_.gp;const auto preserved=load(0,0x30b56du,g[4]+0x5cu);
        g[2]=g[4]+0x40u;s_.flags.minus(g[3],g[2],4);
        if(g[3]!=g[4]+0x40u){
            constexpr std::array reads{0x30b581u,0x30b588u,0x30b58fu},writes{0x30b584u,0x30b58bu,0x30b592u};
            constexpr std::array registers{1u,2u,1u};
            copy_variables(0x30b579u,0x30b57cu,reads,writes,registers);
        }
        store(0x30b59au,g[4]+0x5cu,preserved,[]{});
    }
    void advance(const Progress&d,std::uint32_t program){
        auto&g=s_.gp;
        if(d.family==Family::jump){
            if(d.count==2u){g[1]=native_mopp_aabb_abi_detail::shifted_immediate<8>(s_,g[1]);g[2]=s_.flags.plus(g[2],d.program,4);}
            else if(d.count==3u){combine(0,1);g[0]=native_mopp_aabb_abi_detail::shifted_immediate<8>(s_,g[0]);g[2]=s_.flags.plus(g[2],d.program,4);}
            g[6]=program;
        }else g[6]=s_.flags.plus(d.program,d.count+1u,4);
    }
    Entry next(){
        auto&g=s_.gp;const auto opcode=load(0,0x30ae02u,g[6],1);
        s_.flags.minus(opcode,0x6bu,4);store(0x30ae08u,g[4]+0x3cu,999u,[]{});
        return Entry{0,g[6],opcode,g[3],g[7],opcode<=0x6bu};
    }
};
} // namespace halo2::native_mopp_ray_caller_detail

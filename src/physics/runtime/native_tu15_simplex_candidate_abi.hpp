#pragma once
#include "halo2/native_tu15_simplex_candidate_kernel.hpp"
#include "physics/runtime/native_tu15_simplex_direction_abi.hpp"

namespace halo2::native_simplex_candidate_detail {
using Data=NativeTu15SimplexCandidate;
using Side=NativeTu15SimplexCandidateSide;
using P=NativeTu15SimplexCandidatePhase;
using Noop=native_simplex_direction_detail::Noop;
template<class State>std::uint32_t shift4(State&s,std::uint32_t value){
    const auto context=s.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);
    std::uint32_t overflow{};
    // This source uses the immediate4 encoding. Seed its complete incoming
    // arithmetic context before recording hardware-defined/undefined bits.
    asm volatile("addb $127,%%al;sahf;shll $4,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"=c"(overflow),"+r"(value)::"cc");
    s.flags.load((flags&0xd5u)|((overflow&1u)<<11u));return value;
}
// Private/unbound continuation. This does not own or fabricate a shape virtual
// support call. State is the true input resumed by that actual child.
template<class State,class Before=Noop>struct Access {
    State&s;native_raw_x87_detail::Scope<State>&fp;Before before{};
    std::uint32_t last_branch{};
    bool second(const Data&d)const{return d.side==Side::second;}
    std::uint32_t first_pc(const Data&d,std::uint32_t pc)const{
        if(d.side!=Side::first_late)return pc;
        // The last Jcc is short in the late copy. Its count store and JMP
        // therefore have a different delta; authenticate explicit phase PCs.
        switch(pc){
        case 0x30eeb1u:return 0x30f491u;case 0x30eeb3u:return 0x30f493u;
        case 0x30eeb7u:return 0x30f497u;case 0x30eec0u:return 0x30f4a0u;
        case 0x30eeeau:return 0x30f4cau;case 0x30eeeeu:return 0x30f4ceu;
        case 0x30eef4u:return 0x30f4d4u;case 0x30eef8u:return 0x30f4d8u;
        case 0x30ef01u:return 0x30f4e1u;case 0x30ef0fu:return 0x30f4efu;
        case 0x30ef15u:return 0x30f4f5u;case 0x30ef3fu:return 0x30f51fu;
        case 0x30ef43u:return 0x30f523u;case 0x30ef49u:return 0x30f529u;
        case 0x30ef4du:return 0x30f52du;case 0x30ef56u:return 0x30f536u;
        case 0x30ef5cu:return 0x30f53cu;case 0x30ef80u:return 0x30f560u;
        case 0x30ef84u:return 0x30f564u;case 0x30ef8au:return 0x30f56au;
        case 0x30ef8eu:return 0x30f56eu;case 0x30ef97u:return 0x30f577u;
        case 0x30ef9eu:return 0x30f57au;case 0x30efa0u:return 0x30f57cu;
        default:throw std::runtime_error("Unbound late simplex candidate phase");
        }
    }
    void at(std::uint32_t pc){
        if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;
        fp.io([&]{before(s,pc);});
    }
    std::uint32_t score_address(const Data&d)const{
        return s.gp[4]+(second(d)?0x34u:(d.side==Side::first_early?0x28u:0x40u))+d.comparison*4u;
    }
    std::uint32_t word_read(P,const Data&d,std::uint32_t address){
        at(second(d)?0x30f315u:first_pc(d,0x30eeb1u));std::uint32_t value;
        fp.io([&]{value=physics_step_detail::r(s.memory,address,4);});s.gp[1]=value;return value;
    }
    __m128 vector_read(P phase,const Data&d,std::uint32_t address){
        std::uint32_t pc{};unsigned r{};
        switch(phase){
        case P::first:pc=second(d)?0x30f317u:first_pc(d,0x30eeb3u);r=second(d)?0u:1u;break;
        case P::direction:pc=second(d)?0x30f31eu:first_pc(d,0x30eeb7u);r=2u;break;
        case P::candidate:
            s.gp[0]=(d.count+(second(d)?14u:2u));
            s.gp[0]=shift4(s,s.gp[0]);
            pc=second(d)?0x30f329u:first_pc(d,0x30eec0u);r=second(d)?1u:0u;break;
        case P::third:pc=second(d)?0x30f381u:first_pc(d,0x30ef15u);r=second(d)?0u:1u;break;
        case P::second:pc=second(d)?0x30f3cbu:first_pc(d,0x30ef5cu);r=second(d)?0u:1u;break;
        default:throw std::runtime_error("Invalid simplex candidate vector phase");
        }
        at(pc);if(address&15u)throw std::runtime_error("Native simplex candidate MOVAPS alignment");
        __m128 value;fp.io([&]{value=physics_step_detail::v(s.memory,address);});s.xmm[r]=value;return value;
    }
    void prepare(P,const Data&d){
        s.gp[0]=s.flags.minus(s.gp[1],2u,4);
        if(s.gp[0])s.gp[0]=s.flags.increment(s.gp[0],4,true);
        last_branch=second(d)?0x30f37bu:first_pc(d,0x30ef0fu);
    }
    void bytes(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        std::array<std::byte,4>written{};std::memcpy(written.data(),&value,4);
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s.observe){event.before.resize(4);fp.io([&]{s.memory.read(address,event.before);});}
        fp.io([&]{s.memory.write(address,written);});
        if(s.observe){event.written.assign(written.begin(),written.end());fp.io([&]{s.observe(event);});}
    }
    void score_write(const Data&d,std::uint32_t bits){
        auto&x=s.xmm;const bool b=second(d);const bool last=d.comparison==2u;
        if(!last){
            x[b?0u:1u]=d.products;x[3]=d.xy;x[4]=d.sum;
            if(b)s.gp[2]=d.candidate_address;
        }else{
            x[b?1u:0u]=d.products;x[b?0u:1u]=d.xy;x[2]=d.sum;
        }
        const auto address=score_address(d);
        if(b)s.gp[0]=address;
        else s.gp[d.comparison==1u?0u:2u]=address;
        const std::array first_sites{0x30eeeau,0x30ef3fu,0x30ef80u};
        const std::array second_sites{0x30f356u,0x30f3aeu,0x30f3f2u};
        const auto pc=b?second_sites[d.comparison]:first_pc(d,first_sites[d.comparison]);
        at(pc);bytes(pc,address,bits);
    }
    std::uint16_t compare_score(const Data&d){
        const std::array first_load{0x30eeeeu,0x30ef43u,0x30ef84u};
        const std::array first_multiply{0x30eef4u,0x30ef49u,0x30ef8au};
        const std::array first_compare{0x30eef8u,0x30ef4du,0x30ef8eu};
        const std::array first_branch{0x30ef01u,0x30ef56u,0x30ef97u};
        const std::array second_load{0x30f35au,0x30f3b2u,0x30f3f6u};
        const std::array second_multiply{0x30f360u,0x30f3b8u,0x30f3fcu};
        const std::array second_compare{0x30f364u,0x30f3bcu,0x30f400u};
        const std::array second_branch{0x30f36du,0x30f3c5u,0x30f409u};
        const auto pc=[&](const auto&a,const auto&b){return second(d)?b[d.comparison]:first_pc(d,a[d.comparison]);};
        const auto read=[&](std::uint32_t site,std::uint32_t address){at(site);std::uint32_t bits;fp.io([&]{bits=physics_step_detail::r(s.memory,address,4);});return bits;};
        auto bits=read(pc(first_load,second_load),score_address(d));fp.require_capacity(1u);
        asm volatile("flds %0;fabs"::"m"(bits):"memory","st");
        bits=read(pc(first_multiply,second_multiply),score_address(d));
        asm volatile("fmuls %0"::"m"(bits):"memory","st");
        bits=read(pc(first_compare,second_compare),s.gp[4]+0x10u);
        std::uint16_t relation;asm volatile("fcomps %1;fnstsw %0":"=a"(relation):"m"(bits):"memory","st");
        fp.publish();s.set(0,relation,2,0);
        s.flags.logic((relation>>8u)&(d.comparison?5u:0x41u),1);
        last_branch=pc(first_branch,second_branch);return relation;
    }
    void word_write(P,const Data&d,std::uint32_t address,std::uint32_t value){
        s.gp[1]=s.flags.increment(s.gp[1],4,false);
        const auto pc=first_pc(d,0x30ef9eu);at(pc);bytes(pc,address,value);
    }
    void complete(const Data&d){
        const auto pc=d.accepted&&!second(d)?first_pc(d,0x30efa0u):last_branch;
        if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;
    }
};
template<class State,class Before=Noop>
bool run_private_stage(State&s,Side side,Before before={}){
    native_raw_x87_detail::Scope<State>fp{s};Access<State,Before>memory{s,fp,std::move(before)};
    Data data;data.side=side;data.work=s.gp[6];
    data.count_address=side==Side::second?s.gp[7]:s.gp[6];
    data.direction_address=side==Side::second?s.gp[4]+0x70u:s.gp[3];
    return native_tu15_test_simplex_candidate(memory,data);
}
} // namespace halo2::native_simplex_candidate_detail

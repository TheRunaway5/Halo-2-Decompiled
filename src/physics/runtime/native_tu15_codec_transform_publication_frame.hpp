#pragma once
#include "halo2/native_tu15_codec_transform_publication_kernel.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_transform_chain_frame.hpp"
#include "physics/runtime/native_tu15_codec_transform_packed_frame.hpp"

namespace halo2::native_codec_transform_publication_detail {
using native_codec_quaternion_product_detail::Io;
using native_codec_quaternion_product_detail::Noop;
template<class State,class Transport>class Schedule {
    State& s_;
    Transport& io_;
    static std::uint32_t signed_word(std::uint32_t value) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    }
    void word(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        io_.load(pc,address,4u,[&](std::uint32_t value){s_.gp[reg]=value;});
    }
    void signed_half(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        io_.load(pc,address,2u,[&](std::uint32_t value){s_.gp[reg]=signed_word(value);});
    }
    void low_byte(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        io_.load(pc,address,1u,[&](std::uint32_t value){s_.set(reg,value,1u,0u);});
    }
    template<unsigned Count,bool Right=false>std::uint32_t shift(std::uint32_t value) {
        if constexpr(!Transport::physical) {
            if constexpr(Right)return s_.flags.right(value,Count,4u,true);
            else return s_.flags.shift(value,Count,4u);
        } else {
            const auto context=s_.flags.bits();auto seed=((context&0xd5u)<<8u)|((context>>11u)&1u);
            unsigned overflow{};
            if constexpr(Right)
                asm volatile("addb $127,%%al;sahf;sarl %3,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
                    :"+&a"(seed),"=c"(overflow),"+&r"(value):"i"(Count):"cc");
            else
                asm volatile("addb $127,%%al;sahf;shll %3,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
                    :"+&a"(seed),"=c"(overflow),"+&r"(value):"i"(Count):"cc");
            s_.flags.load((seed&0xd5u)|((overflow&1u)<<11u));return value;
        }
    }
    std::uint32_t shift_count(std::uint32_t value,std::uint32_t count) {
        if constexpr(!Transport::physical)return s_.flags.shift(value,count,4u);
        else {
            const auto context=s_.flags.bits();auto seed=((context&0xd5u)<<8u)|((context>>11u)&1u);
            unsigned overflow{};
            asm volatile("addb $127,%%al;sahf;shll %%cl,%2;seto %%dl;lahf;movzbl %%ah,%%eax"
                :"+&a"(seed),"=&d"(overflow),"+&r"(value):"c"(count):"cc");
            s_.flags.load((seed&0xd5u)|((overflow&1u)<<11u));return value;
        }
    }
public:
    Schedule(State& state,Transport& io):s_(state),io_(io){}
    bool first_output() {
        signed_half(0x283f7bu,3u,s_.gp[3]+6u);io_.note(0x283f7fu);s_.flags.logic(s_.gp[3],4u);
        io_.note(0x283f81u);
        return s_.flags.sf==s_.flags.of;
    }
    bool output_in_range() {
        io_.load(0x283f87u,s_.gp[5]+20u,4u,[&](std::uint32_t value){s_.flags.minus(s_.gp[3],value,4u);});
        io_.note(0x283f8au);
        return s_.flags.sf!=s_.flags.of;
    }
    void copy_result() {
        auto& g=s_.gp;io_.note(0x283f90u);g[7]=g[3];
        io_.note(0x283f92u);g[7]=shift<5u>(g[7]);
        io_.note(0x283f95u);g[7]=s_.flags.plus(g[7],0x50ef10u,4u);
        io_.note(0x283f9bu);g[1]=8u;io_.note(0x283fa0u);g[6]=g[4]+144u;
        io_.repeat(0x283fa7u);
    }
    void publish_planes() {
        auto& g=s_.gp;
        word(0x283fa9u,6u,g[4]+28u);low_byte(0x283fadu,0u,g[6]+2u);
        io_.note(0x283fb0u);g[1]=g[3];io_.note(0x283fb2u);g[1]=s_.flags.logic(g[1]&31u,4u);
        io_.note(0x283fb5u);g[2]=1u;io_.note(0x283fbau);g[2]=shift_count(g[2],g[1]&255u);
        io_.note(0x283fbcu);s_.flags.logic(g[0]&1u,1u);io_.note(0x283fbeu);g[0]=g[3];
        io_.note(0x283fc0u);
        if(!s_.flags.zf) {
            io_.note(0x283fc2u);g[0]=shift<5u,true>(g[0]);io_.note(0x283fc5u);g[0]=shift<2u>(g[0]);
            io_.or_word(0x283fc8u,0x56e000u+g[0],g[2]);
            io_.note(0x283fceu);
        } else {
            io_.note(0x283fd0u);g[0]=shift<5u,true>(g[0]);io_.note(0x283fd3u);g[0]=shift<2u>(g[0]);
            word(0x283fd6u,7u,0x56e000u+g[0]);io_.note(0x283fdcu);g[1]=g[2];
            io_.note(0x283fdeu);g[1]=~g[1];io_.note(0x283fe0u);g[7]=s_.flags.logic(g[7]&g[1],4u);
            io_.store(0x283fe2u,0x56e000u+g[0],4u,g[7]);
        }

        low_byte(0x283fe8u,1u,g[6]+2u);io_.note(0x283febu);s_.set(1,s_.flags.right(g[1]&255u,1u,1u,false),1u,0u);
        io_.note(0x283fedu);s_.flags.logic(g[1]&1u,1u);io_.note(0x283ff0u);
        if(!s_.flags.zf) {
            io_.or_word(0x283ff2u,0x56e020u+g[0],g[2]);io_.note(0x283ff8u);
        }
        else {
            word(0x283ffau,6u,0x56e020u+g[0]);io_.note(0x284000u);g[1]=g[2];
            io_.note(0x284002u);g[1]=~g[1];io_.note(0x284004u);g[6]=s_.flags.logic(g[6]&g[1],4u);
            io_.store(0x284006u,0x56e020u+g[0],4u,g[6]);
        }

        word(0x28400cu,1u,g[4]+28u);low_byte(0x284010u,1u,g[1]+2u);
        io_.note(0x284013u);s_.set(1,s_.flags.right(g[1]&255u,2u,1u,false),1u,0u);
        io_.note(0x284016u);s_.flags.logic(g[1]&1u,1u);
        word(0x284019u,1u,0x56e040u+g[0]);
        io_.note(0x28401fu);
        if(!s_.flags.zf) {
            io_.note(0x284021u);g[1]=s_.flags.logic(g[1]|g[2],4u);io_.note(0x284023u);
        } else {
            io_.note(0x284025u);g[2]=~g[2];io_.note(0x284027u);g[1]=s_.flags.logic(g[1]&g[2],4u);
        }
        io_.store(0x284029u,0x56e040u+g[0],4u,g[1]);
        io_.store(0x28402fu,g[4]+27u,1u,1u);
    }
    bool next_output() {
        auto& g=s_.gp;word(0x284034u,2u,g[5]+8u);word(0x284037u,0u,g[2]+16u);
        io_.note(0x28403au);g[3]=shift<5u>(g[3]);signed_half(0x28403du,3u,g[3]+g[0]+4u);
        io_.note(0x284042u);s_.flags.logic(g[3],4u);io_.note(0x284044u);
        return s_.flags.sf==s_.flags.of;
    }
};

template<class State,class Before>class PhysicalIo:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>> {
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::io;
public:
    static constexpr bool physical=true;
    PhysicalIo(State& state,Before& before,Scope& scope):Base{state,before,{&scope}}{}
    void note(std::uint32_t) {}
    template<class Commit>void load(std::uint32_t pc,std::uint32_t address,unsigned width,Commit commit) {
        at(pc);commit(word(address,width));
    }
    void store(std::uint32_t pc,std::uint32_t address,unsigned width,std::uint32_t value) {
        if(width==4u){Base::store(pc,address,value,[]{});return;}
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(1);io([&]{s_.memory.read(address,event.before);});}
        const std::array bytes{std::byte(value)};io([&]{s_.memory.write(address,bytes);});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());io([&]{s_.observe(event);});}
    }
    void or_word(std::uint32_t pc,std::uint32_t address,std::uint32_t mask) {
        at(pc);const auto prior=word(address);auto committed=s_.flags;
        const auto value=committed.logic(prior|mask,4u);
        // A backing provider failure preserves pre-instruction flags.
        // A completed observer sees the raw OR's published flags.
        Base::store(pc,address,value,[&]{s_.flags=committed;},true);
    }
    void repeat(std::uint32_t pc) {
        auto& g=s_.gp;
        while(g[1]) {
            at(pc);const auto value=word(g[6]);
            Base::store(pc,g[7],value,[&] {
                const auto delta=(s_.other_flags&0x400u)?0xfffffffcu:4u;
                g[6]+=delta;g[7]+=delta;--g[1];
            },true);
        }
    }
};

// The authored source computes OR flags before the store helper, and its
// REP observer runs before the live cursors advance. Keep those contracts
// separate from the raw CPU publication above.
template<bool Prepared,class State,class Checkpoint>class SourceIo {
    State& s_;Checkpoint& checkpoint_;
    void direct(std::uint32_t pc){if(s_.services.instruction)s_.services.instruction(pc);}
    void at(std::uint32_t pc){if constexpr(Prepared)s_.instruction_literal(pc);else direct(pc);}
    void retired(std::uint32_t pc){checkpoint_(s_,pc);}
public:
    static constexpr bool physical=false;
    SourceIo(State& state,Checkpoint& checkpoint):s_(state),checkpoint_(checkpoint){}
    void note(std::uint32_t pc){at(pc);}
    template<class Commit>void load(std::uint32_t pc,std::uint32_t address,unsigned width,Commit commit) {
        at(pc);commit(physics_step_detail::r(s_.memory,address,width));retired(pc);
    }
    void store(std::uint32_t pc,std::uint32_t address,unsigned width,std::uint32_t value) {
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,width,value);retired(pc);
    }
    void or_word(std::uint32_t pc,std::uint32_t address,std::uint32_t mask) {
        at(pc);const auto prior=physics_step_detail::r(s_.memory,address,4u);
        physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,s_.flags.logic(prior|mask,4u));retired(pc);
    }
    void repeat(std::uint32_t pc) {
        auto& g=s_.gp;at(pc);
        while(g[1]) {
            const auto value=physics_step_detail::r(s_.memory,g[6],4u);
            physics_step_detail::w(s_.memory,s_.observe,pc,g[7],4u,value);
            const auto delta=(s_.other_flags&0x400u)?0xfffffffcu:4u;
            // Operand preparation retags the outer statement only. The
            // authored trailing callback is direct, even after ECX reaches 0.
            g[6]+=delta;g[7]+=delta;--g[1];retired(pc);direct(pc);
        }
    }
};

template<class State,class Before=Noop>void run_private_complete(State& state,Before before={}) {
    native_integer_fp_detail::Scope scope{state};
    native_codec_transform_chain_detail::Frame ancestry{state,before,scope};
    native_codec_transform_packed_detail::Frame packed{state,before,scope};
    PhysicalIo io{state,before,scope};Schedule output{state,io};
    compose_rows(ancestry,packed,output);
}

template<bool Prepared=false,class State,class Checkpoint=Noop>
void run_private_source_suffix(State& state,Checkpoint checkpoint={}) {
    SourceIo<Prepared,State,Checkpoint> io{state,checkpoint};Schedule output{state,io};publish_result(output);
}
} // namespace halo2::native_codec_transform_publication_detail
#endif

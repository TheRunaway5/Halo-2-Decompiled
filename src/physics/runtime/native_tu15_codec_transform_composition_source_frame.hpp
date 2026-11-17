#pragma once
#include "physics/runtime/native_tu15_codec_transform_composition_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_codec_transform_composition_detail {
// Private retained-provider view. Unlike the physical adapter, authored PUSH
// predecrements SP and REP retires the write before advancing live SI/DI/CX
// and delivering its trailing repeated instruction callback.
template<bool Ordered,class State,class Before,class Checkpoint,class Call>class SourceFrame {
    State& s_;Before& before_;Checkpoint& checkpoint_;Call& call_;
    using Product=native_codec_quaternion_product_detail::Frame<State,Before>;
    void at(std::uint32_t pc){s_.instruction_literal(pc);before_(s_,pc);}
    void retired(std::uint32_t pc){checkpoint_(s_,pc);}
    std::uint32_t word(std::uint32_t address){return physics_step_detail::r(s_.memory,address);}
    static __m128 scalar(std::uint32_t bits) {
        return std::bit_cast<__m128>(std::array<std::uint32_t,4>{bits,0,0,0});
    }
    std::uint32_t address(Field field,unsigned offset)const {
        switch(field) {
        case Field::left:return s_.gp[3]+offset;
        case Field::right:return s_.gp[5]+offset;
        case Field::output:return s_.gp[2]+offset;
        case Field::two:return 0x46a11cu;
        case Field::one:return 0x46a0d0u;
        }
        throw std::runtime_error("Invalid retained composition field");
    }
    static __m128 mul(__m128 left,__m128 right) {
        if constexpr(Ordered)return Product::multiply(left,right);
        else return _mm_mul_ss(left,right);
    }
    static __m128 sum(__m128 left,__m128 right) {
        if constexpr(Ordered)return Product::add(left,right);
        else return _mm_add_ss(left,right);
    }
    static __m128 sub(__m128 left,__m128 right) {
        if constexpr(Ordered)return Product::subtract(left,right);
        else return _mm_sub_ss(left,right);
    }
    void push(std::uint32_t pc,std::uint32_t value){at(pc);s_.push(pc,value);retired(pc);}
    void pop(std::uint32_t pc,unsigned reg){at(pc);s_.gp[reg]=s_.pop();retired(pc);}
    void stage(unsigned input,std::uint32_t pc) {
        auto& g=s_.gp;g[6]=g[input];g[1]=8u;g[7]=g[4]+16u;at(pc);
        while(g[1]) {
            const auto value=word(g[6]);
            physics_step_detail::w(s_.memory,s_.observe,pc,g[7],4u,value);
            const auto delta=(s_.other_flags&0x400u)?0xfffffffcu:4u;
            g[6]+=delta;g[7]+=delta;--g[1];
            // A handoff here resumes the exact repeat callback, including
            // the final callback when live CX has become zero.
            retired(pc);s_.instruction_literal(pc);
        }
        g[input]=g[4]+16u;
    }
public:
    SourceFrame(State& state,Before& before,Checkpoint& checkpoint,Call& call):
        s_(state),before_(before),checkpoint_(checkpoint),call_(call){}
    bool begin() {
        auto& g=s_.gp;g[4]=s_.flags.minus(g[4],32u,4u);
        push(0x120573u,g[3]);at(0x120574u);g[3]=word(g[4]+40u);retired(0x120574u);
        s_.flags.minus(g[3],g[2],4u);push(0x12057au,g[5]);
        at(0x12057bu);g[5]=word(g[4]+48u);retired(0x12057bu);
        push(0x12057fu,g[6]);push(0x120580u,g[7]);return s_.flags.zf;
    }
    void stage_left(){stage(3u,0x12058eu);}
    bool right_aliases_output(){s_.flags.minus(s_.gp[5],s_.gp[2],4u);return s_.flags.zf;}
    void stage_right(){stage(5u,0x1205a3u);}
    void quaternion_product() {
        auto& g=s_.gp;g[0]=g[5];g[1]=g[3];
        at(0x1205adu);s_.push(0x1205adu,0x1205b2u);
        call_(s_,0x1201e0u,0x1205adu);retired(0x1205adu);
    }
    void load(unsigned destination,std::uint32_t pc,Field field,unsigned offset) {
        at(pc);s_.xmm[destination]=scalar(word(address(field,offset)));retired(pc);
    }
    void multiply(unsigned destination,std::uint32_t pc,Field field,unsigned offset) {
        at(pc);const auto operand=scalar(word(address(field,offset)));
        s_.xmm[destination]=mul(s_.xmm[destination],operand);retired(pc);
    }
    void add(unsigned destination,std::uint32_t pc,Field field,unsigned offset) {
        at(pc);const auto operand=scalar(word(address(field,offset)));
        s_.xmm[destination]=sum(s_.xmm[destination],operand);retired(pc);
    }
    void multiply(unsigned destination,unsigned source){s_.xmm[destination]=mul(s_.xmm[destination],s_.xmm[source]);}
    void add(unsigned destination,unsigned source){s_.xmm[destination]=sum(s_.xmm[destination],s_.xmm[source]);}
    void subtract(unsigned destination,unsigned source){s_.xmm[destination]=sub(s_.xmm[destination],s_.xmm[source]);}
    void copy(unsigned destination,unsigned source){s_.xmm[destination]=s_.xmm[source];}
    void store(std::uint32_t pc,unsigned offset,unsigned source) {
        at(pc);const auto bits=std::bit_cast<std::array<std::uint32_t,4>>(s_.xmm[source])[0];
        physics_step_detail::w(s_.memory,s_.observe,pc,s_.gp[2]+offset,4u,bits);retired(pc);
    }
    bool scale_differs_from_one() {
        s_.flags.scalar_compare(s_.xmm[0],s_.xmm[5],true);
        s_.set(0,s_.flags.bits()|2u,1u,8u);s_.flags.logic((s_.gp[0]>>8u)&0x44u,1u);return s_.flags.pf;
    }
    void copy_right_scale() {
        at(0x1206f7u);s_.gp[0]=word(s_.gp[5]+28u);retired(0x1206f7u);
        at(0x1206fau);physics_step_detail::w(s_.memory,s_.observe,0x1206fau,s_.gp[2]+28u,4u,s_.gp[0]);
        retired(0x1206fau);
    }
    void restore_index_registers(){pop(0x120716u,7u);pop(0x120717u,6u);}
    void restore_frame_register(){pop(0x120727u,5u);}
    void finish() {
        pop(0x12072du,3u);s_.gp[4]=s_.flags.plus(s_.gp[4],32u,4u);
        at(0x120731u);s_.pop();s_.gp[4]+=8u;
    }
};

template<bool Ordered,class State,class Call,class Before,class Checkpoint>
void run_private_source(State& state,Call call,Before before,Checkpoint checkpoint) {
    SourceFrame<Ordered,State,Before,Checkpoint,Call> view{state,before,checkpoint,call};compose_transform(view);
}
} // namespace halo2::native_codec_transform_composition_detail
#endif

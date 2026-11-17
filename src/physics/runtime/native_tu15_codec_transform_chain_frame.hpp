#pragma once
#include "halo2/native_tu15_codec_transform_chain_kernel.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_inverse_transform_frame.hpp"
#include "physics/runtime/native_tu15_codec_transform_composition_frame.hpp"

namespace halo2::native_codec_transform_chain_detail {
using native_codec_quaternion_product_detail::Io;
using native_codec_quaternion_product_detail::Noop;

template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>> {
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::before_;using Base::io;
    Scope& scope_;
    static std::uint32_t signed_word(std::uint32_t value) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    }
    std::uint32_t load(std::uint32_t pc,std::uint32_t address,unsigned width=4u) {
        at(pc);return word(address,width);
    }
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        Base::store(pc,address,value,[]{});
    }
    void store_byte(std::uint32_t pc,std::uint32_t address,std::byte value) {
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(1);io([&]{s_.memory.read(address,event.before);});}
        const std::array bytes{value};io([&]{s_.memory.write(address,bytes);});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());io([&]{s_.observe(event);});}
    }
    template<bool Arithmetic>std::uint32_t shift5(std::uint32_t value) {
        const auto context=s_.flags.bits();auto seed=((context&0xd5u)<<8u)|((context>>11u)&1u);
        unsigned overflow{};
        if constexpr(Arithmetic)
            asm volatile("addb $127,%%al;sahf;sarl $5,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
                :"+&a"(seed),"=c"(overflow),"+&r"(value)::"cc");
        else
            asm volatile("addb $127,%%al;sahf;shll $5,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
                :"+&a"(seed),"=c"(overflow),"+&r"(value)::"cc");
        s_.flags.load((seed&0xd5u)|((overflow&1u)<<11u));return value;
    }
    std::uint32_t shift_count(std::uint32_t value,std::uint32_t count) {
        const auto context=s_.flags.bits();auto seed=((context&0xd5u)<<8u)|((context>>11u)&1u);
        unsigned overflow{};
        asm volatile("addb $127,%%al;sahf;shll %%cl,%2;seto %%dl;lahf;movzbl %%ah,%%eax"
            :"+&a"(seed),"=&d"(overflow),"+&r"(value):"c"(count):"cc");
        s_.flags.load((seed&0xd5u)|((overflow&1u)<<11u));return value;
    }
    void finish_stack(std::uint32_t bx,std::uint32_t bp,std::uint32_t ret) {
        auto& g=s_.gp;
        g[3]=Base::pop(bx);g[4]=g[5];g[5]=Base::pop(bp);
        (void)Base::pop(ret);g[4]+=20u;
    }
public:
    Frame(State& state,Before& before,Scope& scope):Base{state,before,{&scope}},scope_(scope){}
    bool open() {
        auto& g=s_.gp;
        Base::push(0x283e10u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4u);
        g[4]=s_.flags.minus(g[4],0xa4u,4u);
        g[1]=load(0x283e1cu,g[5]+12u);g[0]=load(0x283e1fu,g[1]+100u);
        Base::push(0x283e22u,g[3]);Base::push(0x283e23u,g[6]);
        s_.set(2,s_.flags.logic(0,1u),1u,0u);g[6]=s_.flags.logic(0,4u);
        s_.flags.minus(g[0],g[6],4u);Base::push(0x283e2au,g[7]);
        store_byte(0x283e2bu,g[4]+27u,std::byte(g[2]));
        return !s_.flags.zf&&s_.flags.sf==s_.flags.of;
    }
    void initialize_cursor() {
        auto& g=s_.gp;store(0x283e35u,g[4]+36u,g[6]);store(0x283e39u,g[4]+32u,g[6]);
    }
    bool admit_row() {
        auto& g=s_.gp;
        g[2]=load(0x283e40u,g[4]+32u);g[0]=load(0x283e44u,g[1]+104u);
        g[0]=s_.flags.plus(g[0],g[2],4u);g[2]=load(0x283e49u,g[5]+16u);
        s_.flags.logic(g[2],4u);store(0x283e4eu,g[4]+28u,g[0]);
        g[0]=signed_word(load(0x283e52u,g[0],2u));
        if(!s_.flags.zf) {
            g[7]=load(0x283e57u,g[2]+20u);g[1]=s_.flags.logic(g[0]&31u,4u);
            g[3]=shift_count(1u,g[1]);g[6]=shift5<true>(g[0]);
            s_.flags.logic(load(0x283e6bu,g[7]+g[6]*4u)&g[3],4u);
            if(s_.flags.zf)return false;
            g[1]=load(0x283e74u,g[2]+12u);g[0]=signed_word(load(0x283e77u,g[1]+g[0]*2u,2u));
        }
        s_.flags.logic(g[0],4u);
        if(s_.flags.sf!=s_.flags.of)return false;
        s_.flags.minus(g[0],load(0x283e83u,g[5]+20u),4u);
        return s_.flags.sf!=s_.flags.of;
    }
    void stage_transform() {
        auto& g=s_.gp;
        g[2]=load(0x283e8cu,g[5]+8u);g[3]=load(0x283e8fu,g[2]+16u);g[2]=load(0x283e92u,g[5]+24u);
        g[0]=shift5<false>(g[0]);g[1]=g[0];g[0]=signed_word(load(0x283e9au,g[3]+g[1]+8u,2u));
        g[3]=s_.flags.plus(g[3],g[1],4u);s_.flags.minus(g[0],0xffffffffu,4u);
        g[6]=g[1]+g[2];g[1]=8u;g[7]=g[4]+80u;
        while(g[1]) {
            constexpr auto pc=0x283eb0u;
            at(pc);const auto value=word(g[6]);
            Base::store(pc,g[7],value,[&]{
                const auto delta=(s_.other_flags&0x400u)?0xfffffffcu:4u;
                g[6]+=delta;g[7]+=delta;--g[1];
            },true);
        }
    }
    bool has_parent()const{return !s_.flags.zf;}
    void compose_parent() {
        auto& g=s_.gp;
        g[1]=load(0x283eb4u,g[5]+24u);g[6]=g[0];g[0]=g[4]+80u;
        g[6]=shift5<false>(g[6]);g[2]=g[6]+g[1];
        Base::push(0x283ec3u,g[0]);Base::push(0x283ec4u,g[2]);g[2]=g[0];
        Base::push(0x283ec7u,0x283eccu);
        scope_.child([&]{native_codec_transform_composition_detail::run_private(s_,std::ref(before_));});
        g[0]=load(0x283eccu,g[5]+8u);g[1]=load(0x283ecfu,g[0]+16u);
        g[0]=signed_word(load(0x283ed2u,g[6]+g[1]+8u,2u));s_.flags.minus(g[0],0xffffffffu,4u);
    }
    void inverse_base() {
        auto& g=s_.gp;g[1]=g[4]+112u;g[2]=g[4]+80u;Base::push(0x283ee4u,0x283ee9u);
        scope_.child([&]{native_codec_inverse_transform_detail::run_private(s_,std::ref(before_));});
    }
    bool advance_rejected() {
        auto& g=s_.gp;
        g[0]=load(0x28404au,g[4]+36u);g[6]=load(0x28404eu,g[4]+32u);
        g[1]=load(0x284052u,g[5]+12u);g[2]=load(0x284055u,g[1]+100u);
        g[0]=s_.flags.increment(g[0],4u,false);g[6]=s_.flags.plus(g[6],28u,4u);
        s_.flags.minus(g[0],g[2],4u);store(0x28405eu,g[4]+36u,g[0]);store(0x284062u,g[4]+32u,g[6]);
        return s_.flags.sf!=s_.flags.of;
    }
    void finish_empty() {
        auto& g=s_.gp;
        g[7]=Base::pop(0x284079u);g[6]=Base::pop(0x28407au);s_.set(0,g[2],1u,0u);
        finish_stack(0x28407du,0x284080u,0x284081u);
    }
    void finish_rejected() {
        auto& g=s_.gp;
        s_.set(0,load(0x28406cu,g[4]+27u,1u),1u,0u);
        g[7]=Base::pop(0x284070u);g[6]=Base::pop(0x284071u);
        finish_stack(0x284072u,0x284075u,0x284076u);
    }
};

template<class State,class Before=Noop>Boundary run_private_prefix(State& state,Before before={}) {
    native_integer_fp_detail::Scope scope{state};Frame view{state,before,scope};return compose_ancestry(view);
}
} // namespace halo2::native_codec_transform_chain_detail
#endif

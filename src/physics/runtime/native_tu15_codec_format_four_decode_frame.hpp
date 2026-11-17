#pragma once
#include "halo2/native_tu15_codec_format_four_decode_kernel.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_row_copy_frame.hpp"
#include "physics/runtime/native_tu15_codec_scale_row_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_integer_frame.hpp"

namespace halo2::native_codec_format_four_decode_detail {
using native_codec_rotation_interpolation_detail::Noop;
using native_codec_rotation_interpolation_detail::Io;

struct Lane {
    std::uint32_t bitmap,bitmap_slot,skip,limit,lower_limit,clamp;
    std::uint32_t call,returned,plane_test,output,counter,count,counter_store;
    std::uint32_t end,cursor,bitmap_reload,ancestor;
    unsigned output_register,limit_register;
};
inline constexpr std::array<Lane,3> lanes{{
    {0x2932d4u,0x2932dau,0x2932e9u,0x2932f9u,0x2932ffu,0x293303u,
     0x293329u,0x29332fu,0x293340u,0x293347u,0x2933cau,0x2933cfu,0x2933d4u,
     0x2933d9u,0x2933e8u,0x2933f4u,0x50ef10u,1u,0u},
    {0x293420u,0x293426u,0x293434u,0x293443u,0x293449u,0x29344du,
     0x293467u,0x29346du,0x29347eu,0x293485u,0x2934b6u,0x2934bcu,0x2934c1u,
     0x2934c7u,0x2934d6u,0x2934ddu,0x50ef24u,0u,1u},
    {0x293503u,0x293509u,0x293517u,0x293526u,0x29352cu,0x293530u,
     0x29354au,0x293550u,0x293561u,0x293568u,0x29357du,0x293583u,0x293588u,
     0x29358eu,0x29359du,0x2935a4u,0x50ef2cu,0u,1u},
}};

template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::IntegerFrame<State,Before,Io<native_integer_fp_detail::Scope<State>>> {
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::before_;using Base::io;
    Scope& scope_;

    std::uint32_t load(std::uint32_t pc,std::uint32_t address,unsigned width=4u) {
        at(pc);return word(address,width);
    }
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        Base::store(pc,address,value,[]{});
    }
    bool signed_less()const {return s_.flags.sf!=s_.flags.of;}
    template<bool Right>std::uint32_t shifted(std::uint32_t value,std::uint32_t count) {
        const auto incoming=s_.flags.bits();
        auto seed=((incoming&0xd5u)<<8u)|((incoming>>11u)&1u);
        unsigned overflow{};
        if constexpr(Right)
            asm volatile("addb $127,%%al;sahf;sarl $5,%2;seto %%dl;lahf;movzbl %%ah,%%eax"
                :"+&a"(seed),"=&d"(overflow),"+&r"(value)::"cc");
        else
            asm volatile("addb $127,%%al;sahf;shll %%cl,%2;seto %%dl;lahf;movzbl %%ah,%%eax"
                :"+&a"(seed),"=&d"(overflow),"+&r"(value):"c"(count):"cc");
        s_.flags.load((seed&0xd5u)|((overflow&1u)<<11u));
        return value;
    }
    __m128 vector(std::uint32_t pc,std::uint32_t address) {
        at(pc);
        if(address&15u) {
            const auto flags=s_.other_flags|s_.flags.bits();
            throw native_mopp_ray_caller_detail::AlignedLoadFault{pc,address,flags,flags|0x10000u};
        }
        // This physical backing performs one atomic vector access.
        __m128 value{};
        io([&]{s_.memory.read(address,std::as_writable_bytes(std::span(&value,1)));});
        return value;
    }
    __m128 scalar(std::uint32_t pc,std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(load(pc,address))));
    }
    void store_scalar(std::uint32_t pc,std::uint32_t address,__m128 value) {
        store(pc,address,std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value))));
    }

    void child(std::uint32_t target) {
        namespace rows=native_codec_row_copy_detail;
        switch(target) {
        case 0x2963d0u: {
            rows::Frame<rows::Row::quaternion,State,Before> view{s_,before_,scope_};
            rows::decode(view);return;
        }
        case 0x296410u: {
            rows::Frame<rows::Row::translation,State,Before> view{s_,before_,scope_};
            rows::decode(view);return;
        }
        case 0x2cec60u: {
            native_codec_scale_row_detail::Frame view{s_,before_,scope_};
            native_codec_scale_detail::decode(view);return;
        }
        default:throw std::logic_error("Unqualified TU1.5 format-four decoder child target");
        }
    }

    void rotate() {
        auto& g=s_.gp;auto& x=s_.xmm;
        g[0]=s_.flags.logic(0u,4u);
        store(0x293351u,g[4]+36u,g[1]);store(0x293355u,g[4]+40u,g[1]);
        store(0x293359u,g[4]+44u,g[7]);store(0x29335du,g[4]+48u,g[0]);
        store(0x293361u,g[4]+52u,g[0]);store(0x293365u,g[4]+56u,g[0]);
        store(0x293369u,g[4]+60u,0x80000000u);
        g[0]=load(0x293371u,g[4]+44u);x[0]=vector(0x293375u,g[0]);
        g[0]=load(0x293378u,g[4]+40u);x[1]=vector(0x29337cu,g[0]);
        x[3]=x[0];x[7]=x[1];
        x[0]=_mm_shuffle_ps(x[0],x[0],36);x[3]=_mm_shuffle_ps(x[3],x[3],255);
        x[7]=_mm_shuffle_ps(x[7],x[7],63);
        asm volatile("mulps %1,%0":"+x"(x[3]):"x"(x[1]):"memory");
        asm volatile("mulps %1,%0":"+x"(x[7]):"x"(x[0]):"memory");
        x[0]=_mm_shuffle_ps(x[0],x[0],73);x[6]=x[1];
        x[7]=_mm_xor_ps(x[7],vector(0x29339eu,g[4]+48u));
        x[6]=_mm_shuffle_ps(x[6],x[6],82);
        asm volatile("addps %1,%0":"+x"(x[3]):"x"(x[7]):"memory");
        asm volatile("mulps %1,%0":"+x"(x[6]):"x"(x[0]):"memory");
        x[0]=_mm_shuffle_ps(x[0],x[0],73);
        x[6]=_mm_xor_ps(x[6],vector(0x2933b1u,g[4]+48u));
        x[1]=_mm_shuffle_ps(x[1],x[1],137);
        asm volatile("addps %1,%0":"+x"(x[3]):"x"(x[6]):"memory");
        asm volatile("mulps %1,%0":"+x"(x[0]):"x"(x[1]):"memory");
        g[0]=load(0x2933c0u,g[4]+36u);
        asm volatile("subps %1,%0":"+x"(x[3]):"x"(x[0]):"memory");
        Base::vector_store(0x2933c7u,g[0],x[3]);
    }
    void translate() {
        auto& g=s_.gp;auto& x=s_.xmm;
        x[0]=scalar(0x29348cu,g[7]-4u);
        const auto first=scalar(0x293491u,g[0]+16u);
        asm volatile("addss %1,%0":"+x"(x[0]):"x"(first):"memory");
        store_scalar(0x293496u,g[0]+16u,x[0]);
        x[1]=scalar(0x29349bu,g[7]);
        asm volatile("addss %1,%0":"+x"(x[1]):"x"(x[0]):"memory");
        store_scalar(0x2934a3u,g[0]+16u,x[1]);
        x[0]=scalar(0x2934a8u,g[7]+4u);
        asm volatile("addss %1,%0":"+x"(x[0]):"x"(x[1]):"memory");
        store_scalar(0x2934b1u,g[0]+16u,x[0]);
    }
    void scale() {
        auto& g=s_.gp;auto& x=s_.xmm;
        x[0]=scalar(0x29356fu,g[7]);
        const auto factor=scalar(0x293573u,g[0]+28u);
        asm volatile("mulss %1,%0":"+x"(x[0]):"x"(factor):"memory");
        store_scalar(0x293578u,g[0]+28u,x[0]);
    }
    void apply_ancestor(unsigned lane) {
        auto& g=s_.gp;const auto& p=lanes[lane];
        g[1]=s_.flags.logic(g[6]&31u,4u);
        const unsigned mask=lane?2u:0u, group=lane?0u:1u;
        g[mask]=shifted<false>(1u,g[1]&255u);
        g[group]=shifted<true>(g[6],5u);
        const auto flags=load(p.plane_test,0x56e000u+lane*32u+g[group]*4u);
        s_.flags.logic(flags&g[mask],4u);
        g[p.output_register]=load(p.output,0x510fa0u);
        if(!s_.flags.zf) {
            if(lane==0)rotate();
            else if(lane==1)translate();
            else scale();
        }
    }

public:
    Frame(State& state,Before& before,Scope& scope):Base{state,before,{&scope}},scope_(scope){}
    void begin() {
        auto& g=s_.gp;
        Base::push(0x293290u,g[5]);g[5]=g[4];
        g[4]=s_.flags.logic(g[4]&0xfffffff0u,4u);g[4]=s_.flags.minus(g[4],52u,4u);
        g[2]=load(0x293299u,0x510f3cu);g[1]=load(0x29329fu,0x510f7cu);
        Base::push(0x2932a5u,g[3]);Base::push(0x2932a6u,g[6]);
        g[6]=s_.flags.logic(0u,4u);s_.flags.minus(g[2],g[6],4u);
        Base::push(0x2932abu,g[7]);g[7]=load(0x2932acu,0x510f70u);
        store(0x2932b2u,g[4]+32u,g[2]);store(0x2932b6u,0x510f94u,g[6]);
        store(0x2932bcu,0x510f98u,g[6]);store(0x2932c2u,0x510f9cu,g[6]);
        store(0x2932c8u,0x510fa0u,g[1]);
    }
    void begin_lane(unsigned lane) {
        auto& g=s_.gp;
        if(lane==1) {
            s_.flags.minus(g[2],g[6],4u);
            g[0]=load(0x293404u,0x510f7cu);g[7]=load(0x293409u,0x510f74u);
            store(0x29340fu,0x510fa0u,g[0]);
        } else {
            g[0]=load(0x2934e9u,0x510f7cu);g[7]=load(0x2934eeu,0x510f78u);
            g[6]=s_.flags.logic(0u,4u);s_.flags.logic(g[2],4u);
            store(0x2934f8u,0x510fa0u,g[0]);
        }
    }
    bool has_tracks()const {return !s_.flags.zf&&!signed_less();}
    bool load_bitmap(unsigned lane) {
        auto& g=s_.gp;const auto& p=lanes[lane];
        g[3]=load(p.bitmap,g[7],1u);g[7]=s_.flags.increment(g[7],4u,false);
        s_.flags.logic(g[3],4u);store(p.bitmap_slot,g[4]+24u,g[7]);
        return !s_.flags.zf;
    }
    void skip_bitmap(unsigned lane) {
        auto& g=s_.gp;const auto& p=lanes[lane];auto& cursor=g[p.output_register];
        g[6]=s_.flags.plus(g[6],8u,4u);cursor=s_.flags.plus(cursor,256u,4u);
        store(p.skip,0x510fa0u,cursor);
    }
    bool begin_bits(unsigned lane) {
        auto& g=s_.gp;const auto& p=lanes[lane];
        auto& end=g[p.limit_register];
        end=g[6]+8u;s_.flags.minus(end,g[2],4u);store(p.limit,g[4]+28u,g[2]);
        if(s_.flags.zf||signed_less())store(p.lower_limit,g[4]+28u,end);
        const auto limit=load(p.clamp,g[4]+28u);s_.flags.minus(g[6],limit,4u);
        if(!signed_less())return false;
        g[7]=shifted<false>(g[6],5u);g[7]=s_.flags.plus(g[7],p.ancestor,4u);
        return true;
    }
    bool bit_set(){s_.flags.logic(s_.gp[3]&1u,1u);return !s_.flags.zf;}
    void decode_track(unsigned lane) {
        auto& g=s_.gp;const auto& p=lanes[lane];
        const auto target=load(p.call,0x510f64u+lane*4u);
        const auto stack_slot=g[4]-4u;
        Base::store(p.call,stack_slot,p.returned,[&]{g[4]=stack_slot;},true);
        scope_.child([&]{child(target);});
        at(p.returned);
        apply_ancestor(lane);
        auto& counter=g[p.limit_register];
        counter=load(p.counter,0x510f94u+lane*4u);g[2]=load(p.count,g[4]+32u);
        counter=s_.flags.increment(counter,4u,false);store(p.counter_store,0x510f94u+lane*4u,counter);
    }
    void finish_bit(unsigned lane) {
        auto& g=s_.gp;const auto& p=lanes[lane];auto& cursor=g[p.output_register];
        g[p.limit_register]=load(p.end,g[4]+28u);
        cursor=s_.flags.plus(cursor,32u,4u);g[3]=s_.flags.right(g[3],1u,4u,true);
        g[6]=s_.flags.increment(g[6],4u,false);g[7]=s_.flags.plus(g[7],32u,4u);
        s_.flags.minus(g[6],g[p.limit_register],4u);store(p.cursor,0x510fa0u,cursor);
    }
    bool more_tracks()const{return signed_less();}
    void reload_bitmap(unsigned lane){auto& g=s_.gp;g[7]=load(lanes[lane].bitmap_reload,g[4]+24u);}
    bool more_bitmaps(){s_.flags.minus(s_.gp[6],s_.gp[2],4u);return signed_less();}
    void end_lane(unsigned lane){if(!lane)s_.gp[6]=s_.flags.logic(0u,4u);}
    void finish() {
        auto& g=s_.gp;
        g[7]=Base::pop(0x2935b0u);g[6]=Base::pop(0x2935b1u);g[3]=Base::pop(0x2935b2u);
        g[4]=g[5];g[5]=Base::pop(0x2935b5u);(void)Base::pop(0x2935b6u);
    }
};

template<class State,class Before=Noop>void run_private(State& state,Before before={}) {
    native_integer_fp_detail::Scope scope{state};
    Frame view{state,before,scope};native_codec_format_four_decode_detail::decode(view);
}
} // namespace halo2::native_codec_format_four_decode_detail
#endif

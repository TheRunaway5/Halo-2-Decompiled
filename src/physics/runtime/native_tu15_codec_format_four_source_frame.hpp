#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_format_four_decode_frame.hpp"
#include "physics/runtime/native_tu15_codec_format_four_math.hpp"
#include <algorithm>

namespace halo2::native_codec_format_four_source_detail {
namespace math=native_codec_format_four_math_detail;
using native_codec_format_four_decode_detail::lanes;

// The physical decoder and retained owner share the bitmap controller. This
// transport keeps the retained source's ordered scalar/vector memory helpers,
// PUSH-before-I/O publication, and original dispatch on the current State.
inline constexpr std::pair<std::uint32_t,std::uint32_t> retirements[]{
    {0x293290u,0x293291u},{0x293299u,0x29329fu},{0x29329fu,0x2932a5u},{0x2932a5u,0x2932a6u},
    {0x2932a6u,0x2932a7u},{0x2932abu,0x2932acu},{0x2932acu,0x2932b2u},{0x2932b2u,0x2932b6u},
    {0x2932b6u,0x2932bcu},{0x2932bcu,0x2932c2u},{0x2932c2u,0x2932c8u},{0x2932c8u,0x2932ceu},
    {0x2932d4u,0x2932d7u},{0x2932dau,0x2932deu},{0x2932e9u,0x2932efu},{0x2932f9u,0x2932fdu},
    {0x2932ffu,0x293303u},{0x293303u,0x293307u},{0x293329u,0x29332fu},{0x293340u,0x293347u},
    {0x293347u,0x29334du},{0x293351u,0x293355u},{0x293355u,0x293359u},{0x293359u,0x29335du},
    {0x29335du,0x293361u},{0x293361u,0x293365u},{0x293365u,0x293369u},{0x293369u,0x293371u},
    {0x293371u,0x293375u},{0x293375u,0x293378u},{0x293378u,0x29337cu},{0x29337cu,0x29337fu},
    {0x29339eu,0x2933a3u},{0x2933b1u,0x2933b6u},{0x2933c0u,0x2933c4u},{0x2933c7u,0x2933cau},
    {0x2933cau,0x2933cfu},{0x2933cfu,0x2933d3u},{0x2933d4u,0x2933d9u},{0x2933d9u,0x2933ddu},
    {0x2933e8u,0x2933eeu},{0x2933f4u,0x2933f8u},{0x293404u,0x293409u},{0x293409u,0x29340fu},
    {0x29340fu,0x293414u},{0x293420u,0x293423u},{0x293426u,0x29342au},{0x293434u,0x293439u},
    {0x293443u,0x293447u},{0x293449u,0x29344du},{0x29344du,0x293451u},{0x293467u,0x29346du},
    {0x29347eu,0x293485u},{0x293485u,0x29348au},{0x29348cu,0x293491u},{0x293491u,0x293496u},
    {0x293496u,0x29349bu},{0x29349bu,0x29349fu},{0x2934a3u,0x2934a8u},{0x2934a8u,0x2934adu},
    {0x2934b1u,0x2934b6u},{0x2934b6u,0x2934bcu},{0x2934bcu,0x2934c0u},{0x2934c1u,0x2934c7u},
    {0x2934c7u,0x2934cbu},{0x2934d6u,0x2934dbu},{0x2934ddu,0x2934e1u},{0x2934e9u,0x2934eeu},
    {0x2934eeu,0x2934f4u},{0x2934f8u,0x2934fdu},{0x293503u,0x293506u},{0x293509u,0x29350du},
    {0x293517u,0x29351cu},{0x293526u,0x29352au},{0x29352cu,0x293530u},{0x293530u,0x293534u},
    {0x29354au,0x293550u},{0x293561u,0x293568u},{0x293568u,0x29356du},{0x29356fu,0x293573u},
    {0x293573u,0x293578u},{0x293578u,0x29357du},{0x29357du,0x293583u},{0x293583u,0x293587u},
    {0x293588u,0x29358eu},{0x29358eu,0x293592u},{0x29359du,0x2935a2u},{0x2935a4u,0x2935a8u},
    {0x2935b0u,0x2935b1u},{0x2935b1u,0x2935b2u},{0x2935b2u,0x2935b3u},{0x2935b5u,0x2935b6u},
};

template<bool Literal,class State,class Call,class Checkpoint>class Frame {
    State& s_;
    Call& call_;
    Checkpoint& checkpoint_;

    void at(std::uint32_t pc) {
        if constexpr(Literal)s_.instruction_literal(pc);
        else if(s_.services.instruction)s_.services.instruction(pc);
    }
    void retired(std::uint32_t pc) {
        const auto it=std::lower_bound(std::begin(retirements),std::end(retirements),pc,
                                      [](const auto& site,auto value){return site.first<value;});
        if(it==std::end(retirements)||it->first!=pc)throw std::logic_error("Unknown retained format-four retirement");
        checkpoint_(s_,it->second);
    }
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address,unsigned width=4u) {
        at(pc);s_.set(reg,physics_step_detail::r(s_.memory,address,width));retired(pc);
    }
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);retired(pc);
    }
    void push(std::uint32_t pc,std::uint32_t value) {
        at(pc);s_.push(pc,value);retired(pc);
    }
    void pop(std::uint32_t pc,unsigned reg) {
        at(pc);s_.set(reg,s_.pop());retired(pc);
    }
    bool signed_less()const{return s_.flags.sf!=s_.flags.of;}
    void vector(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);s_.xmm[reg]=physics_step_detail::v(s_.memory,address);retired(pc);
    }
    void scalar(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);s_.xmm[reg]=_mm_set_ss(physics_step_detail::f(physics_step_detail::r(s_.memory,address)));retired(pc);
    }
    void store_scalar(std::uint32_t pc,std::uint32_t address,unsigned reg) {
        store(pc,address,physics_step_detail::u(_mm_cvtss_f32(s_.xmm[reg])));
    }
    void xor_vector(std::uint32_t pc,unsigned reg) {
        at(pc);s_.xmm[reg]=_mm_xor_ps(s_.xmm[reg],physics_step_detail::v(s_.memory,s_.gp[4]+48u));retired(pc);
    }

    void rotate() {
        auto& g=s_.gp;auto& x=s_.xmm;
        g[0]=s_.flags.logic(0u,4u);
        store(0x293351u,g[4]+36u,g[1]);store(0x293355u,g[4]+40u,g[1]);
        store(0x293359u,g[4]+44u,g[7]);store(0x29335du,g[4]+48u,g[0]);
        store(0x293361u,g[4]+52u,g[0]);store(0x293365u,g[4]+56u,g[0]);
        store(0x293369u,g[4]+60u,0x80000000u);
        load(0x293371u,0u,g[4]+44u);vector(0x293375u,0u,g[0]);
        load(0x293378u,0u,g[4]+40u);vector(0x29337cu,1u,g[0]);
        x[3]=x[0];x[7]=x[1];
        x[0]=_mm_shuffle_ps(x[0],x[0],36);x[3]=_mm_shuffle_ps(x[3],x[3],255);
        x[7]=_mm_shuffle_ps(x[7],x[7],63);
        x[3]=math::multiply_packed(x[3],x[1]);x[7]=math::multiply_packed(x[7],x[0]);
        x[0]=_mm_shuffle_ps(x[0],x[0],73);x[6]=x[1];
        xor_vector(0x29339eu,7u);
        x[6]=_mm_shuffle_ps(x[6],x[6],82);x[3]=math::add_packed(x[3],x[7]);
        x[6]=math::multiply_packed(x[6],x[0]);x[0]=_mm_shuffle_ps(x[0],x[0],73);
        xor_vector(0x2933b1u,6u);
        x[1]=_mm_shuffle_ps(x[1],x[1],137);x[3]=math::add_packed(x[3],x[6]);x[0]=math::multiply_packed(x[0],x[1]);
        load(0x2933c0u,0u,g[4]+36u);x[3]=math::subtract_packed(x[3],x[0]);
        at(0x2933c7u);physics_step_detail::vw(s_.memory,s_.observe,0x2933c7u,g[0],x[3]);retired(0x2933c7u);
    }
    void translate() {
        using namespace physics_step_detail;
        auto& g=s_.gp;auto& x=s_.xmm;
        scalar(0x29348cu,0u,g[7]-4u);
        at(0x293491u);x[0]=math::add_scalar(x[0],_mm_set_ss(f(r(s_.memory,g[0]+16u))));retired(0x293491u);
        store_scalar(0x293496u,g[0]+16u,0u);
        scalar(0x29349bu,1u,g[7]);x[1]=math::add_scalar(x[1],x[0]);
        store_scalar(0x2934a3u,g[0]+16u,1u);
        scalar(0x2934a8u,0u,g[7]+4u);x[0]=math::add_scalar(x[0],x[1]);
        store_scalar(0x2934b1u,g[0]+16u,0u);
    }
    void scale() {
        using namespace physics_step_detail;
        auto& g=s_.gp;auto& x=s_.xmm;
        scalar(0x29356fu,0u,g[7]);
        at(0x293573u);x[0]=math::multiply_scalar(x[0],_mm_set_ss(f(r(s_.memory,g[0]+28u))));retired(0x293573u);
        store_scalar(0x293578u,g[0]+28u,0u);
    }
    void apply_ancestor(unsigned lane) {
        auto& g=s_.gp;const auto& p=lanes[lane];
        g[1]=s_.flags.logic(g[6]&31u,4u);
        const unsigned mask=lane?2u:0u,group=lane?0u:1u;
        g[mask]=s_.flags.shift(1u,s_.reg(1u,1u),4u);
        g[group]=s_.flags.right(g[6],5u,4u,true);
        at(p.plane_test);
        s_.flags.logic(physics_step_detail::r(s_.memory,0x56e000u+lane*32u+g[group]*4u)&s_.reg(mask),4u);
        retired(p.plane_test);
        load(p.output,p.output_register,0x510fa0u);
        if(!s_.flags.zf) {
            if(lane==0)rotate();
            else if(lane==1)translate();
            else scale();
        }
    }
public:
    Frame(State& state,Call& call,Checkpoint& checkpoint):s_(state),call_(call),checkpoint_(checkpoint){}
    void begin() {
        auto& g=s_.gp;
        push(0x293290u,g[5]);g[5]=g[4];
        g[4]=s_.flags.logic(g[4]&0xfffffff0u,4u);g[4]=s_.flags.minus(g[4],52u,4u);
        load(0x293299u,2u,0x510f3cu);load(0x29329fu,1u,0x510f7cu);
        push(0x2932a5u,g[3]);push(0x2932a6u,g[6]);
        g[6]=s_.flags.logic(0u,4u);s_.flags.minus(g[2],g[6],4u);
        push(0x2932abu,g[7]);load(0x2932acu,7u,0x510f70u);
        store(0x2932b2u,g[4]+32u,g[2]);store(0x2932b6u,0x510f94u,g[6]);
        store(0x2932bcu,0x510f98u,g[6]);store(0x2932c2u,0x510f9cu,g[6]);store(0x2932c8u,0x510fa0u,g[1]);
    }
    void begin_lane(unsigned lane) {
        auto& g=s_.gp;
        if(lane==1) {
            s_.flags.minus(g[2],g[6],4u);
            load(0x293404u,0u,0x510f7cu);load(0x293409u,7u,0x510f74u);
            store(0x29340fu,0x510fa0u,g[0]);
        } else {
            load(0x2934e9u,0u,0x510f7cu);load(0x2934eeu,7u,0x510f78u);
            g[6]=s_.flags.logic(0u,4u);s_.flags.logic(g[2],4u);
            store(0x2934f8u,0x510fa0u,g[0]);
        }
    }
    bool has_tracks()const{return !s_.flags.zf&&!signed_less();}
    bool load_bitmap(unsigned lane) {
        auto& g=s_.gp;const auto& p=lanes[lane];
        load(p.bitmap,3u,g[7],1u);g[7]=s_.flags.increment(g[7],4u,false);
        s_.flags.logic(g[3],4u);store(p.bitmap_slot,g[4]+24u,g[7]);return !s_.flags.zf;
    }
    void skip_bitmap(unsigned lane) {
        auto& g=s_.gp;const auto& p=lanes[lane];auto& cursor=g[p.output_register];
        g[6]=s_.flags.plus(g[6],8u,4u);cursor=s_.flags.plus(cursor,256u,4u);store(p.skip,0x510fa0u,cursor);
    }
    bool begin_bits(unsigned lane) {
        auto& g=s_.gp;const auto& p=lanes[lane];auto& end=g[p.limit_register];
        end=g[6]+8u;s_.flags.minus(end,g[2],4u);store(p.limit,g[4]+28u,g[2]);
        if(s_.flags.zf||signed_less())store(p.lower_limit,g[4]+28u,end);
        at(p.clamp);s_.flags.minus(s_.reg(6u),physics_step_detail::r(s_.memory,g[4]+28u),4u);retired(p.clamp);
        if(!signed_less())return false;
        g[7]=s_.flags.shift(g[6],5u,4u);g[7]=s_.flags.plus(g[7],p.ancestor,4u);return true;
    }
    bool bit_set(){s_.flags.logic(s_.gp[3]&1u,1u);return !s_.flags.zf;}
    void decode_track(unsigned lane) {
        auto& g=s_.gp;const auto& p=lanes[lane];
        at(p.call);
        const auto target=physics_step_detail::r(s_.memory,0x510f64u+lane*4u);
        s_.push(p.call,p.returned);
        call_(s_,target,p.call);
        retired(p.call);
        apply_ancestor(lane);
        auto& counter=g[p.limit_register];
        load(p.counter,p.limit_register,0x510f94u+lane*4u);load(p.count,2u,g[4]+32u);
        counter=s_.flags.increment(counter,4u,false);store(p.counter_store,0x510f94u+lane*4u,counter);
    }
    void finish_bit(unsigned lane) {
        auto& g=s_.gp;const auto& p=lanes[lane];auto& cursor=g[p.output_register];
        load(p.end,p.limit_register,g[4]+28u);
        cursor=s_.flags.plus(cursor,32u,4u);g[3]=s_.flags.right(g[3],1u,4u,true);
        g[6]=s_.flags.increment(g[6],4u,false);g[7]=s_.flags.plus(g[7],32u,4u);
        s_.flags.minus(g[6],g[p.limit_register],4u);store(p.cursor,0x510fa0u,cursor);
    }
    bool more_tracks()const{return signed_less();}
    void reload_bitmap(unsigned lane){load(lanes[lane].bitmap_reload,7u,s_.gp[4]+24u);}
    bool more_bitmaps(){s_.flags.minus(s_.gp[6],s_.gp[2],4u);return signed_less();}
    void end_lane(unsigned lane){if(!lane)s_.gp[6]=s_.flags.logic(0u,4u);}
    void finish() {
        pop(0x2935b0u,7u);pop(0x2935b1u,6u);pop(0x2935b2u,3u);
        s_.gp[4]=s_.gp[5];pop(0x2935b5u,5u);at(0x2935b6u);(void)s_.pop();
    }
};

template<bool Literal,class State,class Call,class Checkpoint>
void run_private_source(State& state,Call& call,Checkpoint& checkpoint) {
    Frame<Literal,State,Call,Checkpoint> view{state,call,checkpoint};
    native_codec_format_four_decode_detail::decode(view);
}
} // namespace halo2::native_codec_format_four_source_detail
#endif

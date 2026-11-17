#pragma once
#include "physics/runtime/native_tu15_codec_scale_interpolation_frame.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_codec_scale_interpolation_detail {
struct SourceContinuation{std::uint32_t pc;const void*owner;};
// Private retained-provider draft. A memory multiply retires only after its
// source read, arithmetic and escaped SIMD publication have all completed.
template<class State,class Before,class Checkpoint>class SourceFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,
        native_codec_rotation_interpolation_detail::Io<native_integer_fp_detail::Scope<State>>>{
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,
        native_codec_rotation_interpolation_detail::Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::io;
    Checkpoint&checkpoint_;
    static __m128 scalar(std::uint32_t value){return std::bit_cast<__m128>(std::array<std::uint32_t,4>{value,0,0,0});}
    static std::uint32_t bits(__m128 value){return std::bit_cast<std::array<std::uint32_t,4>>(value)[0];}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        at(pc);const bool observing=bool(s_.observe);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));
        if(observing){event.before.resize(4);io([&]{s_.memory.read(address,event.before);});}
        io([&]{s_.memory.write(address,bytes);});
        if(observing&&s_.observe){event.written.assign(bytes.begin(),bytes.end());io([&]{s_.observe(event);});}
    }
public:
    using Input=typename Frame<State,Before>::Input;
    SourceFrame(State&s,Before&before,Checkpoint&checkpoint,Scope&scope):Base{s,before,{&scope}},checkpoint_(checkpoint){}
    Input input(){
        auto&g=s_.gp;g[4]=s_.flags.minus(g[4],8u,4);
        at(0x296623u);g[1]=word(0x510f60u);checkpoint_(0x296629u);
        at(0x296629u);g[0]=word(g[1]+28u);checkpoint_(0x29662cu);
        at(0x29662cu);g[0]=s_.flags.product(s_.reg(0,4,0),word(0x510f9cu));checkpoint_(0x296633u);
        at(0x296633u);g[0]=s_.flags.plus(s_.reg(0,4,0),word(g[1]+16u),4);checkpoint_(0x296636u);
        g[0]=s_.flags.plus(g[0],g[1],4);
        at(0x296638u);g[1]=word(0x510f44u);checkpoint_(0x29663eu);
        at(0x29663eu);g[2]=word(g[0]+g[1]*4u);checkpoint_(0x296641u);
        at(0x296641u);g[1]=word(0x510f48u);checkpoint_(0x296647u);
        store(0x296647u,g[4]+4u,g[2]);checkpoint_(0x29664bu);
        at(0x29664bu);g[2]=word(g[0]+g[1]*4u);checkpoint_(0x29664eu);
        at(0x29664eu);s_.xmm[0]=scalar(word(g[4]+4u));checkpoint_(0x296654u);
        at(0x296654u);g[0]=word(0x510fa0u);checkpoint_(0x296659u);
        store(0x296659u,g[4],g[2]);checkpoint_(0x29665cu);
        at(0x29665cu);s_.xmm[1]=scalar(word(g[4]));checkpoint_(0x296661u);
        return{s_.xmm[0],s_.xmm[1]};
    }
    __m128 subtract(__m128,__m128)const{return _mm_sub_ss(s_.xmm[1],s_.xmm[0]);}
    __m128 scale_difference(__m128 difference){
        s_.xmm[1]=difference;at(0x296665u);
        s_.xmm[1]=_mm_mul_ss(s_.xmm[1],scalar(word(0x510f4cu)));checkpoint_(0x29666du);
        return s_.xmm[1];
    }
    __m128 add_first(__m128,__m128)const{return _mm_add_ss(s_.xmm[1],s_.xmm[0]);}
    void publish(__m128 value){s_.xmm[1]=value;store(0x296671u,s_.gp[0]+28u,bits(s_.xmm[1]));checkpoint_(0x296676u);}
    void finish(){s_.gp[4]=s_.flags.plus(s_.gp[4],8u,4);at(0x296679u);(void)word(s_.gp[4]);s_.gp[4]+=4u;}
};
template<class State,class Continue,class Before=Noop>
std::uint32_t run_source(State&s,Continue continuation,Before before={}){
    const std::byte token{};
    const auto checkpoint=[&](std::uint32_t next){if(!continuation(s,next))throw SourceContinuation{next,&token};};
    try{native_integer_fp_detail::Scope scope{s};SourceFrame view{s,before,checkpoint,scope};native_codec_scale_interpolation_detail::decode(view);return 0u;}
    catch(const SourceContinuation&resume){if(resume.owner!=&token)throw;return resume.pc;}
}
} // namespace halo2::native_codec_scale_interpolation_detail
#endif

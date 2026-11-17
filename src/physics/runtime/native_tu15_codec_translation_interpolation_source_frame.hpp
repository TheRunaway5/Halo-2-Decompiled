#pragma once
#include "physics/runtime/native_tu15_codec_translation_interpolation_frame.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_codec_translation_interpolation_detail {
struct SourceContinuation{std::uint32_t pc;const void*owner;};
// Retained publication draft. This view uses the literal helper's flag
// choices and only retires after the complete authored I/O instruction.
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
        auto&g=s_.gp;
        at(0x2962f0u);g[1]=word(0x510f60u);checkpoint_(0x2962f6u);
        at(0x2962f6u);g[0]=word(g[1]+24u);checkpoint_(0x2962f9u);
        at(0x2962f9u);g[2]=word(g[1]+12u);checkpoint_(0x2962fcu);
        at(0x2962fcu);g[0]=s_.flags.product(s_.reg(0,4,0),word(0x510f98u));checkpoint_(0x296303u);
        at(0x296303u);s_.xmm[0]=scalar(word(0x510f4cu));checkpoint_(0x29630bu);
        g[0]=s_.flags.plus(g[0],g[2],4);g[0]=s_.flags.plus(g[0],g[1],4);
        at(0x29630fu);g[1]=word(0x510f44u);checkpoint_(0x296315u);
        g[1]=g[1]+g[1]*2u;g[2]=g[0]+g[1]*4u;
        at(0x29631bu);g[1]=word(g[2]);checkpoint_(0x29631du);
        store(0x29631du,0x56e6acu,g[1]);checkpoint_(0x296323u);
        at(0x296323u);g[1]=word(g[2]+4u);checkpoint_(0x296326u);
        at(0x296326u);s_.xmm[1]=scalar(word(0x56e6acu));checkpoint_(0x29632eu);
        store(0x29632eu,0x56e6b0u,g[1]);checkpoint_(0x296334u);
        at(0x296334u);g[2]=word(g[2]+8u);checkpoint_(0x296337u);
        at(0x296337u);g[1]=word(0x510f48u);checkpoint_(0x29633du);
        store(0x29633du,0x56e6b4u,g[2]);checkpoint_(0x296343u);
        g[1]=g[1]+g[1]*2u;g[2]=g[0]+g[1]*4u;
        at(0x296349u);g[0]=word(g[2]);checkpoint_(0x29634bu);
        store(0x29634bu,0x56e6a0u,g[0]);checkpoint_(0x296350u);
        at(0x296350u);g[1]=word(g[2]+4u);checkpoint_(0x296353u);
        at(0x296353u);s_.xmm[2]=scalar(word(0x56e6a0u));checkpoint_(0x29635bu);
        at(0x29635bu);g[0]=word(0x510fa0u);checkpoint_(0x296360u);
        return {s_.xmm[1],s_.xmm[2],s_.xmm[0]};
    }
    __m128 interpolate(__m128,__m128,__m128)const{
        // A provider may have changed earlier lanes while leaving the current
        // numeric callback protocol intact. Use the actual resumed SIMD file.
        auto value=s_.xmm[2];
        asm volatile("subss %1,%0":"+x"(value):"x"(s_.xmm[1]):"memory");
        asm volatile("mulss %1,%0":"+x"(value):"x"(s_.xmm[0]):"memory");
        asm volatile("addss %1,%0":"+x"(value):"x"(s_.xmm[1]):"memory");
        return value;
    }
    void publish_x(__m128 value){
        s_.xmm[2]=value;store(0x29636cu,0x56e6a4u,s_.gp[1]);checkpoint_(0x296372u);
        at(0x296372u);s_.gp[2]=word(s_.gp[2]+8u);checkpoint_(0x296375u);
        store(0x296375u,0x56e6a8u,s_.gp[2]);checkpoint_(0x29637bu);
        store(0x29637bu,s_.gp[0]+16u,bits(s_.xmm[2]));checkpoint_(0x296380u);
    }
    __m128 first_y(){at(0x296380u);s_.xmm[1]=scalar(word(0x56e6b0u));checkpoint_(0x296388u);return s_.xmm[1];}
    __m128 second_y(){at(0x296388u);s_.xmm[2]=scalar(word(0x56e6a4u));checkpoint_(0x296390u);return s_.xmm[2];}
    void publish_y(__m128 value){s_.xmm[2]=value;store(0x29639cu,s_.gp[0]+20u,bits(value));checkpoint_(0x2963a1u);}
    __m128 first_z(){at(0x2963a1u);s_.xmm[1]=scalar(word(0x56e6b4u));checkpoint_(0x2963a9u);return s_.xmm[1];}
    __m128 second_z(){at(0x2963a9u);s_.xmm[2]=scalar(word(0x56e6a8u));checkpoint_(0x2963b1u);return s_.xmm[2];}
    void publish_z(__m128 value){s_.xmm[2]=value;store(0x2963bdu,s_.gp[0]+24u,bits(value));checkpoint_(0x2963c2u);}
    void finish(){at(0x2963c2u);(void)word(s_.gp[4]);s_.gp[4]+=4u;}
};
template<class State,class Continue,class Before=Noop>
std::uint32_t run_source(State&s,Continue continuation,Before before={}){
    const std::byte token{};
    const auto checkpoint=[&](std::uint32_t next){if(!continuation(s,next))throw SourceContinuation{next,&token};};
    try{native_integer_fp_detail::Scope scope{s};SourceFrame view{s,before,checkpoint,scope};native_codec_translation_interpolation_detail::decode(view);return 0u;}
    catch(const SourceContinuation&resume){if(resume.owner!=&token)throw;return resume.pc;}
}
} // namespace halo2::native_codec_translation_interpolation_detail
#endif

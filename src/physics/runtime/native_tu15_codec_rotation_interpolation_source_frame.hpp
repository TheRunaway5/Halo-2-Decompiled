#pragma once
#include "physics/runtime/native_tu15_codec_rotation_interpolation_frame.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_codec_rotation_interpolation_detail {
struct SourceContinuation{std::uint32_t pc;const void*owner;};
// The retained view keeps the literal helper's undefined flag choices and
// completed-instruction continuations separate from raw hardware transport.
template<class State,class Before,class Checkpoint>class SourceFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>>{
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::io;
    Checkpoint&checkpoint_;
    static __m128 scalar(std::uint32_t value){return std::bit_cast<__m128>(std::array<std::uint32_t,4>{value,0,0,0});}
    static std::uint32_t bits(__m128 value){return std::bit_cast<std::array<std::uint32_t,4>>(value)[0];}
    template<std::size_t Width>void stored(std::uint32_t pc,std::uint32_t address,const std::array<std::byte,Width>&bytes){
        const bool observing=bool(s_.observe);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(observing){event.before.resize(Width);io([&]{s_.memory.read(address,event.before);});}
        io([&]{s_.memory.write(address,bytes);});
        if(observing&&s_.observe){event.written.assign(bytes.begin(),bytes.end());io([&]{s_.observe(event);});}
    }
    void store_source(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        at(pc);std::array<std::byte,4>bytes{};
        for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
        stored(pc,address,bytes);
    }
    __m128 vector_source(std::uint32_t pc,std::uint32_t address){
        at(pc);std::array<std::byte,16>bytes{};io([&]{s_.memory.read_vector(address,bytes);});
        return std::bit_cast<__m128>(bytes);
    }
public:
    using Input=typename Frame<State,Before>::Input;
    SourceFrame(State&s,Before&before,Checkpoint&checkpoint,Scope&scope):Base{s,before,{&scope}},checkpoint_(checkpoint){}
    Input input(){
        auto&g=s_.gp;g[4]=s_.flags.minus(g[4],16u,4);
        at(0x296243u);g[0]=word(0x510f60u);checkpoint_(0x296248u);
        at(0x296248u);g[1]=word(g[0]+20u);checkpoint_(0x29624bu);
        at(0x29624bu);g[1]=s_.flags.product(s_.reg(1,4,0),word(0x510f94u));checkpoint_(0x296252u);
        at(0x296252u);g[2]=word(0x510fa0u);checkpoint_(0x296258u);
        at(0x296258u);s_.xmm[0]=scalar(word(0x510f4cu));checkpoint_(0x296260u);g[0]=g[0]+g[1]+32u;
        at(0x296264u);g[1]=word(0x510f48u);checkpoint_(0x29626au);
        store_source(0x29626au,g[4],g[2]);checkpoint_(0x29626du);
        at(0x29626du);g[2]=word(0x510f44u);checkpoint_(0x296273u);
        g[1]=s_.flags.shift(g[1],4u,4);g[1]=s_.flags.plus(g[1],g[0],4);
        g[2]=s_.flags.shift(g[2],4u,4);g[2]=s_.flags.plus(g[2],g[0],4);
        store_source(0x29627du,g[4]+4u,bits(s_.xmm[0]));checkpoint_(0x296283u);
        store_source(0x296283u,g[4]+8u,g[1]);checkpoint_(0x296287u);
        store_source(0x296287u,g[4]+12u,g[2]);checkpoint_(0x29628bu);
        at(0x29628bu);g[0]=word(g[4]+12u);checkpoint_(0x29628fu);
        at(0x29628fu);g[1]=word(g[4]+8u);checkpoint_(0x296293u);
        at(0x296293u);s_.xmm[3]=scalar(word(g[4]+4u));checkpoint_(0x296299u);
        s_.xmm[1]=vector_source(0x296299u,g[0]);checkpoint_(0x29629cu);
        s_.xmm[2]=vector_source(0x29629cu,g[1]);checkpoint_(0x29629fu);
        return {s_.xmm[1],s_.xmm[2],s_.xmm[3]};
    }
    __m128 subtract(__m128 left,__m128 right)const{
        asm volatile("subps %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    __m128 multiply(__m128 left,__m128 right)const{
        asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    __m128 add(__m128 left,__m128 right)const{
        asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;
    }
    __m128 opposite(__m128 value)const{return _mm_shuffle_ps(value,value,78);}
    __m128 alternate(__m128 value)const{return _mm_shuffle_ps(value,value,17);}
    __m128 interpolate(const Input&input){
        s_.xmm[3]=_mm_shuffle_ps(input.factor,input.factor,0);
        return add(multiply(subtract(input.second,input.first),s_.xmm[3]),input.first);
    }
    __m128 inverse(__m128 norm,__m128 quaternion,__m128 opposite,__m128 alternate){
        s_.xmm[0]=norm;s_.xmm[2]=quaternion;s_.xmm[7]=opposite;s_.xmm[6]=alternate;
        at(0x2962c6u);
        auto lanes=std::bit_cast<std::array<std::uint32_t,4>>(norm);
        for(auto&lane:lanes){
            // The retained helper reacquires its actual provider for each lane.
            if(!s_.services.reciprocal_sqrt)throw std::runtime_error("Required original contact RSQRTSS owner unavailable");
            io([&]{lane=s_.services.reciprocal_sqrt(lane);});
        }
        s_.xmm[0]=std::bit_cast<__m128>(lanes);checkpoint_(0x2962c9u);
        at(0x2962c9u);s_.gp[2]=word(s_.gp[4]);checkpoint_(0x2962ccu);
        return s_.xmm[0];
    }
    __m128 scale(__m128,__m128){return multiply(s_.xmm[2],s_.xmm[0]);}
    void output(__m128 value){
        s_.xmm[2]=value;at(0x2962cfu);
        // Retained vw captures the full payload/address once, while each w
        // independently selects its observer branch. Handoff follows both.
        const auto address=s_.gp[2];const auto bytes=std::bit_cast<std::array<std::byte,16>>(s_.xmm[2]);
        for(unsigned offset:{0u,8u}){std::array<std::byte,8>part{};std::copy_n(bytes.begin()+offset,8,part.begin());stored(0x2962cfu,address+offset,part);}
        checkpoint_(0x2962d2u);s_.gp[4]=s_.flags.plus(s_.gp[4],16u,4);
        at(0x2962d5u);(void)word(s_.gp[4]);s_.gp[4]+=4u;
    }
};
template<class State,class Continue,class Before=Noop>
std::uint32_t run_source(State&s,Continue continuation,Before before={}){
    const std::byte token{};const auto checkpoint=[&](std::uint32_t next){if(!continuation(s,next))throw SourceContinuation{next,&token};};
    try{native_integer_fp_detail::Scope scope{s};SourceFrame view{s,before,checkpoint,scope};decode(view);return 0u;}
    catch(const SourceContinuation&resume){if(resume.owner!=&token)throw;return resume.pc;}
}
} // namespace halo2::native_codec_rotation_interpolation_detail
#endif

#pragma once
#include "physics/runtime/native_tu15_codec_scale_frame.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_codec_scale_detail {
struct SourceContinuation{std::uint32_t pc;const void*owner;};
template<class State,class Before,class Checkpoint>class SourceFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>>{
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::io;
    Checkpoint&checkpoint_;
    void store_source(std::uint32_t address,std::uint32_t value){
        at(0x296239u);
        if(s_.observe){Base::store(0x296239u,address,value,[]{},true);return;}
        std::array<std::byte,4>bytes{};
        for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
        io([&]{s_.memory.write(address,bytes);});
    }
public:
    SourceFrame(State&s,Before&before,Checkpoint&checkpoint,Scope&scope):Base{s,before,{&scope}},checkpoint_(checkpoint){}
    NativeTu15WordCopy scale(){auto&g=s_.gp;
        at(0x296220u);g[0]=word(0x510f60u);checkpoint_(0x296225u);
        at(0x296225u);g[1]=word(g[0]+16u);checkpoint_(0x296228u);
        at(0x296228u);g[2]=word(0x510f9cu);checkpoint_(0x29622eu);g[1]=g[1]+g[2]*4u;
        return {g[0]+g[1],0u,1u,0u};
    }
    std::uint32_t read_word([[maybe_unused]] std::uint32_t address){
        at(0x296231u);const auto value=word(s_.gp[0]+s_.gp[1]);s_.gp[2]=value;checkpoint_(0x296234u);return value;
    }
    void write_word(std::uint32_t,[[maybe_unused]] std::uint32_t value){
        at(0x296234u);s_.gp[0]=word(0x510fa0u);checkpoint_(0x296239u);
        // The actual output-pointer read may change the live stored GP2.
        store_source(s_.gp[0]+28u,s_.gp[2]);checkpoint_(0x29623cu);
    }
    void advanced(const NativeTu15WordCopy&){}
    void finish(){at(0x29623cu);(void)word(s_.gp[4]);s_.gp[4]+=4u;}
};
template<class State,class Continue,class Before=Noop>
std::uint32_t run_source(State&s,Continue continuation,Before before={}){
    const std::byte token{};const auto checkpoint=[&](std::uint32_t next){if(!continuation(s))throw SourceContinuation{next,&token};};
    try{native_integer_fp_detail::Scope scope{s};SourceFrame view{s,before,checkpoint,scope};decode(view);return 0u;}
    catch(const SourceContinuation&resume){if(resume.owner!=&token)throw;return resume.pc;}
}
} // namespace halo2::native_codec_scale_detail
#endif

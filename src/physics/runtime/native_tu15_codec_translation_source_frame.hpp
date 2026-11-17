#pragma once
#include "physics/runtime/native_tu15_codec_translation_frame.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_codec_translation_detail {
struct SourceContinuation{std::uint32_t pc;const void*owner;};
template<class State,class Before,class Checkpoint>class SourceFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>>{
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::io;
    Checkpoint&checkpoint_;unsigned component_{};
    void store_source(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        at(pc);
        // Retained w() selects the empty-observer fast path on entry. An
        // observer installed by this completed write belongs to later stores.
        if(s_.observe){Base::store(pc,address,value,[]{},true);return;}
        std::array<std::byte,4>bytes{};
        for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
        io([&]{s_.memory.write(address,bytes);});
    }
public:
    SourceFrame(State&s,Before&before,Checkpoint&checkpoint,Scope&scope):Base{s,before,{&scope}},checkpoint_(checkpoint){}
    NativeTu15WordCopy translation(){auto&g=s_.gp;
        at(0x2961f0u);g[0]=word(0x510f98u);checkpoint_(0x2961f5u);g[1]=g[0]+g[0]*2u;
        at(0x2961f8u);g[0]=word(0x510f60u);checkpoint_(0x2961fdu);
        at(0x2961fdu);g[2]=word(g[0]+12u);checkpoint_(0x296200u);g[1]=g[2]+g[1]*4u;
        at(0x296203u);g[2]=word(0x510fa0u);checkpoint_(0x296209u);g[1]=s_.flags.plus(g[1],g[0],4);
        return {g[1],g[2]+16u,3u,0u};
    }
    std::uint32_t read_word([[maybe_unused]] std::uint32_t address){
        constexpr std::array sites{0x29620bu,0x296212u,0x296218u},next{0x29620du,0x296215u,0x29621bu};
        at(sites[component_]);const auto value=word(s_.gp[1]+component_*4u);s_.gp[component_==2u?1u:0u]=value;
        checkpoint_(next[component_]);return value;
    }
    void write_word([[maybe_unused]] std::uint32_t address,std::uint32_t value){
        constexpr std::array sites{0x296210u,0x296215u,0x29621bu},next{0x296212u,0x296218u,0x29621eu};
        if(!component_)s_.gp[2]=s_.flags.plus(s_.gp[2],16u,4);
        // Reacquire live EDX after the actual preceding source load. Each
        // scalar store still captures one address/value before its providers.
        store_source(sites[component_],s_.gp[2]+component_*4u,value);checkpoint_(next[component_]);
    }
    void advanced(const NativeTu15WordCopy&){++component_;}
    void finish(){at(0x29621eu);(void)word(s_.gp[4]);s_.gp[4]+=4u;}
};
// No completed provider instruction is replayed after policy handoff. The
// private token prevents a provider's unrelated exception from being consumed.
template<class State,class Continue,class Before=Noop>
std::uint32_t run_source(State&s,Continue continuation,Before before={}){
    const std::byte token{};const auto checkpoint=[&](std::uint32_t next){if(!continuation(s))throw SourceContinuation{next,&token};};
    try{native_integer_fp_detail::Scope scope{s};SourceFrame view{s,before,checkpoint,scope};decode(view);return 0u;}
    catch(const SourceContinuation&resume){if(resume.owner!=&token)throw;return resume.pc;}
}
} // namespace halo2::native_codec_translation_detail
#endif

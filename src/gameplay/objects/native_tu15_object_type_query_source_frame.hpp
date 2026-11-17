#pragma once
#include "gameplay/objects/native_tu15_object_type_query_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_type_query_detail {
// Private retained-provider view. Each continuation names the first untouched
// source statement, after its predecessor published the full live ABI.
struct SourceContinuation {std::uint32_t pc;const void*owner{};};
template<class State,class Call,class Continue,class Before>class SourceFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,HostIo> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,HostIo>;
    using Base::s_;using Base::at;using Base::word;using Base::io;
    Call&call_;Continue&continue_;const void*owner_;
    void checkpoint(std::uint32_t next) {
        if(!continue_(s_))throw SourceContinuation{next,owner_};
    }
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {
        at(pc);return word(address,width);
    }
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4) {
        // Match retained w(): entering without an observer selects its fast
        // path, even when the write itself installs a subsequent observer.
        const bool observed=bool(s_.observe);
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(observed){event.before.resize(width);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};
        for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));
        const auto data=std::span(bytes).first(width);
        io([&]{s_.memory.write(address,data);});
        if(observed&&s_.observe){event.written.assign(data.begin(),data.end());io([&]{s_.observe(event);});}
    }
    void push(std::uint32_t pc,std::uint32_t value,std::uint32_t next) {
        at(pc);s_.gp[4]-=4u;store(pc,s_.gp[4],value);checkpoint(next);
    }
    std::uint32_t pop(std::uint32_t pc) {
        at(pc);const auto value=word(s_.gp[4]);s_.gp[4]+=4u;return value;
    }
    std::uint32_t finish(std::uint32_t first,std::uint32_t second,std::uint32_t ret) {
        auto&g=s_.gp;
        g[6]=pop(first);checkpoint(second);
        g[5]=pop(second);checkpoint(ret);
        pop(ret);g[4]+=8u;return g[0];
    }
public:
    SourceFrame(State&s,Call&call,Continue&continuation,Before&before,const void*owner)
        :Base{s,before,HostIo{}},call_(call),continue_(continuation),owner_(owner){}
    void begin() {
        auto&g=s_.gp;
        g[1]=read(0x10de40u,0x4ecbd0u);checkpoint(0x10de46u);
        g[2]=read(0x10de46u,g[1]+0x44u);checkpoint(0x10de49u);
        s_.xmm[0]=_mm_xor_ps(s_.xmm[0],s_.xmm[0]);
        push(0x10de4cu,g[5],0x10de4du);
        g[5]=read(0x10de4du,g[4]+8u);checkpoint(0x10de51u);
        g[0]=g[5];g[0]=s_.flags.logic(g[0]&0xffffu,4);g[0]+=g[0]*2u;
        g[0]=read(0x10de5bu,g[2]+g[0]*4u+8u);checkpoint(0x10de5fu);
        g[1]=std::uint32_t(std::int32_t(std::int8_t(read(0x10de5fu,g[0]+0xaau,1))));checkpoint(0x10de66u);
        g[1]=read(0x10de66u,0x474b90u+g[1]*4u);checkpoint(0x10de6du);
        s_.set(0,s_.flags.logic(0u,1),1,0);
        push(0x10de6fu,g[6],0x10de70u);
        at(0x10de70u);store(0x10de70u,g[3],std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s_.xmm[0])));checkpoint(0x10de74u);
        at(0x10de74u);store(0x10de74u,g[7],s_.reg(0,1,0),1);checkpoint(0x10de76u);
        g[6]=g[1]+0x84u;
    }
    bool method_present() {
        s_.gp[1]=read(0x10de80u,s_.gp[6]);checkpoint(0x10de82u);
        s_.flags.logic(s_.gp[1],4);return !s_.flags.zf;
    }
    bool callback_present() {
        s_.gp[1]=read(0x10de86u,s_.gp[1]+0x4cu);checkpoint(0x10de89u);
        s_.flags.logic(s_.gp[1],4);return !s_.flags.zf;
    }
    void invoke() {
        auto&g=s_.gp;
        g[2]=read(0x10de8du,g[4]+16u);checkpoint(0x10de91u);
        push(0x10de91u,g[7],0x10de92u);push(0x10de92u,g[3],0x10de93u);
        push(0x10de93u,g[2],0x10de94u);push(0x10de94u,g[5],0x10de95u);
        at(0x10de95u);const auto target=g[1];g[4]-=4u;store(0x10de95u,g[4],0x10de97u);
        call_(s_,target,0x10de95u,false);checkpoint(0x10de97u);
    }
    void advance(){s_.gp[6]=s_.flags.plus(s_.gp[6],4u,4);}
    bool accepted(){s_.flags.logic(s_.reg(0,1,0),1);return !s_.flags.zf;}
    std::uint32_t finish_found(){return finish(0x10de9eu,0x10de9fu,0x10dea0u);}
    std::uint32_t finish_empty() {
        s_.flags.logic(s_.reg(0,1,0),1);
        if(s_.flags.zf){
            s_.xmm[0]=_mm_xor_ps(s_.xmm[0],s_.xmm[0]);
            at(0x10deaau);store(0x10deaau,s_.gp[3],std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s_.xmm[0])));checkpoint(0x10deaeu);
            at(0x10deaeu);store(0x10deaeu,s_.gp[7],s_.reg(0,1,0),1);checkpoint(0x10deb0u);
        }
        return finish(0x10deb0u,0x10deb1u,0x10deb2u);
    }
};
template<class State,class Call,class Continue,class Before=Noop>
std::uint32_t run_source_frame(State&s,Call call,Continue continuation,Before before={}) {
    const unsigned token{};
    try{SourceFrame view{s,call,continuation,before,&token};query(view);return 0u;}
    catch(const SourceContinuation&resume){if(resume.owner!=&token)throw;return resume.pc;}
}
} // namespace halo2::native_object_type_query_detail
#endif

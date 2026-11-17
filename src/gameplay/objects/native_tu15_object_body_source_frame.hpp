#pragma once
#include "gameplay/objects/native_tu15_object_body_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_body_detail {
// Private retained-provider transport. Physical-prefix proofs use Frame;
// this view preserves the existing source API's arithmetic flag choices and
// publishes each completed provider instruction before a possible handoff.
struct SourceContinuation{std::uint32_t pc;const void*owner{};};
struct ActualSites{constexpr std::uint32_t operator()(std::uint32_t pc)const{return pc;}};
template<class State,class Continue,class Before,class Sites=ActualSites>class SourceFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,HostIo>{
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,HostIo>;
    using Base::s_;using Base::word;using Base::io;
    Continue&continue_;
    const void*owner_;
    Sites sites_;
    void at(std::uint32_t pc){Base::at(sites_(pc));}
    std::uint32_t pop(std::uint32_t pc){
        at(pc);const auto value=word(s_.gp[4]);s_.gp[4]+=4u;return value;
    }
    void checkpoint(std::uint32_t next){
        if(!continue_(s_))throw SourceContinuation{sites_(next),owner_};
    }
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4){
        at(pc);return word(address,width);
    }
    void store_source(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        // Retained w() selects its no-observer fast path before entering the
        // memory provider. A newly installed observer belongs to the next
        // instruction; it must not receive this completed store retroactively.
        if(s_.observe){Base::store(sites_(pc),address,value,[]{},true);return;}
        std::array<std::byte,4>bytes{};
        for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
        io([&]{s_.memory.write(address,bytes);});
    }
    void push(std::uint32_t pc,std::uint32_t value,std::uint32_t next){
        at(pc);s_.gp[4]-=4u;store_source(pc,s_.gp[4],value);checkpoint(next);
    }
public:
    SourceFrame(State&s,Continue&continuation,Before&before,const void*owner,Sites sites={})
        :Base{s,before,HostIo{}},continue_(continuation),owner_(owner),sites_(sites){}
    std::uint32_t begin(){
        auto&g=s_.gp;push(0xbcdf0u,g[6],0xbcdf1u);push(0xbcdf1u,g[7],0xbcdf2u);
        g[6]=g[0];g[7]=s_.flags.logic(0u,4);return g[6];
    }
    bool present(std::uint32_t datum){s_.flags.minus(datum,0xffffffffu,4);return s_.flags.zf==false;}
    std::uint32_t objects(){
        s_.gp[2]=read(0xbcdfbu,0x4ecbd0u);checkpoint(0xbce01u);return s_.gp[2];
    }
    std::uint32_t limit(std::uint32_t){
        s_.gp[0]=read(0xbce01u,s_.gp[2]+0x38u);checkpoint(0xbce04u);return s_.gp[0];
    }
    bool in_bounds(std::uint32_t,std::uint32_t){
        auto&g=s_.gp;g[1]=g[6];g[1]=s_.flags.logic(g[1]&65535u,4);
        s_.flags.minus(g[1],g[0],4);return s_.flags.sf!=s_.flags.of;
    }
    std::uint32_t record(std::uint32_t){
        auto&g=s_.gp;g[0]=read(0xbce10u,g[2]+0x24u);checkpoint(0xbce13u);
        g[0]=s_.flags.product(s_.reg(0,4,0),s_.reg(1,4,0));
        // Keep the retained expression's operand evaluation contract when a
        // real memory provider changes the live source register file.
        g[0]=s_.flags.plus(s_.reg(0,4,0),read(0xbce16u,g[2]+0x44u),4);
        checkpoint(0xbce19u);return g[0];
    }
    std::uint16_t salt(std::uint32_t){
        s_.set(1,read(0xbce19u,s_.gp[0],2),2,0);checkpoint(0xbce1cu);
        return std::uint16_t(s_.reg(1,2,0));
    }
    bool live(std::uint16_t){s_.flags.logic(s_.reg(1,2,0),2);return !s_.flags.zf;}
    bool matches(std::uint32_t,std::uint16_t){
        auto&g=s_.gp;g[1]=std::uint32_t(std::int32_t(std::int16_t(s_.reg(1,2,0))));
        g[6]=s_.flags.right(g[6],16u,4,true);s_.flags.minus(g[1],g[6],4);
        return s_.flags.zf;
    }
    void admit(std::uint32_t){s_.gp[7]=s_.gp[0];}
    std::uint32_t finish(){
        auto&g=s_.gp;g[0]=s_.flags.logic(0u,4);s_.flags.logic(g[7],4);
        if(!s_.flags.zf){
            s_.set(1,read(0xbce33u,g[7]+3u,1),1,0);checkpoint(0xbce36u);
            g[2]=1u;g[2]=s_.flags.shift(s_.reg(2,4,0),s_.reg(1,1,0),4);
            s_.flags.logic(read(0xbce3du,g[4]+12u)&s_.reg(2,4,0),4);checkpoint(0xbce41u);
            if(!s_.flags.zf){g[0]=read(0xbce43u,g[7]+8u);checkpoint(0xbce46u);}
        }
        g[7]=pop(0xbce46u);checkpoint(0xbce47u);
        g[6]=pop(0xbce47u);checkpoint(0xbce48u);
        pop(0xbce48u);g[4]+=4u;return g[0];
    }
};
// Zero means complete; a nonzero result identifies the exact untouched
// retained suffix. No provider instruction is replayed after it completed.
template<class State,class Continue,class Before=Noop,class Sites=ActualSites>
std::uint32_t run_source_frame(State&s,Continue continuation,Before before={},Sites sites={}){
    const unsigned token{};
    try{SourceFrame view{s,continuation,before,&token,sites};lookup(view);return 0u;}
    catch(const SourceContinuation&resume){if(resume.owner!=&token)throw;return resume.pc;}
}
} // namespace halo2::native_object_body_detail
#endif

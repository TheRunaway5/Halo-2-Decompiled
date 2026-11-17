#pragma once
#include "halo2/native_tu15_object_body_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_host_fp_io.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_body_detail {
struct HostIo{template<class Function>void operator()(Function&&function)const{
    native_host_fp_io_detail::preserve(std::forward<Function>(function));
}};
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
enum class Publication{architectural,source_provider};
// Observe the physical integer instruction's unspecified arithmetic flags too.
// GP and all non-arithmetic flags remain in the caller's software ABI file.
template<unsigned Operation,class State>
std::uint32_t arithmetic(State&s,std::uint32_t value,std::uint32_t operand=0){
    const auto context=s.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);
    unsigned overflow=operand;
    if constexpr(Operation==0)
        asm volatile("addb $127,%%al;sahf;imull %3,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
            :"+&a"(flags),"+c"(overflow),"+r"(value):"r"(operand):"cc");
    else if constexpr(Operation==1)
        asm volatile("addb $127,%%al;sahf;sarl $16,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
            :"+a"(flags),"+c"(overflow),"+r"(value)::"cc");
    else
        asm volatile("addb $127,%%al;sahf;shll %%cl,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
            :"+a"(flags),"+c"(overflow),"+r"(value)::"cc");
    s.flags.load((flags&0xd5u)|((overflow&1u)<<11u));return value;
}
template<class State,class Before,Publication publication=Publication::architectural>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,HostIo>{
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,HostIo>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4){at(pc);return word(address,width);}
    void push(std::uint32_t pc,std::uint32_t value){
        if constexpr(publication==Publication::source_provider){
            // The retained virtual-provider ABI publishes PUSH's ESP before
            // preimage reads/writes. Keep this separate from a CPU stop before
            // the instruction, including provider/observer exceptions.
            at(pc);s_.gp[4]-=4u;Base::store(pc,s_.gp[4],value,[]{},true);
        }else Base::push(pc,value);
    }
public:
    Frame(State&s,Before&before):Base{s,before,HostIo{}}{}
    std::uint32_t begin(){auto&g=s_.gp;push(0xbcdf0u,g[6]);push(0xbcdf1u,g[7]);g[6]=g[0];g[7]=s_.flags.logic(0u,4);return g[6];}
    bool present(std::uint32_t datum){s_.flags.minus(datum,0xffffffffu,4);return datum!=0xffffffffu;}
    std::uint32_t objects(){return s_.gp[2]=read(0xbcdfbu,0x4ecbd0u);}
    std::uint32_t limit(std::uint32_t objects){return s_.gp[0]=read(0xbce01u,objects+0x38u);}
    bool in_bounds(std::uint32_t datum,std::uint32_t limit){
        s_.gp[1]=s_.flags.logic(datum&65535u,4);s_.flags.minus(s_.gp[1],limit,4);
        return s_.flags.sf!=s_.flags.of;
    }
    std::uint32_t record(std::uint32_t objects){
        auto&g=s_.gp;g[0]=read(0xbce10u,objects+0x24u);g[0]=arithmetic<0>(s_,g[0],g[1]);
        const auto rows=read(0xbce16u,g[2]+0x44u);return g[0]=s_.flags.plus(g[0],rows,4);
    }
    std::uint16_t salt(std::uint32_t record){const auto salt=read(0xbce19u,record,2);s_.set(1,salt,2,0);return std::uint16_t(salt);}
    bool live(std::uint16_t salt){s_.flags.logic(salt,2);return salt!=0;}
    bool matches(std::uint32_t,std::uint16_t salt){
        s_.gp[1]=std::uint32_t(std::int32_t(std::int16_t(salt)));
        s_.gp[6]=arithmetic<1>(s_,s_.gp[6]);s_.flags.minus(s_.gp[1],s_.gp[6],4);return s_.flags.zf;
    }
    void admit(std::uint32_t record){s_.gp[7]=record;}
    std::uint32_t finish(){
        auto&g=s_.gp;g[0]=s_.flags.logic(0u,4);s_.flags.logic(g[7],4);
        if(g[7]){
            s_.set(1,read(0xbce33u,g[7]+3u,1),1,0);g[2]=1u;g[2]=arithmetic<2>(s_,g[2],s_.reg(1,1,0));
            const auto mask=read(0xbce3du,g[4]+12u);s_.flags.logic(mask&g[2],4);
            if(!s_.flags.zf)g[0]=read(0xbce43u,g[7]+8u);
        }
        g[7]=pop(0xbce46u);g[6]=pop(0xbce47u);pop(0xbce48u);g[4]+=4u;return g[0];
    }
};
template<class State,class Before=Noop>std::uint32_t run_private(State&s,Before before={}){
    Frame view{s,before};return lookup(view);
}
template<class State,class Before=Noop>std::uint32_t run_source_provider(State&s,Before before={}){
    Frame<State,Before,Publication::source_provider>view{s,before};return lookup(view);
}
} // namespace halo2::native_object_body_detail
#endif

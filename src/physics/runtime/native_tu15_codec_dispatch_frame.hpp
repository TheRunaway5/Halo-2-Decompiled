#pragma once
#include "halo2/native_tu15_codec_dispatch_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_integer_fp_scope.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"

namespace halo2::native_codec_dispatch_detail {

inline constexpr std::uint32_t format_entry=0x283570u;
inline constexpr std::array field_entries{0x2840d0u,0x284140u,0x2841b0u,0x284220u,0x284290u};
inline constexpr std::array format_jumps{0x283579u,0x283583u,0x28358du,0x283597u,0x2835a1u};
inline constexpr std::array field_jumps{0x1bu,0x20u,0x29u,0x2eu,0x4au,0x4fu,0x58u,0x5du,0x67u};
inline constexpr std::array field_targets{
    std::array{0x2843f0u,0x2847d0u,0x2849f0u,0x284d30u,0x284ee0u,0x285370u,0x285630u,0x285a70u,0x284300u},
    std::array{0x286b30u,0x287130u,0x287480u,0x287a20u,0x287d00u,0x2883c0u,0x2887f0u,0x288e70u,0x284330u},
    std::array{0x28a7d0u,0x28ada0u,0x28b080u,0x28b5f0u,0x28b870u,0x28bf00u,0x28c2c0u,0x28c8f0u,0x284360u},
    std::array{0x28e0b0u,0x28e730u,0x28eaf0u,0x28f110u,0x28f460u,0x28fba0u,0x290030u,0x290710u,0x284390u},
    std::array{0x292280u,0x2928e0u,0x292c90u,0x293290u,0x2935c0u,0x293cb0u,0x2940f0u,0x2947b0u,0x2843c0u},
};
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
template<class Scope>struct Io {
    Scope* scope;
    template<class Callback>void operator()(Callback&& callback)const {scope->io(std::forward<Callback>(callback));}
};

template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>> {
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::word;using Base::at;
    unsigned profile_{};

    void load_eax(std::uint32_t pc,std::uint32_t address) {
        at(pc);s_.gp[0]=word(address);
    }
    bool equals(unsigned value) {
        if(!value)s_.flags.logic(s_.gp[0],4);
        else s_.flags.minus(s_.gp[0],value,4);
        return s_.flags.zf;
    }
    Tail tail(std::uint32_t instruction,std::uint32_t target) {
        at(instruction);return{instruction,target};
    }
    Tail finish(std::uint32_t instruction) {
        (void)Base::pop(instruction);return{instruction,0u};
    }
public:
    Frame(State& state,Before& before,Scope& scope,unsigned profile):Base{state,before,{&scope}},profile_(profile){}
    void load_format(){load_eax(format_entry,0x510f30u);}
    bool format_matches(unsigned value){return equals(value);}
    Tail tail_format(unsigned value){return tail(format_jumps[value],field_entries[value]);}
    Tail return_format(){return finish(0x2835a6u);}

    void load_field(){load_eax(field_entries[profile_],0x510f34u);}
    bool field_matches(unsigned value){return equals(value);}
    void load_variants(unsigned kind) {
        const auto pc=field_entries[profile_]+(kind?0x38u:0x09u);
        load_eax(pc,0x510f5cu);s_.flags.logic(s_.gp[0],4);
        at(pc+7u);const auto value=word(0x510f80u,1);
        s_.gp[0]=(s_.gp[0]&0xffffff00u)|value;
    }
    bool wide_nonzero()const{return !s_.flags.zf;}
    bool test_low_byte(){s_.flags.logic(s_.gp[0]&0xffu,1);return !s_.flags.zf;}
    Tail tail_field(unsigned index){return tail(field_entries[profile_]+field_jumps[index],field_targets[profile_][index]);}
    Tail return_field(){return finish(field_entries[profile_]+0x6cu);}
};

// Physical selection proof uses real tail-entry stops and real RET backing.
// It does not execute or synthesize any descendant result.
template<class State,class Before=Noop>Tail run_private(State& state,std::uint32_t entry,Before before={}) {
    native_integer_fp_detail::Scope scope{state};
    if(entry==format_entry) {
        Frame<State,Before> view{state,before,scope,0u};return format(view);
    }
    for(unsigned index=0;index<field_entries.size();++index)if(entry==field_entries[index]) {
        Frame<State,Before> view{state,before,scope,index};return field(view);
    }
    throw std::invalid_argument("Unknown private TU1.5 codec selector entry");
}

} // namespace halo2::native_codec_dispatch_detail
#endif

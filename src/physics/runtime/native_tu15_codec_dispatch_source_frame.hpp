#pragma once
#include "physics/runtime/native_tu15_codec_dispatch_frame.hpp"
#include "physics/runtime/native_tu15_codec_copy_source_io.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_codec_dispatch_detail {

struct SourceResult {
    Tail transfer;
    std::uint32_t continuation{};
};

template<class State,class Checkpoint>class SourceFrame:
    public native_codec_copy_source_detail::Io<State,Checkpoint> {
    using Base=native_codec_copy_source_detail::Io<State,Checkpoint>;
    using Base::s_;using Base::at;using Base::read_gp;using Base::retire;
    unsigned profile_{};

    bool equals(unsigned value) {
        if(!value)s_.flags.logic(s_.gp[0],4);
        else s_.flags.minus(s_.gp[0],value,4);
        return s_.flags.zf;
    }
    Tail tail(std::uint32_t instruction,std::uint32_t target) {
        at(instruction);return{instruction,target};
    }
    Tail finish(std::uint32_t instruction) {
        Base::finish_at(instruction);return{instruction,0u};
    }
public:
    SourceFrame(State& state,Checkpoint& checkpoint,unsigned profile):Base{state,checkpoint},profile_(profile){}
    void load_format(){read_gp(0u,format_entry,format_entry+5u,0x510f30u);}
    bool format_matches(unsigned value){return equals(value);}
    Tail tail_format(unsigned value){return tail(format_jumps[value],field_entries[value]);}
    Tail return_format(){return finish(0x2835a6u);}

    void load_field() {
        const auto entry=field_entries[profile_];
        read_gp(0u,entry,entry+5u,0x510f34u);
    }
    bool field_matches(unsigned value){return equals(value);}
    void load_variants(unsigned kind) {
        const auto pc=field_entries[profile_]+(kind?0x38u:0x09u);
        read_gp(0u,pc,pc+5u,0x510f5cu);
        s_.flags.logic(s_.gp[0],4);
        at(pc+7u);
        const auto value=physics_step_detail::r(s_.memory,0x510f80u,1u);
        s_.gp[0]=(s_.gp[0]&0xffffff00u)|value;
        retire(pc+12u);
    }
    bool wide_nonzero()const{return !s_.flags.zf;}
    bool test_low_byte(){s_.flags.logic(s_.gp[0]&0xffu,1);return !s_.flags.zf;}
    Tail tail_field(unsigned index){return tail(field_entries[profile_]+field_jumps[index],field_targets[profile_][index]);}
    Tail return_field(){return finish(field_entries[profile_]+0x6cu);}
};

// Exact source-side integer publication. A tail selection is returned before
// the external owner runs, and a policy revocation returns the first unrun PC.
// This helper does not supply, intercept, or reinterpret a child outcome.
template<class State,class Continue>
SourceResult run_private_source(State& state,std::uint32_t entry,Continue continuation) {
    SourceResult result{};
    result.continuation=native_codec_copy_source_detail::run(state,continuation,[&](auto& checkpoint) {
        using View=SourceFrame<State,std::remove_reference_t<decltype(checkpoint)>>;
        if(entry==format_entry) {
            View view{state,checkpoint,0u};result.transfer=format(view);return;
        }
        for(unsigned index=0;index<field_entries.size();++index)if(entry==field_entries[index]) {
            View view{state,checkpoint,index};result.transfer=field(view);return;
        }
        throw std::invalid_argument("Unknown retained TU1.5 codec selector entry");
    });
    return result;
}

} // namespace halo2::native_codec_dispatch_detail
#endif

#pragma once
#include "physics/runtime/physics_step_internal.hpp"

namespace halo2::native_codec_copy_source_detail {
struct Continuation{std::uint32_t pc;const void*owner;};

// Integer source copies use the authored provider and observer protocol.
// Unlike the physical view, no host FP image is parked around these calls.
// The source owner must establish its live cursor policy before using this IO.
template<class State,class Checkpoint>class Io {
protected:
    State&s_;Checkpoint&checkpoint_;
    void at(std::uint32_t pc){if(s_.instruction_cursor&&s_.instruction_cursor->position)*s_.instruction_cursor->position=pc;}
    void retire(std::uint32_t next){checkpoint_(next);}
    std::uint32_t word(std::uint32_t address){return physics_step_detail::r(s_.memory,address);}
    std::uint32_t read_gp(unsigned reg,std::uint32_t pc,std::uint32_t next,std::uint32_t address){
        at(pc);const auto value=word(address);s_.gp[reg]=value;retire(next);return value;
    }
    void write(std::uint32_t pc,std::uint32_t next,std::uint32_t address,std::uint32_t value){
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);retire(next);
    }
    void finish_at(std::uint32_t pc){at(pc);(void)s_.pop();}
public:
    Io(State&s,Checkpoint&checkpoint):s_(s),checkpoint_(checkpoint){}
};
template<class State,class Continue,class Run>
std::uint32_t run(State&s,Continue continuation,Run operation){
    const std::byte token{};
    const auto checkpoint=[&](std::uint32_t next){if(!continuation(s,next))throw Continuation{next,&token};};
    try{operation(checkpoint);return 0u;}
    catch(const Continuation&resume){if(resume.owner!=&token)throw;return resume.pc;}
}
} // namespace halo2::native_codec_copy_source_detail

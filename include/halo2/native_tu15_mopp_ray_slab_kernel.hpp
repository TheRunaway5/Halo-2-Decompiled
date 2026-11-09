#pragma once
#include <array>
#include <cstdint>

namespace halo2::native_mopp_ray_slab_detail {
using Vector=std::array<std::uint32_t,4>;
enum class Read {
    low_high,low_middle,low_low,high_middle,high_high,high_low,
    cache_low,cache_high,decode_low,decode_high,cache_scale_low,cache_scale_high,
    variable_scale_low,variable_scale_high,translation_low,translation_high,
    start,end,direction_start,direction_end,lower_gate,upper_gate,clip_start,clip_end,delta_test,delta_ratio,
    epsilon,numerator,ratio
};
enum class Write {decode_low,decode_high,start,end,delta,ratio,bounds};
enum class VectorRead {start,end,one};
enum class Comparison {direction,lower_gate,upper_gate,clipping};
enum class VectorMath {complement,last_term,first_term,result};
enum class Decision {continue_program,rejected};
struct Progress {
    std::uint32_t program{},opcode{},axis{},low{},high{},middle{},cache{};
    bool wide{},reverse{};unsigned clipping{};
    Vector first{},last{},one{},ratio{},last_term{},complement{},result{};
};

// Authored slab decode and two-plane segment clipping. Storage and optional
// retained publication are supplied as semantic views. Numerical decisions
// never inspect a virtual register bank, instruction cursor, or CPU flags.
template<class Frame>
Decision clip(Frame&frame,std::uint32_t&program,std::uint32_t opcode,Progress&d){
    d={};d.program=program;d.opcode=opcode;d.wide=opcode>=0x29u;
    d.axis=opcode-(d.wide?0x29u:0x26u);
    if(!d.wide){
        d.low=frame.scalar(Read::low_high,d);d.high=frame.scalar(Read::low_middle,d);
        frame.store(Write::decode_low,d.low,d);frame.axis(d);
        program+=3u;frame.advance(program,d);
        frame.integer(frame.scalar(Read::decode_low,d));
        frame.store(Write::decode_high,d.high,d);frame.integer(frame.scalar(Read::decode_high,d));
    }else{
        d.low=frame.scalar(Read::low_high,d);d.low=(d.low<<8u)+frame.scalar(Read::low_middle,d);
        d.low=(d.low<<8u)+frame.scalar(Read::low_low,d);
        d.middle=frame.scalar(Read::high_middle,d);frame.store(Write::decode_low,d.low,d);
        d.cache=frame.scalar(Read::cache_low,d);frame.axis(d);
        frame.integer(frame.scalar(Read::decode_low,d));frame.multiply_scalar(frame.scalar(Read::cache_scale_low,d));
        d.high=frame.scalar(Read::high_high,d);d.high=(d.high<<8u)+d.middle;
        d.high=(d.high<<8u)+frame.scalar(Read::high_low,d);
        frame.multiply_scalar(frame.scalar(Read::variable_scale_low,d));frame.store(Write::decode_high,d.high,d);
        d.cache=frame.scalar(Read::cache_high,d);frame.subtract_scalar(frame.scalar(Read::translation_low,d));
        program+=7u;frame.advance(program,d);
        frame.integer(frame.scalar(Read::decode_high,d));frame.multiply_scalar(frame.scalar(Read::cache_scale_high,d));
        frame.multiply_scalar(frame.scalar(Read::variable_scale_high,d));frame.subtract_scalar(frame.scalar(Read::translation_high,d));
    }
    frame.floating(frame.scalar(Read::start,d));frame.spill(Write::start,true,d);
    frame.floating(frame.scalar(Read::end,d));frame.spill(Write::end,true,d);
    frame.floating(frame.scalar(Read::direction_start,d));
    d.reverse=frame.compare_scalar(frame.scalar(Read::direction_end,d),Comparison::direction,d).not_less();
    frame.floating(frame.scalar(Read::lower_gate,d));
    if(frame.compare_stack(true,Comparison::lower_gate,d).less()){
        frame.discard();frame.discard();return Decision::rejected;
    }
    frame.floating(frame.scalar(Read::upper_gate,d));
    if(frame.compare_stack(false,Comparison::upper_gate,d).greater()){
        frame.discard();frame.discard();return Decision::rejected;
    }
    frame.direction(d);
    // Source caches both full vectors once. Later spills and clipped writes
    // may alias those bounds, while scratch scalar reads remain live.
    frame.floating(frame.scalar(Read::clip_start,d));
    d.first=frame.vector(VectorRead::start,d);d.last=frame.vector(VectorRead::end,d);
    for(d.clipping=0;d.clipping<2;++d.clipping){
        if(d.clipping)frame.floating(frame.scalar(Read::clip_start,d));
        frame.subtract_second();frame.spill(Write::delta,true,d);
        frame.reverse_subtract(frame.scalar(Read::clip_end,d));
        frame.floating(frame.scalar(Read::delta_test,d));frame.multiply_second();
        if(frame.compare_scalar(frame.scalar(Read::epsilon,d),Comparison::clipping,d).less()){
            frame.floating(frame.scalar(Read::delta_ratio,d));
            if(!d.clipping)d.one=frame.vector(VectorRead::one,d);
            if(d.clipping)frame.prepare_ratio(d);
            frame.subtract_second();
            if(!d.clipping)frame.prepare_ratio(d);
            frame.reverse_divide(frame.scalar(Read::numerator,d));frame.ratio_target(d);
            frame.spill(Write::ratio,true,d);d.ratio=frame.broadcast(frame.scalar(Read::ratio,d));
            if(!d.clipping)d.complement=frame.subtract(d.one,d.ratio,VectorMath::complement,d);
            d.last_term=frame.multiply(d.ratio,d.last,VectorMath::last_term,d);
            if(d.clipping){d.one=frame.vector(VectorRead::one,d);d.complement=frame.subtract(d.one,d.ratio,VectorMath::complement,d);}
            const auto first_term=frame.multiply(d.complement,d.first,VectorMath::first_term,d);
            d.result=frame.add(first_term,d.last_term,VectorMath::result,d);
            frame.store_vector(Write::bounds,d.result,d);
        }
        frame.discard();
    }
    return Decision::continue_program;
}
} // namespace halo2::native_mopp_ray_slab_detail

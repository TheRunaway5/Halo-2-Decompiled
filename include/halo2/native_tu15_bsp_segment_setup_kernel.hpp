#pragma once
#include "halo2/native_tu15_bsp_segment_kernel.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_bsp_segment_setup_detail {
using native_bsp_segment_detail::Scalar;
using native_bsp_segment_detail::scalar;
using native_bsp_segment_detail::bits;
using native_bsp_segment_detail::compare;
using Comparison=native_bsp_segment_detail::Comparison;
enum class Phase {begin,context_flags,argument_limit,context_descriptor,argument_bitmap,context_limit,
    argument_origin,context_bitmap,argument_displacement,context_origin,context_displacement,context_output,
    select_initial,clear_cursor,output_fraction,output_count,second_comparison,previous_leaf,previous_kind,
    partition,constant_one,cap_comparison,maximum_zero,maximum_one,maximum_epsilon,maximum,traverse,finished};
enum class Write {flags,descriptor,limit,bitmap,origin,displacement,output,fraction,count,previous_leaf,previous_kind,partition,maximum_zero,maximum_one,maximum_epsilon};
struct Progress {
    std::uint32_t context{},flags{},descriptor{},output{},limit{},bitmap{},origin{},displacement{},one{},maximum{};
    Scalar epsilon{},zero{},initial{};Comparison first,second,cap;bool accepted{};
};
// Actual1E68E0's initialized query lifetime and epsilon policy. The public
// value contract and retained caller frame acquire arguments through separate
// adapters, preserving each authored read/store point and real traversal.
template<class Memory,class Inputs,class Notify>
bool setup(Memory&memory,Inputs&input,Notify notify){
    Progress d;d.context=input.context();d.flags=input.flags();d.descriptor=input.descriptor();d.output=input.output();d.epsilon=input.epsilon();
    const auto write=[&](Phase phase,Write kind,std::uint32_t address,std::uint32_t value,unsigned width=4){notify(phase,d);input.store(kind,address,value,width);};
    notify(Phase::begin,d);d.first=compare(d.zero,d.epsilon);
    write(Phase::context_flags,Write::flags,d.context,d.flags);
    notify(Phase::argument_limit,d);d.limit=input.limit();
    write(Phase::context_descriptor,Write::descriptor,d.context+4u,d.descriptor);
    notify(Phase::argument_bitmap,d);d.bitmap=input.bitmap();
    write(Phase::context_limit,Write::limit,d.context+8u,d.limit,2);
    notify(Phase::argument_origin,d);d.origin=input.origin();
    write(Phase::context_bitmap,Write::bitmap,d.context+12u,d.bitmap);
    notify(Phase::argument_displacement,d);d.displacement=input.displacement();
    write(Phase::context_origin,Write::origin,d.context+16u,d.origin);
    write(Phase::context_displacement,Write::displacement,d.context+20u,d.displacement);
    write(Phase::context_output,Write::output,d.context+24u,d.output);
    d.initial=d.first.above()?d.zero:d.epsilon;notify(Phase::select_initial,d);notify(Phase::clear_cursor,d);
    write(Phase::output_fraction,Write::fraction,d.output,bits(d.initial));
    write(Phase::output_count,Write::count,d.output+24u,0);
    d.second=compare(d.zero,d.epsilon);notify(Phase::second_comparison,d);
    write(Phase::previous_leaf,Write::previous_leaf,d.context+28u,0xffffffffu);
    write(Phase::previous_kind,Write::previous_kind,d.context+32u,0,1);
    write(Phase::partition,Write::partition,d.context+36u,0xffffffffu);
    if(d.second.above())write(Phase::maximum_zero,Write::maximum_zero,0,0);
    else{
        notify(Phase::constant_one,d);d.one=memory.word(0x46a0d0u);
        d.cap=compare(d.epsilon,scalar(d.one));notify(Phase::cap_comparison,d);
        d.maximum=d.one;write(Phase::maximum_one,Write::maximum_one,0,d.maximum);
        if(!d.cap.above()){d.maximum=bits(d.epsilon);write(Phase::maximum_epsilon,Write::maximum_epsilon,0,d.maximum);}
    }
    notify(Phase::maximum,d);d.maximum=input.maximum(d.maximum);
    notify(Phase::traverse,d);d.accepted=input.traverse(d);notify(Phase::finished,d);return d.accepted;
}
} // namespace halo2::native_bsp_segment_setup_detail
#endif

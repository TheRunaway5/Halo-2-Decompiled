#pragma once
#include <array>
#include <cstdint>

namespace halo2::native_mopp_ray_scale_detail {
using Vector=std::array<std::uint32_t,4>;
enum class Read {
    x,y,z,start_initial,end_initial,translation,start_scaled,end_scaled,
    variable_translation,variable_shift,variable_scale,variable_property,variable_key,
    scratch_x,scratch_y,scratch_z,scratch_scale,scratch_broadcast
};
enum class Write {
    scratch_x,scratch_y,scratch_z,scratch_scale,translation_x,translation_y,translation_z,
    translation_w,start_translated,end_translated,scale_word,start_scaled,end_scaled,
    variable_translation,variable_shift,variable_scale,variable_property,variable_key
};
struct Progress {
    std::uint32_t opcode{},program{},x{},y{},z{},scale_integer{},scale_bits{},shift{},property{},key{};
    Vector start{},end{},translation{},scale{},variable_translation{};
};

// Native quantization of a MOPP ray. Named accesses retain the actual lazy
// DATA order, including private scratch reuse and interleaved vector writes.
// Frame supplies ordinary value storage or a separately qualified caller view;
// no numerical decision depends on a processor register file or flags.
template<class Frame>
void scale(Frame&frame,std::uint32_t&program,std::uint32_t opcode){
    Progress d{};d.opcode=opcode;d.program=program;
    d.x=frame.scalar(Read::x,d);d.y=frame.scalar(Read::y,d);
    d.start=frame.vector(Read::start_initial,d);
    frame.store(Write::scratch_x,d.x,d);d.z=frame.scalar(Read::z,d);
    frame.integer(frame.scalar(Read::scratch_x,d));frame.store(Write::scratch_y,d.y,d);
    d.scale_integer=1u;frame.spill(Write::translation_x,true,d);
    frame.store(Write::translation_w,0u,d);
    frame.integer(frame.scalar(Read::scratch_y,d));frame.store(Write::scratch_z,d.z,d);
    const auto scale_integer=1u<<opcode;d.scale_integer=scale_integer;
    frame.spill(Write::translation_y,true,d);
    frame.integer(frame.scalar(Read::scratch_z,d));frame.spill(Write::translation_z,true,d);
    d.translation=frame.vector(Read::translation,d);
    d.start=frame.subtract(d.start,d.translation);frame.store_vector(Write::start_translated,d.start,d);
    d.end=frame.vector(Read::end_initial,d);frame.store(Write::scratch_scale,d.scale_integer,d);
    frame.integer(frame.scalar(Read::scratch_scale,d));
    d.end=frame.subtract(d.end,d.translation);frame.store_vector(Write::end_translated,d.end,d);
    d.start=frame.vector(Read::start_scaled,d);
    frame.spill(Write::scale_word,false,d);d.scale_bits=frame.scalar(Read::scratch_broadcast,d);
    d.scale={d.scale_bits,d.scale_bits,d.scale_bits,d.scale_bits};
    d.start=frame.multiply(d.scale,d.start);frame.store_vector(Write::start_scaled,d.start,d);
    d.end=frame.vector(Read::end_scaled,d);
    d.end=frame.multiply(d.scale,d.end);frame.store_vector(Write::end_scaled,d.end,d);
    d.variable_translation=frame.vector(Read::variable_translation,d);
    d.variable_translation=frame.add(d.variable_translation,d.translation);
    d.variable_translation=frame.multiply(d.scale,d.variable_translation);
    frame.store_vector(Write::variable_translation,d.variable_translation,d);
    d.shift=frame.scalar(Read::variable_shift,d)+opcode;frame.store(Write::variable_shift,d.shift,d);
    frame.multiply_scalar(frame.scalar(Read::variable_scale,d));
    program+=4u;frame.advance(program,d);frame.spill(Write::variable_scale,true,d);
    d.property=frame.scalar(Read::variable_property,d);frame.store(Write::variable_property,d.property,d);
    d.key=frame.scalar(Read::variable_key,d);frame.store(Write::variable_key,d.key,d);
    frame.finish(d);
}
} // namespace halo2::native_mopp_ray_scale_detail

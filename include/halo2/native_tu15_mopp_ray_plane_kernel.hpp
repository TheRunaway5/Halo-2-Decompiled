#pragma once
#include "halo2/native_tu15_mopp_ray_projection_kernel.hpp"
#include <cstdint>

namespace halo2::native_mopp_ray_plane_detail {
enum class Family {projected,axis,thin_axis,long_axis};
enum class Read {low,high,scratch_low,scratch_high,left_low,right_low,left_high,right_high,
                 right_short,left_offset,thin_delta};
enum class Write {scratch_low,scratch_high,low_plane,high_plane,axis,left_offset,right_scratch,end_projection};
struct Progress {
    Family family{};
    std::uint32_t program{},opcode{},axis{999u},a{},b{},low_integer{},high_integer{};
    std::uint32_t left_low{},right_low{},left_high{},right_high{},left{},right{},endpoint{};
    std::uint32_t low_bits{},high_bits{},end_bits{};
};
inline std::uint32_t boundary(std::uint32_t opcode,std::uint32_t byte){
    if(opcode<0x13u||opcode>0x1cu)return byte;
    const bool triple=opcode>=0x19u;
    const auto bias=(opcode==0x14u||opcode==0x16u||opcode==0x18u)?255:
        opcode==0x1au||opcode==0x1bu?85:opcode==0x1cu?170:0;
    const auto value=std::int32_t(byte);
    return std::uint32_t(triple?3*(value-bias):2*value-bias);
}

// Native authored split-plane DATA and endpoint projection. Named scratch
// phases preserve lazy/overlapping caller views without exposing GP registers,
// processor flags or instruction execution to the numerical controller.
template<class Frame>
bool prepare(Frame&frame,std::uint32_t&program,std::uint32_t opcode,Progress&d){
    d={};d.program=program;d.opcode=opcode;
    if(opcode>=0x13u&&opcode<=0x1cu)d.family=Family::projected;
    else if(opcode>=0x10u&&opcode<=0x12u){d.family=Family::axis;d.axis=opcode-0x10u;}
    else if(opcode>=0x20u&&opcode<=0x22u){d.family=Family::thin_axis;d.axis=opcode-0x20u;}
    else if(opcode>=0x23u&&opcode<=0x25u){d.family=Family::long_axis;d.axis=opcode-0x23u;}
    else return false;
    d.a=frame.scalar(Read::low,d);d.low_integer=boundary(opcode,d.a);
    if(opcode==0x1bu)frame.store(Write::scratch_low,d.low_integer,d);
    if(d.family==Family::thin_axis){
        d.right=frame.scalar(Read::right_short,d);
        frame.store(Write::scratch_low,d.low_integer,d);
        program+=3u;frame.advance(program,d);
        frame.integer(frame.scalar(Read::scratch_low,d));
        frame.store(Write::axis,d.axis,d);frame.store(Write::left_offset,0u,d);
        d.high_bits=frame.spill(Write::high_plane,false,d);
        frame.store(Write::right_scratch,d.right,d);
        frame.add_scalar(frame.scalar(Read::thin_delta,d));
        d.low_bits=frame.spill(Write::low_plane,true,d);
    }else{
        d.b=frame.scalar(Read::high,d);d.high_integer=boundary(opcode,d.b);
        if(opcode!=0x1bu)frame.store(Write::scratch_low,d.low_integer,d);
        if(d.family==Family::axis)frame.store(Write::axis,d.axis,d);
        if(d.family==Family::long_axis)d.left_low=frame.scalar(Read::left_low,d);
        frame.integer(frame.scalar(Read::scratch_low,d));
        if(opcode==0x1bu){
            d.low_bits=frame.spill(Write::low_plane,true,d);
            frame.store(Write::scratch_high,d.high_integer,d);
        }else{
            frame.store(Write::scratch_high,d.high_integer,d);
            if(d.family==Family::long_axis)d.right_low=frame.scalar(Read::right_low,d);
            d.low_bits=frame.spill(Write::low_plane,true,d);
        }
        frame.integer(frame.scalar(Read::scratch_high,d));
        if(d.family==Family::long_axis){
            frame.store(Write::axis,d.axis,d);program+=7u;frame.advance(program,d);
        }
        d.high_bits=frame.spill(Write::high_plane,true,d);
    }
    for(d.endpoint=0;d.endpoint<2;++d.endpoint){
        if(d.family==Family::projected){
            struct Arithmetic {
                Frame&frame;
                void begin(std::uint32_t value){frame.floating(value);}
                void add(std::uint32_t value){frame.add_scalar(value);}
                void subtract(std::uint32_t value){frame.subtract_scalar(value);}
            };
            native_mopp_ray_projection_detail::project(opcode,
                [&](unsigned axis,unsigned term){return frame.endpoint(axis,term,d);},Arithmetic{frame});
        }else frame.floating(frame.endpoint(d.axis,0u,d));
    }
    if(d.family==Family::long_axis){
        d.left_high=frame.scalar(Read::left_high,d);d.left=(d.left_high<<8u)+d.left_low;
        frame.store(Write::left_offset,d.left,d);
        d.right_high=frame.scalar(Read::right_high,d);d.right=(d.right_high<<8u)+d.right_low;
    }else if(d.family!=Family::thin_axis){
        d.right=frame.scalar(Read::right_short,d);program+=4u;frame.advance(program,d);
        frame.store(Write::left_offset,0u,d);
    }
    if(d.family!=Family::thin_axis){
        d.left=frame.scalar(Read::left_offset,d);frame.store(Write::right_scratch,d.right,d);
    }
    d.end_bits=frame.spill(Write::end_projection,true,d);
    return true;
}
}

#pragma once
#include "halo2/native_tu15_affine_inverse_kernel.hpp"
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_affine_inverse_detail {
template<class ReadWord,class WriteWord>struct Values {
    ReadWord read;WriteWord write;std::uint32_t source{},destination{};
    void begin(const Progress&){}
    bool zero_scale(const Progress&d){return compare(d.initial_scale,0).same();}
    bool unit_scale(const Progress&d){return compare(d.retained_scale,d.one).same();}
    void clear_begin(const Progress&){}
    void clear_word(const Progress&,unsigned n){write(0x1441b7u,destination+n*4u,0u);}
    void finish(const Progress&,bool){}
    std::uint32_t word(Read field,const Progress&){
        switch(field){
        case Read::scale:case Read::retained_scale:return read(source);
        case Read::one:return read(0x46a0d0u);
        case Read::position_x:return read(source+40u);
        case Read::position_y:return read(source+44u);
        case Read::position_z:return read(source+48u);
        case Read::forward_x:return read(source+4u);
        case Read::left_y:return read(source+20u);
        case Read::output_forward_x:return read(destination+4u);
        case Read::up_z:return read(source+36u);
        case Read::left_x:return read(source+16u);
        case Read::forward_y:return read(source+8u);
        case Read::up_x:return read(source+28u);
        case Read::forward_z:return read(source+12u);
        case Read::output_up_x:return read(destination+28u);
        case Read::output_left_x:return read(destination+16u);
        case Read::up_y:return read(source+32u);
        case Read::left_z:return read(source+24u);
        case Read::output_forward_y:return read(destination+8u);
        case Read::output_left_z:return read(destination+32u);
        case Read::output_left_y:return read(destination+20u);
        case Read::output_forward_z:return read(destination+12u);
        case Read::output_up_z:return read(destination+36u);
        }
        throw std::runtime_error("Invalid affine inverse value acquisition");
    }
    void store(Write field,const Progress&d,std::uint32_t value){
        std::uint32_t pc{},offset{};
        switch(field){
        case Write::scale:pc=d.unit_scale?0x1440f6u:0x1440e2u;break;
        case Write::forward_x:pc=0x1440fdu;offset=4;break;
        case Write::left_y:pc=0x144108u;offset=20;break;
        case Write::up_z:pc=0x14410eu;offset=36;break;
        case Write::forward_y:pc=0x144119u;offset=8;break;
        case Write::left_x:pc=0x14411eu;offset=16;break;
        case Write::up_x:pc=0x14412du;offset=28;break;
        case Write::forward_z:pc=0x144142u;offset=12;break;
        case Write::position_x:pc=0x14415cu;offset=40;break;
        case Write::left_z:pc=0x144165u;offset=32;break;
        case Write::position_y:pc=0x144181u;offset=44;break;
        case Write::up_y:pc=0x144194u;offset=24;break;
        case Write::position_z:pc=0x1441a9u;offset=48;break;
        }
        write(pc,destination+offset,value);
    }
};
} // namespace halo2::native_affine_inverse_detail
#endif

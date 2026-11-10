#pragma once
#include <array>
#include <stdexcept>
#include "halo2/native_tu15_camera_collision_publication_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_camera_collision_publication_detail {
template<class ReadWord,class WriteWord>struct Values {
    ReadWord read;WriteWord write;
    void begin(const Progress&){}
    void copy_begin(const Progress&){}
    void copy_end(const Progress&){}
    void finish(const Progress&){}
    std::uint32_t integer(Read field,const Progress&d){
        switch(field){
        case Read::fraction:return read(d.collision);
        case Read::plane:return read(d.collision+4u);
        case Read::index:return read(d.collision+16u);
        case Read::copied_value:return read(d.plane+d.lane*4u);
        case Read::point:return read(d.collision+8u+d.lane*4u);
        case Read::kind:return read(d.collision+20u,1u);
        case Read::surface:return read(d.collision+21u,1u);
        case Read::material:return read(d.collision+22u,2u);
        default:throw std::runtime_error("Invalid camera publication integer field");
        }
    }
    Scalar scalar(Read field,const Progress&d){
        std::uint32_t address{};
        switch(field){
        case Read::plane_y:address=d.plane+4u;break;
        case Read::plane_z:address=d.plane+8u;break;
        case Read::plane_x:address=d.plane;break;
        case Read::row_x_z:address=d.transform+28u;break;
        case Read::row_x_y:address=d.transform+16u;break;
        case Read::row_x_x:address=d.transform+4u;break;
        case Read::row_y_z:address=d.transform+32u;break;
        case Read::row_y_y:address=d.transform+20u;break;
        case Read::row_y_x:address=d.transform+8u;break;
        case Read::row_z_z:address=d.transform+36u;break;
        case Read::row_z_y:address=d.transform+24u;break;
        case Read::row_z_x:address=d.transform+12u;break;
        case Read::distance_z:address=d.transform+48u;break;
        case Read::plane_w:address=d.plane+12u;break;
        case Read::scale:address=d.transform;break;
        case Read::distance_y:address=d.transform+44u;break;
        case Read::stored_y:address=d.output+44u;break;
        case Read::stored_x:address=d.output+40u;break;
        case Read::distance_x:address=d.transform+40u;break;
        case Read::negated_value:address=(d.transformed?d.output+40u:d.plane)+d.lane*4u;break;
        default:throw std::runtime_error("Invalid camera publication scalar field");
        }
        return N::scalar(read(address));
    }
    void store(Write field,const Progress&d,std::uint32_t value){
        std::uint32_t pc{},address{};unsigned width=4;
        switch(field){
        case Write::fraction:pc=0x16b9a5u;address=d.output+4u;break;
        case Write::normal:{constexpr std::array sites{0x16b9e2u,0x16ba0au,0x16ba32u,0x16ba69u};pc=sites[d.lane];address=d.output+40u+d.lane*4u;break;}
        case Write::negated_value:{constexpr std::array transformed{0x16ba84u,0x16ba91u,0x16ba9eu,0x16baa8u};constexpr std::array plain{0x16bac3u,0x16bad0u,0x16baddu,0x16bae7u};pc=d.transformed?transformed[d.lane]:plain[d.lane];address=d.output+40u+d.lane*4u;break;}
        case Write::copied_value:{constexpr std::array sites{0x16baf7u,0x16bafcu,0x16bb02u,0x16bb08u};pc=sites[d.lane];address=d.output+40u+d.lane*4u;break;}
        case Write::point:{constexpr std::array sites{0x16bb0fu,0x16bb15u,0x16bb1bu};pc=sites[d.lane];address=d.output+76u+d.lane*4u;break;}
        case Write::kind:pc=0x16bb21u;address=d.output+88u;width=1;break;
        case Write::surface:pc=0x16bb27u;address=d.output+89u;width=1;break;
        case Write::material:pc=0x16bb2eu;address=d.output+90u;width=2;break;
        }
        write(pc,address,value,width);
    }
};
} // namespace halo2::native_camera_collision_publication_detail
#endif

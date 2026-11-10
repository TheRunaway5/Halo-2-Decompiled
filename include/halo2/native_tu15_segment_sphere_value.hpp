#pragma once
#include "halo2/native_tu15_segment_sphere_kernel.hpp"
#include <array>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_segment_sphere_detail {
// Value facade has caller-owned local scratch. The raw adapter supplies the
// actual virtual stack instead, so no synthetic stack accesses leak into this
// existing public DATA interface.
template<class ReadWord>struct Values {
    ReadWord read;std::uint32_t first{},center{},direction{},radius{};
    std::array<std::uint32_t,3>scratch{};
    Scalar scalar(Read field,const Progress&){
        switch(field){
        case Read::center_y:return N::scalar(read(center+4u));
        case Read::first_y:return N::scalar(read(first+4u));
        case Read::center_z:return N::scalar(read(center+8u));
        case Read::first_z:return N::scalar(read(first+8u));
        case Read::center_x:return N::scalar(read(center));
        case Read::first_x:return N::scalar(read(first));
        case Read::radius:return N::scalar(radius);
        case Read::scratch_z:return N::scalar(scratch[2]);
        case Read::scratch_y:return N::scalar(scratch[1]);
        case Read::scratch_x:return N::scalar(scratch[0]);
        default:throw std::runtime_error("Invalid value segment sphere scalar field");
        }
    }
    std::uint32_t integer(Read field,const Progress&){
        switch(field){
        case Read::direction_x:return read(direction);
        case Read::direction_y:return read(direction+4u);
        case Read::direction_z:return read(direction+8u);
        default:throw std::runtime_error("Invalid value segment sphere integer field");
        }
    }
    void store(Write field,const Progress&,std::uint32_t value){scratch[field==Write::scratch_z?2u:field==Write::scratch_y?1u:0u]=value;}
    void reserve(const Progress&){}
    Comparison test(Test,const Progress&,Scalar first_value,Scalar second_value){return compare(first_value,second_value);}
    void finish(Result,const Progress&){}
};
} // namespace halo2::native_segment_sphere_detail
#endif

#pragma once
#include "halo2/cache.hpp"
#include "halo2/native_tu15_packed_rotation_kernel.hpp"
#include <array>
#include <cmath>
#include <cstdint>

namespace halo2::native_packed_rotation_detail {
class ValueView{
    std::array<std::int16_t,4>packed_;float seed_;
public:
    using Vector=std::array<float,4>;Vector result{};
    ValueView(std::array<std::int16_t,4>packed,float seed):packed_(packed),seed_(seed){}
    Vector input()const{return {float(packed_[0]),float(packed_[1]),float(packed_[2]),float(packed_[3])};}
    Vector multiply(Vector left,Vector right)const{for(unsigned n=0;n<4;++n)left[n]*=right[n];return left;}
    Vector add(Vector left,Vector right)const{for(unsigned n=0;n<4;++n)left[n]+=right[n];return left;}
    Vector scale(Vector left,Vector right)const{return multiply(left,right);}
    Vector opposite(Vector value)const{return {value[2],value[3],value[0],value[1]};}
    Vector alternate(Vector value)const{return {value[1],value[0],value[1],value[0]};}
    Vector inverse(Vector norm,Vector,Vector,Vector)const{
        if(!(norm[0]>0)||!std::isfinite(seed_)||seed_<=0)
            throw FormatError("invalid original grip quaternion seed");
        return {seed_,seed_,seed_,seed_};
    }
    void output(Vector value,Vector){result=value;}
};
inline float norm_value(std::array<std::int16_t,4>packed){
    ValueView view{packed,0};return squared_norm(view,view.input()).value[0];
}
inline std::array<float,4> with_seed(std::array<std::int16_t,4>packed,float seed){
    ValueView view{packed,seed};decode(view);return view.result;
}
} // namespace halo2::native_packed_rotation_detail

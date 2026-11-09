#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_polygon_support_kernel.hpp"
#include <xmmintrin.h>
#include <type_traits>

namespace halo2::native_polygon_support_detail {
// Public DATA view. This owns no retained registers or source instruction
// stream; the environment protects the geometric FP policy around providers.
template<class Memory,class Environment>class ValueFrame {
    Memory&m_;Environment&environment_;
    std::uint32_t shape_,direction_,output_,scratch_;
    template<class Function>decltype(auto) io(Function&&function){
        environment_.publish();
        try{
            if constexpr(std::is_void_v<std::invoke_result_t<Function>>){
                function();environment_.resume();return;
            }else{
                auto result=function();environment_.resume();return result;
            }
        }catch(...){environment_.resume();throw;}
    }
    static __m128 packed(Vector value){return std::bit_cast<__m128>(value);}
    static Vector words(__m128 value){return std::bit_cast<Vector>(value);}
public:
    ValueFrame(Memory&m,Environment&environment,std::uint32_t shape,std::uint32_t direction,std::uint32_t output,std::uint32_t scratch)
        :m_(m),environment_(environment),shape_(shape),direction_(direction),output_(output),scratch_(scratch){}
    void begin(){}
    std::uint32_t count(){return io([&]{return m_.word(shape_+0x14u);});}
    Vector initial_best(){return {io([&]{return m_.word(0x44aac8u);}),0,0,0};}
    void initialize(){}
    void count_test(const Progress&){}
    Vector direction(){return words(io([&]{return m_.vector(direction_);}));}
    void vertices(){}
    Vector vertex(const Progress&d){return words(io([&]{return m_.vector(shape_+0x20u+d.index*16u);}));}
    Vector multiply(Vector left,Vector right){auto value=packed(left);const auto other=packed(right);asm volatile("mulps %1,%0":"+x"(value):"x"(other):"memory");return words(value);}
    Vector add_low(Vector left,Vector right){auto value=packed(left);const auto other=packed(right);asm volatile("addss %1,%0":"+x"(value):"x"(other):"memory");return words(value);}
    Vector shuffle_y(Vector value){const auto input=packed(value);return words(_mm_shuffle_ps(input,input,0x55));}
    Vector shuffle_z(Vector value){const auto input=packed(value);return words(_mm_shuffle_ps(input,input,0xaa));}
    void store_dot(const Progress&d){io([&]{m_.word(0x1f6656u,scratch_,d.dot[0]);});}
    Vector load_dot(){return {io([&]{return m_.word(scratch_);}),0,0,0};}
    bool greater(const Progress&d){const auto candidate=packed(d.candidate),best=packed(d.best);unsigned char above;asm volatile("comiss %2,%1;seta %0":"=qm"(above):"x"(candidate),"x"(best):"cc","memory");return above;}
    void improved(const Progress&){}
    void advance(const Progress&){}
    std::uint32_t output(){return output_;}
    void select(const Progress&){}
    Vector selected_vertex(const Progress&d){return words(io([&]{return m_.vector(shape_+(d.selected+2u)*16u);}));}
    void restore_before_store(){}
    void write_result(const Progress&d){io([&]{m_.vector(0x1f6681u,d.output,packed(d.result));});}
    void finish(){}
};
}
#endif

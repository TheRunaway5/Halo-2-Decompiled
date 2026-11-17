#pragma once
#include <array>

namespace halo2::native_codec_quaternion_product_detail {
enum class Field { left_x,left_y,left_z,left_w,right_x,right_y,right_z,right_w };
struct Term {Field first,second;bool subtract;};

// The left operand of each binary32 product and its addition/subtraction
// order are observable for exceptional inputs. Store each component before
// acquiring the next: partial output aliases can affect those later reads.
using enum Field;
inline constexpr std::array terms{
    std::array{Term{right_w,left_x,false},Term{right_z,left_y,false},Term{left_w,right_x,false},Term{right_y,left_z,true}},
    std::array{Term{left_z,right_x,false},Term{right_w,left_y,false},Term{left_w,right_y,false},Term{left_x,right_z,true}},
    std::array{Term{right_w,left_z,false},Term{right_y,left_x,false},Term{left_w,right_z,false},Term{right_x,left_y,true}},
    std::array{Term{right_w,left_w,false},Term{left_x,right_x,true},Term{right_y,left_y,true},Term{left_z,right_z,true}},
};

template<class View>void quaternion_product(View& view) {
    if(view.begin())view.stage_left();
    if(view.right_aliases_output())view.stage_right();
    auto& result=view.accumulator();
    auto& term=view.term();
    for(unsigned component=0;component<terms.size();++component) {
        const auto& row=terms[component];
        result=view.read(component,0u,row[0].first);
        result=view.multiply(result,view.read(component,1u,row[0].second));
        for(unsigned index=1;index<row.size();++index) {
            term=view.read(component,index*2u,row[index].first);
            term=view.multiply(term,view.read(component,index*2u+1u,row[index].second));
            result=row[index].subtract?view.subtract(result,term):view.add(result,term);
        }
        view.store_component(component,result);
    }
    view.finish();
}
} // namespace halo2::native_codec_quaternion_product_detail

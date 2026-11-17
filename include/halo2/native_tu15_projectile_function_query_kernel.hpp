#pragma once
#include <cstdint>

namespace halo2::native_projectile_function_query_detail {
enum class Kind {unsupported,direct_value,first_flag,ratio,second_flag,scaled_value};
// Original100E70 has five supported selectors. Only the scaled path clamps;
// ratio/direct paths publish their original bits, including signed zero/NaN.
// Each view operation owns the authored lazy reads and observable ABI boundary.
template<class View>std::uint32_t query(View&view){
    switch(view.begin()){
        case Kind::unsupported:return view.unsupported();
        case Kind::direct_value:view.direct_value();break;
        case Kind::first_flag:view.flag_value(false);break;
        case Kind::second_flag:view.flag_value(true);break;
        case Kind::ratio:
            if(view.ratio_enabled())view.ratio();else view.zero();
            break;
        case Kind::scaled_value:
            if(view.selected_one()||!view.first_parameter()||!view.second_parameter())view.one();
            else{
                view.scaled_value();
                if(view.below_zero())view.zero();
                else{view.upper_limit();if(view.above_upper_limit())view.take_upper_limit();}
            }
            break;
    }
    return view.publish();
}
} // namespace halo2::native_projectile_function_query_detail

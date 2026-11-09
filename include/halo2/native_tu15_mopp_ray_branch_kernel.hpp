#pragma once
#include <cstdint>

namespace halo2::native_mopp_ray_branch_detail {
enum class Comparison {end_high,start_high,start_low,end_low};
enum class Decision {continue_program,crossing};

// Native segment/plane classification. Frame supplies the real lazy plane and
// offset views and native FP comparisons. The controller never branches on a
// virtual processor flag; retained flag publication belongs to the caller view.
template<class Frame>
Decision choose(Frame&frame,std::uint32_t&program){
    frame.floating(frame.end_projection(false));
    const auto end_high=frame.compare(Comparison::end_high);
    frame.publish_comparison(Comparison::end_high,end_high);
    if(end_high.less()){
        const auto start_high=frame.compare(Comparison::start_high);
        frame.publish_comparison(Comparison::start_high,start_high);
        if(start_high.less()){
            program+=frame.left_offset();frame.advance_left(program);
            frame.discard();return Decision::continue_program;
        }
    }
    const auto start_low=frame.compare(Comparison::start_low);
    program+=frame.right_offset();frame.advance_right(program);
    frame.publish_comparison(Comparison::start_low,start_low);
    if(start_low.greater()){
        frame.floating(frame.end_projection(true));
        const auto end_low=frame.compare(Comparison::end_low);
        frame.publish_comparison(Comparison::end_low,end_low);
        if(end_low.greater()){
            frame.discard();return Decision::continue_program;
        }
    }
    return Decision::crossing;
}
}

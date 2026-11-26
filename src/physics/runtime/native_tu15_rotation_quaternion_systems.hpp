#pragma once
#include <cstdint>

namespace halo2::native_rotation_quaternion {
inline constexpr std::uint32_t zero=0x46a0e8u,one=0x46a0d0u,half=0x46a0ccu;
inline constexpr unsigned stride=4u,w=3u;

constexpr std::uint32_t element(std::uint32_t matrix,unsigned row,unsigned column) {
    return matrix+4u*(row+stride*column);
}

enum class Access {
    matrix_arg,trace_y,save_output,trace_x,trace_z,trace_zero,
    positive_one,positive_reciprocal_half,positive_x_first,positive_x_second,positive_x_result,
    positive_y_first,positive_y_second,positive_y_result,positive_z_first,positive_z_second,
    positive_z_result,positive_w_half,positive_w,
    diagonal_y,diagonal_x,cycle_first,cycle_second,cycle_third,diagonal_z,diagonal_leader,axis_next,axis_last,
    trace_last,trace_next,trace_major,restore_output,diagonal_one,diagonal_reciprocal_half,
    diagonal_result_half,major_result,diagonal_w_first,diagonal_w_second,diagonal_w_result,
    next_first,next_second,next_result,last_first,last_second,last_result,count,
};

template<bool Add,class Memory,class Frame>
void cross_term(Memory &memory,Frame &frame,std::uint32_t a,std::uint32_t b,std::uint32_t result,
                Access first,Access second,Access store) {
    frame.load_extended(memory.load32(first,a));
    const auto other=memory.load32(second,b);
    if constexpr(Add)frame.add(other);
    else frame.subtract(other);
    frame.apply_reciprocal();
    memory.store32(store,result,frame.word());
    frame.pop();
}

// The source's 3 x 3 rotation uses a four-word column stride. It emits
// quaternion XYZ first when the trace is positive. Otherwise it emits
// the largest diagonal component followed by W and the other two axes.
template<class Memory,class Frame>
void convert(Memory &memory,Frame &frame,std::uint32_t output) {
    using enum Access;
    const auto matrix=memory.load32(matrix_arg,frame.matrix_argument());
    frame.load_extended(memory.load32(trace_y,element(matrix,1u,1u)));
    memory.store32(save_output,frame.output_slot(),output);
    frame.add(memory.load32(trace_x,element(matrix,0u,0u)));
    frame.add(memory.load32(trace_z,element(matrix,2u,2u)));
    if(frame.positive_trace(memory.load32(trace_zero,zero))) {
        frame.add(memory.load32(positive_one,one));
        frame.square_root();
        frame.reciprocal(memory.load32(positive_reciprocal_half,half));
        cross_term<false>(memory,frame,element(matrix,2u,1u),element(matrix,1u,2u),output,
                          positive_x_first,positive_x_second,positive_x_result);
        cross_term<false>(memory,frame,element(matrix,0u,2u),element(matrix,2u,0u),output+4u,
                          positive_y_first,positive_y_second,positive_y_result);
        cross_term<false>(memory,frame,element(matrix,1u,0u),element(matrix,0u,1u),output+8u,
                          positive_z_first,positive_z_second,positive_z_result);
        frame.pop();
        frame.multiply(memory.load32(positive_w_half,half));
        memory.store32(positive_w,output+12u,frame.word());
        frame.pop();
        frame.finish_positive();
        return;
    }

    frame.begin_diagonal();
    frame.load_extended(memory.load32(diagonal_y,element(matrix,1u,1u)));
    frame.save_diagonal_frame();
    frame.compare(memory.load32(diagonal_x,element(matrix,0u,0u)),1u);
    frame.save_diagonal_axes();
    const auto cycle=frame.cycle_table();
    memory.store32(cycle_first,cycle,1u);
    frame.save_comparison();
    memory.store32(cycle_second,cycle+4u,2u);
    memory.store32(cycle_third,cycle+8u,0u);
    const unsigned leader=frame.leader_xy();
    frame.load_extended(memory.load32(diagonal_z,element(matrix,2u,2u)));
    frame.leader_address(leader);
    frame.compare(memory.load32(diagonal_leader,element(matrix,leader,leader)),1u);
    const unsigned major=frame.leader_xyz(leader);
    const unsigned next=memory.load32(axis_next,cycle+4u*major);
    const unsigned last=memory.load32(axis_last,cycle+4u*next);

    frame.load_extended(memory.load32(trace_last,element(matrix,last,last)));
    frame.add(memory.load32(trace_next,element(matrix,next,next)));
    frame.reverse_subtract(memory.load32(trace_major,element(matrix,major,major)));
    const auto restored_output=memory.load32(restore_output,frame.output_slot());
    frame.add(memory.load32(diagonal_one,one));
    frame.square_root();
    frame.reciprocal(memory.load32(diagonal_reciprocal_half,half));
    frame.swap_top();
    frame.multiply(memory.load32(diagonal_result_half,half));
    memory.store32(major_result,restored_output+4u*major,frame.word());
    frame.pop();
    cross_term<false>(memory,frame,element(matrix,last,next),element(matrix,next,last),restored_output+4u*w,
                      diagonal_w_first,diagonal_w_second,diagonal_w_result);
    cross_term<true>(memory,frame,element(matrix,major,next),element(matrix,next,major),restored_output+4u*next,
                     next_first,next_second,next_result);
    frame.last_term_address(major,last);
    cross_term<true>(memory,frame,element(matrix,major,last),element(matrix,last,major),restored_output+4u*last,
                     last_first,last_second,last_result);
    frame.finish_diagonal();
}
} // namespace halo2::native_rotation_quaternion

#pragma once
// Four-wide Havok support search. The private frame keeps exact source
// read/write boundaries while the search works with vertex groups and lanes.
#include "physics/runtime/native_tu15_packed_sse.hpp"
#include <cstdint>

namespace halo2::native_soa_support_world {
inline constexpr std::uint32_t coordinates_offset=0x30u,group_count_offset=0x34u;
inline constexpr std::uint32_t group_stride=0x30u,axis_stride=0x10u,lanes=4u;
inline constexpr std::uint32_t initial_best=0xff7fffffu,feature_tag=0x3f000000u;
inline constexpr std::uint32_t mask_codes=0x3079a4u,mask_destinations=0x307988u;

enum class Access {
    coordinates,count,seed_best,read_best,direction_arg,direction_x,direction_y,direction_z,
    keep_x,keep_y,keep_z,read_x,read_y,read_z,group_z,group_y,group_x,candidate_dots,
    mask_code,mask_destination,keep_maximum,read_maximum,output_arg,
    vertex_x,vertex_y,vertex_z,output_x,output_y,output_z,output_feature,count_access,
};
enum class Compare {x_y,x_z,x_w,y_z,y_w,z_w};
enum class Search {only_x,only_y,only_z,only_w,without_y,without_x,all};

template<class Frame> unsigned scan_lane(Frame &frame,Search search) {
    if(search<=Search::only_w)return unsigned(search);
    bool test_x=search==Search::without_y;
    if(search==Search::all)test_x=frame.greater(Compare::x_y,0u,1u);
    if(test_x) {
        if(frame.greater(Compare::x_z,0u,2u))
            return frame.greater(Compare::x_w,0u,3u)?0u:3u;
    } else if(frame.greater(Compare::y_z,1u,2u)) {
        return frame.greater(Compare::y_w,1u,3u)?1u:3u;
    }
    return frame.greater(Compare::z_w,2u,3u)?2u:3u;
}

template<class Memory,class Frame>
void find(Memory &memory,Frame &frame,std::uint32_t shape) {
    using enum Access;
    using halo2::native_packed_sse::add;
    const auto groups=memory.load32(coordinates,shape+coordinates_offset);
    frame.save_scan_registers();
    const auto group_count=memory.load32(count,shape+group_count_offset);
    const auto best=frame.best_slot();
    memory.store32(seed_best,best,initial_best);
    frame.seed(memory.load32(read_best,best));
    std::uint32_t vertex=0u;

    if(frame.begin_scan(group_count,groups)) {
        const auto direction=memory.load32(direction_arg,frame.direction_slot());
        const auto x=memory.load32(direction_x,direction);
        memory.store32(keep_x,best,x);
        const auto y=memory.load32(direction_y,direction+4u);
        frame.load_axis(3u,memory.load32(read_x,best));
        const auto z=memory.load32(direction_z,direction+8u);
        memory.store32(keep_y,best,y);
        frame.load_axis(2u,memory.load32(read_y,best));
        memory.store32(keep_z,best,z);
        frame.load_axis(1u,memory.load32(read_z,best));
        const auto dz=frame.broadcast_axis(1u);
        const auto dy=frame.broadcast_axis(2u);
        const auto dx=frame.broadcast_axis(3u);
        auto group=groups;
        std::uint32_t first=0u;
        frame.begin_groups();
        do {
            const auto z_terms=frame.product(5u,dz,memory.load_vector(group_z,group+2u*axis_stride));
            const auto y_terms=frame.product(6u,dy,memory.load_vector(group_y,group+axis_stride));
            const auto x_terms=frame.product(7u,dx,memory.load_vector(group_x,group));
            const auto dots=add(add(x_terms,y_terms),z_terms);
            const auto mask=frame.improvements(dots);
            memory.store_vector(candidate_dots,frame.dots_slot(),dots);
            if(mask!=0u) {
                auto search=Search::all;
                if(frame.uses_table(mask)) {
                    const auto code=memory.load8(mask_code,mask_codes+mask);
                    const auto target=memory.load32(mask_destination,mask_destinations+4u*code);
                    search=frame.search_path(target);
                }
                const auto lane=scan_lane(frame,search);
                vertex=first+lane;
                const auto best_bits=frame.winner(lane,vertex);
                memory.store32(keep_maximum,best,best_bits);
                frame.pop_winner();
                frame.broadcast_best(memory.load32(read_maximum,best));
            }
            first+=lanes;
            group+=group_stride;
        } while(frame.next_group(first));
    }

    const auto output=memory.load32(output_arg,frame.output_slot());
    const auto point=frame.point_address(groups,vertex);
    const auto x=memory.load32(vertex_x,point);
    memory.store32(output_x,output,x);
    const auto y=memory.load32(vertex_y,point+axis_stride);
    memory.store32(output_y,output+4u,y);
    const auto z=memory.load32(vertex_z,point+2u*axis_stride);
    memory.store32(output_z,output+8u,z);
    const auto feature=frame.feature(vertex|feature_tag);
    memory.store32(output_feature,output+12u,feature);
}
} // namespace halo2::native_soa_support_world

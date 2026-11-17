#pragma once
#include <cstdint>

namespace halo2::native_codec_transform_composition_detail {
enum class Field : unsigned { left, right, output, two, one };

template<class View>void rotate_translation(View& v) {
    using enum Field;
    v.load(0,0x1205b2u,left,4);    v.multiply(0,0x1205b7u,right,20);
    v.load(3,0x1205bcu,right,16);  v.multiply(3,0x1205c1u,left,0);
    v.load(1,0x1205c5u,left,12);   v.load(2,0x1205cau,two,0);
    v.load(4,0x1205d2u,left,12);   v.load(6,0x1205d7u,left,4);
    v.multiply(6,0x1205dcu,right,16);
    v.load(7,0x1205e1u,right,20);  v.multiply(7,0x1205e6u,left,8);
    v.load(5,0x1205ebu,one,0);     v.add(0,3);
    v.load(3,0x1205f7u,right,24);  v.multiply(3,0x1205fcu,left,8);
    v.multiply(4,2);v.add(0,3);
    v.load(3,0x120609u,left,0);    v.multiply(3,0x12060du,right,24);
    v.multiply(0,2);v.multiply(1,1);v.multiply(1,2);
    v.load(2,0x12061eu,right,16);  v.multiply(2,0x120623u,left,8);
    v.subtract(2,3);
    v.load(3,0x12062cu,left,0);    v.multiply(3,0x120630u,right,20);
    v.subtract(3,6);
    v.load(6,0x120639u,left,4);    v.multiply(6,0x12063eu,right,24);
    v.subtract(6,7);v.load(7,0x120647u,right,16);
    v.multiply(6,4);v.subtract(1,5);v.multiply(7,1);v.add(6,7);
    v.load(7,0x12065cu,left,0);v.multiply(7,0);v.add(6,7);
    v.store(0x120668u,16,6);

    v.load(6,0x12066du,left,4);v.multiply(2,4);v.multiply(6,0);v.add(6,2);
    v.copy(2,1);v.multiply(2,0x120681u,right,20);v.add(6,2);
    v.store(0x12068au,20,6);

    v.load(2,0x12068fu,right,24);v.multiply(0,0x120694u,left,8);
    v.multiply(2,1);v.multiply(3,4);v.add(2,3);v.add(2,0);
    v.store(0x1206a9u,24,2);
}

template<class View>void compose_transform(View& v) {
    using enum Field;
    if(v.begin())v.stage_left();
    if(v.right_aliases_output())v.stage_right();
    v.quaternion_product();
    rotate_translation(v);

    v.load(0,0x1206aeu,left,28);
    if(v.scale_differs_from_one()) {
        v.load(1,0x1206bcu,output,16);v.multiply(1,0);v.store(0x1206c5u,16,1);
        v.load(1,0x1206cau,output,20);v.multiply(1,0);v.store(0x1206d3u,20,1);
        v.load(1,0x1206d8u,output,24);v.multiply(1,0);v.store(0x1206e1u,24,1);
        v.load(0,0x1206e6u,right,28);v.multiply(0,0x1206ebu,left,28);v.store(0x1206f0u,28,0);
    } else v.copy_right_scale();

    v.load(0,0x1206fdu,left,16);v.add(0,0x120702u,output,16);v.store(0x120707u,16,0);
    v.load(0,0x12070cu,left,20);v.add(0,0x120711u,output,20);
    v.restore_index_registers();v.store(0x120718u,20,0);
    v.load(0,0x12071du,left,24);v.add(0,0x120722u,output,24);
    v.restore_frame_register();v.store(0x120728u,24,0);
    v.finish();
}
} // namespace halo2::native_codec_transform_composition_detail

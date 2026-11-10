#pragma once
#include <bit>
#include <cstdint>

namespace halo2::native_camera_instance_query_detail {
enum class Read {
    scenario,instances,definitions,mask,definition_index,
    inverse_scale,origin_one,origin_x,origin_y,origin_z,
    matrix_28,matrix_1c,matrix_20,displacement_one,matrix_10,matrix_34,
    matrix_2c,matrix_14,matrix_38,matrix_30,matrix_24,matrix_18,
    displacement_z,displacement_x,displacement_y,matrix_3c,
    delta_matrix_1c,delta_matrix_10,delta_matrix_20,delta_matrix_14,
    delta_matrix_24,delta_matrix_18,fraction,material,material_scenario,
    material_table,selected_material,
};
enum class Write {origin_x,origin_y,origin_z,delta_x,delta_y,delta_z,
    result_kind,material,object,region,instance};
enum class Child {admission,sphere,inverse,bitmap,bsp,publication};
enum class Phase {origin_pointer,origin_decision,displacement_pointer,
    displacement_decision,bitmap_arguments,bitmap_index};
enum class Comparison {origin_scale,displacement_scale};

template<class Scalar>struct Progress {
    std::uint32_t index{},scenario{},instances{},definitions{},mask{},definition_index{},instance{},definition{},bitmap{},fraction{},material{},material_pointer{},selected_material{};
    Scalar scale{},one{},ox{},oy{},oz{},dx{},dy{},dz{};
    Scalar m28{},m1c{},m20{},m2c{},m30{},m24{},m18{},m10{},m14{};
    Scalar m34{},m38{},m3c{},m1c_y{},m20_y{},term{},x{},y{},z{},delta_x{},delta_y{},delta_z{};
    bool scale_origin{},scale_displacement{};
};

// Typed query policy over current instance/definition, affine-frame, segment
// and collision DATA. The view owns caller ABI publication and the genuine
// six dependency calls; no decoded instruction or virtual register is used
// to choose geometric/material behavior here.
template<class Access>bool query_hit(Access&a,Progress<typename Access::Scalar>&d){
    d.material=a.word(Read::material,d);a.store_word(Write::result_kind,3u,d);
    if(std::uint16_t(d.material)!=0xffffu){
        d.material=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(d.material))));
        d.scenario=a.word(Read::material_scenario,d);d.material_pointer=a.word(Read::material_table,d)+d.material*20u+8u;
    }else d.material_pointer=a.default_material(d);
    d.selected_material=a.word(Read::selected_material,d);a.store_word(Write::material,d.selected_material,d);
    a.store_word(Write::object,0xffffffffu,d);a.store_word(Write::region,0xffffffffu,d);a.store_word(Write::instance,d.index,d);
    a.publish(d);a.resume(Child::publication,d);return a.finish(true,d);
}
template<class Access>bool query_resolved(Access&a,Progress<typename Access::Scalar>&d){
    a.phase(Phase::origin_pointer,d);d.scale=a.scalar(Read::inverse_scale,d);
    d.one=a.scalar(Read::origin_one,d);d.scale_origin=a.different(d.scale,d.one,Comparison::origin_scale,d);
    d.ox=a.scalar(Read::origin_x,d);d.oy=a.scalar(Read::origin_y,d);d.oz=a.scalar(Read::origin_z,d);
    a.phase(Phase::origin_decision,d);
    if(d.scale_origin){d.ox=a.multiply(d.scale,d.ox);d.oy=a.multiply(d.scale,d.oy);d.oz=a.multiply(d.scale,d.oz);}
    d.m28=a.scalar(Read::matrix_28,d);d.m1c=a.scalar(Read::matrix_1c,d);d.m20=a.scalar(Read::matrix_20,d);
    a.phase(Phase::displacement_pointer,d);d.m1c_y=a.multiply(d.m1c,d.oy);
    d.one=a.scalar(Read::displacement_one,d);d.scale_displacement=a.different(d.scale,d.one,Comparison::displacement_scale,d);
    d.x=a.multiply(d.m28,d.oz);d.x=a.add(d.x,d.m1c_y);
    d.m10=a.scalar(Read::matrix_10,d);d.term=a.multiply(d.m10,d.ox);d.x=a.add(d.x,d.term);
    d.m34=a.scalar(Read::matrix_34,d);d.x=a.add(d.x,d.m34);d.m20_y=a.multiply(d.m20,d.oy);
    a.store(Write::origin_x,d.x,d);
    d.m2c=a.scalar(Read::matrix_2c,d);d.y=a.multiply(d.m2c,d.oz);d.y=a.add(d.y,d.m20_y);
    d.m14=a.scalar(Read::matrix_14,d);d.term=a.multiply(d.m14,d.ox);d.y=a.add(d.y,d.term);
    d.m38=a.scalar(Read::matrix_38,d);d.y=a.add(d.y,d.m38);a.store(Write::origin_y,d.y,d);
    d.m30=a.scalar(Read::matrix_30,d);d.z=a.multiply(d.m30,d.oz);
    d.m24=a.scalar(Read::matrix_24,d);d.term=a.multiply(d.m24,d.oy);d.m18=a.scalar(Read::matrix_18,d);d.z=a.add(d.z,d.term);
    d.dz=a.scalar(Read::displacement_z,d);d.term=a.multiply(d.m18,d.ox);d.dx=a.scalar(Read::displacement_x,d);
    d.z=a.add(d.z,d.term);d.dy=a.scalar(Read::displacement_y,d);d.m3c=a.scalar(Read::matrix_3c,d);d.z=a.add(d.z,d.m3c);
    a.phase(Phase::displacement_decision,d);a.store(Write::origin_z,d.z,d);
    if(d.scale_displacement){d.dx=a.multiply(d.dx,d.scale);d.dy=a.multiply(d.dy,d.scale);d.dz=a.multiply(d.dz,d.scale);}

    d.m1c=a.scalar(Read::delta_matrix_1c,d);d.delta_x=a.multiply(d.m28,d.dz);d.term=a.multiply(d.m1c,d.dy);d.delta_x=a.add(d.delta_x,d.term);
    d.m10=a.scalar(Read::delta_matrix_10,d);d.term=a.multiply(d.m10,d.dx);d.delta_x=a.add(d.delta_x,d.term);a.store(Write::delta_x,d.delta_x,d);
    d.m20=a.scalar(Read::delta_matrix_20,d);d.term=a.multiply(d.m20,d.dy);d.delta_y=a.multiply(d.m2c,d.dz);d.delta_y=a.add(d.delta_y,d.term);
    d.m14=a.scalar(Read::delta_matrix_14,d);d.term=a.multiply(d.m14,d.dx);d.delta_y=a.add(d.delta_y,d.term);
    d.m24=a.scalar(Read::delta_matrix_24,d);d.term=a.multiply(d.m24,d.dy);a.phase(Phase::bitmap_arguments,d);
    d.delta_z=a.multiply(d.m30,d.dz);d.delta_z=a.add(d.delta_z,d.term);d.m18=a.scalar(Read::delta_matrix_18,d);
    a.prepare_bitmap(d);d.term=a.multiply(d.m18,d.dx);d.delta_z=a.add(d.delta_z,d.term);a.phase(Phase::bitmap_index,d);
    a.store(Write::delta_y,d.delta_y,d);a.store(Write::delta_z,d.delta_z,d);
    d.bitmap=a.bitmap(d);a.resume(Child::bitmap,d);d.fraction=a.word(Read::fraction,d);
    if(!a.bsp(d))return a.finish(false,d);a.resume(Child::bsp,d);

    return query_hit(a,d);
}
template<class Access>bool query(Access&a,Progress<typename Access::Scalar>&d){
    d={};d.index=a.begin(d);
    d.scenario=a.word(Read::scenario,d);d.instances=a.word(Read::instances,d);
    d.definitions=a.word(Read::definitions,d);d.mask=a.word(Read::mask,d);
    d.instance=d.instances+d.index*0x58u;
    d.definition_index=a.word(Read::definition_index,d);
    d.definition=d.definitions+std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(d.definition_index))))*0xc8u;
    if(!a.admit(d))return a.finish(false,d);a.resume(Child::admission,d);
    if(!a.near_sphere(d))return a.finish(false,d);a.resume(Child::sphere,d);
    a.inverse(d);a.resume(Child::inverse,d);
    return query_resolved(a,d);
}


}

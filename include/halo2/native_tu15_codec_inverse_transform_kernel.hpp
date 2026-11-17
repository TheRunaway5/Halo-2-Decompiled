#pragma once

namespace halo2::native_codec_inverse_transform_detail {

// Operand names distinguish real rereads of the same field. A copy or store
// may alias the input, so a later rotation term must acquire its own value.
enum class Read {
    two, one, output_w, conjugate_sign,
    dot_x_point, dot_x_quaternion, dot_y_point, dot_y_quaternion,
    cross_z_x_point, cross_z_y_quaternion, cross_x_y_point, cross_x_z_quaternion,
    dot_z_quaternion, dot_z_point, cross_y_x_quaternion, cross_y_z_point,
    cross_y_z_quaternion, cross_y_x_point, cross_z_y_point, cross_z_x_quaternion,
    cross_x_y_quaternion, cross_x_z_point, rotated_x_point, rotated_x_quaternion,
    rotated_y_point, rotated_y_quaternion, rotated_z_quaternion, rotated_z_point,
    scale_zero_test, zero_scale_value, scale_one_test, scale_divisor,
    scaled_x, scaled_y, scaled_z, output_sign, signed_x, signed_y, signed_z
};
enum class Store {
    conjugated_w, rotated_x, rotated_y, rotated_z, scaled_x, scaled_y, scaled_z,
    signed_x, signed_y, reciprocal_scale, signed_z
};

template<class View>void rotate_translation(View& view) {
    auto& norm=view.norm();
    auto& dot=view.dot();
    auto& w=view.conjugated_w();
    auto& y=view.two_or_cross_y();
    auto& z=view.cross_z();
    auto& x=view.cross_x();
    auto& scratch=view.scratch();

    dot=view.read(Read::dot_x_point);
    dot=view.multiply(dot,view.read(Read::dot_x_quaternion));
    z=view.read(Read::dot_y_point);
    z=view.multiply(z,view.read(Read::dot_y_quaternion));
    x=view.read(Read::cross_z_x_point);
    x=view.multiply(x,view.read(Read::cross_z_y_quaternion));
    scratch=view.read(Read::cross_x_y_point);
    scratch=view.multiply(scratch,view.read(Read::cross_x_z_quaternion));
    dot=view.add(dot,z);
    z=view.read(Read::dot_z_quaternion);
    z=view.multiply(z,view.read(Read::dot_z_point));
    dot=view.add(dot,z);
    z=view.read(Read::cross_y_x_quaternion);
    z=view.multiply(z,view.read(Read::cross_y_z_point));

    dot=view.multiply(dot,y);
    norm=view.multiply(w,w);
    norm=view.multiply(norm,y);
    w=view.multiply(w,y);
    y=view.read(Read::cross_y_z_quaternion);
    y=view.multiply(y,view.read(Read::cross_y_x_point));
    y=view.subtract(y,z);
    z=view.read(Read::cross_z_y_point);
    z=view.multiply(z,view.read(Read::cross_z_x_quaternion));
    z=view.subtract(z,x);
    x=view.read(Read::cross_x_y_quaternion);
    x=view.multiply(x,view.read(Read::cross_x_z_point));
    x=view.subtract(x,scratch);

    scratch=view.read(Read::rotated_x_point);
    x=view.multiply(x,w);
    norm=view.subtract(norm,view.reciprocal());
    scratch=view.multiply(scratch,norm);
    x=view.add(x,scratch);
    scratch=view.read(Read::rotated_x_quaternion);
    scratch=view.multiply(scratch,dot);
    x=view.add(x,scratch);
    view.store(Store::rotated_x,x);

    x=view.read(Read::rotated_y_point);
    scratch=view.read(Read::rotated_y_quaternion);
    x=view.multiply(x,norm);
    y=view.multiply(y,w);
    scratch=view.multiply(scratch,dot);
    x=view.add(x,scratch);
    x=view.add(x,y);
    view.store(Store::rotated_y,x);

    y=view.read(Read::rotated_z_quaternion);
    norm=view.multiply(norm,view.read(Read::rotated_z_point));
    y=view.multiply(y,dot);
    z=view.multiply(z,w);
    y=view.add(y,z);
    y=view.add(y,norm);
    norm=view.zero();
    view.store(Store::rotated_z,y);
}

template<class View>void inverse_scale(View& view) {
    auto& reciprocal=view.reciprocal();
    view.test_scale(view.norm(),Read::scale_zero_test);
    view.restore_saved_registers();
    if(!view.scale_test_passed())reciprocal=view.read(Read::zero_scale_value);
    else {
        view.test_scale(reciprocal,Read::scale_one_test);
        if(view.scale_test_passed())reciprocal=view.divide(reciprocal,view.read(Read::scale_divisor));
    }
}

template<class View>void rescale_translation(View& view) {
    auto& work=view.norm();
    auto& signed_value=view.dot();
    const auto& reciprocal=view.reciprocal();
    work=reciprocal;
    work=view.multiply(work,view.read(Read::scaled_x));
    view.store(Store::scaled_x,work);
    work=view.read(Read::scaled_y);
    work=view.multiply(work,reciprocal);
    view.store(Store::scaled_y,work);
    work=view.read(Read::scaled_z);
    work=view.multiply(work,reciprocal);
    view.store(Store::scaled_z,work);

    work=view.read(Read::output_sign);
    signed_value=view.read(Read::signed_x);
    signed_value=view.multiply(signed_value,work);
    view.store(Store::signed_x,signed_value);
    signed_value=view.read(Read::signed_y);
    signed_value=view.multiply(signed_value,work);
    view.store(Store::signed_y,signed_value);
    signed_value=view.read(Read::signed_z);
    signed_value=view.multiply(signed_value,work);
    view.store(Store::reciprocal_scale,reciprocal);
    view.store(Store::signed_z,signed_value);
}

template<class View>void inverse_transform(View& view) {
    view.two_or_cross_y()=view.read(Read::two);
    view.copy_quaternion();
    auto& w=view.conjugated_w();
    w=view.read(Read::output_w);
    w=view.multiply(w,view.read(Read::conjugate_sign));
    view.store(Store::conjugated_w,w);
    rotate_translation(view);
    inverse_scale(view);
    rescale_translation(view);
    view.finish();
}

} // namespace halo2::native_codec_inverse_transform_detail

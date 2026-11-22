#pragma once

#include <cstddef>

namespace halo2::native_light_point_transform {
struct Point {
    float x, y, z;
};

struct Transform {
    float scale;
    Point x_axis, y_axis, z_axis, translation;
};

static_assert(offsetof(Point, y) == 4 && offsetof(Point, z) == 8 && sizeof(Point) == 12);
static_assert(offsetof(Transform, x_axis) == 4 && offsetof(Transform, y_axis) == 16 &&
    offsetof(Transform, z_axis) == 28 && offsetof(Transform, translation) == 40);

template<class Frame> void transform(Frame& frame) {
    frame.read_transform(offsetof(Transform, scale), offsetof(Transform, x_axis),
        offsetof(Transform, y_axis), offsetof(Transform, z_axis), offsetof(Transform, translation));
    if(frame.read_count() > 0) {
        frame.begin_array();
        do {
            frame.transform_point(offsetof(Point, x), offsetof(Point, y), offsetof(Point, z));
            frame.write_point(offsetof(Point, x), offsetof(Point, y));
        } while(frame.next_point(sizeof(Point)));
    }
    frame.finish();
}
} // namespace halo2::native_light_point_transform

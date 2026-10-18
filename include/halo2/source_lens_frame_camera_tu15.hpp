#pragma once
#include "halo2/cache.hpp"
#include <cstring>
namespace halo2 {
// The existing native lens query/draw wrappers publish this shared original
// camera layout. Physical rectangles count actual attachment pixels; filter
// or query sample counts are never rescaled or normalized by this adapter.
inline std::array<std::byte,0x74> source_lens_frame_camera_tu15(
    Vec3 position,Vec3 forward,Vec3 up,float angle,
    std::array<std::int16_t,4> rectangle,unsigned logical_width,unsigned logical_height,
    unsigned attachment_width,unsigned attachment_height,Vec2 clip_planes) {
    if(!logical_width || !logical_height || !attachment_width || !attachment_height)
        throw FormatError("Original lens camera lacks its actual logical/attachment dimensions");
    for(unsigned n=0;n<4;++n) {
        const auto scaled=std::int64_t(rectangle[n])*(n%2?attachment_width:attachment_height)/
            (n%2?logical_width:logical_height);
        if(scaled<0 || scaled>32767)
            throw FormatError("Original lens camera rectangle exceeds signed-short hardware coordinates");
        rectangle[n]=std::int16_t(scaled);
    }
    std::array<std::byte,0x74> camera{};
    const auto copy=[&](unsigned at,const auto &v){std::memcpy(camera.data()+at,&v,sizeof(v));};
    copy(0,position);copy(12,forward);copy(24,up);copy(0x28,angle);
    std::memcpy(camera.data()+0x30,rectangle.data(),8);
    std::memcpy(camera.data()+0x38,rectangle.data(),8);
    copy(0x40,clip_planes.x);copy(0x44,clip_planes.y);
    return camera;
}
}

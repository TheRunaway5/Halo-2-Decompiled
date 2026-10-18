#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/scene.hpp"
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const auto content=cache.load_content(0,false);
    const auto loader=[&](auto graph,auto name){return cache.load_named_animation(graph,name);};
    Presentation camera(content,loader);camera.control(true,Camera{});
    camera.camera_animation(0xe9d90864u,0x07001c9cu,1);
    unsigned moving{};
    for(unsigned frame=0;frame<269;++frame) {
        const auto first=camera.camera(Camera{},0),half=camera.camera(Camera{},.5f),end=camera.camera(Camera{},1);
        const auto time=camera.camera_time();
        // Rendering does not advance the authored clock or mutate the pose.
        if(camera.camera_time()!=time || length(camera.camera(Camera{},0).position-first.position)>1e-6f)
            throw FormatError("Render sampling mutated the authoritative camera clock");
        camera.tick();const auto next=camera.camera(Camera{},0);
        if(length(next.position-first.position)>1e-5f) {
            ++moving;
            if(length(half.position-(first.position+next.position)*.5f)>.0001f)
                throw FormatError("Authored moving camera remains capped between simulation frames");
        }
        if(length(end.position-next.position)>.0001f || length(end.forward-next.forward)>.0001f)
            throw FormatError("Camera interpolation jumps at the next tick boundary");
    }
    if(moving<200)throw FormatError("Moving authored camera fixture lost coverage");
    const auto last=camera.camera(Camera{},0);
    if(length(camera.camera(Camera{},.75f).position-last.position)>1e-6f)
        throw FormatError("Completed camera track did not hold its last authored frame");
    camera.camera_animation(0xe9d90864u,0x06001ca2u,2);
    Presentation cut(content,loader);cut.control(true,Camera{});
    cut.camera_animation(0xe9d90864u,0x06001ca2u,2);
    if(length(camera.camera(Camera{},0).position-cut.camera(Camera{},0).position)>1e-6f)
        throw FormatError("Camera cut interpolated from the departed shot");
    std::cout<<"PASS authored camera subframes, all269 boundaries, end hold, read-only clock and cut reset; moving="<<moving<<'\n';
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

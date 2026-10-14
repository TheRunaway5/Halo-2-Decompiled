#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/gameplay_fov.hpp"
#include <iostream>

int main(int argc,char **argv)try {
    if(argc!=6)throw std::runtime_error("MAP SHARED_MAPS OUTPUT FOV WIDTH required");
    SDL_SetMainReady();
    halo2::MultiplayerOptions options;
    options.map=argv[1];options.shared_maps=argv[2];options.screenshot=argv[3];
    options.width=std::stoul(argv[5]);options.height=720;options.frames=3;
    options.hidden=true;options.mute=true;options.validation=true;
    options.input_observer=[](auto,auto,auto&){};
    options.reference_tick_cadence=1;options.reference_render_fraction=1;
    options.desktop=std::make_shared<halo2::DesktopSession>();
    options.desktop->display_settings.field_of_view=std::stoul(argv[4]);
    options.desktop->display_settings.vehicle_field_of_view=70;
    const auto content=halo2::Cache(options.map,options.shared_maps).load_content(0,true);
    unsigned frames{};
    options.view_observer=[&](unsigned,const halo2::Camera &camera,float original,std::int16_t) {
        const float fov=halo2::gameplay_observer_field_of_view(original,content.player.camera_field_of_view,
            options.desktop->display_settings.field_of_view);
        const float expected=halo2::gameplay_camera_angles(fov,halo2::gameplay_field_of_view(
            options.desktop->display_settings.field_of_view),float(options.width)/options.height).projection_angle;
        if(std::abs(camera.field_of_view*.01745329238474369f-expected)>3e-7f)
            throw std::runtime_error("selected FOV did not reach the submitted runtime camera");
        if(std::abs(original-content.player.camera_field_of_view)>1e-6f)
            throw std::runtime_error("presentation FOV modified the original observer packet");
        ++frames;
    };
    if(halo2::run_multiplayer(options)!=0 || frames!=3)
        throw std::runtime_error("FOV rendered runtime did not complete");
    std::cout<<"PASS selected FOV reached "<<frames<<" rendered cameras; source lens preserved\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

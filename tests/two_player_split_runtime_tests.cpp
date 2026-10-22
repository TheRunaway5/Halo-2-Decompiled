#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/gameplay_fov.hpp"
#include <iostream>

using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=8)throw FormatError("MAP SHARED_MAPS VISTA_MENU OUTPUT horizontal|vertical WIDTH HEIGHT required");
    SDL_SetMainReady();MultiplayerOptions options;
    options.map=argv[1];options.shared_maps=argv[2];options.screenshot=argv[4];
    options.width=std::stoul(argv[6]);options.height=std::stoul(argv[7]);
    options.local_players=2;options.frames=4;options.hidden=true;options.mute=true;options.validation=true;
    options.reference_tick_cadence=1;options.reference_render_fraction=1;
    options.desktop=std::make_shared<DesktopSession>();
    const std::string initial=argv[5];
    if(initial!="horizontal" && initial!="vertical")throw FormatError("Invalid split test layout");
    options.desktop->display_settings.two_player_split=initial=="horizontal"?TwoPlayerSplit::Horizontal:TwoPlayerSplit::Vertical;
    auto menu_content=Cache(options.shared_maps/"mainmenu.map").load_frontend();
    menu_content.vista=load_vista_settings(argv[3]);MenuState video(menu_content);
    video.adopt_pc_settings({},options.desktop->display_settings,{options.width,options.height,0});
    video.set_pc_display_adapter([&](const DisplaySettings &value){options.desktop->display_settings=value;return std::string{};});
    video.open(vista_menu(256));video.select(12);
    bool switched=false;
    options.input_observer=[&](unsigned tick,auto,auto &) {
        if(tick>=2 && !switched){video.accept();switched=true;}
    };
    std::array<unsigned,2> rectangles{},cameras{},layouts{};
    options.view_rectangle_observer=[&](unsigned view,const UiRect &actual) {
        const bool vertical=options.desktop->display_settings.two_player_split==TwoPlayerSplit::Vertical;
        const UiRect expected=vertical?UiRect{0,float(view*320),480,float((view+1)*320)}:
            UiRect{float(view*240),0,float((view+1)*240),640};
        if(actual.top!=expected.top || actual.left!=expected.left || actual.bottom!=expected.bottom || actual.right!=expected.right)
            throw FormatError("Selected split did not reach the actual submitted world rectangle");
        ++rectangles.at(view);++layouts[vertical?1:0];
    };
    const auto biped=Cache(options.map,options.shared_maps).load_content(0,true).player;
    options.view_observer=[&](unsigned view,const Camera &camera,float original,std::int16_t) {
        const bool vertical=options.desktop->display_settings.two_player_split==TwoPlayerSplit::Vertical;
        const unsigned half_w=options.width/2,half_h=options.height/2;
        const float width=vertical?float(view?options.width-half_w:half_w):float(options.width);
        const float height=vertical?float(options.height):float(view?options.height-half_h:half_h);
        const float horizontal=gameplay_field_of_view(78);
        const auto expected=gameplay_camera_angles(horizontal,horizontal,width/height).projection_angle;
        if(std::abs(camera.field_of_view*.01745329238474369f-expected)>3e-7f ||
            std::abs(original-biped.camera_field_of_view)>1e-6f)
            throw FormatError("Split camera lens does not match its physical viewport or modified the source observer");
        ++cameras.at(view);
    };
    if(run_multiplayer(options)!=0 || !switched || rectangles!=std::array<unsigned,2>{4,4} || cameras!=rectangles || !layouts[0] || !layouts[1])
        throw FormatError("Two-player runtime did not render both selected layouts");
    std::cout<<"PASS both players rendered four frames, live Video Settings divider change, physical camera aspects and source lenses preserved\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

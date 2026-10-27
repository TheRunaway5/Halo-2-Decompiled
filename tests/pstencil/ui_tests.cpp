#include "halo2/renderer.hpp"
#include "halo2/game_pause_menu.hpp"
#include "halo2/desktop_session.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void require(bool value,const char* why){if(!value)throw FormatError(why);}
std::vector<char> bytes(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
void check_mode(const MenuState& state,pstencil::GraphicsMode mode) {
    require(state.frame().entries.front().label=="Graphics Mode","Graphics Mode is not the first Video Settings option");
    require(state.pc_display_settings().graphics_mode==mode,"Video Settings lost the live graphics mode");
    require(state.pc_value_label(state.frame(),0)==pstencil::graphics_mode_name(mode),"Graphics Mode displays the wrong choice");
}
void check_path_resources(const Renderer& renderer,bool active) {
    const auto resources=renderer.path_tracing_resources();
    if(active)require(resources.views>0 && resources.cpu_bytes>0 && resources.gpu_bytes>0,
        "Active Path Tracing draw did not own its scene and GPU buffers");
    else require(resources==RendererPathTracingResources{},
        "Disabled Path Tracing retained a view, CPU scene bank, or GPU allocation");
}
}
int main(int argc,char** argv){std::ostringstream diagnostics;auto* previous=std::cerr.rdbuf(diagnostics.rdbuf());try{
    require(argc==2,"provide pStencil UI evidence directory");const std::filesystem::path out=std::getenv("HALO2_TEST_SCRATCH")?std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/"graphics-menu":std::filesystem::path(argv[1]);std::filesystem::create_directories(out);
    const std::filesystem::path root=HALO2_SOURCE_DIR;
    DesktopSession desktop;auto* window=desktop.window(960,720,true);
    {
        Mesh mesh;Material material;material.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};mesh.materials.push_back(material);
        mesh.vertices={{{4,-2,-1},{1,1,1},{0,0},0},{{4,2,-1},{1,1,1},{1,0},0},{{4,0,2},{1,1,1},{.5f,1},0}};
        const auto preferences=out/"menu.cfg";
        GamePauseMenu pause(root/"assets/disc/maps",root/"Halo 2 Vista installer",mesh,false,preferences);
        GamePauseMenu solo(root/"assets/disc/maps",root/"Halo 2 Vista installer",mesh,true,preferences);
        auto content=Cache(root/"assets/disc/maps/mainmenu.map").load_frontend();
        content.vista=load_vista_settings(root/"Halo 2 Vista installer/maps/mainmenu.map");
        MenuState title(content,preferences);
        FrontendPainter painter(content,root/"assets/disc/maps/fonts",mesh);
        Renderer renderer(window,mesh,true);renderer.path_tracing_source(mesh);
        check_path_resources(renderer,false);
        Camera camera;camera.field_of_view=60;
        renderer.share_display_settings(desktop.display_settings);pause.bind(window,desktop,renderer);solo.bind(window,desktop,renderer);
        title.set_pc_display_adapter([&](const DisplaySettings& settings){
            const auto error=renderer.validate_display_settings(settings);
            if(error.empty())desktop.display_settings=settings;
            return error;
        });
        title.set_pc_video_adapter([&](unsigned w,unsigned h,FullscreenMode mode) {
            const auto error=desktop.apply_video(w,h,mode);
            if(error.empty())renderer.resized();
            return error;
        });
        title.set_pc_video_query([&] {
            int w{},h{};SDL_GetWindowSize(window,&w,&h);
            return std::array<unsigned,3>{unsigned(w),unsigned(h),unsigned(desktop.fullscreen_mode())};
        });
        title.set_pc_video_modes({},960,720,FullscreenMode::Windowed);
        const auto draw=[&](const UiDraw& ui,const char* name){require(renderer.draw(camera,out/name,{},false,{},{},ui.vertices),"Video Settings draw failed");};
        // An opaque interface with thin, high-contrast details must be copied
        // exactly at native resolution while the world uses fewer input pixels.
        std::vector<Vertex> interface;
        const auto quad=[&](float left,float top,float right,float bottom,Vec3 tint) {
            for(const auto p:std::array<Vec3,6>{{{left,top,1},{right,top,1},{left,bottom,1},
                                               {left,bottom,1},{right,top,1},{right,bottom,1}}}) {
                Vertex vertex{};vertex.position=p;vertex.color=tint;interface.push_back(vertex);
            }
        };
        quad(0,0,640,480,{0,0,0});
        for(unsigned x=0;x<160;x+=2)quad(80+x,80,81+x,180,{1,1,1});
        for(unsigned y=0;y<100;y+=2)quad(300,80+y,460,81+y,{1,1,1});
        require(renderer.draw(camera,out/"native-interface-reference.ppm",{},false,{},{},interface),"Native interface reference failed");
        const auto native_interface=bytes(out/"native-interface-reference.ppm");
        const auto initial_display=renderer.display_settings();
        unsigned interface_cases{};
        for(const auto method:{Upscaler::FSR31,Upscaler::DLSS45})for(unsigned quality=0;quality<6;++quality) {
            if(method==Upscaler::DLSS45 && renderer.device_name().find("RTX")==std::string::npos)continue;
            auto setting=initial_display;setting.upscaler=method;setting.upscaling_quality=UpscalingQuality(quality);
            require(renderer.validate_display_settings(setting).empty(),"Interface upscaling choice rejected");renderer.display_settings(setting);
            require(renderer.draw(camera,out/"native-interface-scaled-world.ppm",{},false,{},{},interface),"Native interface composite failed");
            require(bytes(out/"native-interface-scaled-world.ppm")==native_interface,"World upscaling changed native interface pixels");
            ++interface_cases;
        }
        std::array<ModelView,2> split{};
        for(unsigned i=0;i<split.size();++i) {
            auto& view=split[i];view.camera=camera;view.left=320.f*i;view.width=320;view.height=480;
            view.full_surface=true;view.vertices=mesh.vertices;
        }
        auto split_setting=initial_display;split_setting.upscaler=Upscaler::FSR31;split_setting.upscaling_quality=UpscalingQuality::Performance;
        renderer.display_settings(split_setting);
        require(renderer.draw(camera,out/"native-interface-split.ppm",{},false,{},{},interface,split),"Split-screen interface composite failed");
        require(bytes(out/"native-interface-split.ppm")==native_interface,"Split-screen upscaling changed native interface pixels");
        std::cout<<"Native interface pixel cases="<<interface_cases<<" plus two-player split-screen\n";
        renderer.display_settings(initial_display);
        renderer.upscale_world(false);
        require(renderer.draw(camera,out/"Original-before.ppm"),"Original baseline failed");
        title.open(vista_menu(19));title.select(2);title.accept();
        require(title.frame().screen==vista_menu(256),"Settings did not open Video Settings");
        check_mode(title,pstencil::GraphicsMode::Original);require(title.frame().selected==0,"Video Settings did not focus Graphics Mode first");
        // Real menu pointer hit testing, at multiple viewport sizes.
        for(const auto size:{std::array<unsigned,2>{640,480},{960,720},{2560,720}}) {
            auto ui=painter.draw(title,5,-1,10000,size);draw(ui,"title-video.ppm");
            std::optional<FrontendPointerTarget> target;
            for(int y=0;y<480 && !target;y+=2)for(int x=0;x<640 && !target;x+=2) {
                const auto hit=painter.pointer_hit(x,y);
                if(hit && hit->action==FrontendPointerAction::Entry && hit->index==0)target=hit;
            }
            require(target.has_value(),"First Graphics Mode row has no mouse target");
            require(title.activate_pointer(*target),"Graphics Mode click was not handled");
            check_mode(title,pstencil::GraphicsMode::pStencil);
            require(renderer.display_settings().graphics_mode==pstencil::GraphicsMode::pStencil,"Title menu did not switch the live renderer");
            require(MenuState(content,preferences).pc_display_settings().graphics_mode==pstencil::GraphicsMode::pStencil,"Graphics Mode did not persist");
            title.accept();check_mode(title,pstencil::GraphicsMode::PathTracing);
            require(renderer.display_settings().graphics_mode==pstencil::GraphicsMode::PathTracing,"Title menu did not switch to Path Tracing");
            require(MenuState(content,preferences).pc_display_settings().graphics_mode==pstencil::GraphicsMode::PathTracing,"Path Tracing did not persist");
            draw(painter.draw(title,5,-1,10000,size),"title-path-tracing.ppm");
            check_path_resources(renderer,true);
            title.accept();check_mode(title,pstencil::GraphicsMode::Original);
            check_path_resources(renderer,false);
        }
        const auto exercise_upscaling=[&](MenuState& state,auto row,auto back,auto paint,bool all_quality_modes) {
            require(state.frame().entries.at(6).label=="Upscaling","Video Settings retained the old AA field");
            row(6);
            require(state.frame().context==0 && state.frame().entries.size()==2,"Upscaling submenu missing method/quality rows");
            for(unsigned method=1;method<=2;++method) {
                if(method==2 && renderer.device_name().find("RTX")==std::string::npos)continue;
                row(0);require(state.frame().context==1 && state.frame().entries.size()==3,"Upscaling method choices missing");row(method);
                require(desktop.display_settings.upscaler==Upscaler(method),"Actual menu failed to apply upscaling method");
                require(state.frame().entries.at(0).label==std::string("Method: ")+upscaler_name(Upscaler(method)),"Upscaling submenu hid the selected method");
                for(unsigned quality=0;quality<6;++quality) {
                    if(!all_quality_modes || method==2)quality=2;
                    row(1);require(state.frame().context==2 && state.frame().entries.size()==6,"Upscaling quality choices missing");row(quality);
                    require(desktop.display_settings.upscaling_quality==UpscalingQuality(quality),"Actual menu failed to apply quality mode");
                    require(state.frame().entries.at(1).label==std::string("Quality: ")+upscaling_quality_name(UpscalingQuality(quality),Upscaler(method)),"Upscaling submenu hid the selected quality");
                    paint();paint();
                    const auto saved=MenuState(content,preferences).pc_display_settings();
                    require(saved.upscaler==Upscaler(method) && saved.upscaling_quality==UpscalingQuality(quality),"Upscaling settings did not persist");
                    if(!all_quality_modes || method==2)break;
                }
                // The automatic internal-resolution row must change the
                // active upscaler, rather than silently saving an unused size.
                back();row(3);
                require(state.frame().context==2 && state.frame().entries.size()==6,"Automatic internal resolution did not open quality modes");
                row(2);row(6);
                if(method==1) {
                    back();
                    for(const auto mode:{FullscreenMode::Fullscreen,FullscreenMode::BorderlessFullscreen,FullscreenMode::Windowed}) {
                        row(1);row(unsigned(mode));paint();row(0);paint();
                        require(desktop.fullscreen_mode()==mode && renderer.display_settings().upscaler==Upscaler::FSR31,
                            "Window mode switch lost the active upscaler");
                    }
                    row(6);
                }
            }
            row(0);row(0);back();
            require(desktop.display_settings.upscaler==Upscaler::Off,"Upscaling Off did not restore native rendering");paint();
        };
        exercise_upscaling(title,[&](unsigned row){title.select(row);title.accept();},[&]{title.back();},
            [&]{draw(painter.draw(title,5),"title-upscaling.ppm");},true);
        require(!desktop.apply_video(960,720,FullscreenMode(99)).empty() && desktop.fullscreen_mode()==FullscreenMode::Windowed,"Invalid fullscreen mode changed the window");
        // The three window modes use distinct SDL flags and retain confirmation/rollback.
        for(const auto mode:{FullscreenMode::Fullscreen,FullscreenMode::BorderlessFullscreen,FullscreenMode::Windowed}) {
            title.select(1);title.accept();
            require(title.frame().screen==vista_menu(271) && title.frame().entries.size()==3,"Fullscreen Mode is missing its three choices");
            title.select(unsigned(mode));title.accept();
            require(desktop.fullscreen_mode()==mode,"Fullscreen Mode did not apply the requested SDL window mode");
            require(title.frame().screen==vista_menu(293),"Window mode change omitted confirmation");
            title.accept();
            require(title.frame().screen==vista_menu(256) && title.pc_value_label(title.frame(),1)==fullscreen_mode_name(mode),"Window mode choice displays the wrong value");
            FullscreenMode saved=FullscreenMode::Windowed;
            MenuState reload(content,preferences);reload.set_pc_video_adapter([&](unsigned,unsigned,FullscreenMode value){saved=value;return std::string{};});
            reload.set_pc_video_modes({},960,720,FullscreenMode::Windowed);
            require(saved==mode,"Fullscreen Mode did not persist its exact mode");
        }
        // Cancel or time out a preview and retain each distinct previous mode.
        unsigned window_clock=0;
        for(const auto mode:{FullscreenMode::Fullscreen,FullscreenMode::BorderlessFullscreen,FullscreenMode::Windowed}) {
            title.select(1);title.accept();title.select(unsigned(mode));title.accept();title.accept();
            title.select(1);title.accept();title.select((unsigned(mode)+1)%3);title.accept();title.back();
            require(desktop.fullscreen_mode()==mode,"Cancel did not restore the previous fullscreen mode");
            title.select(1);title.accept();title.select((unsigned(mode)+1)%3);title.accept();
            window_clock+=16000;title.ui_clock(window_clock);
            require(title.frame().screen==vista_menu(256) && desktop.fullscreen_mode()==mode,"Preview timeout did not restore the previous fullscreen mode");
        }
        require(desktop.apply_video(960,720,FullscreenMode::Windowed).empty(),"Could not restore window size");renderer.resized();
        renderer.upscale_world(true);
        SDL_Event event{};event.type=SDL_KEYDOWN;event.key.windowID=SDL_GetWindowID(window);event.key.keysym.sym=SDLK_F1;
        require(!pause.event(event) && !pause.visible(),"F1 opened a game overlay");
        event.key.keysym.sym=SDLK_ESCAPE;require(pause.event(event) && pause.visible(),"Escape did not open pause menu");
        const auto accept_row=[&](unsigned row){pause.state().select(row);event.key.keysym.sym=SDLK_RETURN;require(pause.event(event),"Pause menu did not handle Enter");};
        accept_row(1);accept_row(3);require(pause.state().frame().screen==vista_menu(261),"Pause Settings did not open Video Settings");
        check_mode(pause.state(),pstencil::GraphicsMode::Original);require(pause.state().frame().selected==0,"Pause Video Settings did not focus Graphics Mode first");
        accept_row(0);check_mode(pause.state(),pstencil::GraphicsMode::pStencil);
        require(desktop.display_settings.graphics_mode==pstencil::GraphicsMode::pStencil,"Pause menu did not switch the live renderer");
        draw(pause.draw(960,720),"pause-pStencil.ppm");
        require(renderer.draw(camera,out/"pStencil-no-lights.ppm"),"pStencil draw failed");
        require(bytes(out/"Original-before.ppm")==bytes(out/"pStencil-no-lights.ppm"),"pStencil changed a scene with no point lights");
        SDL_SetWindowSize(window,853,479);renderer.resized();require(renderer.draw(camera),"Active pStencil resize failed");
        require(renderer.display_settings().graphics_mode==pstencil::GraphicsMode::pStencil,"Resize lost pStencil mode");
        SDL_SetWindowSize(window,960,720);renderer.resized();
        accept_row(0);check_mode(pause.state(),pstencil::GraphicsMode::PathTracing);
        require(renderer.display_settings().graphics_mode==pstencil::GraphicsMode::PathTracing,"Pause menu did not switch to Path Tracing");
        require(renderer.draw(camera,out/"PathTracing.ppm"),"Path Tracing draw failed");
        check_path_resources(renderer,true);
        SDL_SetWindowSize(window,853,479);renderer.resized();require(renderer.draw(camera),"Active Path Tracing resize failed");
        require(renderer.display_settings().graphics_mode==pstencil::GraphicsMode::PathTracing,"Resize lost Path Tracing mode");
        SDL_SetWindowSize(window,960,720);renderer.resized();
        accept_row(0);check_mode(pause.state(),pstencil::GraphicsMode::Original);
        check_path_resources(renderer,false);
        require(renderer.draw(camera,out/"Original-after.ppm"),"Restored Original draw failed");
        require(bytes(out/"Original-before.ppm")==bytes(out/"Original-after.ppm"),"Menu graphics round trip changed restored Original pixels");
        require(MenuState(content,preferences).pc_display_settings().graphics_mode==pstencil::GraphicsMode::Original,"Pause graphics preference did not persist");
        for(const auto mode:{FullscreenMode::Fullscreen,FullscreenMode::BorderlessFullscreen,FullscreenMode::Windowed}) {
            accept_row(1);accept_row(unsigned(mode));
            require(desktop.fullscreen_mode()==mode,"Pause Fullscreen Mode did not change the SDL window");
            accept_row(0);
            require(pause.state().frame().screen==vista_menu(261) && pause.state().pc_value_label(pause.state().frame(),1)==fullscreen_mode_name(mode),"Pause menu lost the exact fullscreen mode");
            require(renderer.draw(camera),"Pause window mode change failed to render");
        }
        event.key.keysym.sym=SDLK_ESCAPE;
        require(solo.event(event) && solo.pauses_game_time(),"Solo pause menu did not open");
        const auto solo_row=[&](unsigned row){solo.state().select(row);event.key.keysym.sym=SDLK_RETURN;require(solo.event(event),"Solo pause menu did not handle Enter");};
        solo_row(3);solo_row(3);
        require(solo.state().frame().screen==vista_menu(261),"Solo pause Settings did not open Video Settings");
        check_mode(solo.state(),pstencil::GraphicsMode::Original);
        solo_row(0);check_mode(solo.state(),pstencil::GraphicsMode::pStencil);
        solo_row(0);check_mode(solo.state(),pstencil::GraphicsMode::PathTracing);
        solo_row(0);check_mode(solo.state(),pstencil::GraphicsMode::Original);
        for(const auto mode:{FullscreenMode::Fullscreen,FullscreenMode::BorderlessFullscreen,FullscreenMode::Windowed}) {
            solo_row(1);solo_row(unsigned(mode));solo_row(0);
            require(desktop.fullscreen_mode()==mode && solo.state().pc_value_label(solo.state().frame(),1)==fullscreen_mode_name(mode),"Solo pause menu lost fullscreen selection");
            require(renderer.draw(camera),"Solo fullscreen switch failed to render");
        }
        exercise_upscaling(pause.state(),accept_row,[&]{event.key.keysym.sym=SDLK_ESCAPE;require(pause.event(event),"Pause back failed");},
            [&]{draw(pause.draw(960,720),"pause-upscaling.ppm");},false);
        exercise_upscaling(solo.state(),solo_row,[&]{event.key.keysym.sym=SDLK_ESCAPE;require(solo.event(event),"Solo back failed");},
            [&]{draw(solo.draw(960,720),"solo-upscaling.ppm");},false);

    }
    std::cerr.rdbuf(previous);require(diagnostics.str().empty(),"Video Settings Vulkan diagnostics");
    std::cout<<"PASS first-row Graphics Mode in Settings and solo/multiplayer pause Video Settings; mouse and SDL Enter cycle Original -> pStencil -> Path Tracing -> Original; Upscaling submenu with FSR 3.1 / DLSS 4.5 and quality modes, all three fullscreen modes, confirmation/rollback, persistence, resize, exact restored pixels and inert F1\n";return 0;
}catch(const std::exception& e){std::cerr.rdbuf(previous);std::cerr<<diagnostics.str()<<e.what()<<'\n';return 1;}}

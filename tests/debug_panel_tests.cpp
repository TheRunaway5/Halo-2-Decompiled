#include "halo2/debug_panel.hpp"
#include "halo2/renderer.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<char> bytes(const std::filesystem::path& path) {
    std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};
}
void key(SDL_Window* window,SDL_Keycode code) {
    SDL_Event e{};e.type=SDL_KEYDOWN;e.key.windowID=SDL_GetWindowID(window);
    e.key.keysym.sym=code;e.key.keysym.scancode=SDL_GetScancodeFromKey(code);
    check(SDL_PushEvent(&e)==1,"cannot queue panel key");
}
void drain(DebugPanel& panel){SDL_Event e;while(SDL_PollEvent(&e))panel.process_event(e);}
}
int main(int argc,char** argv) {
    try {
        const auto out=std::filesystem::absolute(argc>1?argv[1]:"debug-panel-results");
        std::filesystem::create_directories(out);
        check(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)==0,SDL_GetError());
        auto* window=SDL_CreateWindow("Optional F1 validation",0,0,960,720,SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN);
        check(window,SDL_GetError());
        Mesh mesh;Material material;material.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};
        mesh.materials.push_back(material);
        mesh.vertices={{{4,-2,-1},{1,1,1},{0,0},0},{{4,2,-1},{1,1,1},{1,0},0},{{4,0,2},{1,1,1},{.5f,1},0}};
        Camera camera;camera.field_of_view=60;
        std::ostringstream validation;auto* previous=std::cerr.rdbuf(validation.rdbuf());
        DisplaySettings display_settings;
        try {
            {
                Renderer renderer(window,mesh,true);auto& panel=renderer.debug_panel();
                panel.share_display_settings(display_settings);
                check(!panel.display_settings().crt_filter,"CRT did not start disabled");
                SDL_StopTextInput();SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
                for(unsigned i=0;i<10000;++i){SDL_Event e{};e.type=SDL_MOUSEMOTION;e.motion.windowID=SDL_GetWindowID(window);e.motion.x=int(i%960);panel.process_event(e);}
                check(panel.context()->InputEventsQueue.empty(),"hidden panel retained input events");
                check(renderer.draw(camera,out/"closed-before.ppm"),"closed draw failed");
                key(window,SDLK_F1);drain(panel);
                check(panel.visible() && panel.captures_game_input() && SDL_IsTextInputActive(),"F1 did not own panel/text input");
                for(unsigned i=0;i<3;++i)check(renderer.draw(camera,i==2?out/"open.ppm":std::filesystem::path{}),"open panel draw failed");
                check(bytes(out/"closed-before.ppm")!=bytes(out/"open.ppm"),"F1 produced no visible overlay");
                SDL_Event mouse{};mouse.type=SDL_MOUSEBUTTONDOWN;mouse.button.windowID=SDL_GetWindowID(window);mouse.button.button=SDL_BUTTON_LEFT;
                panel.process_event(mouse);check(renderer.draw(camera),"held button draw failed");
                key(window,SDLK_F1);drain(panel);
                check(!panel.visible() && !SDL_IsTextInputActive(),"closing F1 did not restore text input");
                check(!ImGui::GetIO().MouseDown[0] && panel.context()->InputEventsQueue.empty(),"closing panel retained mouse input");
                check(renderer.draw(camera,out/"closed-after.ppm"),"closed-after draw failed");
                check(bytes(out/"closed-before.ppm")==bytes(out/"closed-after.ppm"),"closed overlay changed game pixels");
                key(window,SDLK_F1);drain(panel);
                check(renderer.draw(camera),"debug tab setup failed");
                auto* settings_window=ImGui::FindWindowByName("Halo 2 Settings (F1)");
                check(settings_window,"settings window missing");
                auto* tabs=panel.context()->TabBars.GetByKey(settings_window->GetID("settings"));
                check(tabs,"settings tab bar missing");
                const auto display_id=ImHashStr("Display",0,tabs->ID);
                check(!ImGui::TabBarFindTabByID(tabs,display_id),"F1 retained its former CRT Display tab");
                display_settings.crt_filter=true;
                check(renderer.draw(camera,out/"crt-open.ppm"),"shared CRT setting draw failed");
                check(panel.display_settings().crt_filter,"renderer did not consume the shared CRT preference");
                key(window,SDLK_F1);drain(panel);
                check(renderer.draw(camera,out/"crt-closed.ppm"),"closed CRT draw failed");
                check(bytes(out/"closed-before.ppm")!=bytes(out/"crt-closed.ppm"),"CRT preference did not change game pixels");
                check(renderer.draw(camera,out/"crt-repeat.ppm"),"repeat CRT draw failed");
                check(bytes(out/"crt-closed.ppm")==bytes(out/"crt-repeat.ppm"),"CRT retained temporal history");
                panel.display_settings({false});
                check(renderer.draw(camera,out/"crt-disabled.ppm"),"disabled CRT draw failed");
                check(bytes(out/"closed-before.ppm")==bytes(out/"crt-disabled.ppm"),"disabling CRT did not restore exact game pixels");
                panel.display_settings({true});
                SDL_StartTextInput();key(window,SDLK_F1);drain(panel);
                SDL_SetWindowSize(window,1280,720);renderer.resized();
                for(unsigned i=0;i<2;++i)check(renderer.draw(camera,i==1?out/"resized-open.ppm":std::filesystem::path{}),"resized open draw failed");
                key(window,SDLK_ESCAPE);drain(panel);
                check(!panel.visible() && SDL_IsTextInputActive(),"Escape did not restore prior text ownership");
                SDL_StopTextInput();
                for(const auto size:{std::pair{1920,1080},std::pair{2560,1080},std::pair{1920,1440},std::pair{640,480}}) {
                    SDL_SetWindowSize(window,size.first,size.second);renderer.resized();
                    check(panel.display_settings().crt_filter,"resize lost CRT setting");
                    const auto prefix=std::to_string(size.first)+"x"+std::to_string(size.second);
                    check(renderer.draw(camera,out/(prefix+"-crt.ppm")),"resized CRT draw failed");
                    panel.display_settings({false});
                    check(renderer.draw(camera,out/(prefix+"-off.ppm")),"resized unfiltered draw failed");
                    check(bytes(out/(prefix+"-crt.ppm"))!=bytes(out/(prefix+"-off.ppm")),"resized CRT produced no effect");
                    panel.display_settings({true});
                }
            }
            // Menu -> gameplay and campaign BSP changes replace the Renderer while
            // the desktop session retains its display preferences.
            {
                Renderer renderer(window,mesh,true);
                renderer.debug_panel().share_display_settings(display_settings);
                check(renderer.debug_panel().display_settings().crt_filter,"renderer replacement lost CRT setting");
                check(renderer.draw(camera,out/"replacement-crt.ppm"),"replacement CRT draw failed");
                check(bytes(out/"replacement-crt.ppm")==bytes(out/"640x480-crt.ppm"),"replacement renderer changed CRT presentation");
            }
        } catch(...) {std::cerr.rdbuf(previous);std::cerr<<validation.str();throw;}
        std::cerr.rdbuf(previous);
        check(validation.str().empty(),validation.str().c_str());
        SDL_DestroyWindow(window);SDL_Quit();
        std::cout<<"F1 input/text/mouse capture without Display tab, shared CRT preference, exact disabled pixels, no temporal history, 4:3/16:9/ultrawide Vulkan resize and cleanup passed\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

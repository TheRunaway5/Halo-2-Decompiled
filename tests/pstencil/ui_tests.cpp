#include "halo2/debug_panel.hpp"
#include "halo2/renderer.hpp"
#include "halo2/game_pause_menu.hpp"
#include "halo2/desktop_session.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void require(bool value,const char* why){if(!value)throw FormatError(why);}
std::vector<char> bytes(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
void click(DebugPanel& panel,Renderer& renderer,const Camera& camera,float x,float y) {
    ImGui::SetCurrentContext(panel.context());
    auto& io=ImGui::GetIO();io.AddMousePosEvent(x,y);io.AddMouseButtonEvent(0,true);
    require(renderer.draw(camera),"pStencil UI mouse-down frame failed");
    io.AddMouseButtonEvent(0,false);require(renderer.draw(camera),"pStencil UI mouse-up frame failed");
    require(renderer.draw(camera),"pStencil UI settling frame failed");
}
}
int main(int argc,char** argv){std::ostringstream diagnostics;auto* previous=std::cerr.rdbuf(diagnostics.rdbuf());try{
    require(argc==2,"provide pStencil UI evidence directory");const std::filesystem::path out=argv[1];std::filesystem::create_directories(out);
    require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto* window=SDL_CreateWindow("pStencil live selector",0,0,960,720,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);require(window,SDL_GetError());
    {
        Mesh mesh;Material material;material.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};mesh.materials.push_back(material);
        mesh.vertices={{{4,-2,-1},{1,1,1},{0,0},0},{{4,2,-1},{1,1,1},{1,0},0},{{4,0,2},{1,1,1},{.5f,1},0}};
        Renderer renderer(window,mesh,true);Camera camera;camera.field_of_view=60;DisplaySettings shared;
        auto& panel=renderer.debug_panel();panel.share_display_settings(shared);
        auto requested=shared;requested.graphics_mode=pstencil::GraphicsMode::pStencil;
        require(renderer.validate_display_settings(requested).empty(),"pStencil unsupported on UI test device");
        require(renderer.draw(camera,out/"Original-before.ppm"),"pStencil UI baseline failed");
        panel.set_visible(true);for(unsigned i=0;i<3;++i)require(renderer.draw(camera),"pStencil UI opening failed");
        const auto choose=[&](unsigned item) {
            const auto* settings=ImGui::FindWindowByName("Halo 2 Settings (F1)");require(settings,"pStencil settings window absent");
            click(panel,renderer,camera,settings->Pos.x+150,settings->Pos.y+60);
            const auto* popup=ImGui::FindWindowByName("##Combo_00");require(popup && popup->Active,"pStencil graphics combo did not open");
            const auto& style=ImGui::GetStyle();const float line=ImGui::GetFontSize()+style.ItemSpacing.y;
            click(panel,renderer,camera,popup->Pos.x+30,popup->Pos.y+style.WindowPadding.y+ImGui::GetFontSize()*.5f+item*line);
        };
        choose(1);require(shared.graphics_mode==pstencil::GraphicsMode::pStencil,"pStencil combo selection did not change shared mode");
        require(renderer.draw(camera,out/"pStencil-selected.ppm"),"pStencil selected UI capture failed");
        panel.set_visible(false);require(renderer.draw(camera,out/"pStencil-no-lights.ppm"),"pStencil live draw failed");
        require(bytes(out/"Original-before.ppm")==bytes(out/"pStencil-no-lights.ppm"),"pStencil changed a scene with no point lights");
        SDL_SetWindowSize(window,853,479);renderer.resized();
        require(renderer.draw(camera,out/"pStencil-resized.ppm"),"active pStencil resize failed");
        require(shared.graphics_mode==pstencil::GraphicsMode::pStencil,"resize lost pStencil mode");
        SDL_SetWindowSize(window,960,720);renderer.resized();
        require(renderer.draw(camera,out/"pStencil-size-restored.ppm"),"pStencil size restoration failed");
        require(bytes(out/"Original-before.ppm")==bytes(out/"pStencil-size-restored.ppm"),"pStencil resize changed unlit pixels");
        panel.set_visible(true);for(unsigned i=0;i<3;++i)require(renderer.draw(camera),"Original UI reopening failed");
        choose(0);require(shared.graphics_mode==pstencil::GraphicsMode::Original,"Original combo selection did not restore shared mode");
        panel.set_visible(false);require(renderer.draw(camera,out/"Original-after.ppm"),"Original live draw failed");
        require(bytes(out/"Original-before.ppm")==bytes(out/"Original-after.ppm"),"Original UI round trip changed pixels");
        const std::filesystem::path root=HALO2_SOURCE_DIR;
        Mesh pause_mesh;GamePauseMenu pause(root/"assets/disc/maps",root/"Halo 2 Vista installer",pause_mesh,false);
        DesktopSession desktop;pause.bind(window,desktop,renderer);
        SDL_Event escape{};escape.type=SDL_KEYDOWN;escape.key.windowID=SDL_GetWindowID(window);escape.key.keysym.sym=SDLK_ESCAPE;
        for(auto mode:{pstencil::GraphicsMode::pStencil,pstencil::GraphicsMode::Original}) {
            desktop.display_settings.graphics_mode=mode;
            require(pause.event(escape) && pause.visible(),"pause settings did not open");
            require(pause.state().pc_display_settings().graphics_mode==mode,"pause settings retained an earlier F1 graphics mode");
            require(pause.event(escape) && !pause.visible(),"pause settings did not close");
        }
    }
    SDL_DestroyWindow(window);SDL_Quit();std::cerr.rdbuf(previous);require(diagnostics.str().empty(),"pStencil UI Vulkan diagnostics");
    std::cout<<"PASS live Graphics combo mouse selection Original -> pStencil -> Original; shared session state and exact restored pixels\n";return 0;
}catch(const std::exception& e){std::cerr.rdbuf(previous);std::cerr<<diagnostics.str()<<e.what()<<'\n';return 1;}}

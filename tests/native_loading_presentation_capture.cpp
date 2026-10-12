#include "halo2/native_loading_presentation.hpp"
#include "halo2/renderer.hpp"
#include <bit>
#include <filesystem>
#include <iostream>
#include <memory>

int main(int argc,char** argv) {
    try {
        if(argc!=10 && argc!=11)throw std::runtime_error("assets, output.ppm, width, height, elapsed-ms, sweep-progress-word, fade-ARGB-or-none, raster-offset-x, raster-offset-y, optional draw-prefix required");
        const auto number=[](const char* text) {
            std::size_t used{};const auto value=std::stoull(text,&used,0);
            if(text[used] || value>0xffffffffull)throw std::runtime_error("invalid loading capture word");
            return std::uint32_t(value);
        };
        const auto width=number(argv[3]),height=number(argv[4]);
        if(!width || !height || width>32767 || height>32767)throw std::runtime_error("invalid loading capture extent");
        const auto output=std::filesystem::absolute(argv[2]);
        std::filesystem::create_directories(output.parent_path());
        const auto assets=halo2::load_vista_loading_assets(argv[1]);
        halo2::NativeLoadingPresentation presentation(assets);
        halo2::VistaLoadingFadeState elapsed_owner{};
        const auto elapsed=halo2::vista_loading_fade_update(elapsed_owner,0,number(argv[5]),{0x027f,0x1f80});
        const auto fade=std::string(argv[7])=="none"?std::optional<std::uint32_t>{}:number(argv[7]);
        const auto draws=halo2::vista_loading_geometry(elapsed.elapsed_seconds_bits,width,height,number(argv[6]),fade,{0x027f,0x1f80});
        presentation.update(draws,width,height,{std::stof(argv[8]),std::stof(argv[9])});
        // Read-only replay controls isolate the first differing original draw.
        // They do not alter the production loading frame or its resident bank.
        const auto count=argc==11?number(argv[10]):std::uint32_t(draws.size());
        if(!count || count>draws.size())throw std::runtime_error("invalid loading capture draw prefix");
        const auto vertices=presentation.vertices().first(count*6);
        if(SDL_Init(SDL_INIT_VIDEO)!=0)throw std::runtime_error(SDL_GetError());
        auto window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(
            SDL_CreateWindow("Original Vista loading presentation replay",0,0,int(width),int(height),
                SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN|SDL_WINDOW_RESIZABLE),&SDL_DestroyWindow);
        if(!window)throw std::runtime_error(SDL_GetError());
        SDL_PumpEvents();
        {
            halo2::Renderer renderer(window.get(),presentation.resident_mesh(),true);
            renderer.prepare_loading();
            bool rendered=false;
            for(unsigned attempt=0;attempt<4 && !rendered;++attempt) {
                SDL_PumpEvents();
                rendered=renderer.draw_loading(0,presentation.materials(),vertices,output);
            }
            if(!rendered || !std::filesystem::is_regular_file(output))throw std::runtime_error("loading renderer did not capture a frame");
            std::cout<<"PASS actual native Renderer loading frame "<<width<<'x'<<height<<" draws="<<draws.size()
                     <<" elapsed-ms="<<argv[5]<<" draw-prefix="<<count
                     <<" offset="<<argv[8]<<','<<argv[9]<<" device="<<renderer.device_name()<<'\n';
        }
        window.reset();SDL_Quit();
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';SDL_Quit();return 1; }
}

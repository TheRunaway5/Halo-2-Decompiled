#include "halo2/native_loading_presentation.hpp"
#include "halo2/native_vista_loading_audio.hpp"
#include "halo2/renderer.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>

// A presentation replay, with an explicitly supplied zero sweep owner. This
// captures the production renderer and loading-audio adapter, not a map load.
// RGB frames go to stdout; the caller encodes/muxes them with audio.f32le.
int main(int argc,char** argv) {
    try {
        if(argc!=3)throw std::runtime_error("usage: capture_vista_loading_video ASSETS NEW_OUTPUT_DIRECTORY");
        constexpr unsigned width=1280,height=720,fps=30,seconds=24,rate=48000;
        constexpr halo2::VistaLoadingFloatingPoint fp{0x027f,0x1f80};
        const auto output=std::filesystem::absolute(argv[2]);
        if(!std::filesystem::create_directory(output))throw std::runtime_error("Output directory must be new");
        const auto assets=halo2::load_vista_loading_assets(argv[1]);
        halo2::NativeLoadingPresentation presentation(assets);
        SDL_setenv("SDL_AUDIODRIVER","dummy",1);
        if(SDL_Init(SDL_INIT_VIDEO)!=0)throw std::runtime_error(SDL_GetError());
        auto window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(
            SDL_CreateWindow("Vista match-loading presentation capture",0,0,width,height,
                SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN),&SDL_DestroyWindow);
        if(!window)throw std::runtime_error(SDL_GetError());
        {
            halo2::Renderer renderer(window.get(),presentation.resident_mesh(),true);
            renderer.prepare_loading();
            halo2::NativeVistaLoadingAudioConfiguration config;
            config.realtime=false;
            std::atomic<std::uint32_t> audio_clock{};
            std::atomic<unsigned> clock_queries{};
            config.clock=[&]{++clock_queries;return audio_clock.load();};
            halo2::NativeVistaLoadingAudio audio(std::filesystem::path(argv[1])/"loading-tone.xbox-adpcm",config);
            audio.begin(false);
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
            for(;;) {
                const auto state=audio.snapshot();
                if(!state.failure.empty())throw std::runtime_error(state.failure);
                if(clock_queries.load() && std::all_of(state.playing.begin(),state.playing.end(),[](bool playing){return playing;}))break;
                if(std::chrono::steady_clock::now()>deadline)throw std::runtime_error("Loading audio did not start");
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            std::ofstream pcm(output/"audio.f32le",std::ios::binary);
            if(!pcm)throw std::runtime_error("Cannot create audio capture");
            halo2::VistaLoadingFadeState fade{};
            const auto origin=std::chrono::steady_clock::now();
            std::vector<char> pixels(width*height*3);
            std::uint32_t published_audio_tick=0;
            for(unsigned frame=0;frame<seconds*fps;++frame) {
                const auto elapsed=frame*1000/fps;
                std::this_thread::sleep_until(origin+std::chrono::milliseconds(elapsed));
                // The end of this supplied replay interval starts the recovered
                // 1.5-second graphics/ambience fade. It is not load completion.
                if(frame==(seconds-2)*fps)halo2::vista_loading_fade_begin(fade,0,elapsed,fp);
                const auto update=halo2::vista_loading_fade_update(fade,0,elapsed,fp);
                if(update.gain_service_bits)audio.gain(*update.gain_service_bits);
                // Capture/readback can run slower than playback. Advance the
                // existing worker with supplied 100ms clock fixtures so that
                // recorded PCM and graphics share the same replay timeline.
                const auto tick=elapsed/100*100;
                if(tick!=published_audio_tick) {
                    auto expected=audio.snapshot().source;
                    halo2::VistaLoadingAudioServices services;
                    services.volume=[](std::uint32_t,std::int32_t){return 0;};
                    halo2::vista_loading_audio_envelope(expected,
                        halo2::vista_loading_audio_elapsed_bits(0,tick,fp),services,fp);
                    const auto previous_queries=clock_queries.load();audio_clock=tick;
                    const auto audio_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
                    for(;;) {
                        const auto state=audio.snapshot();
                        if(!state.failure.empty())throw std::runtime_error(state.failure);
                        if(clock_queries.load()>previous_queries && state.volumes==expected.cached_volumes)break;
                        if(std::chrono::steady_clock::now()>audio_deadline)throw std::runtime_error("Audio worker missed supplied replay clock");
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    published_audio_tick=tick;
                }
                presentation.update(halo2::vista_loading_geometry(update.elapsed_seconds_bits,
                    width,height,0,update.packed_argb,fp),width,height);
                const auto image=output/"last-frame.ppm";
                bool rendered=false;
                for(unsigned attempt=0;attempt<4 && !rendered;++attempt) {
                    SDL_PumpEvents();
                    rendered=renderer.draw_loading(0,presentation.materials(),presentation.vertices(),image);
                }
                if(!rendered)throw std::runtime_error("Loading video frame was not rendered");
                std::ifstream input(image,std::ios::binary);
                std::string magic;unsigned actual_width=0,actual_height=0,maximum=0;
                input>>magic>>actual_width>>actual_height>>maximum;
                if(!input || input.get()!='\n' || magic!="P6" || actual_width!=width || actual_height!=height || maximum!=255)
                    throw std::runtime_error("Loading capture extent/header changed");
                input.read(pixels.data(),std::streamsize(pixels.size()));
                if(!input || input.peek()!=std::char_traits<char>::eof())throw std::runtime_error("Incomplete loading capture pixels");
                std::cout.write(pixels.data(),std::streamsize(pixels.size()));
                if(!std::cout)throw std::runtime_error("Video encoder closed its input");
                const auto samples=audio.render(rate/fps);
                pcm.write(reinterpret_cast<const char*>(samples.data()),std::streamsize(samples.size()*sizeof(float)));
                if(!pcm)throw std::runtime_error("Cannot write loading audio capture");
            }
            const auto state=audio.snapshot();
            if(!state.failure.empty() || state.mixed_output_frames!=seconds*rate)
                throw std::runtime_error("Loading audio capture failed or lost samples");
            audio.end();
            std::cerr<<"PASS Vista loading presentation: "<<seconds*fps<<" RGB frames, "
                <<state.mixed_output_frames<<" stereo PCM frames; device="<<renderer.device_name()<<'\n';
        }
        window.reset();SDL_Quit();
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';SDL_Quit();return 1;
    }
}

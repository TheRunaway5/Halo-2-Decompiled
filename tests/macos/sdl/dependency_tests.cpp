#include <SDL.h>
#include <nlohmann/json.hpp>
#include <dlfcn.h>
#include <array>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char* message) {
    if(!value)throw std::runtime_error(std::string(message)+": "+SDL_GetError());
}
struct Audio {
    std::atomic<unsigned> callbacks{},bytes{};
    static void render(void* context,Uint8* stream,int count) {
        SDL_memset(stream,0,count);
        auto& self=*static_cast<Audio*>(context);
        self.bytes.fetch_add(unsigned(count));self.callbacks.fetch_add(1);
    }
};
struct Resources {
    Audio audio;
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    SDL_GameController* controller{};
    int virtual_joystick=-1;
    SDL_AudioDeviceID device{};
    ~Resources() {
        if(device)SDL_CloseAudioDevice(device);
        if(controller)SDL_GameControllerClose(controller);
        if(virtual_joystick>=0)SDL_JoystickDetachVirtual(virtual_joystick);
        if(renderer)SDL_DestroyRenderer(renderer);
        if(window)SDL_DestroyWindow(window);
        SDL_Quit();
    }
};
}
int main(int argc,char** argv)try {
    require(argc==2,"Supply the app's private library directory");
    Resources owner;
    nlohmann::json report{{"schema","H2_MACOS_SDL_RESOURCES_V1"}};
#if defined(__aarch64__) || defined(__arm64__)
    report["architecture"]="arm64";
#elif defined(__x86_64__)
    report["architecture"]="x86_64";
#endif
    SDL_version version{};SDL_GetVersion(&version);
    require(version.major==2&&version.minor==32&&version.patch==10,"Wrong runtime SDL version");
    require(SDL_MAJOR_VERSION==2&&SDL_MINOR_VERSION==32&&SDL_PATCHLEVEL==10,"Wrong SDL headers");
    report["version"]={version.major,version.minor,version.patch};
    Dl_info image{};
    require(dladdr(reinterpret_cast<const void*>(&SDL_GetVersion),&image)&&image.dli_fname,
            "Cannot locate the actual loaded SDL image");
    const auto loaded=std::filesystem::canonical(image.dli_fname);
    require(loaded.parent_path()==std::filesystem::canonical(argv[1]),"SDL borrowed an external library");
    report["loaded_sdl"]=loaded.string();
    SDL_SetMainReady();
    require(SDL_SetHint(SDL_HINT_RENDER_DRIVER,"metal")==SDL_TRUE,"Cannot select the Metal renderer");
    require(SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1")==SDL_TRUE,
            "Cannot enable this test's virtual controller while another app has focus");
    require(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)==0,
            "Native SDL initialization failed");
    const auto* video=SDL_GetCurrentVideoDriver();const auto* audio=SDL_GetCurrentAudioDriver();
    require(video&&std::string(video)=="cocoa","Cocoa video driver is unavailable");
    require(audio&&std::string(audio)=="coreaudio","CoreAudio output driver is unavailable");
    report["video_driver"]=video;report["audio_driver"]=audio;
    owner.window=SDL_CreateWindow("Halo 2 SDL dependency check",SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,320,180,SDL_WINDOW_ALLOW_HIGHDPI|SDL_WINDOW_SHOWN);
    require(owner.window,"Cannot create a native window");
    require(SDL_GetNumVideoDisplays()>0,"SDL reports no native display");
    SDL_DisplayMode mode{};require(SDL_GetCurrentDisplayMode(0,&mode)==0,"Cannot observe display mode");
    report["display"]={{"width",mode.w},{"height",mode.h},{"refresh",mode.refresh_rate}};
    owner.renderer=SDL_CreateRenderer(owner.window,-1,SDL_RENDERER_ACCELERATED);
    require(owner.renderer,"Cannot create accelerated SDL rendering");
    SDL_RendererInfo renderer{};require(SDL_GetRendererInfo(owner.renderer,&renderer)==0,"Cannot inspect SDL renderer");
    require(renderer.name&&std::string(renderer.name)=="metal","Requested Metal renderer was not selected");
    int width{},height{};require(SDL_GetRendererOutputSize(owner.renderer,&width,&height)==0&&width>0&&height>0,
        "Cannot observe drawable size");
    require(SDL_SetRenderDrawColor(owner.renderer,32,64,128,255)==0&&SDL_RenderClear(owner.renderer)==0,
        "Native Metal clear failed");
    std::array<Uint8,4> pixel{};const SDL_Rect center{width/2,height/2,1,1};
    require(SDL_RenderReadPixels(owner.renderer,&center,SDL_PIXELFORMAT_RGBA32,pixel.data(),4)==0,
        "Cannot read the real Metal render target");
    for(unsigned i=0;i<4;++i)require(std::abs(int(pixel[i])-std::array<int,4>{32,64,128,255}[i])<=1,
        "Metal clear/readback returned a different pixel");
    SDL_RenderPresent(owner.renderer);
    report["renderer"]=renderer.name;report["drawable"]={width,height};report["readback_rgba"]=pixel;
    const auto type=SDL_RegisterEvents(1);require(type!=Uint32(-1),"Cannot allocate an event type");
    SDL_Event event{};event.type=type;event.user.code=0x123456;
    require(SDL_PushEvent(&event)==1,"Cannot queue a native SDL event");
    bool received=false;
    while(SDL_PollEvent(&event))if(event.type==type){require(event.user.code==0x123456,"Event payload changed");received=true;}
    require(received,"Queued event was lost");report["queued_event"]=true;
    owner.virtual_joystick=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
        SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
    require(owner.virtual_joystick>=0&&SDL_IsGameController(owner.virtual_joystick),"Virtual controller mapping failed");
    owner.controller=SDL_GameControllerOpen(owner.virtual_joystick);
    require(owner.controller,"Cannot open the actual SDL virtual controller");
    auto* joystick=SDL_GameControllerGetJoystick(owner.controller);
    require(SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_LEFTX,16384)==0&&
        SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,1)==0,"Cannot set virtual controller inputs");
    SDL_JoystickUpdate();
    require(SDL_GameControllerGetAxis(owner.controller,SDL_CONTROLLER_AXIS_LEFTX)==16384&&
        SDL_GameControllerGetButton(owner.controller,SDL_CONTROLLER_BUTTON_A)==1,"Controller inputs changed");
    report["virtual_controller"]={{"axis",16384},{"button_a",true}};
    SDL_AudioSpec desired{},obtained{};desired.freq=48000;desired.channels=2;
    desired.format=AUDIO_F32SYS;desired.samples=512;desired.callback=Audio::render;desired.userdata=&owner.audio;
    owner.device=SDL_OpenAudioDevice(nullptr,0,&desired,&obtained,0);
    require(owner.device&&obtained.freq==48000&&obtained.channels==2&&obtained.format==AUDIO_F32SYS,
        "Real stereo float output is unavailable");
    const auto frequency=SDL_GetPerformanceFrequency(),start=SDL_GetPerformanceCounter();
    require(frequency>0,"Native performance clock is unavailable");
    SDL_PauseAudioDevice(owner.device,0);
    while(owner.audio.callbacks.load()<3&&SDL_GetPerformanceCounter()-start<frequency*3)SDL_Delay(10);
    SDL_PauseAudioDevice(owner.device,1);
    require(owner.audio.callbacks.load()>=3&&owner.audio.bytes.load()>0,"CoreAudio did not consume output callbacks");
    report["audio"]={{"rate",obtained.freq},{"channels",obtained.channels},{"samples",obtained.samples},
        {"callbacks",owner.audio.callbacks.load()},{"bytes",owner.audio.bytes.load()},{"silent_test_pcm",true}};
    report["passed"]=true;std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

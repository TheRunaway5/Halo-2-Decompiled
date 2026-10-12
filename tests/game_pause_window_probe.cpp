#include <SDL.h>
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

// Inject into the actual native game window after presentation. Test schedules
// use frame:key pairs; each key includes a release before the following press.
namespace {
SDL_Window *window{};
unsigned frame{},next{};
bool release{};
struct Press {unsigned frame;SDL_Keycode key;};
std::vector<Press> schedule;
}
extern "C" SDL_Window *SDL_CreateWindow(const char *title,int x,int y,int w,int h,Uint32 flags) {
  auto original=reinterpret_cast<decltype(&SDL_CreateWindow)>(dlsym(RTLD_NEXT,"SDL_CreateWindow"));
  window=original(title,x,y,w,h,flags);
  if(const char *text=std::getenv("HALO2_PAUSE_PROBE_EVENTS")) {
    std::istringstream input(text);std::string item;
    while(std::getline(input,item,',')) {
      const auto at=item.find(':');const auto name=item.substr(at+1);
      schedule.push_back({unsigned(std::stoul(item.substr(0,at))),SDL_GetKeyFromName(name.c_str())});
    }
  }
  return window;
}
extern "C" int SDL_PollEvent(SDL_Event *event) {
  auto original=reinterpret_cast<decltype(&SDL_PollEvent)>(dlsym(RTLD_NEXT,"SDL_PollEvent"));
  if(event && window && next<schedule.size() && frame>=schedule[next].frame) {
    *event={};event->type=release?SDL_KEYUP:SDL_KEYDOWN;
    event->key.windowID=SDL_GetWindowID(window);event->key.timestamp=SDL_GetTicks();
    event->key.keysym.sym=schedule[next].key;
    event->key.keysym.scancode=SDL_GetScancodeFromKey(schedule[next].key);
    event->key.state=release?SDL_RELEASED:SDL_PRESSED;
    if(release) {release=false;++next;} else release=true;
    return 1;
  }
  int result{};
  do {
    result=original(event);
    if(!result || !event || schedule.empty())return result;
    // Only the declared test inputs may operate this window. Desktop typing
    // or a connected pad must not accidentally dismiss the captured menu.
  } while(event->type==SDL_KEYDOWN || event->type==SDL_KEYUP ||
      event->type==SDL_MOUSEBUTTONDOWN || event->type==SDL_MOUSEBUTTONUP ||
      event->type==SDL_MOUSEMOTION || event->type==SDL_MOUSEWHEEL ||
      event->type==SDL_CONTROLLERBUTTONDOWN || event->type==SDL_CONTROLLERBUTTONUP ||
      event->type==SDL_CONTROLLERAXISMOTION);
  return result;
}
extern "C" VkResult vkQueuePresentKHR(VkQueue queue,const VkPresentInfoKHR *info) {
  auto original=reinterpret_cast<PFN_vkQueuePresentKHR>(dlsym(RTLD_NEXT,"vkQueuePresentKHR"));
  const auto result=original(queue,info);++frame;
  if(window)if(const auto *path=std::getenv("HALO2_PAUSE_PROBE_LOG"))if(auto *file=std::fopen(path,"a")) {
    std::fprintf(file,"%u relative=%d %s\n",frame,int(SDL_GetRelativeMouseMode()),SDL_GetWindowTitle(window));
    std::fclose(file);
  }
  return result;
}

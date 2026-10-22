#include "halo2/native_tu15_input_devices.hpp"
#include <SDL.h>
#include <algorithm>
#include <map>
#include <set>
#include <thread>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p) {
    auto b=m.view(p,4);std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);return v;
}
void encode(std::span<std::byte> b,std::size_t at,std::uint32_t value,unsigned size) {
    for(unsigned i=0;i<size;++i)b[at+i]=std::byte(value>>(i*8));
}
[[noreturn]] void sdl_failure(const char* operation) {throw std::runtime_error(std::string(operation)+": "+SDL_GetError());}
}
struct NativeTu15InputDevices::State {
    struct Controller {
        SDL_GameController* native=nullptr;
        std::int32_t instance=-1;unsigned port=0;
        std::array<std::byte,18> previous{};bool sampled=false;std::uint32_t packet=0;
        ~Controller(){if(native)SDL_GameControllerClose(native);}
    };
    SourceRuntimeMemory& memory;
    NativeTu15InputSelection selection;
    const std::thread::id thread=std::this_thread::get_id();
    bool subsystem=false,initialized=false;
    std::array<std::int32_t,4> observed{-1,-1,-1,-1};
    std::map<std::uint32_t,std::unique_ptr<Controller>> controllers;
    std::uint32_t next_handle=0xf8000001u;
    std::vector<NativeTu15InputCall> calls;
    State(SourceRuntimeMemory& m,NativeTu15InputSelection s):memory(m),selection(std::move(s)) {
        const std::set<std::int32_t> distinct(selection.instances.begin(),selection.instances.end());
        if(selection.instances.size()>4 || distinct.size()!=selection.instances.size() ||
            std::ranges::any_of(selection.instances,[](auto id){return id<0;}) || (!selection.restricted && !selection.instances.empty()))
            throw std::invalid_argument("Native input instance selection is invalid");
    }
    ~State(){controllers.clear();if(subsystem)SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);}
    std::vector<std::int32_t> attached() {
        SDL_GameControllerUpdate();const auto count=SDL_NumJoysticks();if(count<0)sdl_failure("Native controller enumeration");
        std::vector<std::int32_t> ids;
        for(int i=0;i<count;++i)if(SDL_IsGameController(i)) {
            const auto id=SDL_JoystickGetDeviceInstanceID(i);if(id<0)sdl_failure("Native controller identity");
            if(!selection.restricted || std::ranges::find(selection.instances,id)!=selection.instances.end())ids.push_back(id);
        }
        return ids;
    }
    Controller& controller(std::uint32_t handle) {
        const auto at=controllers.find(handle);
        if(at==controllers.end())throw std::invalid_argument("Native input handle has no retained controller");
        return *at->second;
    }
    void write(NativeTu15InputCall& row,std::uint32_t p,std::span<const std::byte> bytes) {
        memory.write(p,bytes);row.writes.push_back({p,{bytes.begin(),bytes.end()}});
    }
    void put(NativeTu15InputCall& row,std::uint32_t p,std::uint32_t v) {
        std::array<std::byte,4> b{};encode(b,0,v,4);write(row,p,b);
    }
    std::array<std::byte,22> sample(Controller& c) {
        SDL_GameControllerUpdate();
        if(!SDL_GameControllerGetAttached(c.native))throw std::runtime_error("Native controller detached between device changes and state sampling");
        std::array<std::byte,18> b{};std::uint32_t buttons=0;
        constexpr std::array<SDL_GameControllerButton,8> digital{SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_DOWN,
            SDL_CONTROLLER_BUTTON_DPAD_LEFT,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,SDL_CONTROLLER_BUTTON_START,SDL_CONTROLLER_BUTTON_BACK,
            SDL_CONTROLLER_BUTTON_LEFTSTICK,SDL_CONTROLLER_BUTTON_RIGHTSTICK};
        for(unsigned i=0;i<digital.size();++i)if(SDL_GameControllerGetButton(c.native,digital[i]))buttons|=1u<<i;
        encode(b,0,buttons,2);
        // Modern native controller profile: RB=black, LB=white. SDL digital
        // face buttons produce 0/255; console pressure hardware is not claimed.
        constexpr std::array<SDL_GameControllerButton,6> analog{SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,
            SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_Y,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,SDL_CONTROLLER_BUTTON_LEFTSHOULDER};
        for(unsigned i=0;i<analog.size();++i)b[2+i]=std::byte(SDL_GameControllerGetButton(c.native,analog[i])?255:0);
        for(unsigned i=0;i<2;++i) {
            const int raw=std::max(0,int(SDL_GameControllerGetAxis(c.native,i?SDL_CONTROLLER_AXIS_TRIGGERRIGHT:SDL_CONTROLLER_AXIS_TRIGGERLEFT)));
            b[8+i]=std::byte((raw*255)/32767);
        }
        for(unsigned i=0;i<4;++i) {
            int raw=SDL_GameControllerGetAxis(c.native,SDL_GameControllerAxis(i));
            if(i&1)raw=std::clamp(-raw,-32768,32767);
            encode(b,10+i*2,std::uint16_t(raw),2);
        }
        if(!c.sampled || b!=c.previous){++c.packet;c.previous=b;c.sampled=true;}
        std::array<std::byte,22> out{};encode(out,0,c.packet,4);std::ranges::copy(b,out.begin()+4);return out;
    }
};
NativeTu15InputDevices::NativeTu15InputDevices(SourceRuntimeMemory& m,NativeTu15InputSelection s):state_(std::make_unique<State>(m,std::move(s))){}
NativeTu15InputDevices::~NativeTu15InputDevices()=default;
std::span<const NativeTu15InputCall> NativeTu15InputDevices::calls()const noexcept{return state_->calls;}
std::size_t NativeTu15InputDevices::live_controllers()const noexcept{return state_->controllers.size();}
std::vector<std::uintptr_t> NativeTu15InputDevices::native_controller_addresses()const {
    std::vector<std::uintptr_t> out;for(const auto& [handle,c]:state_->controllers)out.push_back(reinterpret_cast<std::uintptr_t>(c->native));return out;
}
bool NativeTu15InputDevices::try_call(SourcePhysicsContactMemory& bank,const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    auto& s=*state_;auto& m=s.memory;unsigned argc=0;std::uint32_t expected=0;
    switch(call.target) {
    case 0x4139a0:argc=2;expected=0x1273f4;break;
    case 0x4139a5:argc=3;expected=(call.instruction==0x1276eeu?0x1276eeu:0x1275cbu);break;
    case 0x413a12:argc=4;expected=0x12762f;break;
    case 0x413a68:argc=1;expected=(call.instruction==0x127447u?0x127447u:0x127608u);break;
    case 0x413a74:argc=2;expected=0x127640;break;
    case 0x413c4c:argc=2;expected=0x127744;break;
    case 0x413cbf:argc=2;expected=(call.instruction==0x127a7cu?0x127a7cu:0x127a68u);break;
    default:return false;
    }
    const auto sp=abi.gp[4];
    if(&bank!=&m || std::this_thread::get_id()!=s.thread || call.tail || call.instruction!=expected || word(m,sp)!=expected+5)
        throw std::invalid_argument("Native controller call bank, executor or complete source frame differs");
    NativeTu15InputCall row{call.instruction,call.target,sp,sp+4+argc*4,abi.gp[0],{}, {}};
    for(unsigned i=0;i<argc;++i)row.arguments.push_back(word(m,sp+4+i*4));
    const auto& a=row.arguments;
    if(call.target==0x4139a0u) {
        if(s.initialized || a[0]!=3 || a[1]!=sp+16)throw std::invalid_argument("Native controller initialization frame differs or repeated");
        constexpr std::array<std::uint32_t,3> types{0x4127d8,0x412990,0x41299c};
        for(unsigned i=0;i<3;++i)if(word(m,a[1]+i*8)!=types[i] || word(m,a[1]+i*8+4)!=4)
            throw std::invalid_argument("Native controller preallocation declaration differs");
        if(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)!=0)sdl_failure("Native controller subsystem initialization");
        s.subsystem=true;s.initialized=true;
    } else {
        if(!s.initialized)throw std::invalid_argument("Native controller API used before source initialization");
        if(call.target==0x4139a5u) {
            const auto peripheral=call.instruction==0x1276eeu;
            const auto insertion=peripheral?sp+40:sp+32,removal=peripheral?sp+36:sp+28;
            if(a[0]!=(peripheral?0x412888u:0x4127d8u) || a[1]!=insertion || a[2]!=removal)
                throw std::invalid_argument("Native device changes output frame differs");
            m.mutable_view(a[1],4);m.mutable_view(a[2],4);
            std::uint32_t inserted=0,removed=0;
            if(!peripheral) {
                const auto ids=s.attached();auto observed=s.observed;
                for(unsigned i=0;i<4;++i)if(observed[i]>=0 && std::ranges::find(ids,observed[i])==ids.end()){removed|=1u<<i;observed[i]=-1;}
                for(auto id:ids)if(std::ranges::find(observed,id)==observed.end()) {
                    const auto at=std::ranges::find(observed,-1);if(at==observed.end())break;
                    inserted|=1u<<std::distance(observed.begin(),at);*at=id;
                }
                s.observed=observed;
            }
            // This native registry provides game controllers only. The second
            // console peripheral class has no registered desktop provider.
            s.put(row,a[1],inserted);s.put(row,a[2],removed);row.return_eax=(inserted|removed)?1:0;
        } else if(call.target==0x413a12u) {
            if(a[0]!=0x4127d8u || a[1]>=4 || a[2] || a[3] || s.observed[a[1]]<0 ||
                std::ranges::any_of(s.controllers,[&](const auto& entry){return entry.second->port==a[1];}))
                throw std::invalid_argument("Native controller open frame or slot lifecycle differs");
            int index=-1;for(int i=0;i<SDL_NumJoysticks();++i)if(SDL_JoystickGetDeviceInstanceID(i)==s.observed[a[1]]){index=i;break;}
            if(index<0)throw std::runtime_error("Native controller detached before open");
            auto c=std::make_unique<State::Controller>();c->native=SDL_GameControllerOpen(index);if(!c->native)sdl_failure("Native controller open");
            c->instance=s.observed[a[1]];c->port=a[1];
            if(s.next_handle==0xffffffffu)throw std::overflow_error("Native controller opaque handle space exhausted");
            row.return_eax=s.next_handle++;s.controllers.emplace(row.return_eax,std::move(c));
        } else if(call.target==0x413a68u) {
            s.controller(a[0]);s.controllers.erase(a[0]);
        } else if(call.target==0x413a74u) {
            auto& c=s.controller(a[0]);(void)c;
            if(a[1]!=sp+48)throw std::invalid_argument("Native controller subtype output frame differs");
            m.mutable_view(a[1],1);constexpr std::array<std::byte,1> subtype{std::byte{1}};s.write(row,a[1],subtype);row.return_eax=0;
        } else if(call.target==0x413c4cu) {
            auto& c=s.controller(a[0]);
            if(a[1]!=sp+44)throw std::invalid_argument("Native controller state output frame differs");
            m.mutable_view(a[1],22);const auto sample=s.sample(c);s.write(row,a[1],sample);row.return_eax=0;
        } else {
            auto& c=s.controller(a[0]);const auto feedback=0x4f2970u+c.port*0x46u;
            if(a[1]!=feedback || word(m,feedback+4) || word(m,feedback+12))
                throw std::invalid_argument("Native controller feedback frame/event ownership differs");
            m.mutable_view(feedback,4);const auto motors=m.view(feedback+0x42,4);
            const auto left=std::to_integer<unsigned>(motors[0])|(std::to_integer<unsigned>(motors[1])<<8);
            const auto right=std::to_integer<unsigned>(motors[2])|(std::to_integer<unsigned>(motors[3])<<8);
            if(!SDL_GameControllerGetAttached(c.native))throw std::runtime_error("Native controller detached before rumble");
            if(!SDL_GameControllerHasRumble(c.native))row.return_eax=50; // Native unsupported motor transport.
            else {if(SDL_GameControllerRumble(c.native,Uint16(left),Uint16(right),100)!=0)sdl_failure("Native controller rumble");row.return_eax=0;}
            s.put(row,feedback,row.return_eax);
        }
    }
    abi.gp[0]=row.return_eax;abi.gp[4]=row.return_esp;s.calls.push_back(std::move(row));return true;
}
}

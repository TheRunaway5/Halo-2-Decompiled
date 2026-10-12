#include "halo2/vista_lobby_setup.hpp"
#include "halo2/cache.hpp"
#include <utility>

namespace halo2 {
namespace {
template<class F,class...A> decltype(auto) call(const F& f,A&&...a) {
  if(!f)throw FormatError("Missing consumed Vista lobby setup service");
  return f(std::forward<A>(a)...);
}
constexpr std::array<std::int16_t,7> semantics{0,1,2,3,5,6,7};
constexpr std::array<std::uint32_t,7> labels{0x0a0005d1,0x0e0005d2,0x0d000428,0x1000095d,0x120005db,0x130005da,0x0d0005d6};
constexpr std::array<std::uint32_t,8> callbacks{0x64f9a1,0x64f9dd,0x64ef79,0x64f5fd,0,0x64f015,0x64f676,0x64f68a};
std::uint32_t controller_mask(std::int32_t controller) {
  //Original x86 SHL masks the count to five bits, including negative values.
  return 1u<<(std::uint32_t(controller)&31u);
}
void request(std::uint32_t mask,std::uint32_t factory,const VistaLobbySetupServices& s) {
  call(s.request_context_initialize);
  call(s.screen_request,VistaLobbySetupRequest{mask,4,4,factory});
  call(s.request_context_release);
}
}
void vista_lobby_setup_construct(VistaLobbySetupList& list,std::int32_t controller,const VistaLobbySetupServices& s) {
  call(s.base_list,controller);call(s.child_array,0xb0u,0x84u,7u,0x61ffc9u,0x40a551u);
  call(s.callback_owner);list.callback=0x64fa19;
  const auto kind=call(s.session_enum);
  call(s.row_bank,std::string_view{"squad setting list"},8u,4u);call(s.bank_initialize);
  list.count=0;list.field464=1;
  const auto add=[&](std::int16_t semantic) {
    const auto handle=call(s.allocate_row);call(s.store_row,std::uint16_t(handle),semantic);
    list.handles[list.count]=handle;list.rows[list.count++]=semantic;
  };
  if(kind==1 || kind==3 || kind==5) {
    add(0);add(1);add(2);
    if(kind==5 && call(s.host) && call(s.count)>1) {add(3);list.field464=0;}
  } else if(kind==6) {add(7);add(5);}
  call(s.callback_link,list.callback);
}
bool vista_lobby_setup_label(std::optional<std::uint32_t> handle,const VistaLobbySetupServices& s) {
  if(!handle)return false;
  const auto text=call(s.text,0u);if(!text)return false;
  const auto row=call(s.selected_row,*handle);if(!row)return false;
  for(unsigned i=0;i<semantics.size();++i)if(*row==semantics[i]) {call(s.set_string,text,labels[i]);return true;}
  return false;
}
void vista_lobby_setup_initialize(const VistaLobbySetupServices& s) {
  const auto datum=call(s.screen_tag,23u);
  call(s.register_list,datum,VistaLobbySetupRegistration{});call(s.activate_list);call(s.initial_focus);
  if(const auto text=call(s.text,1u)) {
    const auto kind=call(s.session_enum); //Full616012 is(enum==0 || enum==1).
    call(s.set_string,text,kind==0 || kind==1?0x0b0005f7u:0x0a0005f6u);
  }
}
void vista_lobby_setup_details(VistaLobbySetupState& state,const VistaLobbySetupServices& s) {
  const auto handle=call(s.selection);const auto row=call(s.selected_row,handle).value_or(-1);
  const auto desc=call(s.child,6u,2u,0u);const auto caption=call(s.child,6u,3u,0u);
  const auto value=call(s.child,6u,4u,0u);const auto bitmap=call(s.child,8u,4u,0u);
  VistaLobbySetupMap map;const bool valid=call(s.map_query,map)!=0;
  if(bitmap)call(s.image,bitmap,VistaLobbySetupObject{});
  std::uint32_t description{},heading{},current{},frame{};
  switch(row) {
  case 0:
    description=0x0f0005de;heading=0x0b0005ea;current=0x030005f2;
    if(valid && bitmap) {
      if(!map.identifier4cbc) {
        if(const auto reference=call(s.multiplayer_definition,map.map4dd8))
          call(s.image,bitmap,call(s.resolve_image,*reference,0u));
      } else {
        VistaLobbySetupObject image{};
        if(call(s.identifier_image,map.identifier4cbc,image))call(s.image,bitmap,image);
      }
    }
    break;
  case 1: {
    description=0x130005df;heading=0x0f0005eb;current=0x0700055b;
    const auto engine=call(s.variant_engine);
    if(bitmap && engine) {
      std::optional<std::uint32_t> icon;
      switch(*engine) {case 1:icon=6;break;case 2:icon=0;break;case 3:icon=3;break;case 4:icon=1;break;
        case 7:icon=4;break;case 8:icon=8;break;case 9:icon=7;break;default:break;}
      if(icon)call(s.resource_frame,bitmap,1u,*icon);
    }
    break;
  }
  case 2:description=0x120005dd;heading=0x0f0005eb;current=0x0700055b;frame=2;break;
  case 3:description=0x15000982;heading=0x17000981;current=0x0e00042e;frame=1;break;
  case 5:description=0x1a0005e5;heading=0x120005db;frame=6;break;
  case 6:description=0x180005e7;heading=0x130005da;break;
  case 7:description=0x120005e3;heading=0x0e0005ef;current=0x060005f5;frame=4;break;
  default:break;
  }
  if(desc)call(s.set_string,desc,description);
  if(caption)call(s.set_string,caption,heading);
  if(value)call(s.set_string,value,current);
  if(bitmap)call(s.authored_frame,bitmap,frame);
  const auto party=call(s.find_row,std::int16_t{3});
  if(party && !state.party_removed && call(s.count)<2) {
    call(s.reset_list);call(s.set_selection,0u);call(s.remove_row,party);state.party_removed=1;
  }
  call(s.base_update);
}
void vista_lobby_setup_dispatch(std::uint32_t handle,VistaLobbySetupObject event,const VistaLobbySetupServices& s) {
  if(handle==0xffffffffu)return;
  const auto row=call(s.dispatch_row,std::uint16_t(handle));
  if(row>=0 && row<std::int16_t(callbacks.size()))if(const auto callback=callbacks[unsigned(row)])
    call(s.invoke_callback,callback,event);
}
void vista_lobby_setup_open_map(std::int32_t controller,const VistaLobbySetupServices& s) {
  request(controller_mask(controller),0x64f819,s);
}
void vista_lobby_setup_open_rules(std::int32_t controller,const VistaLobbySetupServices& s) {
  request(controller_mask(controller),0x61f280,s);
}
void vista_lobby_setup_open_quick(std::int32_t controller,const VistaLobbySetupServices& s) {
  const auto engine=call(s.variant_engine);
  if(!engine)throw FormatError("Vista setup quick callback requires its original selected variant");
  std::int32_t key;
  switch(*engine) {case 1:key=22;break;case 2:key=23;break;case 3:key=24;break;case 4:key=25;break;
    case 7:key=28;break;case 8:key=29;break;case 9:key=30;break;default:return;}
  vista_lobby_setup_quick_request(key,4,4,controller_mask(controller),s);
}
void vista_lobby_setup_quick_request(std::int32_t key,std::uint32_t arg2,std::uint32_t arg3,
    std::uint32_t controllers,const VistaLobbySetupServices& s) {
  call(s.request_context_initialize);
  call(s.screen_request,VistaLobbySetupRequest{controllers,arg2,arg3,key>=22 && key<=30?0x658896u:0x6587feu});
  const auto screen=call(s.request_context_release);
  if(!screen)throw FormatError("Vista setup quick request requires its original published screen");
  call(s.set_quick_key,screen,key);
}
} //namespace halo2

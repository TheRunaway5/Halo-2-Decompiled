#include "halo2/vista_menu_return_observer.hpp"
#include "halo2/cache.hpp"
#include <initializer_list>
#include <utility>

namespace halo2 {
namespace {
using Q=VistaMenuReturnQuery;
using A=VistaMenuReturnAction;
class Observer {
 public:
  Observer(VistaMenuReturnState& s,const VistaMenuReturnServices& v):state(s),services(v) {
    if(!services.query)throw FormatError("Vista return observer requires actual query services");
  }
  VistaMenuReturnObservation run() {
    result.screen=q(Q::screen);
    result.previous_ui_state=classify_screen(result.screen);
    result.current_ui_state=classify_lifecycle();
    if(byte(q(Q::ui_active))) {
      const auto protocol=q(Q::protocol);
      if(protocol==4||protocol==5||protocol==6)emit(A::set_ui_active,{1});
    }
    if(!byte(q(Q::ui_disabled)))observe_change();
    emit(A::observer_suffix,{result.screen,result.previous_ui_state,result.current_ui_state});
    return result;
  }
 private:
  VistaMenuReturnState& state;
  const VistaMenuReturnServices& services;
  VistaMenuReturnObservation result;
  static bool byte(std::uint32_t value){return (value&255)!=0;}
  std::uint32_t q(Q query){return services.query(query);}
  void emit(A action,std::initializer_list<std::uint32_t> args={}) {
    if(result.call_count>=result.calls.size())throw FormatError("Vista return observer intent overflow");
    auto& call=result.calls[result.call_count++];call.action=action;call.argument_count=unsigned(args.size());
    unsigned i=0;for(auto value:args)call.arguments[i++]=value;
    if(services.action)services.action(call);
  }
  void factory(std::uint32_t controller,std::uint32_t callback) {
    // Literal60C851: WORD0=0,WORD2=controller, +4=5,+8=4,+C=0,
    // +10/+14/+18=NONE,+1C=callback. Original truncates controller to16 bits.
    emit(A::screen_factory,{(controller&65535u)<<16,5,4,0,~0u,~0u,~0u,callback});
  }
  std::uint32_t classify_screen(std::uint32_t screen) {
    if(byte(q(Q::world_ready))&&!byte(q(Q::world_is_mainmenu)))return 3;
    const auto protocol=q(Q::protocol); // Original615EA9 precedes table admission.
    switch(screen) { // Literal616E04/616DE8 table, including default0.
      case 13:case 248:return 1;
      case 14:case 23:case 24:case 25:case 168:case 169:case 170:
      case 172:case 173:case 175:case 176:case 191:case 220:return 2;
      case 15:case 17:case 60:case 206:case 207:case 208:return protocol!=~0u?2:0;
      case 16:return 5;
      case 210:return 7;
      default:return 0;
    }
  }
  std::uint32_t classify_lifecycle() {
    if(!byte(q(Q::network_enabled)))return 0;
    const auto kind=byte(q(Q::lifecycle_enabled))?q(Q::lifecycle_kind):0;
    switch(kind) {
      case 1:case 2:return 2;
      case 3:return 3;
      case 4:return 5;
      case 5:return 6;
      case 6:return 7;
      default:return 0;
    }
  }
  void close_session(){emit(A::session_close,{0});}
  void observe_change() {
    const auto old=result.previous_ui_state,now=result.current_ui_state,screen=result.screen;
    if(old==3&&byte(q(Q::closing))&&byte(q(Q::world_gate))&&!byte(q(Q::world_gate_b))) {
      if(screen!=231)factory(q(Q::controller),0x646F94);
      return;
    }
    if(!now||now==old) {
      if(screen==9||screen==292)return;
      if(byte(q(Q::loading))||byte(q(Q::loading_secondary)))return;
      if(!byte(q(Q::world_ready))||!byte(q(Q::world_is_mainmenu)))return;
      if((q(Q::local_controller_count)&65535u)!=0)return;
      emit(A::other_mainmenu_request);return;
    }
    if(old==3) {
      if(now==4)return;
      if(now==5)emit(A::mainmenu_request,{7});
      else emit(A::other_mainmenu_request);
      return;
    }
    if(now==3){close_session();return;}
    if(byte(q(Q::task_gate_a))||byte(q(Q::task_gate_b))) {
      const auto current=q(Q::task_state);
      if(state.retained_task_state!=current) {
        state.retained_task_state=q(Q::task_state);
        emit(A::cache_task_state,{state.retained_task_state});return;
      }
    }
    if(byte(q(Q::task_blocks)))return;
    switch(now) {
      case 1:if(old!=1)close_session();return;
      case 2: {
        const bool permitted=old==0||old==1||old==5;
        // Original queries count even when old-state admission is false.
        const auto count=q(Q::local_controller_count)&65535u;
        const bool positive=count!=0&&count<0x8000;
        if(!permitted||!positive){close_session();return;}
        const auto protocol=q(Q::protocol);
        if(protocol==4||protocol==5||protocol==6) {
          emit(A::ui_channel,{4,4,2});factory(~0u,0x6467E9);
        } else factory(~0u,0x646752);
        return;
      }
      case 4:return;
      case 5: {
        const auto protocol=q(Q::protocol);
        if(protocol==4||protocol==5||protocol==6)emit(A::ui_channel_close,{4,4});
        emit(A::pcr16_request);return;
      }
      case 6: {
        const auto task=q(Q::task_state);
        if(std::uint32_t(task-7)<=3||screen==204||screen==210)return;
        factory(q(Q::controller),0x642DA8);return;
      }
      case 7:
        if(screen!=210&&screen!=247)factory(q(Q::controller),0x646CEF);
        return;
      default:close_session();return;
    }
  }
};
} // namespace
VistaMenuReturnObservation vista_menu_return_observe(VistaMenuReturnState& state,const VistaMenuReturnServices& services) {
  return Observer(state,services).run();
}
} // namespace halo2

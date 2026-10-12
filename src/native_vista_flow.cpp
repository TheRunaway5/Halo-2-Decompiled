#include "halo2/native_vista_flow.hpp"
#include "halo2/vista_session_selection.hpp"
#include "halo2/vista_session_map_change.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/native_vista_pcr.hpp"
#include "halo2/vista_pcr_owner_state.hpp"
#include "halo2/vista_pcr_achievement.hpp"
#include "halo2/vista_gameplay_fade.hpp"
#include "halo2/vista_lobby_input.hpp"
#include "halo2/vista_lobby_leave_return.hpp"
#include "halo2/vista_lobby_leave_task.hpp"
#include "halo2/vista_session_disposal.hpp"
#include "halo2/vista_network_browser.hpp"
#include "halo2/vista_network_browser_back.hpp"
#include "halo2/native_vista_announcer.hpp"
#include "halo2/vista_player_birth.hpp"
#include "halo2/vista_slayer_publication.hpp"
#include "halo2/menu_music.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <ctime>
#if defined(__i386__) || defined(__x86_64__) || defined(_M_X64) || defined(_M_IX86)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t get(std::span<const std::byte> b,std::size_t at,unsigned n=4) {
  if(at>b.size() || n>b.size()-at)throw FormatError("Native Vista owner read outside retained storage");
  std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(i*8);return v;
}
std::u16string wide(std::string_view s) {
  std::u16string out;
  for(std::size_t i=0;i<s.size();) {
    auto c=std::uint32_t(static_cast<unsigned char>(s[i++]));unsigned following{};
    if(c>=0xc0 && c<0xe0){c&=31;following=1;}
    else if(c>=0xe0 && c<0xf0){c&=15;following=2;}
    else if(c>=0xf0 && c<0xf8){c&=7;following=3;}
    else if(c>=0x80)throw FormatError("Invalid source locale UTF8");
    while(following--) {
      if(i==s.size() || (static_cast<unsigned char>(s[i])&0xc0)!=0x80)
        throw FormatError("Invalid source locale UTF8 continuation");
      c=(c<<6)|(static_cast<unsigned char>(s[i++])&63);
    }
    if(c>0x10ffff)throw FormatError("Invalid source locale scalar");
    if(c<=0xffff)out+=char16_t(c);
    else {c-=0x10000;out+=char16_t(0xd800+(c>>10));out+=char16_t(0xdc00+(c&1023));}
  }
  return out;
}
std::uint32_t fp_context() {
#if defined(__i386__) || defined(__x86_64__) || defined(_M_X64) || defined(_M_IX86)
  return _mm_getcsr();
#else
  throw FormatError("Native Vista flow requires an actual supported SSE context");
#endif
}
VistaLoadingFloatingPoint floating_point() {
  VistaLoadingFloatingPoint fp{};fp.mxcsr=fp_context();
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("fnstcw %0":"=m"(fp.x87_control_word));
#else
  throw FormatError("Native Vista presentation requires the actual supported x87 context");
#endif
  return fp;
}
std::uint32_t lifecycle_context(bool active,std::uint32_t kind) {
  // Literal6154D8/615520, independently executed by the readiness oracle.
  constexpr std::array<std::uint32_t,7> contexts{0,2,2,3,5,6,7};
  return active && kind<contexts.size()?contexts[kind]:0;
}
VistaSessionSelectedMap selected_map(std::span<const std::byte> b) {
  VistaSessionSelectedMap s;s.type=get(b,0x4cb8);s.id=std::bit_cast<std::int32_t>(get(b,0x4dd8));
  for(unsigned i=0;i<s.path.size();++i)s.path[i]=std::uint16_t(get(b,0x4cbc+i*2,2));
  return s;
}
}
struct NativeVistaOfflineFlow::Impl {
  std::array<std::byte,0x1188> options{};
  std::vector<VistaPcrRgb> palette,teams;
  VistaPcrAppearanceAssets appearance;
  VistaSessionMapChangeServices map_change;
  const MultiplayerMatch* match{};
  MultiplayerOptions runtime_options;
  std::function<void()> resource_load_begin;
  std::shared_ptr<MultiplayerPreparedWorld> world;
  std::unique_ptr<NativeVistaPcr> pcr;
  std::shared_ptr<VistaPcrScreenOwner> screen;
  std::filesystem::path selected_file;
  std::vector<SourcePlayerConfiguration> selected_profiles;
  std::optional<UiLobbyMap> selected_map;
  VistaLobbyCountdownState countdown;
  VistaLobbyCountdownServices countdown_services;
  std::function<void(FrontendUiSound)> sound;
  // Actual offline lifecycle connection-handler6 retains BSS task-state0;
  // this owner never starts the remote connect task.
  std::uint32_t user_context{},medal_count{};
  NativeVistaPcrPublicationState publication;
  VistaPcrOfflineAchievementOwner achievements;
  VistaGameplayFadeState gameplay_fade;
  std::unique_ptr<NativeVistaAnnouncer> announcer;
  std::array<std::optional<VistaPlayerBirthConsumed>,multiplayer_capacity> player_births;
  std::array<bool,multiplayer_capacity> activated{};
  std::optional<VistaLocalActivationIdentity> activation_identity;
  std::vector<std::array<std::uint32_t,2>> activation_notifications;
  unsigned player_activation_count{};
  VistaLobbyLeaveReturnState leave_return;
  std::shared_ptr<VistaLeaveTaskScreenOwner> leave_task;
  VistaBrowserMainmenuReturnState browser_mainmenu;
  // Fresh PE86EE9C header is zero. This native call set has no optional
  // report-dialog producer; it retains the proved factory clear operations.
  std::array<std::byte,0x48> mainmenu_factory_header{};
  std::uint8_t loader_failure_notification{}; // Successful synchronous loads.
  bool ui_active=true,ui_disabled{};
};
NativeVistaOfflineFlow::NativeVistaOfflineFlow(FrontendSession& frontend,std::filesystem::path maps,
    const MultiplayerOptions& runtime_options,std::function<void()> resource_load_begin)
  :impl_(std::make_unique<Impl>()),frontend_(frontend),maps_(std::move(maps)),session_(NativeVistaSessionConfiguration{}),
   manager_{0,0,0,0,0,0,manager_payload_} {
  if(!frontend.has_content() || !frontend.content().vista)
    throw FormatError("Native Vista flow requires the actual imported PC menu");
  impl_->runtime_options=runtime_options;
  impl_->resource_load_begin=std::move(resource_load_begin);
  impl_->runtime_options.prepared_world.reset();
  for(auto& h:handlers_) {
    h.update=[](auto&)->std::uint8_t{throw FormatError("Unbound Vista lifecycle handler reached");};
    h.enter=[](auto&,auto,auto,auto){throw FormatError("Unbound Vista lifecycle enter reached");};
    h.exit=[](auto&,auto){throw FormatError("Unbound Vista lifecycle exit reached");};
  }
  auto& s=services_.transition;
  s.phase.network_clock=[this]{return session_.network_clock();};
  s.phase.send=[](const auto&){throw FormatError("Offline native flow reached remote phase transport");};
  s.termination_reason=[]()->std::uint32_t{throw FormatError("Offline native flow reached unbound termination owner");};
  s.notify_termination=[](unsigned){throw FormatError("Offline native flow reached remote termination notification");};
  s.remote_close=[](unsigned){throw FormatError("Offline native flow reached remote session close");};
  s.closing_flags=[] {return std::array<std::uint8_t,2>{};};
  s.game_over_complete=[this]()->std::uint8_t {
    if(!impl_->world)throw FormatError("Vista completion queried without the actual match owner");
    return impl_->world->match.state().game_over_complete();
  };
  s.world_ready=[this]()->std::uint8_t{return world_ready_;};
  s.force_end=[] {throw FormatError("Offline Vista force-end command is not bound");};
  // Offline source5AE8D1 has no active network-game close owner. World resource
  // teardown belongs to the later real UI-observer mainmenu-load request.
  s.network_close=[] {};
  s.world_options=[this]{return VistaSessionWorldOptions{{get(impl_->options,0x10),get(impl_->options,0x14)},
      {get(impl_->options,0x80),get(impl_->options,0x84)}};};
  s.map_change=[this](auto& state) {
    const auto result=vista_session_apply_map_change(state,session_.bank(),impl_->map_change);
    for(unsigned i=0;i<result.call_count;++i) {
      const auto& call=result.calls[i];
      if(const auto context=vista_session_map_change_presence(call,impl_->options))
        session_.set_user_context(context->context,context->value);
      switch(call.intent) {
      case VistaSessionMapChangeIntent::phase_write:
        vista_session_phase_write(session_.bank(),call.argument,services_.transition.phase);break;
      case VistaSessionMapChangeIntent::remote_close:
        throw FormatError("Native Vista map rejection reached unbound remote close");
      case VistaSessionMapChangeIntent::select_map:
        if(!impl_->selected_map || std::uint32_t(impl_->selected_map->map_id)!=call.argument)
          throw FormatError("Loaded Vista map differs from actual native UI selection");
        break;
      case VistaSessionMapChangeIntent::select_variant:
        if(!std::equal(variant_.bytes.begin(),variant_.bytes.end(),impl_->options.begin()+0x2a0))
          throw FormatError("Loaded Vista variant differs from actual native UI selection");
        break;
      case VistaSessionMapChangeIntent::ui_reason:impl_->user_context=call.argument;break;
      case VistaSessionMapChangeIntent::select_scenario:
        throw FormatError("Native Vista flow does not bind the alternate campaign selector");
      }
    }
  };
  s.postgame_external=[this](unsigned value){impl_->user_context=value;session_.set_user_context(0x8001,value);};
  services_.pregame_external=s.postgame_external;
  services_.null_network_close=[this](unsigned slot,unsigned value){session_.request_connection_mode(slot,value);};
  services_.seed=[this]{return session_.random_session_seed();};
  services_.random_identity=[this]{return session_.random_session_identity();};
  services_.network_busy=[this]{return session_.network_busy();};
  services_.countdown_send=[](const auto&){throw FormatError("Offline native flow reached remote countdown transport");};
  vista_session_bind_start_handlers(handlers_,session_.bank(),start_,services_);
  vista_session_bind_transition_handlers(handlers_,session_.bank(),in_game_,services_.transition);
  //40911B is synchronous. Construct actual Cache/content/Match/local players
  // before publishing current options, matching449E4C's initialization order.
  impl_->map_change.request_load=[this](auto options)->std::uint8_t {
    if(!launch_ || options.size()!=impl_->options.size() || load_requested_)
      throw FormatError("Native Vista world task has no matching launch owner");
    auto request=impl_->runtime_options;
    request.map=launch_->map;request.local_players=launch_->local_players;
    request.variant=launch_->variant;request.selected_profiles=launch_->selected_profiles;
    request.selected_teams=launch_->selected_teams;
    request.session_random_seed=get(options,0x18);
    //449E4C copies initialization options before449EBF creates players. This
    // immutable initializer belongs to the actual synchronous native load;
    // fulfilled-world/current-options readiness is published only on return.
    auto initializing=std::make_shared<VistaWorldOptions>();
    std::copy(options.begin(),options.end(),initializing->begin());
    const std::shared_ptr<const VistaWorldOptions> initialized_options=initializing;
    std::vector<NativeVistaSessionPlayer> actual_players;
    for(unsigned controller=0;controller<request.local_players;++controller) {
      const auto actual=session_.player_for_controller(controller);
      if(!actual)throw FormatError("Vista world birth lost its genuine selected session player");
      actual_players.push_back(*actual);
    }
    const auto identity=vista_player_local_identity_from_initialized_options(*initialized_options,
        std::span<const std::byte,6>(session_.bank().subspan(0x92,6)));
    impl_->player_births={};impl_->activated={};impl_->activation_identity=identity;
    impl_->activation_notifications.clear();impl_->player_activation_count=0;
    request.defer_initial_player_spawn=true;
    request.player_birth_owner=[this,initialized_options,actual_players](MultiplayerPlayer& player,unsigned slot) {
      const auto actual=std::find_if(actual_players.begin(),actual_players.end(),[&](const auto& p){return p.player==slot;});
      if(slot>=impl_->player_births.size() || impl_->player_births[slot] || !player.connected ||
          player.source_current_object!=null_datum || !player.source_profile_request ||
          actual==actual_players.end() || actual->peer)
        throw FormatError("Vista player birth lacks its actual allocated local native owner");
      const auto birth=vista_player_birth_from_initialized_options(*initialized_options,slot,player.source_datum);
      if(birth.identity!=actual->identity || birth.configuration!=actual->final_payload ||
          birth.creation_user!=actual->user || birth.creation_controller!=actual->controller ||
          native_vista_selected_profile_payload(*player.source_profile_request)!=actual->requested_payload ||
          birth.flags!=8)
        throw FormatError("Vista initialized player row differs from actual selected identity or profile");
      // The Xbox constructor is independently executed; this is its native
      // new/non-left flag producer, not a cast of a PC player record.
      player.source_flags=native_player_birth_flags(1,0);
      impl_->player_births[slot]=birth;
    };
    request.pre_player_update_owner=[this](MultiplayerMatch& match){player_activation_tick(match);};
    if(impl_->resource_load_begin)impl_->resource_load_begin();
    auto initialized_world=std::make_shared<MultiplayerPreparedWorld>(request);
    std::copy(options.begin(),options.end(),impl_->options.begin());
    impl_->world=std::move(initialized_world);
    load_requested_=true;world_ready_=true;world_mainmenu_=false;return 1;
  };
  impl_->map_change.loaded_options=[this]{return std::span<const std::byte>(impl_->options);};
  impl_->map_change.pause=[this](bool frozen){session_.freeze_network_clock(frozen);};
  // The native gameplay engine currently runs30Hz. Vista428707 instead returns
  //60. This configured-service difference is explicit, not original parity.
  impl_->map_change.configured_tick_rate=[] {return 30;};
  impl_->map_change.constructor_byte=[]()->std::uint8_t{return 0;}; // Fresh PE882D40.
  impl_->map_change.random_identity=[this] {
    const auto words=session_.random_session_identity();std::array<std::byte,8> bytes{};
    for(unsigned i=0;i<8;++i)bytes[i]=std::byte(words[i/4]>>((i%4)*8));
    return bytes;
  };
  auto& c=impl_->countdown_services;
  c.ui_state=[this]{return vista_frontend_readiness(readiness(),false).state;};
  c.lifecycle_active=[this]()->std::uint8_t{return begun_;};
  c.lifecycle_slot=[this]{return manager_.current_kind;};
  c.session_stage=[this]{return countdown();};
  c.ui_sound=[this](std::uint32_t slot) {
    if(!impl_->sound)throw FormatError("Native Vista countdown has no active frontend sound owner");
    impl_->sound(FrontendUiSound(slot));
  };
  c.ui_clock=[this]{return ui_clock_;};
  c.duration_context=[this] {(void)get(session_.bank(),0x1258);}; // Actual ignored membership query.
  c.controller_user=[this](std::int32_t controller) {
    return std::bit_cast<std::int32_t>(vista_controller_local_user(session_.controllers(),std::uint32_t(controller)));
  };
  c.host_permission=[this]()->std::uint8_t{return vista_session_pcr_host(session_.bank(),begun_);};
  c.write=[this](std::uint8_t running,std::int32_t stage,std::uint8_t reason,std::int32_t user) {
    return std::uint8_t(vista_lobby_countdown_write_request(session_.bank(),session_.local_users(),begun_,
        running,stage,reason,user,0,[this](const auto& write) {
          return std::uint8_t(vista_session_countdown_write(session_.bank(),write,services_));
        }));
  };
  frontend_.state().set_lobby_session_action([this](auto action){return lobby_action(action);});
}
NativeVistaOfflineFlow::~NativeVistaOfflineFlow(){frontend_.state().set_lobby_session_action({});}
bool NativeVistaOfflineFlow::lobby_action(FrontendLobbySessionAction action) {
  if(action==FrontendLobbySessionAction::MainMenuNetwork)return false;
  if(action==FrontendLobbySessionAction::BrowserBack) {
    auto& state=frontend_.state();
    if(!begun_ || manager_.current_kind || get(session_.bank(),0x73a0) ||
        !world_ready_ || !world_mainmenu_ || impl_->leave_task || launch_ ||
        load_requested_ || menu_requested_ || state.frame().screen!=vista_menu(13))
      throw FormatError("Vista browser Back lacks its actual disposed mainmenu owner");
    VistaBrowserBackServices back;
    back.channel=[] {return 5u;};back.window=[] {return 4u;};
    back.screen_present=[&state](auto channel,auto window,auto screen)->std::uint8_t {
      if(channel!=5 || window!=4)throw FormatError("Vista browser Back queried an unbound channel");
      return state.vista_session_screen()==vista_menu(screen);
    };
    back.mainmenu_reason=[this,&state](std::uint32_t reason) {
      if(reason!=2)throw FormatError("Vista browser Back reached a different mainmenu reason");
      VistaBrowserMainmenuReturnServices menu;
      menu.initialize_profiles=[this]{session_.refresh_selected_profile_catalog();};
      menu.ui_clock=[this]{return frontend_.ui_clock();};
      menu.controller_count=[this]{return vista_lobby_controller_count(session_.controllers());};
      menu.controller_mask=[this]{return vista_lobby_controller_mask(session_.controllers());};
      menu.create_screen=[this,&state](const auto& descriptor) {
        if(descriptor[7]==0x63f6b7u) {
          const std::array<std::uint32_t,8> expected{0xffff0004u,5,4,0,
              0xffffffffu,0xffffffffu,0xffffffffu,0x63f6b7u};
          if(descriptor!=expected)throw FormatError("Vista multi-user title factory descriptor changed");
          // Original63F57F constructs screen9. Native presentation continues
          // to use its retained Xbox title artwork, with both profiles intact.
          state.open_local_title();
          std::cout<<"Vista lifecycle: original multi-user browser Back; newly allocated native title9\n";
          return;
        }
        const auto mask=vista_lobby_controller_mask(session_.controllers())&0xffffu;
        const std::array<std::uint32_t,8> expected{(mask<<16)|4u,5,4,0,
            0xffffffffu,0xffffffffu,0xffffffffu,0x40b7b9};
        if(descriptor!=expected)throw FormatError("Vista PC6 factory differs from its actual controller owner");
        // Actual40B7B9 publishes presence0 before registration, then clears
        // its menu/input state. The native application starts with Xbox6;
        // adapt this factory to a fresh instance of that same presentation.
        state.ui_clock(frontend_.ui_clock());
        state.open_native_mainmenu([this]{session_.set_user_context(0x8001,0);});
        if(impl_->mainmenu_factory_header[3]!=std::byte{})
          throw FormatError("Native PC6 factory reached an unbound optional report dialog");
        impl_->mainmenu_factory_header[1]=std::byte{};
        start_.pregame_bank.fill(std::byte{});
        impl_->loader_failure_notification=0;
        std::cout<<"Vista lifecycle: original browser Back; newly allocated native mainmenu6\n";
      };
      vista_network_browser_mainmenu_return_local(impl_->browser_mainmenu,menu);
    };
    //61A238 constructs the nonmodal browser (+1F00=0). A modal browser has
    // a distinct close/base-input owner and is not admitted by this adapter.
    return vista_network_browser_back(0,back)!=0;
  }
  if(action==FrontendLobbySessionAction::Create) {
    auto& state=frontend_.state();
    if(!begun_ || manager_.current_kind || get(session_.bank(),0x73a0) ||
        !world_ready_ || !world_mainmenu_ || impl_->leave_task || launch_ ||
        load_requested_ || menu_requested_ || state.frame().screen!=vista_menu(13))
      throw FormatError("Vista browser Create lacks its actual disposed local session owner");
    const auto request=state.selected_multiplayer_configuration();
    const auto lobby=state.lobby_view();
    if(!supports(request) || !lobby || !lobby->map)
      throw FormatError("Vista browser Create requires the actual admitted selected configuration");
    VistaBrowserCreateServices create;
    create.disconnect=[this](bool immediate) {
      if(!immediate)throw FormatError("Native Vista Create reached remote disconnect");
      (void)session_.dispose_local();
    };
    create.clear_report_counts=[this] {
      // These retained reports own the native projections of86EEE8/86EEF0.
      // Clearing their complete lifetime cannot leave stale report rows alive.
      impl_->screen.reset();impl_->pcr.reset();
      report_results_.reset();report_source_.reset();selected_row_=0;native_slot_.reset();native_slots_.clear();
    };
    create.birth_local=[this]{return session_.birth_local().accepted;};
    create.configure_selected=[this,&request,&lobby](std::uint32_t mode) {
      if(mode!=2)throw FormatError("Vista browser Create reached an unbound configuration mode");
      configure_lobby(request,*lobby->map,true);
    };
    if(!vista_network_browser_create_local(create))
      throw FormatError("Original Vista browser local session creation failed");
    //6176B0 owns the subsequent lobby factory in update_frontend. Creating
    // the genuine session does not start its countdown or open a UI here.
    return true;
  }
  // After Leave disposes the source session, a lobby reached from the native
  // main menu must supply a new Start request just like the initial lobby.
  if(!begun_ || (manager_.current_kind==0 && !get(session_.bank(),0x73a0)))return false;
  auto& state=frontend_.state();
  if(!world_mainmenu_ || !world_ready_ || load_requested_ || manager_.current_kind>2)
    throw FormatError("Vista lobby input lacks its actual pregame mainmenu owner");
  VistaLobbyInputServices s;
  s.pregame=[this]()->std::uint8_t{return vista_lobby_event_admitted(session_.bank(),begun_);};
  // This hook is reached only for the active native screen14; a native modal
  // is a separate top frame and cannot dispatch these lobby actions.
  s.channel_in_use=[&state](unsigned channel)->std::uint8_t {
    if(channel!=1)throw FormatError("Vista lobby queried an unbound UI channel");
    return state.frame().screen!=14;
  };
  s.signed_in=[this](std::int32_t controller)->std::uint8_t {
    return vista_lobby_controller_signed_in(session_.controllers(),controller);
  };
  s.team_edit_active=[&state](std::int32_t controller)->std::uint8_t {
    if(controller!=0)throw FormatError("Vista lobby queried an unbound controller");
    const auto lobby=state.lobby_view();
    if(!lobby)throw FormatError("Vista lobby input has no actual UI owner");
    return lobby->team_edit.valid;
  };
  s.host_permission=impl_->countdown_services.host_permission;
  s.ready=[this]()->std::uint8_t{return vista_lobby_setup_ready(session_.bank(),begun_);};
  s.countdown_active=[this]()->std::uint8_t{return vista_lobby_countdown_active(impl_->countdown_services);};
  s.start_action=[this](auto controller){(void)vista_lobby_countdown_start_action(controller,impl_->countdown_services);};
  s.cancel_action=[this](auto controller)->std::uint8_t {
    const bool accepted=vista_lobby_cancel_action(controller,impl_->countdown_services);
    if(accepted && countdown()==-1) {
      launch_.reset();impl_->countdown={};
      frontend_.state().set_session_countdown({});
      pump(); // Let the actual lifecycle observe its cleared countdown.
    }
    return accepted;
  };
  s.finish_team_edit=[](auto){throw FormatError("FFA lobby reached an unbound team-edit owner");};
  s.settings_change=[this](auto value){
    (void)vista_lobby_settings_change_local(session_.bank(),begun_,value);
  };
  const auto source_action=action==FrontendLobbySessionAction::Start?VistaLobbyInputAction::start:
      action==FrontendLobbySessionAction::Setup?VistaLobbyInputAction::setup:
      action==FrontendLobbySessionAction::Back?VistaLobbyInputAction::back:VistaLobbyInputAction::auxiliary;
  const auto route=vista_lobby_input(source_action,0,s);
  switch(route) {
  case VistaLobbyInputRoute::consumed:return true;
  case VistaLobbyInputRoute::base_input:case VistaLobbyInputRoute::start_and_base_input:
    state.set_session_countdown(countdown());return false;
  case VistaLobbyInputRoute::setup:case VistaLobbyInputRoute::team_edit:return false;
  case VistaLobbyInputRoute::leave: {
    VistaLobbyLeaveServices leave;
    leave.peer_count=[this]{return vista_lobby_peer_count(session_.bank(),begun_);};
    leave.browser_reset=[this]{session_.reset_local_browser();};
    leave.disconnect=[this](bool immediate) {
      if(!immediate)throw FormatError("Sole native Vista peer reached remote disconnect");
      (void)session_.dispose_local();
    };
    leave.create_task=[this,&state](std::int32_t controller) {
      if(impl_->leave_task)throw FormatError("Native Vista Leave already owns a progress task");
      impl_->leave_task=std::make_shared<VistaLeaveTaskScreenOwner>();
      state.open_vista_leave_task(std::uint32_t(controller),impl_->leave_task);
    };
    if(!vista_lobby_leave_single(0,leave))
      throw FormatError("Native Vista sole-peer Leave lost its genuine session owner");
    return true;
  }
  case VistaLobbyInputRoute::unsigned_controller:
    throw FormatError("Actual signed-in Vista lobby lost its controller profile");
  }
  throw FormatError("Unknown Vista lobby input route");
}
bool NativeVistaOfflineFlow::supports(const FrontendLaunch& launch) {
  if(!launch.multiplayer || launch.campaign || !launch.local_players || launch.local_players>4 || launch.host_port ||
      launch.online_match || launch.selected_profiles.size()!=launch.local_players || !launch.selected_teams.empty())return false;
  try {
    const auto variant=source_game_variant(launch.variant);
    if(source_game_variant_get(variant,91)!=0)return false;
    (void)vista_pcr_slayer_variant(variant);
    for(const auto& profile:launch.selected_profiles)(void)native_vista_selected_profile_payload(profile);
    return true;
  } catch(const std::exception&){return false;}
}
void NativeVistaOfflineFlow::configure_lobby(const FrontendLaunch& request,const UiLobbyMap& map,bool browser_create) {
  if(!supports(request) || launch_ || load_requested_ || menu_requested_ ||
      (begun_ && manager_.current_kind>1))
    throw FormatError("Native Vista offline start requires a fresh admitted lobby request");
  for(unsigned controller=0;controller<4;++controller) {
    if(controller<request.local_players) {
      if(!session_.signed_in(controller) && !session_.sign_in_offline(controller))
        throw FormatError("The original offline sign-in returned failure");
      session_.publish_profile(controller,native_vista_selected_profile_payload(request.selected_profiles[controller]));
    } else session_.remove_profile(controller);
  }
  if(get(session_.bank(),0x73a0)==0 && !session_.birth_local().accepted)
    throw FormatError("Native Vista local session birth failed");
  if(browser_create)(void)vista_network_browser_preflight_local(session_.bank(),begun_,1);
  variant_=vista_pcr_slayer_variant(source_game_variant(request.variant));
  //617138(2)->6169C5 supplies campaign ID NONE for the selected multiplayer
  // map. Passing cache type2 here instead makes615EA9 classify this as co-op.
  if(!vista_session_select_map(session_.bank(),null_datum,std::uint32_t(map.map_id),wide(map.scenario_path)) ||
      !vista_session_select_variant(session_.bank(),&variant_.bytes))
    throw FormatError("Original Vista session rejected the selected map or variant");
  impl_->selected_file=request.map;
  if(impl_->selected_file.is_relative())impl_->selected_file=maps_/impl_->selected_file;
  impl_->selected_profiles=request.selected_profiles;
  impl_->selected_map=map;
  begun_=true;world_ready_=true;world_mainmenu_=true;pump();
  for(unsigned controller=0;controller<request.local_players;++controller)
    if(!session_.player_for_controller(controller))throw FormatError("Actual offline reconciliation has not inserted the selected player");
}
void NativeVistaOfflineFlow::begin(const FrontendLaunch& request,const UiLobbyMap& map) {
  configure_lobby(request,map);
  launch_=request;launch_->map=impl_->selected_file;
  report_results_.reset();report_source_.reset();native_slot_.reset();native_slots_.clear();
  impl_->screen.reset();impl_->pcr.reset();
  if(!vista_lobby_countdown_start_action(0,impl_->countdown_services))
    throw FormatError("Original Vista countdown admission rejected the native lobby");
  std::cout<<"Vista lifecycle: actual offline lobby countdown started\n";
}
VistaFrontendReadinessQueries NativeVistaOfflineFlow::readiness() const {
  VistaFrontendReadinessQueries q;
  q.lifecycle_active=begun_;q.lifecycle_slot=manager_.current_kind;q.current_session=begun_;
  q.session_state=get(session_.bank(),0x73a0);
  q.mode=vista_session_frontend_protocol(session_.bank(),begun_,
      std::bit_cast<std::int32_t>(manager_.current_kind));
  q.map_status=std::uint32_t(map_readiness_.status);
  q.map_progress=map_readiness_.progress;q.local_map_status=map_readiness_.status;
  q.variant=VistaFrontendVariantView{std::bit_cast<std::int32_t>(get(variant_.bytes,0x44)),
      std::uint8_t(get(variant_.bytes,0x48,1))};q.countdown=countdown();
  q.host_permission=vista_session_pcr_host(session_.bank(),begun_);q.mxcsr=fp_context();return q;
}
void NativeVistaOfflineFlow::network_update() {
  if(!begun_)return;
  const auto available=native_vista_available_map_readiness(impl_->selected_file,maps_/"mainmenu.map");
  if(!available)throw FormatError("Original Vista available-map branch requires actual map and mainmenu files");
  VistaSessionMapReadinessQueries q;q.lifecycle_slot=manager_.current_kind;
  q.session=VistaSessionMapReadinessSession{get(session_.bank(),0x73a0),selected_map(session_.bank())};
  q.loaded_world_valid=world_ready_;
  if(world_ready_) {
    if(world_mainmenu_) {
      //408786/408EFE keep448D6F's campaign ID NONE at options+80;
      // mainmenu's world-kind3 is options+0, outside this selected-map view.
      VistaSessionSelectedMap menu;menu.type=null_datum;menu.id=-1;
      const auto path=u"scenarios\\ui\\mainmenu\\mainmenu";
      std::copy_n(path,std::char_traits<char16_t>::length(path),menu.path.begin());q.loaded_options=menu;
    } else q.loaded_options=q.session->selected;
  }
  q.map_query=available->query;q.map_progress_bits=available->progress_bits;q.mxcsr=fp_context();
  vista_session_map_readiness(map_readiness_,q);
  session_.reconcile_offline(std::uint32_t(map_readiness_.status),std::uint32_t(map_readiness_.progress),
      !world_ready_?NativeVistaWorldKind::unloaded:
          world_mainmenu_?NativeVistaWorldKind::mainmenu:NativeVistaWorldKind::multiplayer);
}
void NativeVistaOfflineFlow::pump() {
  if(!begun_)return;
  vista_lifecycle_pump(manager_,handlers_,[this](auto&) {
    // Original5B53C7 samples the clock around network/reconciliation, then
    // unfreezes before the normal5AD83F lifecycle pump consumes live time.
    session_.freeze_network_clock(true);
    try {network_update();}catch(...) {session_.freeze_network_clock(false);throw;}
    session_.freeze_network_clock(false);
  });
}
std::int32_t NativeVistaOfflineFlow::countdown() const {
  const auto b=session_.bank();const auto state=get(b,0x73a0);
  return state>=3 && state<=8 && get(b,0x4c94,1)?std::bit_cast<std::int32_t>(get(b,0x4c98)):-1;
}
bool NativeVistaOfflineFlow::launch_ready() const noexcept{return load_requested_ && manager_.current_kind==3;}
FrontendLaunch NativeVistaOfflineFlow::take_launch() {
  if(!launch_ready() || !launch_)throw FormatError("Native Vista world request is not ready");
  auto launch=*launch_;launch_.reset();return launch;
}
void NativeVistaOfflineFlow::world_loaded(const MultiplayerMatch& match,std::span<const unsigned> slots) {
  if(!load_requested_ || manager_.current_kind!=3 || slots.size()!=impl_->selected_profiles.size() || slots.empty())
    throw FormatError("Native Vista world completion lacks its actual local-player task");
  const auto state=match.state();
  if(!impl_->world || &impl_->world->match!=&match)
    throw FormatError("Native Vista runtime did not reuse its actual selected-profile world");
  for(unsigned view=0;view<slots.size();++view) {
    const auto slot=slots[view];const auto actual=session_.player_for_controller(view);
    if(slot>=multiplayer_capacity || !actual || actual->player!=slot ||
        !state.players[slot].connected || state.players[slot].source_player==null_datum ||
        match.player(slot).source_profile_request!=impl_->selected_profiles[view])
      throw FormatError("Native Vista controller cannot bind to an absent or different native source player");
    const auto& birth=impl_->player_births[slot];
    if(!birth || birth->native_datum!=match.player(slot).source_datum ||
        match.player(slot).source_flags!=native_player_birth_flags(1,0) ||
        match.player(slot).source_current_object!=null_datum)
      throw FormatError("Vista world completion lost its unactivated genuine player birth");
  }
  native_slots_.assign(slots.begin(),slots.end());native_slot_=slots.front();
  load_requested_=false;world_ready_=true;world_mainmenu_=false;
  impl_->gameplay_fade={};
  std::uint32_t audio_crt=1;
  const auto audio_seed=source_secondary_random_seed(std::uint32_t(std::time(nullptr)),
      session_.raw_network_clock(),source_crt_random(audio_crt));
  // The selected PC scenario owns a separate Cache/sound namespace. The
  // native secondary stream/SDL adapter does not claim Windows mixer ordering.
  impl_->announcer=NativeVistaAnnouncer::open({frontend_.content().vista->source_fonts.parent_path(),
      impl_->selected_map->scenario_path,impl_->world->cache->original_data(),session_.language(),
      30,floating_point(),impl_->runtime_options.mute,audio_seed});
  // This actual returned local-view0 maps to the verified native player slot.
  for(unsigned view=0;view<slots.size();++view)vista_gameplay_fade_activate(impl_->gameplay_fade,view);
  impl_->world->match.set_game_engine_presentation_owner(
      [this](const MultiplayerMatch& actual){engine_presentation_tick(actual);});
  for(unsigned view=0;view<slots.size();++view)
    std::cout<<"Vista lifecycle: actual native world running; controller"<<view<<" source datum="
        <<state.players[slots[view]].source_player<<'\n';
}
unsigned NativeVistaOfflineFlow::player_activation_count() const noexcept {
  return impl_->player_activation_count;
}
void NativeVistaOfflineFlow::player_activation_tick(MultiplayerMatch& match) {
  if(!impl_->world || &impl_->world->match!=&match || !native_slot_ || !impl_->activation_identity || !impl_->announcer || world_mainmenu_)
    throw FormatError("Vista player activation requires its actual initialized local world");
  for(unsigned view=0;view<native_slots_.size();++view) {
  const auto slot=native_slots_[view];
  const auto& birth=impl_->player_births[slot];
  const auto& player=match.player(slot);
  if(!birth || !player.connected || player.source_datum!=birth->native_datum)
    throw FormatError("Vista player activation lost its genuine salted birth owner");
  const auto mode=std::uint8_t(get(impl_->options,4,1));
  if(!vista_player_activation_query_admitted(mode,player.source_flags))continue;
  const auto active=vista_player_local_activation_query(*impl_->activation_identity,*birth);
  const auto before=player.source_flags;
  const auto update=vista_player_activation_after_query(player.source_flags,active);
  //45339B writes the changed active bit before selected-engine dispatch.
  match.apply_source_player_activation(player.source_datum,before,update.flags);
  for(const auto call:update.call_intents()) {
    if(call==VistaPlayerActivationService::engine_activation) {
      if(impl_->activated[slot] || player.source_current_object!=null_datum)
        throw FormatError("Vista initial activation must precede the genuine first spawn");
      std::array<std::byte,vista_slayer_activation_storage_size> storage{};
      bool published=false;
      switch(match.source_engine()){
        case 2:published=vista_slayer_activation_publication(mode,player.source_datum,storage);break;
        case 4:published=source_king_activation_publication(mode,player.source_datum,storage);break;
        default:throw FormatError("Native activation has no recovered selected-engine publication");
      }
      if(!published)throw FormatError("Admitted native engine activation did not publish its event");
      const auto event=vista_announcer_event(std::span<const std::byte,34>(storage.data(),34));
      std::vector<VistaAnnouncerLocalPlayer> recipients;
      for(unsigned local=0;local<native_slots_.size();++local) {
        const auto& p=match.player(native_slots_[local]);
        recipients.push_back({p.source_datum,p.source_flags,p.source_membership,std::int16_t(local)});
      }
      (void)impl_->announcer->publish_player_activation(event,mode,match.state().finished,
          std::uint32_t(match.simulation().ticks()),recipients);
      impl_->activated[slot]=true;++impl_->player_activation_count;
    } else {
      // The actual5B6662(recipient,200h) intent is retained. The native
      // local-only adapter does not claim original replication/network delivery.
      impl_->activation_notifications.push_back({player.source_datum,0x200});
    }
  }
  }
}

void NativeVistaOfflineFlow::engine_presentation_tick(const MultiplayerMatch& match) {
  if(!impl_->world || &impl_->world->match!=&match || !native_slot_ || world_mainmenu_)
    throw FormatError("Vista engine presentation requires its actual local world");
  const auto& round=match.source_round_lifecycle();
  VistaGameplayFadeTick input;
  input.engine_present=true;input.playback=false;input.tick_rate=30;
  input.phase=round.phase;input.pending_phase=round.status;
  input.tick=std::uint32_t(match.simulation().ticks());input.round_start_tick=round.start_tick;
  for(unsigned view=0;view<native_slots_.size();++view) {
    const auto slot=native_slots_[view];const auto& p=match.player(slot);
    VistaGameplayFadePlayer player;player.player_present=p.connected;
    player.unit_present=p.source_current_object!=null_datum;
    player.respawn_countdown=p.respawn.countdown;
    player.active_player=[this,slot] {
      const auto& actual=impl_->world->match.player(slot);
      return source_player_respawn_allowed(impl_->world->rules.life_limit,actual.source_flags,
          actual.source_membership,actual.source_current_object,actual.source_lives_remaining);
    };
    input.users[view]=std::move(player);
  }
  if(!impl_->announcer)throw FormatError("Vista engine update lost its actual announcer owner");
  impl_->announcer->pre_world_tick(); //475924 precedes475B40 opacity update.
  vista_gameplay_fade_tick(impl_->gameplay_fade,input,floating_point());
}
void NativeVistaOfflineFlow::world_audio_update(float volume,
    const std::array<float,sound_class_count>& gains) {
  if(!impl_->announcer || !impl_->world || !world_ready_ || world_mainmenu_)
    throw FormatError("Vista announcer accounting lacks its actual native world");
  impl_->announcer->audio_update(volume,gains);
}
std::optional<std::uint32_t> NativeVistaOfflineFlow::world_view_black(unsigned view) {
  if(view>=native_slots_.size() || !impl_->world || !native_slot_ || !world_ready_ || world_mainmenu_)
    throw FormatError("Vista black-quad draw lacks its actual loaded local viewport");
  // Called by the runtime only after synchronous resident geometry/GPU setup.
  // There are no pending native geometry-cache preload requests or cubemap
  // capture in this admitted viewport. The Windows cache implementation is
  // not represented by a fabricated render-frame counter.
  VistaGameplayFadeRender input;
  input.engine_present=true;input.user=view;input.tick_rate=30;
  input.unresolved_world_geometry_count=0;input.cubemap_capture_in_progress=false;
  return vista_gameplay_fade_render(impl_->gameplay_fade,input,floating_point());
}
std::shared_ptr<MultiplayerPreparedWorld> NativeVistaOfflineFlow::prepared_world() const {
  if(!impl_->world)throw FormatError("Native Vista session has no initialized world");
  return impl_->world;
}
bool NativeVistaOfflineFlow::world_update(const MultiplayerMatch& match) {
  if(!impl_->world || &impl_->world->match!=&match || !native_slot_ || world_mainmenu_)
    throw FormatError("Vista update requires the actual active world owner");
  impl_->match=&match;
  std::vector<VistaAnnouncerLocalPlayer> recipients;
  for(unsigned view=0;view<native_slots_.size();++view) {
    const auto& player=match.player(native_slots_[view]);
    recipients.push_back({player.source_datum,player.source_flags,player.source_membership,std::int16_t(view)});
  }
  // Native SourceRoundContext owns raw_mode1. The recovered end owner tests
  // only!=4, consumes actual game-over, and publishes its one-shot after the
  // real world/phase/result update. Initial activation has its earlier owner.
  (void)impl_->announcer->post_world_tick(true,1,match.state().finished,
      std::uint32_t(match.simulation().ticks()),recipients);
  try {pump();observe_menu(nullptr);}catch(...) {impl_->match=nullptr;throw;}
  impl_->match=nullptr;
  if(!menu_requested_)return false;
  if(manager_.current_kind!=4 || !match.state().game_over_complete())
    throw FormatError("Native Vista mainmenu handoff has no completed postgame owner");
  if(!impl_->pcr)publish(match);
  return true;
}
void NativeVistaOfflineFlow::publish(const MultiplayerMatch& match) {
  if(!native_slot_ || manager_.current_kind!=4 || !impl_->world || &impl_->world->match!=&match)
    throw FormatError("Vista results publication lacks the actual completed world");
  const auto actual=match.state();
  if(!actual.finished || !actual.results_finalized || !actual.game_over_complete())
    throw FormatError("Vista report cannot consume unfinished native results");
  // This native retained owner is current only at this actual capture event;
  // full469DFD event/machine/header publication is deliberately not claimed.
  impl_->publication={1,std::uint8_t(actual.results_finalized),true};
  const auto fp=floating_point();
  NativeVistaPcrServices services;
  services.publication=[this]{return impl_->publication;};
  services.lifecycle_active=[this]()->std::uint8_t{return begun_;};
  services.lifecycle_kind=[this]{return manager_.current_kind;};
  // Execute the actual initialized offline manager admission. Its reached
  // type12 request is retained; no Live achievement child is dispatched.
  services.medal_achievement=[this](unsigned count) {
    impl_->achievements.request_medals(count,std::bit_cast<std::int32_t>(get(session_.identity().user_info,0xc)));
  };
  services.floating_point=fp;
  impl_->pcr=std::make_unique<NativeVistaPcr>(match,*native_slot_,session_,frontend_.content(),variant_,services);
  report_source_=impl_->pcr->source();report_results_=impl_->pcr->results();
  std::cout<<"Vista lifecycle: retained actual completed PCR; players="<<report_results_->player_count<<'\n';
}
void NativeVistaOfflineFlow::world_unloaded() {
  if(!menu_requested_ || !report_results_ || impl_->match)
    throw FormatError("Native Vista world teardown lacks its actual report handoff");
  impl_->world.reset();world_ready_=false;world_mainmenu_=false;native_slot_.reset();native_slots_.clear();
  impl_->announcer.reset();
}
void NativeVistaOfflineFlow::mainmenu_loaded() {
  if(!menu_requested_ || impl_->world || !report_results_)
    throw FormatError("Native Vista mainmenu completion lacks actual world teardown");
  world_ready_=true;world_mainmenu_=true;impl_->ui_active=true;
  observe_menu(&frontend_.state());
  if(frontend_.state().frame().screen!=vista_menu(16))
    throw FormatError("Original Vista observer did not request its actual PCR factory");
  menu_requested_=false;
  std::cout<<"Vista lifecycle: actual mainmenu resources initialized; new PCR16 factory\n";
}
std::shared_ptr<const VistaPcrScreenOwner> NativeVistaOfflineFlow::report() {
  if(!impl_->pcr || !report_results_)throw FormatError("Native Vista has no actual retained report");
  if(impl_->screen)return impl_->screen;
  auto owner=std::make_shared<VistaPcrScreenOwner>();
  owner->results=report_results_;owner->rows=impl_->pcr->rows(16);
  owner->appearance=[this](unsigned row) {
    const auto& state=frontend_.state();const auto pane=state.frame().vista_pcr?state.frame().context:1u;
    return impl_->pcr->appearance(row,16,pane);
  };
  owner->voice=[this](unsigned row) {
    return vista_pcr_offline_voice(*report_results_,row,0,session_.bank(),begun_,manager_.current_kind);
  };
  owner->input.host=[this]()->std::uint8_t{return vista_session_pcr_host(session_.bank(),begun_);};
  owner->input.details_available=[this]()->std::uint8_t{return session_.pcr_details_available(begun_);};
  owner->input.continue_session=[this] {
    if(!vista_session_pcr_continue(session_.bank(),manager_.current_kind,services_.transition.phase))
      throw FormatError("Actual Vista Done action rejected the local session");
  };
  owner->input.close=[](unsigned){throw FormatError("Automatic PCR16 reached review-screen close");};
  owner->input.dialog=[](const auto&){throw FormatError("Local Vista host reached remote leave-party dialog");};
  owner->select_row=[this](unsigned row){selected_row_=row;impl_->pcr->select_row(row);};
  owner->elapsed=[this](std::uint32_t creation){return session_.elapsed_network_clock(creation);};
  owner->floating_point=impl_->pcr->floating_point();owner->controller=0;
  owner->creation_clock=session_.network_clock();
  const auto focus=vista_pcr_initial_focus(*report_results_);
  owner->pane=focus.pane;owner->selected_row=focus.selected_row;
  selected_row_=focus.selected_row;impl_->pcr->select_row(selected_row_);
  impl_->screen=owner;return owner;
}
void NativeVistaOfflineFlow::observe_menu(MenuState* state) {
  using Q=VistaMenuReturnQuery;using A=VistaMenuReturnAction;
  VistaMenuReturnServices services;
  services.query=[this,state](Q query)->std::uint32_t {
    const auto tasks=session_.connection_task_queries();
    switch(query) {
    case Q::screen:return (state?state->vista_session_screen():frontend_.state().vista_session_screen())&0xffffu;
    case Q::world_ready:return world_ready_;
    case Q::world_is_mainmenu:return world_mainmenu_;
    case Q::protocol:return std::bit_cast<std::uint32_t>(vista_session_frontend_protocol(
        session_.bank(),begun_,std::bit_cast<std::int32_t>(manager_.current_kind)));
    case Q::network_enabled:case Q::lifecycle_enabled:return begun_;
    case Q::lifecycle_kind:return manager_.current_kind;
    case Q::ui_active:return state!=nullptr;
    case Q::ui_disabled:return impl_->ui_disabled;
    case Q::task_gate_a:return tasks.gate_a;
    case Q::task_gate_b:return tasks.gate_b;
    case Q::task_state:return tasks.state;
    case Q::task_blocks:return tasks.blocks;
    case Q::local_controller_count:return vista_lobby_controller_count(session_.controllers());
    case Q::closing:return 0; // No native offline disconnect event is pending.
    case Q::loading:case Q::loading_secondary:return load_requested_ || menu_requested_;
    case Q::controller:return vista_lobby_controller_mask(session_.controllers());
    case Q::world_gate:case Q::world_gate_b:
      throw FormatError("Native Vista offline observer reached an unbound disconnect world gate");
    }
    throw FormatError("Unbound native Vista menu-return query");
  };
  services.action=[this,state](const VistaMenuReturnIntent& intent) {
    switch(intent.action) {
    case A::mainmenu_request:
      if(intent.argument_count!=1 || intent.arguments[0]!=7 || world_mainmenu_)
        throw FormatError("Unexpected native Vista mainmenu request");
      menu_requested_=true;break;
    case A::pcr16_request:
      if(!state || !world_ready_ || !world_mainmenu_ || !report_results_)
        throw FormatError("Vista PCR factory requires its actual new mainmenu resources");
      state->open_vista_pcr(16,report());break;
    case A::screen_factory:
      if(!state || intent.arguments[7]!=0x646752 || manager_.current_kind!=1)
        throw FormatError("Unbound Vista return-screen factory");
      state->set_session_countdown({});state->open_vista_lobby();
      std::cout<<"Vista lifecycle: actual Done phase1; newly allocated lobby14\n";break;
    case A::set_ui_active:impl_->ui_active=intent.arguments[0]!=0;break;
    case A::cache_task_state:break;
    case A::observer_suffix: {
      VistaLobbyLeaveReturnServices suffix;
      suffix.protocol=[this] {return std::bit_cast<std::uint32_t>(vista_session_frontend_protocol(
          session_.bank(),begun_,std::bit_cast<std::int32_t>(manager_.current_kind)));};
      // Native factories execute synchronously; no deferred request occupies
      //4080BA's source service boundary in this admitted offline adapter.
      suffix.request_pending=[]()->std::uint8_t{return 0;};
      suffix.controller_mask=[this]{return vista_lobby_controller_mask(session_.controllers());};
      suffix.screen_present=[this,state](auto channel,auto window,auto screen)->std::uint8_t {
        if(channel!=5 || window!=4)throw FormatError("Vista Leave queried an unbound UI channel");
        return ((state?state->vista_session_screen():frontend_.state().vista_session_screen())&0xffffu)==screen;
      };
      suffix.create_screen=[this,state](const auto& descriptor) {
        if(!state || !impl_->leave_task || manager_.current_kind || descriptor[7]!=0x61a238 ||
            descriptor[0]!=((vista_lobby_controller_mask(session_.controllers())&0xffffu)<<16|7u))
          throw FormatError("Vista browser factory lacks its actual completed Leave owner");
        state->ui_clock(frontend_.ui_clock());
        state->open_vista_browser();
        std::cout<<"Vista lifecycle: original Leave return; newly allocated PC browser13\n";
      };
      (void)vista_lobby_leave_observe(impl_->leave_return,intent.arguments[0],
          intent.arguments[1],intent.arguments[2],suffix);
      break;
    }
    case A::session_close:case A::other_mainmenu_request:case A::ui_channel:case A::ui_channel_close:
      throw FormatError("Native Vista observer reached a session route without its genuine service owner");
    }
  };
  vista_menu_return_observe(menu_return_,services);
}
void NativeVistaOfflineFlow::update_frontend(MenuState& state,std::uint32_t ui_clock,
    const std::function<void(FrontendUiSound)>& sound) {
  ui_clock_=ui_clock;impl_->sound=sound;
  if(state.launch && supports(*state.launch)) {
    const auto lobby=state.lobby_view();
    if(!lobby || !lobby->map)throw FormatError("Native Vista Start has no actual selected map metadata");
    const auto request=*state.launch;state.launch.reset();begin(request,*lobby->map);
  }
  if(!begun_)return;
  pump();
  if(impl_->leave_task) {
    const auto task=impl_->leave_task;
    VistaLobbyLeaveTaskFrameServices services;
    services.lifecycle_context=[this]{return lifecycle_context(begun_,manager_.current_kind);};
    services.set_status=[&state,task](auto sid){state.set_vista_leave_task_status(task->instance,sid);};
    services.close=[this,&state,task](auto reason) {
      state.close_vista_leave_task(task->instance,reason);impl_->leave_task.reset();
    };
    // The native base widget's animations/rendering remain in MenuState and
    // FrontendPainter. This callback does not claim to execute612CD8's body.
    services.base_update=[] {};
    vista_lobby_leave_task_frame(task->storage,services);
  }
  if(launch_) {
    state.set_session_countdown(countdown());
    vista_lobby_countdown_update(impl_->countdown,impl_->countdown_services);
  }
  observe_menu(&state);
}
} // namespace halo2

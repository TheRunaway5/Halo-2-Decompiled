#include "halo2/frontend_session.hpp"
#include "halo2/native_vista_widget_managers.hpp"
#include "halo2/native_vista_widget_cache.hpp"
#include "halo2/native_vista_widget_memory.hpp"
#include "halo2/multiplayer.hpp"
#include <array>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

using namespace halo2;
namespace {
void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
template<class F> void rejects(F &&function) {
  try { function(); }
  catch (const FormatError &) { return; }
  throw std::runtime_error("unsupported frontend owner operation accepted");
}
void choose(MenuState &state, unsigned entry, unsigned screen) {
  state.select(entry);
  state.accept();
  require(state.frame().screen == screen, "ordinary frontend route changed");
}
struct Expected {
  std::vector<MenuFrame> stack;
  FrontendLaunch launch;
  unsigned revision{};
  const FrontendContent *content{};
  MenuState *state{};
  NativeVistaWidgetManagers *managers{};
  std::byte *manager_storage{};
  NativeVistaWidgetCache *loaded_cache{};
  std::byte *cache_storage{};
  NativeVistaWidgetMemory *memory{};
};

// These stand in only for borrowed native resource lifetimes. They supply no
// original session/readiness/phase values. Each callback owns a distinct token
// so leaving even one adapter installed is observable after its scope dies.
void adapter_scope(MenuState &state, bool exceptional) {
  unsigned calls = 0;
  std::array<std::weak_ptr<int>,4> borrowed;
  try {
    FrontendRuntimeAdapterScope scope(state);
    std::array<std::shared_ptr<int>,4> resources;
    for (unsigned i=0;i<resources.size();++i) {
      resources[i]=std::make_shared<int>(int(i));
      borrowed[i]=resources[i];
    }
    state.set_pc_display_adapter([resource=resources[0],&calls](const DisplaySettings &) {
      ++calls; return std::string{};
    });
    state.set_pc_video_adapter([resource=resources[1],&calls](unsigned,unsigned,FullscreenMode) {
      ++calls; return std::string{};
    });
    state.set_pc_video_query([resource=resources[2],&calls] {
      ++calls; return std::array<unsigned,3>{800,600,0};
    });
    state.set_multiplayer_map_validator([resource=resources[3],&calls](const FrontendLaunch &) {
      ++calls;
    });
    require(calls == 1, "binding display adapter no longer applies current settings once");
    if (exceptional) throw std::runtime_error("test invocation exception");
  } catch (const std::runtime_error &error) {
    if (!exceptional || std::string(error.what()) != "test invocation exception") throw;
  }
  require(calls == 1, "runtime adapter detach invoked a borrowed callback");
  for (const auto &resource:borrowed)
    require(resource.expired(), "frontend retained a borrowed runtime resource");
  // Detachment remains safe and does not apply settings even when repeated.
  state.clear_runtime_adapters();
  require(calls == 1, "repeated detach invoked a borrowed callback");
}

std::unique_ptr<FrontendSession> launch_from_owned_assets(
    const std::filesystem::path &maps, const std::filesystem::path &vista, Expected &expected) {
  auto session=std::make_unique<FrontendSession>();
  {
    Cache cache(maps / "mainmenu.map");
    session->retain_content(cache.load_frontend());
    session->content().vista=load_vista_settings(vista);
  } // Original Cache is already gone before this MenuState is constructed.
  auto &state=session->create_state();
  state.accept(); state.accept(); state.text("Retained Pilot");
  state.commit_text(); state.back();
  require(state.frame().screen==6 && state.signed_in_profile()=="Retained Pilot",
          "disposable profile did not use the ordinary sign-in route");
  choose(state,2,30); choose(state,0,14);
  choose(state,1,23); choose(state,0,206);
  unsigned map_index=unsigned(state.frame().entries.size());
  for (unsigned i=0;i<state.frame().entries.size();++i)
    if (state.frame().entries[i].label=="Ivory Tower") map_index=i;
  require(map_index<state.frame().entries.size(), "authored Ivory Tower map choice missing");
  choose(state,map_index,14);
  choose(state,1,23); choose(state,1,207); choose(state,0,208);
  std::optional<UiVariant> selected;
  unsigned variant_index{};
  // Choose an actual supported nondefault Slayer variant. The fixture does
  // not manufacture rules or add unsupported native weapon/vehicle behavior.
  for (unsigned i=0;i<state.frame().entries.size();++i) {
    const auto &entry=state.frame().entries[i];
    if (entry.destination!=-7) continue;
    const auto &variant=session->content().variants.at(unsigned(entry.value));
    if (variant.name=="Slayer") continue;
    try {
      if (slayer_rules(variant,true).primary_light_vehicle) continue;
      selected=variant; variant_index=i; break;
    } catch (const FormatError &) {}
  }
  require(bool(selected), "no supported authored nondefault Slayer variant found");
  choose(state,variant_index,14);
  adapter_scope(state,false);
  adapter_scope(state,true);
  state.select(0); state.accept();
  require(state.launch && state.launch->multiplayer,
          "ordinary menu Start did not retain a multiplayer launch");
  require(state.launch->map=="cyclotron.map" && state.launch->variant.name==selected->name &&
          state.launch->variant.engine==selected->engine &&
          state.launch->variant.values==selected->values &&
          state.launch->local_players==1 && state.launch->selected_profiles.size()==1,
          "retained launch lost the actual map/variant/selected local profile");
  expected.stack=state.stack(); expected.launch=*state.launch;
  expected.revision=state.revision; expected.content=&session->content(); expected.state=&state;
  expected.managers=&session->vista_widget_managers();
  expected.manager_storage=expected.managers->bytes(0xd71ebc,0x3c).data();
  expected.memory=&session->vista_widget_memory();
  require(expected.memory->mutable_view(0xd71ebc,0x3c).data()==expected.manager_storage,
          "session registry copied the actual manager storage");
  rejects([&]{session->vista_widget_cache();});
  const auto clock=expected.managers->read(0xd71900,4);
  rejects([&]{session->retain_vista_widget_cache(NativeVistaWidgetManagers::source_base);});
  rejects([&]{session->vista_widget_cache();});
  require(expected.memory->read(0xd71900,4)==clock &&
          expected.memory->mutable_view(0xd71ebc,0x3c).data()==expected.manager_storage,
          "rejected cache overlap changed the retained manager authority");
  // Explicit registry mapping is a native lifetime input, not a claim about
  // Vista's physical heap address or an automatic source global publication.
  expected.loaded_cache=&session->retain_vista_widget_cache(0x20000000u);
  expected.cache_storage=expected.loaded_cache->bytes(0x20000000u,8).data();
  require(expected.memory->mutable_view(0x20000000u,8).data()==expected.cache_storage,
          "session registry copied the actual loaded cache storage");
  require(&session->retain_vista_widget_cache(0x20000000u)==expected.loaded_cache,
          "same retained cache mapping was allocated twice");
  rejects([&]{session->retain_vista_widget_cache(0x30000000u);});
  return session;
}

std::shared_ptr<VistaPcrResults> original_report(const std::filesystem::path& fixture) {
  std::ifstream file(fixture);require(bool(file),"original PCR fixture unavailable");
  std::string line;
  while(std::getline(file,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream input(line);unsigned id;std::string words,hex;
    input>>id>>words>>hex;if(id!=12)continue;
    std::replace(words.begin(),words.end(),',',' ');std::istringstream values(words);
    std::vector<std::uint32_t> q;std::uint32_t value;while(values>>value)q.push_back(value);
    require(q.size()>=12 && hex.size()==0x1940*2,"truncated original PCR fixture");
    auto report=std::make_shared<VistaPcrResults>();
    report->player_count=q[9];report->team_count=q[10];report->variant=q[11];
    unsigned cursor{};auto copy=[&](auto& rows){for(auto& row:rows)for(auto& byte:row){
      byte=std::byte(std::stoul(hex.substr(cursor,2),nullptr,16));cursor+=2;}};
    copy(report->players);copy(report->teams);return report;
  }
  throw std::runtime_error("unchanged original PCR fixture12 unavailable");
}

void new_factories(FrontendSession& session,const Expected& expected,
    const std::filesystem::path& fixture) {
  auto& state=session.state();
  const auto unchanged=state.revision;
  rejects([&]{session.enter(FrontendOptions::Entry::initial);});
  rejects([&]{session.enter(FrontendOptions::Entry::postgame_report);});
  require(state.revision==unchanged && state.launch,"rejected entry changed the retained launch");
  auto owner=std::make_shared<VistaPcrScreenOwner>();owner->results=original_report(fixture);
  owner->pane=1;owner->selected_row=0;unsigned continuations{};
  owner->input.host=[] {return std::uint8_t(1);};
  owner->input.continue_session=[&]{++continuations;};
  const auto bank=*owner->results;
  for(unsigned round=0;round<2;++round) {
    // The real renderer is absent while a native world owns the application.
    // A new PCR factory must sample that elapsed gap before stamping its pane;
    // the next invocation's first update must continue the same accumulator.
    const auto before_factory=session.ui_clock();
    state.ui_clock(before_factory);
    const auto gap_begin=std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    const auto old_instance=state.frame().instance;
    const auto factory_begin=std::chrono::steady_clock::now();
    session.enter(FrontendOptions::Entry::postgame_report,owner);
    const auto factory_clock=state.frame().pane_created_clock;
    const auto factory_elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now()-gap_begin).count();
    require(factory_clock-before_factory>=4 &&
            factory_clock-before_factory<=std::uint64_t(factory_elapsed)+2,
            "PCR factory failed to include the actual elapsed renderer/world gap");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    const auto first_update_clock=session.ui_clock();
    state.ui_clock(first_update_clock);
    const auto update_elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now()-factory_begin).count();
    require(first_update_clock-factory_clock>=4 &&
            first_update_clock-factory_clock<=std::uint64_t(update_elapsed)+2,
            "next frontend invocation reset or wrapped its new PCR pane clock");
    require(&session.state()==expected.state && &session.content()==expected.content,
            "new factory moved the application-owned content/state");
    require(&session.vista_widget_managers()==expected.managers &&
            session.vista_widget_managers().bytes(0xd71ebc,0x3c).data()==expected.manager_storage &&
            session.vista_widget_managers().read(0xd71900,4)==first_update_clock,
            "frontend handoff lost retained manager storage or its actual UI clock publication");
    require(&session.vista_widget_cache()==expected.loaded_cache &&
            session.vista_widget_cache().bytes(0x20000000u,8).data()==expected.cache_storage,
            "frontend handoff lost the admitted writable UI cache owner");
    require(&session.vista_widget_memory()==expected.memory &&
            session.vista_widget_memory().mutable_view(0x20000000u,8).data()==expected.cache_storage &&
            session.vista_widget_memory().mutable_view(0xd71ebc,0x3c).data()==expected.manager_storage &&
            session.vista_widget_memory().read(0xd71900,4)==first_update_clock,
            "frontend handoff moved or shadowed the retained source memory authorities");
    require(state.frame().screen==vista_menu(16) && state.stack().size()==1 &&
            state.frame().instance!=old_instance && !state.launch && !state.quit &&
            state.signed_in_profile()=="Retained Pilot",
            "PCR factory reused stale lobby navigation or lost selected profile");
    state.back();require(continuations==round+1,"actual B Done input did not reach its source continuation owner");
    const auto report_instance=state.frame().instance;
    rejects([&]{session.enter(FrontendOptions::Entry::fresh_lobby,owner);});
    session.enter(FrontendOptions::Entry::fresh_lobby);
    const auto lobby_clock=state.frame().pane_created_clock;
    require(lobby_clock>=first_update_clock && session.ui_clock()>=lobby_clock,
            "fresh lobby factory or next clock observation reset the retained accumulator");
    require(state.frame().screen==14 && state.stack().size()==1 &&
            state.frame().instance!=report_instance && !state.frame().vista_pcr &&
            !state.launch && !state.quit && state.lobby_view()->map->name=="Ivory Tower",
            "fresh lobby resumed report/history instead of allocating its new frame");
    state.select(0);state.accept();
    require(state.launch && state.launch->map==expected.launch.map &&
            state.launch->variant.name==expected.launch.variant.name &&
            state.launch->variant.values==expected.launch.variant.values &&
            state.launch->selected_profiles==expected.launch.selected_profiles &&
            state.launch->selected_teams==expected.launch.selected_teams,
            "second match Start lost the actual selected configuration");
  }
  require(*owner->results==bank,"new frontend factories mutated retained original results");
}
} // namespace

int main(int argc, char **argv) {
  try {
    static_assert(!std::is_copy_constructible_v<FrontendSession> &&
                  !std::is_move_constructible_v<FrontendSession>);
    if (argc!=4) throw std::runtime_error("maps, Vista cache and original PCR fixture required");
    const std::filesystem::path maps(argv[1]);
    if (!std::filesystem::is_regular_file(maps / "mainmenu.map") ||
        !std::filesystem::is_regular_file(argv[2])) return 77;
    FrontendSession empty;
    require(!empty.has_content() && !empty.has_state(), "empty owner invented menu state");
    rejects([&] { empty.content(); }); rejects([&] { empty.state(); });
    rejects([&] { empty.create_state(); });
    rejects([&] { empty.enter(FrontendOptions::Entry::fresh_lobby); });
    rejects([&] { empty.vista_widget_managers(); });
    rejects([&] { empty.vista_widget_memory(); });
    rejects([&] { std::as_const(empty).vista_widget_memory(); });
    rejects([&] { empty.vista_widget_cache(); });
    rejects([&] { empty.retain_vista_widget_cache(0x20000000u); });
    Expected expected;
    auto session=launch_from_owned_assets(maps,argv[2],expected);
    const auto &state=session->state();
    require(&state==expected.state && &session->content()==expected.content,
            "handoff copied or moved the menu's referenced owner");
    require(state.signed_in_profile()=="Retained Pilot" && state.profile_name()=="Retained Pilot" &&
            state.frame().screen==14 && state.stack().size()==expected.stack.size() &&
            state.revision==expected.revision && state.launch && !state.quit,
            "resource teardown changed retained navigation/profile state");
    for (unsigned i=0;i<state.stack().size();++i) {
      const auto &actual=state.stack()[i], &before=expected.stack[i];
      require(actual.screen==before.screen && actual.selected==before.selected &&
              actual.context==before.context && actual.instance==before.instance &&
              actual.pane_generation==before.pane_generation &&
              actual.pane_created_clock==before.pane_created_clock,
              "resource teardown reconstructed an existing menu frame");
    }
    require(state.lobby_view() && state.lobby_view()->map &&
            state.lobby_view()->map->name=="Ivory Tower" &&
            state.launch->map==expected.launch.map &&
            state.launch->variant.values==expected.launch.variant.values,
            "state lost its owned content/map/variant after input scope destruction");
    rejects([&] { session->retain_content(FrontendContent{}); });
    rejects([&] { session->create_state(); });
    require(&session->state()==expected.state && session->state().revision==expected.revision,
            "rejected reuse changed the retained menu");
    new_factories(*session,expected,argv[3]);
    // The retained source cell becomes authoritative after manager birth.
    // An actual producer write must not be overwritten by an old host mirror.
    session->ui_clock();
    auto& managers=session->vista_widget_managers();
    constexpr std::uint32_t published=0xfffffff8u;
    session->vista_widget_memory().write(0xd71900,published,4);
    const auto clock_begin=std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(12));
    const auto observed=session->ui_clock();
    const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now()-clock_begin).count();
    require(observed-published>=10 && observed-published<=std::uint64_t(elapsed)+2 &&
            managers.read(0xd71900,4)==observed,
            "native UI clock ignored the retained source accumulator or failed DWORD wrap");
    std::cout << "PASS actual frontend content, profile, lobby map, variant, launch and frame lifetimes survive handoff\n"
              << "PASS all four runtime adapters detach without callbacks on success and exceptions; unsupported reuse rejects\n"
              << "PASS original PCR bank, B Done, fresh lobby and two new Start factories retain configuration; no live transition implied\n";
    std::cout << "PASS real elapsed UI clock survives world gaps, PCR factories, fresh lobbies and subsequent updates\n";
    std::cout << "PASS native source32 UI manager banks and clock publication remain retained across both frontend/world handoffs\n";
    std::cout << "PASS one retained source UI clock authority accepts producer writes and preserves DWORD wrap\n";
    std::cout << "PASS source memory registry borrows actual retained managers/cache across two handoffs and rejects overlapping publication\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n'; return 1;
  }
}

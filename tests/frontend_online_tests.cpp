#include "halo2/frontend.hpp"
#include "halo2/audio.hpp"
#include "halo2/frontend_online.hpp"
#include "halo2/online_service.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "ui/mcc_frontend_string_ids.hpp"
#include "ui/frontend_mcc_logical_layout.hpp"
#include "ui/frontend_og_matchmaking_layout.hpp"
#include "halo2/frontend_vista.hpp"
#include "halo2/frontend_canvas.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/renderer.hpp"
#include "halo2/frontend_model.hpp"
#include "halo2/frontend_material.hpp"
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/math.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <thread>
using namespace halo2;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
void matchmaking_progress_kernels() {
  using U=MatchmakingProgressUi;
  // 0x72700: 10000 ms, published as ceil seconds, ready only at zero.
  check(matchmaking_start_countdown(0).seconds==10 && !matchmaking_start_countdown(0).ready,"Start countdown did not begin at 10");
  check(matchmaking_start_countdown(1).seconds==10 && matchmaking_start_countdown(1000).seconds==9,"Start countdown is not ceil seconds");
  check(matchmaking_start_countdown(9999).seconds==1 && !matchmaking_start_countdown(9999).ready,"Start countdown finished early");
  check(matchmaking_start_countdown(10000).seconds==0 && matchmaking_start_countdown(10000).ready &&
        matchmaking_start_countdown(0xffffffffu).ready,"Start countdown did not finish at 10000 ms");
  check(matchmaking_published_start_countdown(6500).seconds==7 &&
        !matchmaking_published_start_countdown(6500).ready &&
        matchmaking_published_start_countdown(0).ready &&
        matchmaking_published_start_countdown({}).seconds==matchmaking_countdown_none &&
        !matchmaking_published_start_countdown({}).ready,
        "A late countdown observer restarted the clock or launched without a published countdown");
  // 0x25142F, including pane3 only once B's countdown exists.
  check(matchmaking_progress_pane(U::searching,-1)==0 && matchmaking_progress_pane(U::waiting,-1)==1 &&
        matchmaking_progress_pane(U::joining,-1)==0 && matchmaking_progress_pane(U::starting,-1)==2 &&
        matchmaking_progress_pane(U::start_match,-1)==2 && matchmaking_progress_pane(U::start_match,0)==3 &&
        matchmaking_progress_pane(U::have_fun,-1)==3 && matchmaking_progress_pane(U::failed,5)==0,"TU1.5 pane selector differs");
  check(matchmaking_progress_pc_pane(U::joining,-1)==2 && matchmaking_progress_pc_pane(U::start_match,4)==2,"PC pane host differs");
  // 0x251BFF.
  check(matchmaking_progress_title(U::start_match)->xbox==0x0d000440 && matchmaking_progress_title(U::have_fun)->xbox==0x16000441 &&
        matchmaking_progress_title(U::joining)->pc==0x0c00043d,"TU1.5 title table differs");
  // 0x22D775.
  check(matchmaking_countdown_text(7)==":07" && matchmaking_countdown_text(10)==":10" && matchmaking_countdown_text(0)==":00" &&
        matchmaking_countdown_text(-1)==":--","Countdown token format differs");
  // 0x25217B and 0x251AFA.
  check(matchmaking_progress_cancel_allowed(1,0) && !matchmaking_progress_cancel_allowed(2,59999) &&
        matchmaking_progress_cancel_allowed(3,60000) && matchmaking_progress_consumes_y(2) && !matchmaking_progress_consumes_y(1),
        "Cancel lock differs");
  // 0x2514FD pane3 order and 0x2517E2 team coloring.
  const std::vector<MatchmakingProgressPlayer> players{{1,10},{0,5},{1,30},{0,20}};
  check(matchmaking_progress_order(players,true,true)==std::vector<unsigned>{3,1,2,0},"Ranked team order differs");
  check(matchmaking_progress_order(players,true,false)==std::vector<unsigned>{2,3,0,1},"Ranked order differs");
  check(matchmaking_progress_order(players,false,true)==std::vector<unsigned>{1,3,0,2},"Team order differs");
  check(matchmaking_progress_order(players,false,false)==std::vector<unsigned>{0,1,2,3},"Unsorted order changed");
  check(matchmaking_progress_team_colors(3,true) && !matchmaking_progress_team_colors(-1,true) &&
        !matchmaking_progress_team_colors(3,false),"Team color gate differs");
  // 0x2514FD: ranked names remain in party A until pane3; filled game B
  // seats show Matched Player and the remaining maximum slots stay blank.
  using R=MatchmakingProgressRow;
  check(matchmaking_progress_rows(1,true,1,3,4)==std::vector<R>{R::player,R::matched,R::matched,R::empty},
        "Ranked waiting roster exposed game B or lost matched/blank slots");
  check(matchmaking_progress_rows(2,true,2,3,4)==std::vector<R>{R::player,R::player,R::matched,R::empty},
        "Ranked configuring roster lost party A");
  check(matchmaking_progress_rows(1,false,1,3,4)==std::vector<R>{R::player,R::player,R::player,R::empty} &&
        matchmaking_progress_rows(2,false,1,3,4)==std::vector<R>{R::player,R::player,R::player} &&
        matchmaking_progress_rows(3,true,1,3,4)==std::vector<R>{R::player,R::player,R::player},
        "Casual or countdown roster did not select game B");
  check(matchmaking_progress_rows(0,false,2,3,{})==std::vector<R>{R::player,R::player},
        "Search/join roster did not select party A");
  // Expected rows come from unchanged original TU1.5 decision blocks, not
  // a second implementation of the native selection rule.
  std::ifstream roster_file(std::filesystem::path(__FILE__).parent_path()/"fixtures/matchmaking_roster_tu15_original.json");
  check(bool(roster_file),"Missing original TU1.5 roster fixture");
  nlohmann::json roster_fixture;roster_file>>roster_fixture;
  check(roster_fixture.at("schema")=="H2_MATCHMAKING_ROSTER_TU15_ORIGINAL_V1" &&
        roster_fixture.at("original_tu15_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        roster_fixture.at("cases").size()==504,"Unexpected original TU1.5 roster fixture provenance");
  for(const auto &c:roster_fixture.at("cases")) {
    std::optional<unsigned> maximum;
    if(!c.at("maximum").is_null())maximum=c.at("maximum").get<unsigned>();
    const auto pane=c.at("pane").get<unsigned>();const auto ranked=c.at("ranked").get<bool>();
    check(matchmaking_progress_uses_party(pane,ranked)==c.at("use_party").get<bool>(),
          "Native roster source differs from original TU1.5");
    const auto rows=matchmaking_progress_rows(pane,ranked,c.at("party_players").get<unsigned>(),c.at("game_players").get<unsigned>(),maximum);
    std::vector<std::string> names;
    for(const auto row:rows)names.push_back(row==R::player?"player":row==R::matched?"matched":"empty");
    check(names==c.at("rows").get<std::vector<std::string>>(),"Native roster rows differ from original TU1.5");
  }
  // Lobby START GAME: 0x19A78E(10,3), 0x6E360 1000 ms steps, 0x250A8B beeps, 0x22D730.
  check(matchmaking_lobby_countdown_press(-1)==10 && matchmaking_lobby_countdown_press(10)==9 &&
        matchmaking_lobby_countdown_press(4)==3 && matchmaking_lobby_countdown_press(3)==3,"Lobby countdown press differs");
  check(!matchmaking_lobby_countdown_due(5,999) && matchmaking_lobby_countdown_due(5,1000) && !matchmaking_lobby_countdown_due(0,5000),
        "Lobby countdown step differs");
  check(!matchmaking_lobby_countdown_beeps(5,4) && matchmaking_lobby_countdown_beeps(4,3) && matchmaking_lobby_countdown_beeps(1,0) &&
        !matchmaking_lobby_countdown_beeps(2,2),"Lobby countdown beep stages differ");
  check(matchmaking_lobby_countdown_text(10)=="0:10" && matchmaking_lobby_countdown_text(75)=="1:15" &&
        matchmaking_lobby_countdown_text(-1)=="-:--","Lobby countdown token differs");
  // PCR16 follower: a member who is not the leader follows the leader out of
  // the report once it holds the room open and the leader does not.
  OnlineParty party;party.members.resize(2);
  party.members[0].leader=true;party.members[1].self=true;
  check(online_party_follower(party) && !online_party_leader_left_postgame(party),"Unreported presence moved a party follower");
  party.members[1].postgame=true;party.members[0].postgame=true;
  check(!online_party_leader_left_postgame(party),"Follower left while the leader reviewed the report");
  party.members[0].postgame=false;
  check(online_party_leader_left_postgame(party),"Follower ignored the leader's Done");
  party.members[1].postgame=false;
  check(!online_party_leader_left_postgame(party),"Follower outside the report followed a stale room");
  party.members[1].postgame=true;party.leader=true;party.members[0].leader=false;party.members[1].leader=true;
  check(!online_party_follower(party) && !online_party_leader_left_postgame(party),"Party leader followed itself");
  party.members.resize(1);party.leader=false;
  check(!online_party_follower(party),"A one-member party has a follower");
}
void party_notifications() {
  OnlineAccountSnapshot before,after;
  before.authenticated=after.authenticated=true;
  before.party=OnlineParty{};
  before.party->members={{"one","Same Name",{},true,true,true}};
  after=before;after.party->members.push_back({"two","Same Name",{},false,false,true});
  check(frontend_party_notification_sounds(before,after)==std::vector{FrontendUiSound::Advance},"Duplicate names suppressed a real party join cue");
  check(frontend_party_notification_sounds(after,before)==std::vector{FrontendUiSound::Back},"Party leave cue missing");
  check(frontend_party_notification_sounds(after,after).empty(),"Unchanged poll repeats membership cues");
  before=after;after.party->members[0].name="Changed";after.party->members[1].leader=true;
  after.party->members[1].appearance=FrontendAppearance{};
  check(frontend_party_notification_sounds(before,after).empty(),"Presentation or leadership change fabricated a party join");
  before=after;after.party_invitations.push_back({"invite","Same Name",1});
  check(frontend_party_notification_sounds(before,after)==std::vector{FrontendUiSound::ReceiveMessage},"Party invitation cue missing");
  before.authenticated=false;
  check(frontend_party_notification_sounds(before,after).empty(),"Initial account snapshot replayed old party notifications");
  before=after;before.busy=true;after.party_message="Party changed; refresh it";
  check(frontend_party_notification_sounds(before,after)==std::vector{FrontendUiSound::Error},"Failed party action has no error cue");
}
std::size_t discard(char *, std::size_t size, std::size_t count, void *) { return size * count; }
struct Preferences {
  std::filesystem::path path = std::filesystem::temp_directory_path() /
      ("halo2-online-name-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".cfg");
  ~Preferences() { std::filesystem::remove(path); }
};
// Synthetic UI records are deliberately independent of the live SDK probe.
// They exercise duplicate focus, scrolling, long names and request actions
// without recording or mutating any real Discord relationships.
struct FriendsFixture final : DiscordFriends {
  DiscordSocialSnapshot state;
  std::string target,action;
  std::weak_ptr<OnlineAccount> account;
  unsigned activity_accepts{};
  FriendsFixture(){state.available=true;}
  void enable() override {state.enabled=state.connecting=true;++state.revision;}
  void disconnect() override {state={};state.available=true;++state.revision;}
  void pump() override {}
  void presence(const std::string &,bool) override {}
  DiscordSocialSnapshot snapshot() const override {return state;}
  bool command(const std::string &handle,const std::string &value) override {
    target=handle;action=value;return true;
  }
  void bind_account(std::shared_ptr<OnlineAccount> value) override {account=std::move(value);}
  bool invite_to_party(const std::string &handle) override {
    const auto client=account.lock();const auto recipient=party_recipient(handle);
    return client && recipient && client->party_command({"invite_discord",*recipient});
  }
  bool accept_activity(const std::string &handle) override {
    const auto row=std::find_if(state.invitations.begin(),state.invitations.end(),[&](const auto &i){return i.handle==handle;});
    if(row==state.invitations.end() || state.busy)return false;
    target=handle;action="accept_activity";++activity_accepts;state.busy=true;row->status="Accepting invitation...";++state.revision;return true;
  }
  bool dismiss_activity(const std::string &handle) override {
    target=handle;action="dismiss_activity";state.busy=false;
    const auto removed=std::erase_if(state.invitations,[&](const auto &i){return i.handle==handle;});
    ++state.revision;return removed>0;
  }
  std::optional<std::string> party_recipient(const std::string &handle) const override {
    return handle=="private-local-friend"?std::optional<std::string>{"222222222222222222"}:std::nullopt;
  }
};
void private_identity(const MenuState &menu) {
  const auto hidden = menu.online_identity()->id;
  for (const auto &frame : menu.stack()) {
    check(menu.expand(frame.title).find(hidden) == std::string::npos, "Discord ID appears in a heading");
    check(menu.expand(frame.description).find(hidden) == std::string::npos, "Discord ID appears in account information");
    for (const auto &entry : frame.entries)
      check(menu.expand(entry.label).find(hidden) == std::string::npos, "Discord ID appears in a menu action");
  }
}
}
int main(int argc, char **argv) {
  try {
    party_notifications();
    matchmaking_progress_kernels();
    if (argc < 2) return 77;
    std::filesystem::path maps(argv[1]), sampled, selected_menu_assets, captures;
    std::string matchmaking_url, account_url, capture_prefix;
    unsigned callback_port=42103;
    bool party_run{}, gathering_run{}, party_render{}, rank_run{}, return_lobby_run{}, capture_validation=true;
    if(const auto validation=std::getenv("H2_TEST_VK_VALIDATION")) {
      check(std::string_view(validation)=="0" || std::string_view(validation)=="1",
            "H2_TEST_VK_VALIDATION must be 0 or 1");
      capture_validation=std::string_view(validation)=="1";
    }
    std::shared_ptr<OnlineAccount> account;
    for (int i = 2; i < argc; ++i) {
      const std::string option(argv[i]);
      check(i + 1 < argc, "Missing ONLINE test option value");
      const std::filesystem::path value(argv[++i]);
      if (option == "--account-url") account_url = value.string();
      else if(option=="--party-url"){account_url=value.string();party_run=true;}
      else if(option=="--gathering-ui")gathering_run=value=="1";
      else if(option=="--rank-ui")rank_run=value=="1";
      else if(option=="--return-lobby-ui")return_lobby_run=value=="1";
      else if(option=="--party-render")party_render=value=="1";
      else if(option=="--validation") {
        check(value=="0" || value=="1","Invalid Vulkan validation option");
        capture_validation=value=="1";
      }
      else if (option == "--matchmaking-url") matchmaking_url = value.string();
      else if (option == "--ui-assets") sampled = value;
      else if (option == "--menu-ui-assets") selected_menu_assets = value;
      else if (option == "--capture") captures = value;
      else if (option == "--capture-prefix") capture_prefix = value.string();
      else if(option=="--callback-port")callback_port=unsigned(std::stoul(value.string()));
      else if (option == "--asset-sources") {
        if (!std::filesystem::is_regular_file(value)) return 77;
        std::ifstream registry(value);
        std::string line;
        check(bool(std::getline(registry, line)) && line == "H2_ASSET_SOURCES_V1", "Invalid launcher asset registry");
        bool xbox = false, vista = false;
        while (std::getline(registry, line)) {
          const auto tab = line.find('\t');
          check(tab != std::string::npos && line.find('\t', tab + 1) == std::string::npos, "Invalid asset registry row");
          std::filesystem::path path(line.substr(tab + 1));
          if (path.is_relative()) path = value.parent_path().parent_path() / path;
          if (line.substr(0, tab) == "xbox" && !xbox) { maps = path / "maps"; xbox = true; }
          else if (line.substr(0, tab) == "vista" && !vista) { sampled = path; vista = true; }
          else throw std::runtime_error("Unknown or duplicate asset source");
        }
        check(xbox && vista, "Both launcher asset sources are required");
      } else throw std::runtime_error("Unknown ONLINE test option");
    }
    if (!std::filesystem::exists(maps / "mainmenu.map") ||
        (!sampled.empty() && !std::filesystem::exists(sampled / "pack.tsv"))) return 77;
    Cache cache(maps / "mainmenu.map");
    auto content = cache.load_frontend();
    if(cache.is_mcc()) content.vista=load_vista_settings(maps/"mainmenu.map");
    if (!sampled.empty() && std::filesystem::exists(sampled / "maps/mainmenu.map"))
      content.vista = load_vista_settings(sampled / "maps/mainmenu.map");
    if(content.vista && !cache.is_mcc()) {
      const auto &progress=content.vista->screen(210);
      check(progress.panes.size()==3,"Vista progress lost its three authored panes");
      for(unsigned pane=0;pane<3;++pane) {
        const auto &p=progress.panes[pane];
        check(p.texts.size()==(pane==1?1u:4u) && p.lists.size()==1 &&
          p.lists[0].skin==20 && p.lists[0].visible==16 &&
          p.lists[0].position.x==81 && p.lists[0].position.y==150,
          "Vista progress geometry was replaced by a different screen's layout");
        check(p.texts[0].bounds.top==263 && p.texts[0].bounds.left==100 &&
          p.texts[0].bounds.bottom==183 && p.texts[0].bounds.right==450,
          "Vista progress title bounds changed");
      }
    }
    auto logical_layout = detail::mcc_logical_layout::make_view(content);
    const auto selected=selected_menu_assets.empty()?sampled:selected_menu_assets;
    const auto ui_assets = load_frontend_ui_assets(content, selected);
    if(logical_layout)check(detail::og_matchmaking_layout::augment(*logical_layout,content,ui_assets),
        "Selected cache did not admit its actual original matchmaking definitions");
    const auto &native_layout = logical_layout ? logical_layout->content : content;
    // Provenance assertions always compare ONLINE against the Vista bank,
    // even when the surrounding shell has another selected menu pack.
    const auto vista_ui_assets = load_frontend_ui_assets(content, sampled);
    Preferences preferences;
    // One application SDL owner across renderer captures, just like the game.
    // Quit must follow voice teardown, not occur between voice-connected shots.
    auto capture_desktop=std::make_unique<DesktopSession>();
    MenuState menu(content, preferences.path);
    menu.accept(); menu.select(0); menu.accept(); menu.text("Pilot"); menu.commit_text(); menu.back();
    check(menu.frame().screen == 6 && menu.signed_in_profile() == "Pilot", "Local screen name fixture did not sign in");
    check(menu.frame().entries[1].label == "ONLINE", "Main menu does not use uppercase ONLINE");
    Mesh mesh;
    FrontendPainter painter(content, cache.is_mcc()?maps.parent_path()/"h2_fonts":maps/"fonts", mesh, maps / "ui/frame_edges", ui_assets,
                            selected.empty() ? std::filesystem::path{} : selected / "fonts");
    // Bounded renderer evidence: use the game's actual scenery idle and pose,
    // not a synthetic circle. Xbox model/material provenance remains explicit
    // until the independent Vista D3D material path has been implemented.
    std::optional<RenderModel> ring_model;
    std::optional<AnimationClip> ring_idle;
    Transform ring_root;
    unsigned ring_material_base{};
    if(!captures.empty()) {
      const auto level=cache.load_content(0);
      for(const auto &object:level.objects) {
        if(object.name<0 || level.object_names.at(object.name).name!="halo_ring2")continue;
        ring_model=cache.load_model(object.definition,true,object.variant);
        ring_idle=cache.load_initialized_object_idle(object.definition);
        check(ring_idle && ring_idle->frame_count==121 && ring_idle->loop_frame==1,
            "Original progress ring idle is missing");
        const auto basis=authored_basis(object.rotation);
        ring_root=source_object_transform(object.position,basis.forward,basis.up,object.scale,false);
        for(auto &material:ring_model->mesh.materials) {
          if(!material.lods.empty())
            check(prepare_dynamic_material(cache,material,{},true,0.f),"Ring shader is unresolved");
        }
        prepare_frontend_materials(cache,ring_model->mesh);
        ring_material_base=unsigned(mesh.materials.size());
        for(auto &material:ring_model->mesh.materials) {
          mesh.materials.push_back(std::move(material));
        }
        break;
      }
      check(ring_model.has_value(),"Original progress ring scenery is missing");
    }
    bool sampled_online_font = false, sampled_online_bitmap = false;
    std::vector<bool> vista_fonts(mesh.materials.size()), vista_bitmaps(mesh.materials.size());
    std::vector<bool> original_fonts(mesh.materials.size()), original_bitmaps(mesh.materials.size());
    if (!sampled.empty())
      for (std::size_t i = 0; i < mesh.materials.size(); ++i) {
        const auto &material = mesh.materials[i];
        const auto same = [&](const Texture &texture) {
          return texture.width == material.diffuse.width && texture.height == material.diffuse.height &&
                 texture.format == material.diffuse.format && texture.pixels == material.diffuse.pixels;
        };
        if (material.name.starts_with("font/")) {
          const auto name = material.name.substr(5);
          // PC13 uses conduit-9, which has no Xbox font-table counterpart
          // in the sampled overlay. Its own imported PC atlas is authoritative.
          if (content.vista && std::filesystem::is_regular_file(content.vista->source_fonts / name))
            vista_fonts[i] = same(load_retail_font(content.vista->source_fonts / name).atlas);
          if (std::filesystem::is_regular_file(maps / "fonts" / name) &&
              std::filesystem::is_regular_file(sampled / "fonts" / name)) {
            auto font = load_retail_font(maps / "fonts" / name);
            const auto vista_font = load_retail_font(sampled / "fonts" / name);
            apply_retail_font_sampling(font, vista_font);
            vista_fonts[i] = vista_fonts[i] || same(font.atlas) || same(vista_font.atlas);
          }
        } else if (auto bank = vista_ui_assets.find(material.name); bank != vista_ui_assets.end())
          vista_bitmaps[i] = std::any_of(bank->second.begin(),bank->second.end(),same);
        if (content.vista)
          for (const auto &[id,image] : content.vista->images)
            if (image.name==material.name && std::any_of(image.images.begin(),image.images.end(),same))
              vista_bitmaps[i]=true;
      }
    if (cache.is_mcc()) {
      const auto font_source = load_gameplay_font_source(content.source_fonts);
      check(font_source.domain == GameplayFontDomain::Mcc, "ONLINE selected legacy fonts");
      const auto prompts = font_prompt_asset_directory(content.source_fonts);
      std::vector<Texture> atlases;
      for (unsigned role = 0; role < font_source.names.size(); ++role) {
        auto font = load_retail_font(content.source_fonts / font_source.names[role]);
        apply_vista_prompt_sampling(font, prompts, role);
        atlases.push_back(std::move(font.atlas));
      }
      for (unsigned role = 0; role < 11; ++role) {
        auto font = load_gameplay_font(font_source, role);
        apply_vista_prompt_sampling(font, prompts, role);
        atlases.push_back(std::move(font.atlas));
      }
      for (std::size_t i = 0; i < mesh.materials.size(); ++i) {
        const auto &material = mesh.materials[i];
        const auto same = [&](const Texture &texture) {
          return texture.width == material.diffuse.width && texture.height == material.diffuse.height &&
              texture.format == material.diffuse.format && texture.pixels == material.diffuse.pixels;
        };
        if (material.name.starts_with("font/"))
          vista_fonts[i] = std::any_of(atlases.begin(), atlases.end(), same);
        for (const auto &[id, image] : content.images)
          if (image.name == material.name && std::any_of(image.images.begin(), image.images.end(), same))
            vista_bitmaps[i] = true;
      }
      for(std::size_t i=0;i<mesh.materials.size();++i) {
        const auto &material=mesh.materials[i];
        const auto same=[&](const Texture&texture){return texture.width==material.diffuse.width &&
          texture.height==material.diffuse.height && texture.format==material.diffuse.format && texture.pixels==material.diffuse.pixels;};
        if(logical_layout)if(const auto bank=logical_layout->rasters.find(material.name);bank!=logical_layout->rasters.end() &&
            material.name.ends_with("#tu15-logical-mcc"))original_bitmaps[i]=std::any_of(bank->second.begin(),bank->second.end(),same);
      }
    }
    std::set<std::pair<unsigned,unsigned>> audited_panels;
    constexpr std::pair<unsigned,unsigned> viewports[]{
        {640,480},{1280,720},{1280,800},{1920,1080},{2560,1080},
        {3440,1440},{3840,1080},{2560,1437},{1001,601}};
    unsigned bounded_glyphs{}, pointer_projections{};
    auto audit_vista = [&](const MenuState &state, const UiDraw &draw,
                           std::array<unsigned,2> viewport = {640,480}) {
      if (sampled.empty() && !cache.is_mcc()) return;
      // Notice/prompt materials may be reserved lazily. Admit a clone only
      // when its exact raster already belongs to a verified imported bank.
      if(vista_fonts.size()<mesh.materials.size()) {
        const auto count=vista_fonts.size();
        vista_fonts.resize(mesh.materials.size());vista_bitmaps.resize(mesh.materials.size());
        original_fonts.resize(mesh.materials.size());original_bitmaps.resize(mesh.materials.size());
        for(auto i=count;i<mesh.materials.size();++i)for(std::size_t j=0;j<count;++j) {
          const auto &a=mesh.materials[i].diffuse,&b=mesh.materials[j].diffuse;
          if(a.width==b.width && a.height==b.height && a.format==b.format && a.pixels==b.pixels) {
            vista_fonts[i]=vista_fonts[i] || vista_fonts[j];
            vista_bitmaps[i]=vista_bitmaps[i] || vista_bitmaps[j];
            original_fonts[i]=original_fonts[i] || original_fonts[j];
            original_bitmaps[i]=original_bitmaps[i] || original_bitmaps[j];
          }
        }
      }
      const auto surface=frontend_canvas(viewport[0],viewport[1]);
      const float surface_left=draw.full_surface_bounds?-surface.left/surface.scale:0;
      const float surface_top=draw.full_surface_bounds?-surface.top/surface.scale:0;
      const float surface_right=draw.full_surface_bounds?surface_left+surface.surface_width/surface.scale:640;
      const float surface_bottom=draw.full_surface_bounds?surface_top+surface.surface_height/surface.scale:480;
      bool font = false, bitmap = false;
      const bool original_goal=cache.is_mcc() && detail::og_matchmaking_layout::goal_frame(state.frame(),&state);
      // Source5 is a separate background submission. A non-goal dialog can
      // occupy the foreground while this retained goal channel still owns
      // the chrome; match the painter's ancestor walk and background gates.
      const bool title_popup_backdrop=(state.frame().screen==10 || state.frame().screen==167) &&
          std::any_of(state.stack().begin(),state.stack().end(),[](const auto&f){return f.screen==9;});
      bool original_background=false;
      if(cache.is_mcc() && !content.gameplay_overlay && !draw.full_surface_bounds && !title_popup_backdrop &&
          state.frame().screen!=9 && state.frame().screen!=6 && content.screens.contains(5))
        for(auto it=state.stack().rbegin();it!=state.stack().rend();++it) {
          if(detail::og_matchmaking_layout::goal_frame(*it,&state)){original_background=true;break;}
          if(!(content.screen(state.screen_layout(*it)).flags&(1|8|16)))break;
        }
      std::set<std::string> original_background_images;
      if(original_background)
        for(const auto &pane:detail::og_matchmaking_layout::data().at("original").at("background").at("panes"))
          for(const auto &image:pane.at("bitmaps"))
            original_background_images.insert(detail::og_matchmaking_layout::embedded_image_name(image.at("datum").get<std::string>()));
      std::set<std::string> missing;
      for (const auto &v : draw.vertices) {
        const auto &name = mesh.materials.at(v.material).name;
        if(draw.full_surface_bounds)
          check(!name.starts_with("ui\\global_bitmaps\\track") &&
                name!="ui\\global_bitmaps\\framing_center",
                "Clan popup retains decorative shell backdrop assets");
        if(draw.full_surface_bounds && state.frame().screen!=26)
          check(!name.starts_with("ui\\list_skins\\y_menu_list\\") &&
                name!="ui\\screens\\game_shell\\online_y_menu\\tabs",
                "Clan dialog retains the roster beneath it");
        if(state.frame().screen==26)
          check(name.find("rank_icons")==std::string::npos,"Social screen rendered an unowned placeholder rank");
        const auto &material=mesh.materials.at(v.material);
        if(material.ui_emblem_pass) {
          const auto &shaders=content.emblem_shaders;
          check(std::any_of(shaders.begin(),shaders.end(),[&](const auto &shader){
            return shader.second.name==name && shader.second.ui_emblem_pass;
          }),"ONLINE emblem does not belong to its selected native shader bank");
          check(material.forward_vertex==11 && material.forward_blend==1 &&
                material.animated_textures[0].size()==32 && material.animated_textures[1].size()==64 &&
                material.texture_frames[0]<32 && material.texture_frames[1]<64,
                "ONLINE emblem lost its source pass or bounded foreground/background pages");
          {
            const auto &owner=shaders.at(detail::mcc_logical_layout::Codec::named(shaders,name));
            for(unsigned stage=0;stage<2;++stage)for(unsigned page=0;page<owner.animated_textures[stage].size();++page) {
              const auto &expected=owner.animated_textures[stage][page],&actual=material.animated_textures[stage][page];
              check(expected.width==actual.width && expected.height==actual.height && expected.format==actual.format && expected.pixels==actual.pixels,
                  "Actual goal emblem lost an admitted MCC animation page");
            }
            const Texture*selected[]{&material.diffuse,&material.detail};
            for(unsigned stage=0;stage<2;++stage)check(selected[stage]->pixels==owner.animated_textures[stage].at(material.texture_frames[stage]).pixels,
                "Actual goal emblem did not bind its admitted appearance-selected MCC page");
          }
          bitmap=true; // The verified emblem pass samples the imported bitmap banks.
        } else if (name.starts_with("font/")) {
          check(vista_fonts.at(v.material),
              "ONLINE text differs from its selected admitted MCC/imported font atlas");
          if(!std::isfinite(v.position.x) || !std::isfinite(v.position.y) ||
             v.position.x<surface_left-.001f || v.position.x>surface_right+.001f ||
             v.position.y<surface_top-.001f || v.position.y>surface_bottom+.001f)
            throw FormatError("ONLINE screen "+std::to_string(state.frame().screen)+" text escapes the foreground canvas: "+
                std::to_string(v.position.x)+","+std::to_string(v.position.y));
          check(!mesh.materials.at(v.material).ui_widescreen_background,
                "ONLINE text is stretched with the widescreen background");
          ++bounded_glyphs;
          font = true;
        } else if (name.starts_with("ui\\") && name.find("/native-") == std::string::npos) {
          const bool original_owner=original_goal || original_background_images.contains(name);
          if (!vista_bitmaps.at(v.material) && !(original_owner && original_bitmaps.at(v.material))) missing.insert(name);
          bitmap = true;
        }
      }
      if(!missing.empty()) {
        std::string message="ONLINE bitmaps missing selected imported sampling:";
        for(const auto &name:missing)message+='\n'+name;
        throw std::runtime_error(message);
      }
      if(cache.is_mcc()) {
        const auto ui=state.online_matchmaking_ui();
        const bool expected=original_goal && state.online_friends_available() &&
            ui!=MatchmakingProgressUi::starting && ui!=MatchmakingProgressUi::start_match;
        const auto name=detail::og_matchmaking_layout::embedded_image_name("ui\\screens\\game_shell\\online_y_menu\\y_menu_compact");
        const bool visible=std::any_of(draw.vertices.begin(),draw.vertices.end(),[&](const auto&vertex){return mesh.materials.at(vertex.material).name==name;});
        check(visible==expected,"Actual collapsed Friends widget differs from its admitted capability/context/source-phase gates");
      }
      if(!font || (!bitmap && !state.party_invite_picker()))throw FormatError("ONLINE panel "+std::to_string(state.frame().screen)+
          " context "+std::to_string(state.frame().context)+" has imported font="+std::to_string(font)+
          " bitmap="+std::to_string(bitmap));
      sampled_online_font |= font; sampled_online_bitmap |= bitmap;
      audited_panels.emplace(state.frame().screen,state.frame().context);
      if(cache.is_mcc() && state.screen_layout(state.frame())==vista_menu(13)) {
        const auto fonts=load_gameplay_font_source(content.source_fonts);
        const auto column_font="font/"+fonts.names.at(5);
        float first_column_ink=480;
        for(const auto &v:draw.vertices)
          if(mesh.materials.at(v.material).name==column_font)
            first_column_ink=std::min(first_column_ink,v.position.y);
        check(first_column_ink>=60,
              "ONLINE browser columns overlap the original shell title bar");
      }
      if(cache.is_mcc() && state.frame().screen==186) {
        const auto &bank=content.images.at(content.screen(186).panes.front().bitmaps.at(1).datum);
        check(bank.images.size()>4,"MCC Bungie artwork page is missing");
        check(std::any_of(draw.vertices.begin(),draw.vertices.end(),[&](const auto &v) {
          const auto &m=mesh.materials.at(v.material);
          return m.name==bank.name && m.diffuse.pixels==bank.images.at(4).pixels;
        }),"Imported Bungie.net message panel is hidden on ONLINE");
      }
      if(state.frame().screen==210 && content.vista && !state.frame().entries.empty() &&
          !state.frame().entries.front().label.empty()) {
        const bool original=native_layout.original_matchmaking_layout;
        const auto &layout=original?native_layout:*content.vista;
        const auto &pane=layout.screen(210).panes.at(original?state.online_matchmaking_progress_pane():state.online_matchmaking_pane());
        const auto &list=pane.lists.at(0);
        const auto &skin=layout.skins.at(list.skin);
        const auto &field=original?layout.skin_groups.at(skin.grids.front().skin).texts.front():skin.texts.at(0);
        const auto group=original?skin.grids.front().position:Vec2{};
        const float scale=original?1.f:.4f;
        const float left=320+scale*(list.position.x+group.x+field.bounds.left);
        const float right=320+scale*(list.position.x+group.x+field.bounds.right);
        const float top=240-scale*(list.position.y+group.y+field.bounds.top);
        const float bottom=240-scale*(list.position.y+group.y+field.bounds.bottom);
        float ink_top=bottom,ink_bottom=top;
        for(const auto &v:draw.vertices) {
          if(!mesh.materials.at(v.material).name.starts_with("font/") ||
              v.color.x+v.color.y+v.color.z<.1f || v.position.x<left || v.position.x>right ||
              v.position.y<top || v.position.y>bottom)continue;
          ink_top=std::min(ink_top,v.position.y);ink_bottom=std::max(ink_bottom,v.position.y);
        }
        if(ink_bottom-ink_top<4)
          throw FormatError("Admitted source matchmaking roster clips the chosen screen name: ink="+
              std::to_string(ink_top)+","+std::to_string(ink_bottom)+"; box="+
              std::to_string(top)+","+std::to_string(bottom)+"; name="+state.frame().entries.front().label);
      }
      if(state.frame().screen==26 && (state.frame().context==1 ||
          (cache.is_mcc() && state.frame().context==0))) {
        check(draw.full_surface_bounds,"Friends/Clans page is still restricted to the centered canvas");
        const auto &pane=content.screen(26).panes.at(1);
        const auto &header=content.images.at(pane.bitmaps.at(1).datum);
        float header_left=surface_right,header_right=surface_left,header_top=surface_bottom;
        bool opaque_body=false;
        for(std::size_t i=0;i+5<draw.vertices.size();i+=6) {
          const auto &a=draw.vertices[i],&b=draw.vertices[i+4];
          const auto &name=mesh.materials.at(a.material).name;
          if(name==header.name) {
            header_left=std::min(header_left,a.position.x);
            header_right=std::max(header_right,b.position.x);
            header_top=std::min(header_top,a.position.y);
            // Preserve the original tab/corner region while its flat center grows.
            if(a.uv.x==0 && a.uv.y==0)
              check(std::abs((b.uv.x-a.uv.x)*529-(b.position.x-a.position.x))<.001f,
                    "Clan tab artwork was horizontally stretched");
          }
          if(i==0 && name.empty()) {
            check(std::abs(a.position.x-surface_left)<.001f &&
                  std::abs(b.position.x-surface_right)<.001f &&
                  std::abs(a.position.y-surface_top)<.001f &&
                  std::abs(b.position.y-surface_bottom)<.001f,
                  "Clan backing does not cover the entire surface");
            check(a.position.z==1 && b.position.z==1 &&
                  mesh.materials.at(a.material).diffuse.pixels.empty(),
                  "Clan backing lets the old background show through");
            opaque_body=true;
          }
        }
        check(std::abs(header_left-surface_left)<.001f &&
              std::abs(header_right-surface_right)<.001f &&
              std::abs(header_top-surface_top)<.001f,
              "Friends/Clans panel does not extend from the top left to the right edge");
        check(opaque_body,"Clan full-surface backing was not audited");
      }
      if(state.frame().screen==14)if(const auto lobby=state.lobby_view();lobby &&
          lobby->party_names && !lobby->party_names->empty()) {
        const auto &pane=native_layout.screen(14).panes.at(2);
        const auto &grid=pane.grids.front();
        const auto &skin=native_layout.skin_groups.at(grid.skin);
        const auto name=std::find_if(skin.texts.begin(),skin.texts.end(),
            [&](const auto &t){return detail::frontend_compiled_string_id(content,0x0b000698)==t.string;});
        check(name!=skin.texts.end(),"Party roster name field is missing");
        const float left=320+grid.position.x+name->bounds.left;
        const float right=320+grid.position.x+name->bounds.right;
        const float top=240-grid.position.y-name->bounds.top;
        const float bottom=240-grid.position.y-name->bounds.bottom-
            float(lobby->party_names->size()-1)*grid.step.y;
        // 0x2508A8 shows original pane2 help only for an ONLINE session with
        // at most one machine, including a machine with several couch seats.
        const auto help=state.online_party_text(state.frame(),47);
        check(!help.empty()==(lobby->protocol==2 && lobby->machines<=1),
              "ONLINE party help differs from the original machine-count visibility gate");
        if(!help.empty()) {
          const auto &bounds=pane.texts.at(47).bounds;
          check(320+bounds.right<=left || 320+bounds.left>=right ||
                240-bounds.top>=bottom || 240-bounds.bottom<=top,
                "Source ONLINE party help bounds overlap actual player names");
        }
        for(const unsigned helper:{4u,5u,6u}) {
          check(state.online_party_text(state.frame(),helper).empty(),
                "ONLINE party added text to an unauthored helper field");
        }
      }
      for(const auto [width,height]:viewports) {
        if(draw.full_surface_bounds && (width!=viewport[0] || height!=viewport[1]))continue;
        const auto canvas=frontend_canvas(width,height);
        check(std::abs(canvas.width/640-canvas.height/480)<.0001f,
              "ONLINE foreground loses its aspect ratio");
        for(float y=surface_top+4;y<surface_bottom;y+=8)for(float x=surface_left+4;x<surface_right;x+=8) {
          const auto target=painter.pointer_hit(x,y);
          if(!target || painter.pointer_hit(x-1,y)!=target || painter.pointer_hit(x+1,y)!=target ||
              painter.pointer_hit(x,y-1)!=target || painter.pointer_hit(x,y+1)!=target)continue;
          // Window pixel centers follow SDL's logical-window coordinates.
          const float px=std::floor(canvas.left+x*canvas.scale)+.5f;
          const float py=std::floor(canvas.top+y*canvas.scale)+.5f;
          check(px>=0 && px<width && py>=0 && py<height &&
                painter.pointer_hit((px-canvas.left)/canvas.scale,(py-canvas.top)/canvas.scale)==target,
                "ONLINE mouse target drifts in widescreen");
          ++pointer_projections;
        }
      }
    };
    const auto &logo = content.images.at(content.screens.at(9).panes.front().bitmaps.front().datum).name;
    const auto has_logo = [&](const MenuState &state) {
      const auto draw = painter.draw(state, 5);
      if (state.frame().screen != 6) audit_vista(state,draw);
      return std::any_of(draw.vertices.begin(), draw.vertices.end(), [&](const auto &v) {
        return mesh.materials.at(v.material).name == logo;
      });
    };
    check(has_logo(menu), "Main menu lost its original title logo");
    menu.select(1); menu.accept();
    check(menu.frame().screen == 20 && menu.frame().entries.size() == 1 && menu.frame().entries[0].label == "Sign in with Discord", "Online bypasses the Discord gate");
    check(!has_logo(menu), "Main-menu logo overlaps Discord sign-in");
    MenuState direct(content); direct.open(20);
    check(!has_logo(direct), "Retained title logo overlaps direct Discord sign-in");
    menu.accept();
    check(!menu.online_identity() && !menu.launch, "Preview manufactured an authenticated identity");
    menu.back(); menu.back();
    menu.open(186);
    check(menu.frame().screen == 20, "Direct Online screen bypasses login");
    menu.back(); menu.open(26);
    check(menu.frame().screen == 20, "Players shortcut bypasses login");
    menu.back();
    DiscordLoginOptions options; options.callback_port = callback_port;
    std::string url;
    auto login = std::make_shared<DiscordLogin>([&](const std::string &value) { url = value; return true; }, options,
      [](const DiscordHttpRequest &r, std::stop_token) -> DiscordHttpResponse {
        if (r.url.ends_with("/token")) return {200, R"({"access_token":"t","token_type":"Bearer","scope":"identify","expires_in":3600})"};
        return {200, R"({"id":"123456789012345678","username":"verified_user","global_name":"Name"})"};
      });
    menu.set_discord_login(login);
    OnlineAccountOptions account_options;
    if (!account_url.empty()) {
      account_options.endpoint=account_url; account_options.allow_loopback_http=true;
      account_options.enable_parties=party_run;
      // Each poll makes two requests against the service's 120/min social
      // limit; the party run's matchmaking steps outlast its token bucket.
      account_options.poll_interval=std::chrono::milliseconds(party_run?1500:captures.empty()?250:1000);
      account=std::make_shared<OnlineAccount>(login,account_options); menu.set_online_account(account);
    }
    menu.open(20); menu.accept();
    check(menu.frame().screen == 20 && !menu.online_identity(), "Pending login admits Online");
    menu.back();
    check(login->snapshot().status == DiscordLoginStatus::SignedOut, "Back did not cancel pending login");
    menu.open(20); menu.accept();
    const auto at = url.find("state=") + 6;
    const auto state = url.substr(at, url.find('&', at) - at);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> client(curl_easy_init(), curl_easy_cleanup);
    const auto callback = "http://127.0.0.1:"+std::to_string(callback_port)+"/discord/callback?code=test&state=" + state;
    curl_easy_setopt(client.get(), CURLOPT_URL, callback.c_str());
    curl_easy_setopt(client.get(), CURLOPT_NOPROXY, "*");
    curl_easy_setopt(client.get(), CURLOPT_TIMEOUT, 3L);
    curl_easy_setopt(client.get(), CURLOPT_WRITEFUNCTION, discard);
    check(curl_easy_perform(client.get()) == CURLE_OK, "Frontend callback failed");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
    while (!menu.online_identity() && std::chrono::steady_clock::now() < deadline) { menu.update_online_login(); std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
    menu.update_online_login();
    check(menu.frame().screen == 186 && menu.online_identity()->id == "123456789012345678" && menu.frame().description.starts_with("Pilot | "), "Discord login replaced the chosen screen name");
    check(menu.expand("\xee\x90\x8f") == "Pilot", "ONLINE name token uses the Discord username");
    check(!has_logo(menu), "Title logo overlaps ONLINE service information");
    private_identity(menu);
    // Audit direct authenticated routes as well as the scripted service flow.
    // A legacy pane can introduce a bitmap that the ordinary path never uses.
    for (const auto id : {20u,21u,26u,27u,31u,34u,35u,36u,182u,186u,
                          210u,211u,215u,216u,217u,219u,236u,237u}) {
      MenuState sample(content);
      sample.set_discord_login(login);
      sample.open(186);
      sample.open(id);
      if (sample.frame().screen != id) continue;
      const bool original_progress=id==210 && native_layout.original_matchmaking_layout;
      const bool pc_progress=id==210 && !original_progress && content.vista && content.vista->screens.contains(210);
      const auto first_pane=id==210 && !pc_progress && !original_progress?1u:0u;
      const auto pane_count=original_progress?unsigned(native_layout.screen(210).panes.size()):pc_progress?3u:id==210?2u:unsigned(content.screen(id).panes.size());
      for (unsigned pane=first_pane;pane<pane_count;++pane) {
        try {
          for(const auto [width,height]:viewports) {
            const auto draw=painter.draw(sample,5,int(pane),10000,{width,height});
            audit_vista(sample,draw,{width,height});
            if(cache.is_mcc() && (pc_progress || original_progress)) {
              unsigned rings=0;
              const auto &source=(original_progress?native_layout:*content.vista).screen(210).panes.at(pane).scenes.at(0);
              for(const auto &scene:draw.scenes) {
                if(scene.definition->objects!=source.objects)continue;
                ++rings;const auto &a=scene.definition->viewport, &b=source.viewport;
                check(scene.vista_viewport==!original_progress && a.top==b.top && a.left==b.left &&
                    a.bottom==b.bottom && a.right==b.right,
                    "Matchmaking ring uses a different canvas/anchor from its admitted source progress widgets");
              }
              check(rings==1,"Matchmaking pane lost its one authored spinning ring");
            }
          }
        }
        catch(const std::exception &e) { throw std::runtime_error("ONLINE screen "+
            std::to_string(id)+" pane "+std::to_string(pane)+": "+e.what()); }
        if(id==210) {
          if(original_progress) {
            // Pinned Xbox210 has subtitle enum0. 2369F4 ->259EC6 ->14C233
            // maps that to SID0, absent from its actual shared key bank.
            // B remains an event in25217B; a PC Cancel caption/hitbox must
            // not be introduced into this original definition.
            check(native_layout.screen(210).button_key==0,
                "Original matchmaking subtitle changed from its authenticated empty enum");
            for(float y=430;y<480;y+=2)for(float x=0;x<640;x+=2)
              if(const auto hit=painter.pointer_hit(x,y))
                check(hit->action!=FrontendPointerAction::Back,
                    "Original matchmaking added an unauthored Cancel pointer caption");
            sample.back();
          } else {
          const auto &bounds=pc_progress?content.vista->header_bounds.at(3)
              :content.screen(id).panes.at(pane).texts.at(1).bounds;
          const float scale=pc_progress?.4f:1.f;
          std::optional<FrontendPointerTarget> cancel;
          for(float y=240-scale*bounds.top;y<240-scale*bounds.bottom;y+=1)
            for(float x=320+scale*bounds.left;x<320+scale*bounds.right;x+=1)
              if(const auto hit=painter.pointer_hit(x,y);hit && hit->action==FrontendPointerAction::Back)
                cancel=hit;
          check(cancel.has_value(),"ONLINE queue Cancel prompt cannot be clicked with a mouse");
          check(sample.activate_pointer(*cancel),"ONLINE queue mouse Cancel was not dispatched");
          }
          // TU1.5 0x25217B: Cancel asks with the leave-matchmaking dialog.
          check(!sample.frame().entries.empty() && sample.frame().entries.front().destination==-16,
                "ONLINE queue Cancel did not ask to leave matchmaking");
          sample.select(0);sample.accept();
          check(sample.frame().screen==186,"ONLINE queue mouse Cancel did not return to ONLINE");
          if(pane+1<pane_count)sample.open(id);
        }
      }
    }
    std::unique_ptr<Renderer> capture_renderer;
    nlohmann::json capture_report={{"vulkan_validation",capture_validation},{"viewports",nlohmann::json::array()},
        {"captures",nlohmann::json::array()}};
    for(const auto [width,height]:viewports)capture_report["viewports"].push_back({width,height});
    auto capture = [&](const std::string &name) {
      audit_vista(menu,painter.draw(menu,5,-1,10000));
      const bool capture_gpu=!captures.empty() && name.starts_with(capture_prefix) &&
          (account_url.empty() || name.starts_with("clan-") || name.starts_with("party-"));
      if (!capture_gpu && !(menu.frame().screen==26 && menu.frame().context==1)) return;
      if(capture_gpu)std::filesystem::create_directories(captures);
      for (const auto dimensions : viewports) {
        const auto ui = painter.draw(menu, 5, -1, 10000, {dimensions.first,dimensions.second});
        audit_vista(menu,ui,{dimensions.first,dimensions.second});
        if(!capture_gpu)continue;
        auto *window = capture_desktop->window(dimensions.first, dimensions.second, true);
        check(capture_desktop->apply_video(dimensions.first,dimensions.second,false).empty(),"Capture mode change failed");
        SDL_PumpEvents();
        if(!capture_renderer)capture_renderer=std::make_unique<Renderer>(window,mesh,capture_validation);
        else capture_renderer->resized();
        bool has_ring=false;
        for(const auto &scene:ui.scenes)
          has_ring|=std::find(scene.definition->objects.begin(),scene.definition->objects.end(),"halo_ring2")!=scene.definition->objects.end();
        for(const unsigned tick:has_ring?std::vector<unsigned>{0,30}:std::vector<unsigned>{0}) {
          std::vector<Vertex> ring_vertices;
          std::vector<ModelView> views;
          nlohmann::json rectangles=nlohmann::json::array();
          if(has_ring) {
            FrontendModelClock clock;
            for(unsigned i=0;i<tick;++i)frontend_model_clock(clock,ring_idle->frame_count,ring_idle->loop_frame,1.f/30.f);
            ring_vertices=pose_model_world_frame(*ring_model,ring_root,&*ring_idle,clock.frame);
            for(auto &vertex:ring_vertices)vertex.material+=ring_material_base;
            for(const auto &scene:ui.scenes) {
              const auto &definition=*scene.definition;
              if(std::find(definition.objects.begin(),definition.objects.end(),"halo_ring2")==definition.objects.end())continue;
              check(scene.vista_viewport,"Vista progress model lost its PC viewport route");
              const auto &r=definition.viewport;
              const auto rectangle=frontend_vista_model_viewport(
                  {std::int16_t(r.top),std::int16_t(r.left),std::int16_t(r.bottom),std::int16_t(r.right)},
                  {0,0,std::int16_t(dimensions.second),std::int16_t(dimensions.first)});
              const auto camera=frontend_model_camera(definition,scene.animation_offset);
              SourceFrustumProjectionInput projection_input;
              projection_input.projection_angle=*camera.projection_vertical_angle;
              for(unsigned i=0;i<4;++i)projection_input.outer[i]=projection_input.safe[i]=std::int16_t(rectangle[i]);
              projection_input.layout_state_present=true;projection_input.layout_state_flag=1;
              projection_input.layout_state_mode=3;
              const auto projection=source_frustum_projection(projection_input);
              const auto source_camera=source_view_projection({camera.position,camera.forward,camera.up,
                  .015625f,1024.f,{},projection.projection_program});
              ModelView view{camera,320+r.left,240-r.top,r.right-r.left,r.top-r.bottom,
                  scene.before_vertex,ring_vertices};
              view.source_ui_viewport=rectangle;view.projection_clip_planes=Vec2{.015625f,1024.f};
              SourceViewShaderConstants constants;
              constants.world=source_view_shader_constants(source_camera,1024.f,false);
              constants.reflection=constants.world;view.source_view_shader_constants=constants;
              views.push_back(view);rectangles.push_back(rectangle);
            }
            check(!views.empty() && !ring_vertices.empty(),"Ring renderer submitted no model geometry");
          }
          const auto suffix=tick?"-tick"+std::to_string(tick):std::string{};
          const auto path=captures/(name+suffix+"-"+std::to_string(dimensions.first)+"x"+std::to_string(dimensions.second)+".ppm");
          bool captured{};
          for(unsigned attempt=0;attempt<3 && !captured;++attempt)
            captured=capture_renderer->draw(Camera{},path,{},false,{}, {},ui.vertices,views,{},ui.backdrops,nullptr,nullptr,ui.full_surface_bounds);
          check(captured,"ONLINE UI capture failed");
          capture_report["captures"].push_back({{"name",name},{"screen",menu.frame().screen},
              {"layout",menu.screen_layout(menu.frame())},
              {"context",menu.frame().context},{"width",dimensions.first},{"height",dimensions.second},
              {"vertices",ui.vertices.size()},{"file",path.filename().string()},
              {"ring_tick",tick},{"ring_views",views.size()},{"ring_physical_rectangles",rectangles},
              {"ring_model_source",has_ring
                  ? cache.is_mcc()?"Selected imported MCC mainmenu cache":"Xbox mainmenu cache; Vista material parity unresolved"
                  : "none"}});
        }
      }
      if(capture_gpu)std::ofstream(captures/"report.json")<<capture_report.dump(2)<<'\n';
    };
    {
      for(unsigned p=1;p<4;++p) {
        check(menu.controller_slot(100+p,true)==p,"ONLINE controller failed to join its local seat");
        capture("couch-profile-"+std::to_string(p+1));
        const auto &entries=menu.frame().entries;
        const auto guest=std::find_if(entries.begin(),entries.end(),[](const auto &e){return e.destination==-80;});
        check(guest!=entries.end(),"ONLINE chooser omitted guest profile");menu.select(unsigned(guest-entries.begin()));menu.accept();
        check(menu.online_couch_screen(menu.frame()),"Local guest options missing");
        private_identity(menu);capture("couch-guest-"+std::to_string(p+1));menu.select(0);menu.accept();menu.update_online_login();
      }
      check(menu.local_players().size()==4 && !menu.controller_slot(105,true),"ONLINE admitted more than four local players");
      capture("couch-four-ready");
      menu.open_online_local_player(2);menu.select(2);menu.accept();
      check(menu.local_players().size()==3,"Remove Local Player failed");
      menu.leave_local_controller(1);menu.leave_local_controller(3);menu.update_online_login();
      check(menu.local_players().size()==1,"Couch test left extra profiles in later tests");
    }
    if(account_url.empty()) {
      const auto friends=std::make_shared<FriendsFixture>();menu.set_discord_social(friends);
      menu.open(26,0);capture("friends-enable");menu.accept();menu.update_online_login();
      check(friends->state.connecting,"Enable Friends did not request separate consent");capture("friends-connecting");
      friends->state.ready=true;friends->state.connecting=false;
      for(unsigned i=0;i<100;++i)friends->state.friends.push_back({"test-friend-"+std::to_string(i),
          i<2?"Duplicate":i==98?std::string(128,'W'):"Friend "+std::to_string(i),
          "fixture_"+std::to_string(i),DiscordFriendKind::Friend,
          i%2?DiscordFriendPresence::Offline:DiscordFriendPresence::Playing});
      friends->state.friends.push_back({"test-incoming","Incoming request","fixture_in",DiscordFriendKind::Incoming});
      friends->state.friends.push_back({"test-outgoing","Outgoing request","fixture_out",DiscordFriendKind::Outgoing});
      friends->state.friends.push_back({"test-blocked","Blocked user","fixture_block",DiscordFriendKind::Blocked});
      ++friends->state.revision;menu.update_online_login();
      check(menu.frame().entries.size()==102,"Friends list lost requests or exposed blocked users");
      check(!has_logo(menu),"Title logo overlaps Friends list");private_identity(menu);capture("friends-full");
      // Original24BC12/2BAFA4 advances skin6 by its41-unit union.
      // Sampling the last row catches the former28-unit packing, whose
      // second-line status overlapped the following friend's name.
      painter.draw(menu,5,-1,10000);
      const auto &friends_list=content.screen(26).panes.at(0).lists.front();
      const auto last_friend=painter.pointer_hit(320+friends_list.position.x+20,
          240-friends_list.position.y-41+7*41+20-(cache.is_mcc()?36:0));
      check(last_friend && last_friend->action==FrontendPointerAction::Entry && last_friend->index==7,
          "Friends two-line rows lost the original nonoverlapping pitch");
      menu.select(1);std::swap(friends->state.friends[0],friends->state.friends[1]);
      ++friends->state.revision;menu.update_online_login();
      check(menu.frame().selected==0,"Duplicate friend names changed focus to another person");
      menu.accept();check(menu.frame().context==43 && menu.frame().entries.size()==1,
          "Friend options exposed a party invite without leader authority");capture("friends-options");menu.back();
      menu.select(98);capture("friends-long-name-scroll");
      menu.select(100);menu.accept();capture("friends-incoming-options");menu.select(0);menu.accept();
      check(friends->target=="test-incoming" && friends->action=="accept","Incoming friend action targeted a different row");
      menu.select(101);menu.accept();capture("friends-outgoing-options");menu.select(0);menu.accept();
      check(friends->target=="test-outgoing" && friends->action=="cancel","Outgoing friend action targeted a different row");
      friends->state.invitations={{"activity-a","Duplicate"},{"activity-b","Duplicate"},{"activity-long",std::string(128,'W')}};
      ++friends->state.revision;menu.update_online_login();
      check(menu.frame().entries.size()==105,"SDK invitations displaced friends or requests");
      menu.select(0);private_identity(menu);capture("friends-game-invitations");
      menu.select(1);std::swap(friends->state.invitations[0],friends->state.invitations[1]);
      ++friends->state.revision;menu.update_online_login();
      check(menu.frame().selected==0,"Duplicate invite names changed the selected SDK record");
      menu.accept();check(menu.frame().context==44 && menu.frame().entries.size()==3,"SDK invitation did not reuse the selected-player dialog");
      check(!has_logo(menu),"Title logo overlaps an SDK invitation");capture("friends-game-invitation-options");
      friends->state.invitations.erase(friends->state.invitations.begin());++friends->state.revision;menu.update_online_login();
      check(menu.frame().entries.size()==1,"Expired SDK invitation retained actionable buttons");
      menu.accept();check(friends->activity_accepts==0 && menu.frame().screen==26,"Expired invitation accepted another duplicate sender");
      menu.select(0);menu.accept();menu.select(0);menu.accept();
      check(friends->activity_accepts==1 && friends->target=="activity-a","SDK acceptance targeted the wrong private record");
      capture("friends-game-invitation-accepting");menu.select(0);menu.accept();
      check(friends->activity_accepts==1,"Repeated click submitted another SDK acceptance");
      friends->state.busy=false;friends->state.invitations.front().status="Try accepting again.";
      ++friends->state.revision;menu.update_online_login();capture("friends-game-invitation-retry");
      menu.select(1);menu.accept();check(friends->action=="dismiss_activity" && menu.frame().screen==26,"Dismissal did not return to Friends");
      menu.select(0);capture("friends-game-invitation-long-name");
      menu.accept();check(menu.frame().context==44 && !has_logo(menu),"Long invitation name lost its Vista dialog");
      capture("friends-game-invitation-long-name-options");menu.back();
      friends->state.invitations.clear();++friends->state.revision;menu.update_online_login();
      menu.auxiliary();check(menu.frame().screen==216 && menu.frame().context==43,"Friends options routed to party controls");
      capture("friends-menu-options");menu.select(1);menu.accept();menu.update_online_login();
      check(menu.online_identity().has_value() && !friends->state.enabled,"Disconnecting friends signed out ONLINE");
      while(menu.frame().screen!=186 && menu.stack().size()>1)menu.back();
      menu.set_discord_social({});
    }
    // Drive the same rendered pointer target -> select -> repaint path as
    // frontend.cpp. Checking frame.entries alone misses disappearing widgets.
    const auto visible_online_rows = [&] {
      std::set<unsigned> rows;
      for (float y = 0; y < 480; y += 2)
        for (float x = 0; x < 640; x += 2)
          if (const auto hit = painter.pointer_hit(x, y);
              hit && hit->action == FrontendPointerAction::Entry)
            rows.insert(hit->index);
      return rows;
    };
    const auto online_visible = std::min(content.screen(186).panes.front().lists.front().visible,
                                         unsigned(menu.frame().entries.size()));
    check(online_visible >= 2, "ONLINE hover fixture needs multiple visible rows");
    for (unsigned index = 0; index < online_visible; ++index) {
      painter.draw(menu, 5, -1, 10000);
      std::optional<FrontendPointerTarget> hovered;
      for (float y = 0; y < 480 && !hovered; y += 2)
        for (float x = 0; x < 640 && !hovered; x += 2)
          if (const auto hit = painter.pointer_hit(x, y);
              hit && hit->action == FrontendPointerAction::Entry && hit->index == index)
            hovered = hit;
      check(hovered.has_value(), "ONLINE row is missing before hover");
      menu.select(hovered->index);
      const auto draw = painter.draw(menu, 5, -1, 10000);
      const auto rows = visible_online_rows();
      std::cout << "ONLINE hover row=" << index << " visible_rows=" << rows.size()
                << " vertices=" << draw.vertices.size() << '\n';
      check(rows.size() == online_visible, "ONLINE options disappear after hovering row");
      std::map<unsigned, unsigned> glyphs;
      for (std::size_t i = 0; i + 5 < draw.vertices.size(); i += 6) {
        const auto &a = draw.vertices[i], &b = draw.vertices[i + 4];
        if (a.position.z <= 0 || (a.color.x == 0 && a.color.y == 0 && a.color.z == 0) ||
            !mesh.materials.at(a.material).name.starts_with("font/")) continue;
        if (const auto hit = painter.pointer_hit((a.position.x + b.position.x) * .5f,
                                                (a.position.y + b.position.y) * .5f);
            hit && hit->action == FrontendPointerAction::Entry)
          ++glyphs[hit->index];
      }
      for (const auto row : rows) {
        const auto letters = std::count_if(menu.frame().entries[row].label.begin(),
            menu.frame().entries[row].label.end(), [](char c) { return c != ' '; });
        check(glyphs[row] >= letters, "ONLINE hover retained targets but lost visible option text");
      }
      capture("online-hover-" + std::to_string(index));
    }
    MenuState direct_browser(content); direct_browser.set_discord_login(login); direct_browser.open(219, online_server_browser_context);
    check(direct_browser.screen_layout(direct_browser.frame()) == (content.vista ? vista_menu(13) : 219),
          "ONLINE discovery does not use the Network Browser presentation");
    check(!has_logo(direct_browser), "Direct authenticated browser retains the title logo");
    direct_browser.open(237, online_server_browser_context);
    check(!has_logo(direct_browser), "Direct authenticated server details retain the title logo");
    // Native219 retains authentication and directory ownership while PC13
    // supplies the Network Browser's authored multi-column presentation.
    // Optimatch and local session navigation retain their separate routes.
    MenuState optimatch(content); optimatch.set_discord_login(login); optimatch.open(219);
    check(optimatch.screen_layout(optimatch.frame()) == 219, "Network Browser replaced Optimatch");
    for (const auto screen : {13u, vista_menu(13)}) {
      bool rejected = false;
      try { direct_browser.open(screen); } catch (const FormatError &) { rejected = true; }
      check(rejected, "ONLINE presentation re-enabled the retired session browser route");
    }
    using Json = nlohmann::json;
    Json rows = Json::array();
    for (unsigned i = 0; i < 20; ++i) rows.push_back({
      {"server_id", "00000000-0000-0000-0000-0000000000" + std::to_string(i + 10)},
      {"owner_id", "00000000-0000-0000-0000-000000000002"}, {"origin_master_id", "00000000-0000-0000-0000-000000000003"},
      {"registered_at", 1000}, {"last_heartbeat", 1005}, {"expires_at", 1090}, {"reachability", "unverified"},
      {"name", i == 0 ? "Community Server" : "Server " + std::to_string(i + 1)}, {"host", "203.0.113.10"}, {"port", 2002},
      {"map", "lockout"}, {"gametype", "Slayer"}, {"players", 3}, {"max_players", 16},
      {"game_version", "simulator-v1"}, {"visibility", "public"}, {"server_type", "dedicated"}, {"region", "US-EAST"}});
    rows[1]["name"] = "Community Server " + std::string(47, 'W'); // Schema maximum64 bytes.
    auto changed = rows; changed.erase(changed.begin()); changed.back()["map"] = "ascension"; changed.back()["players"] = 7;
    const auto initial = Json{{"servers", rows}}.dump(), refreshed = Json{{"servers", changed}}.dump();
    std::atomic<unsigned> directory_requests{}, phase{};
    std::atomic<bool> hold_directory{true};
    std::atomic<std::int64_t> directory_time{1010};
    auto directory = std::make_shared<OnlineDirectory>(OnlineDirectoryOptions{{"https://master.example"}, false},
      [&](const std::string &endpoint, std::stop_token stop) -> OnlineDirectoryResponse {
        ++directory_requests;
        check(endpoint == "https://master.example/v1/servers", "Frontend discovery sent identity or contacted a server");
        while (hold_directory && !stop.stop_requested()) std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return {phase == 2 ? 503 : 200, phase == 1 ? refreshed : initial};
      }, [&] { return directory_time.load(); });
    MenuState gated(content); gated.set_online_directory(directory); gated.open(219, online_server_browser_context); gated.update_online_directory();
    check(gated.frame().screen == 20 && directory_requests == 0, "Directory bypassed Discord gate");
    menu.set_online_directory(directory);
    menu.select(4); menu.accept();
    check(menu.online_directory_text(menu.frame(), 4) == "Finding servers...", "Network Browser lacks its loading state");
    capture("browser-loading");
    hold_directory = false;
    auto wait_directory = [&] {
      const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(4);
      while (directory->snapshot().status == OnlineDirectoryStatus::Loading && std::chrono::steady_clock::now() < until)
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      check(directory->snapshot().status != OnlineDirectoryStatus::Loading, "Frontend directory fetch timed out");
      menu.update_online_directory();
    };
    wait_directory();
    check(menu.frame().screen == 219 && menu.frame().title == "ONLINE SERVERS" && menu.frame().entries.size() == 20, "ONLINE browser did not populate");
    check(!has_logo(menu), "Retained Halo 2 logo overlaps server discovery");
    check(menu.online_directory_text(menu.frame(), 0).find("lockout") != std::string::npos &&
          menu.online_directory_text(menu.frame(), 0).find("3 / 16") != std::string::npos, "Focused server metadata is missing");
    check(menu.online_directory_column(menu.frame(), 0, 0) == "Community Server" &&
          menu.online_directory_column(menu.frame(), 0, 1) == "lockout" &&
          menu.online_directory_column(menu.frame(), 0, 2) == "Slayer" &&
          menu.online_directory_column(menu.frame(), 0, 3).empty() &&
          menu.online_directory_column(menu.frame(), 0, 4) == "3 / 16" &&
          menu.online_directory_column(menu.frame(), 0, 5) == "Listed" &&
          menu.online_directory_column(menu.frame(), 0, 6).empty(),
          "Network Browser columns do not reflect advertised metadata");
    const auto browser_geometry = [&] {
      if (!content.vista) return;
      const auto &authored = content.vista->screen(13).panes.at(0).lists.at(0);
      check(authored.visible == 19 && authored.skin == 5 && authored.position.x == -670 &&
            authored.position.y == 412, "Network Browser source list was mutated");
      const auto &pane = logical_layout && logical_layout->browser
          ? logical_layout->browser->panes.at(0) : content.vista->screen(13).panes.at(0);
      const auto &list = pane.lists.at(0);
      const auto &skin = content.vista->skins.at(list.skin);
      check(list.visible == (cache.is_mcc()?16:19) && list.skin == 5 && list.position.x == -670 &&
            list.position.y == (cache.is_mcc()?367:412) && skin.texts.size() == 7,
            "Network Browser no longer uses its source list definition");
      const auto draw = painter.draw(menu, 5, -1, 10000);
      const auto has = [&](const std::string &name) {
        return std::any_of(draw.vertices.begin(), draw.vertices.end(), [&](const auto &v) {
          return mesh.materials.at(v.material).name == name;
        });
      };
      check(has("ui\\screens\\game_shell\\network_squad_browser_screen\\list_frame") &&
            has("ui\\list_skins\\game_browser\\item_background"),
            "ONLINE discovery still paints the Optimatch layout");
      const auto first = menu.frame().selected >= list.visible ? menu.frame().selected - list.visible + 1 : 0;
      const auto row = menu.frame().selected - first;
      // Source background height32 determines pitch; text6 spans the entire
      // selection bar, including the map/player/status columns.
      const auto target = painter.pointer_hit(580, 240 - .4f * (list.position.y + 30) + .4f * (float(row) * 32 + 16));
      check(target && target->action == FrontendPointerAction::Entry && target->index == menu.frame().selected,
            "Network Browser row columns do not share the correct mouse target");
    };
    browser_geometry();
    capture("browser");
    menu.select(1); browser_geometry(); capture("browser-long-name");
    menu.select(19); browser_geometry(); capture("browser-scroll");
    menu.accept();
    check(menu.frame().screen == 237 && menu.frame().entries.size() == 8 && !menu.launch, "Server details launched gameplay");
    check(!has_logo(menu), "Retained Halo 2 logo overlaps server details");
    menu.select(7);
    check(menu.online_directory_text(menu.frame(), 2) == "203.0.113.10:2002", "Server address metadata changed");
    menu.accept(); check(!menu.launch && menu.frame().screen == 237, "Details admitted an unimplemented game transport");
    capture("details");
    private_identity(menu);
    menu.back(); phase = 1; menu.auxiliary(); wait_directory();
    check(menu.frame().entries.size() == 19 && menu.frame().selected == 18 && menu.frame().entries[18].label == "Server 20" &&
          menu.online_directory_text(menu.frame(), 0).find("ascension") != std::string::npos, "Refresh lost selected server identity or updated metadata");
    menu.accept(); directory_time = 1090; menu.update_online_directory();
    check(menu.frame().screen == 237 && menu.frame().entries.size() == 1 && menu.frame().entries[0].label == "Back", "Expired server remained in details");
    menu.back();
    check(menu.frame().entries.size() == 1 && menu.frame().entries[0].label == "Refresh", "Expired listing remained selectable");
    browser_geometry(); capture("browser-empty");
    phase = 2; menu.auxiliary(); wait_directory();
    check(menu.online_directory_text(menu.frame(), 2).find("Cannot reach") != std::string::npos, "Failed master lookup lacks retry state");
    browser_geometry(); capture("browser-unavailable");
    phase = 0; directory_time = 1010; menu.auxiliary(); wait_directory();
    check(menu.frame().entries.size() == 20, "Frontend retry did not recover listings");
    menu.back();
    check(menu.frame().screen == 186 && directory->snapshot().status == OnlineDirectoryStatus::Idle && directory->snapshot().servers.empty(), "Leaving browser retained directory work or cached servers");
    capture("online-service");
    if(!party_run) {
    menu.select(0); menu.accept();
    check(menu.frame().screen == 14 && !menu.launch, "Unconfigured Quickmatch skipped its pregame lobby");
    // Direct internal progress-screen coverage keeps an unavailable service
    // honest; the user-facing Quickmatch route above remains lobby-first.
    menu.return_online();menu.open(210);
    check(menu.frame().screen == 210 && !menu.launch && menu.online_matchmaking_text(menu.frame(), 5).find("configured") != std::string::npos, "Unconfigured matchmaking lacks an honest status");
    check(!has_logo(menu), "Title logo overlaps matchmaking status");
    // TU1.5 0x25217B: Cancel opens leave dialog 0x8D; its Cancel keeps
    // searching and Leave Matchmaking returns to the pregame lobby.
    menu.back();
    check(!menu.frame().entries.empty() && menu.frame().entries.front().destination==-16 &&
          menu.frame().title==content.dialogs.at(matchmaking_leave_dialog_local).title,"Queue Cancel did not ask to leave matchmaking");
    menu.back();
    check(menu.frame().screen==210,"Leave dialog Cancel abandoned the search screen");
    }
    // Leave Matchmaking itself is exercised by the queue run; leaving here
    // would form the party that the party run creates explicitly later.
    menu.return_online();
    if (!matchmaking_url.empty()) {
      OnlineMatchmakingOptions queue_options; queue_options.endpoint = matchmaking_url; queue_options.allow_loopback_http = true;
      queue_options.compatibility=gathering_run?"halo2-native-queue-v8":"halo2-native-queue-v7";
      queue_options.maps = {"lockout"}; queue_options.poll_interval = std::chrono::milliseconds(25);
      auto matchmaking = std::make_shared<OnlineMatchmaking>(login, queue_options);
      menu.set_online_matchmaking(matchmaking);
      OnlineAccountOptions queue_account_options;queue_account_options.endpoint=matchmaking_url;
      queue_account_options.allow_loopback_http=true;queue_account_options.enable_parties=true;
      queue_account_options.poll_interval=std::chrono::milliseconds(25);
      auto queue_account=std::make_shared<OnlineAccount>(login,queue_account_options);
      menu.set_online_account(queue_account);menu.update_online_login();
      const auto account_until=std::chrono::steady_clock::now()+std::chrono::seconds(8);
      while(!queue_account->snapshot().authenticated && std::chrono::steady_clock::now()<account_until) {
        menu.update_online_login();std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
      check(queue_account->snapshot().authenticated,"Matchmaking fixture's actual party service did not authenticate");
      unsigned queue_stage{};
      std::uint32_t queue_clock{};
      auto wait_queue = [&](auto predicate) {
        ++queue_stage;
        const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(20);
        for(;;) {
          menu.update_online_login();menu.update_online_matchmaking();
          if(predicate(matchmaking->snapshot()))return;
          if(std::chrono::steady_clock::now()>=until)break;
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        const auto state=matchmaking->snapshot();
        const auto party=queue_account->snapshot();
        nlohmann::json readiness={{"selected_hopper",menu.expand("\xee\x90\xba")},{"queue_playlist",state.playlist_id},{"account_message",party.message},
          {"party_command_serial",party.party_command_serial},{"party_command_successful",party.party_command_successful?nlohmann::json(*party.party_command_successful):nlohmann::json(nullptr)},
          {"catalog",nlohmann::json::array()},{"member_ready",nlohmann::json::array()}};
        for(const auto&p:state.playlists)readiness["catalog"].push_back({{"id",p.id},{"name",p.name},{"revision",p.revision},{"eligible",p.eligible}});
        if(party.party)for(const auto&m:party.party->members)readiness["member_ready"].push_back({{"self",m.self},{"ready_playlist",m.ready_playlist}});
        throw FormatError("Native matchmaking stage "+std::to_string(queue_stage)+" timed out, status="+std::to_string(unsigned(state.status))+": "+state.message+" screen="+std::to_string(menu.frame().screen)+" "+menu.frame().description+
            " party="+std::to_string(bool(party.party))+" busy="+std::to_string(party.busy)+" party_message="+party.party_message+" readiness="+readiness.dump());
      };
      auto check_return_online_lobby=[&] {
        // TU1.5 PCR Done (0x233792) returns party A to pregame lobby14 pane2.
        const auto retained_playlist=menu.expand("\xee\x90\xba"); // Original E43A hopper name.
        const auto retained_party=menu.online_party_observation();
        const auto retained_profile=menu.signed_in_profile();
        check(!retained_playlist.empty() && retained_party && !retained_party->members.empty(),
            "ONLINE return control lost its real party/playlist input");
        menu.return_online_lobby();
        check(menu.frame().screen==14 && menu.stack().size()>=2,"Postgame return skipped the pregame lobby");
        const auto returned_lobby=menu.lobby_view();
        check(returned_lobby && returned_lobby->protocol==2 && returned_lobby->input.session_enum==6 &&
            frontend_lobby_presentation(returned_lobby->input).pane==2 && !menu.launch,
            "Left/abandoned ONLINE match did not return to the original matchmaking pregame pane");
        check(menu.expand("\xee\x90\xba")==retained_playlist && menu.online_party_observation() &&
            menu.online_party_observation()->leader==retained_party->leader &&
            std::equal(menu.online_party_observation()->members.begin(),menu.online_party_observation()->members.end(),
                retained_party->members.begin(),retained_party->members.end(),
                [](const auto&a,const auto&b){return a.handle==b.handle;}) &&
            menu.signed_in_profile()==retained_profile,
            "ONLINE match return discarded its retained playlist, party or profile");
        check(!painter.draw(menu,5,-1,10000).vertices.empty() && !has_logo(menu),
            "Returned pregame lobby did not draw its original menu presentation");
        menu.ui_clock(queue_clock+=60000);menu.update_online_matchmaking();
        check(menu.frame().screen==14 && !menu.launch,"Returned pregame lobby relaunched an abandoned match");
        std::cout<<"PASS left/abandoned ONLINE match -> original pregame lobby14 pane2 with retained party, playlist and profile\n";
      };
      auto start_lobby_search=[&] {
        check(menu.frame().screen==14,"Search did not begin in the pregame lobby");
        wait_queue([&](const auto &s){
          const auto a=queue_account->snapshot();
          if(s.status!=OnlineMatchmakingStatus::Ready || !a.party || a.busy)return false;
          const auto self=std::find_if(a.party->members.begin(),a.party->members.end(),[](const auto &m){return m.self;});
          // A hopper change can retain the preceding accepted consent until
          // the account worker publishes the new ready response.
          return self!=a.party->members.end() && std::any_of(s.playlists.begin(),s.playlists.end(),[&](const auto &p){
            return p.id==self->ready_playlist && p.name==menu.expand("\xee\x90\xba");
          });
        });
        check(matchmaking->snapshot().playlist_id.empty(),"Selecting a playlist enqueued before lobby START GAME");
        const auto party=queue_account->snapshot().party;
        const auto self=std::find_if(party->members.begin(),party->members.end(),[](const auto &m){return m.self;});
        const auto catalog=matchmaking->snapshot().playlists;
        const auto hopper=std::find_if(catalog.begin(),catalog.end(),[&](const auto &p){return self!=party->members.end() && p.id==self->ready_playlist;});
        check(hopper!=catalog.end() && menu.expand("\xee\x90\xba")==hopper->name &&
            menu.online_party_text(menu.frame(),1)==hopper->description &&
            menu.online_party_text(menu.frame(),3).find(hopper->name)!=std::string::npos,
            "Pregame hopper text did not bind its actual accepted playlist");
        const auto lobby_draw=painter.draw(menu,5,-1,10000);
        audit_vista(menu,lobby_draw);
        for(const auto name:{"ui\\screens\\game_shell\\pregame_lobby\\matchmaking",
                             "ui\\screens\\game_shell\\pregame_lobby\\matchmaking2"}) {
          const bool original_lobby=cache.is_mcc() && detail::og_matchmaking_layout::goal_frame(menu.frame(),&menu);
          const auto actual_name=original_lobby?detail::og_matchmaking_layout::embedded_image_name(name):std::string(name);
          check(std::any_of(lobby_draw.vertices.begin(),lobby_draw.vertices.end(),[&](const auto &v){
              return mesh.materials.at(v.material).name==actual_name &&
                  (!original_lobby || original_bitmaps.at(v.material));}),
              "Matchmaking lobby hid its authored matchmaking illustration");
        }
        const auto start=std::find_if(menu.frame().entries.begin(),menu.frame().entries.end(),[](const auto &e){return e.destination==-17;});
        check(start!=menu.frame().entries.end(),"Authenticated party lobby lost START GAME");
        menu.select(unsigned(start-menu.frame().entries.begin()));menu.accept();
        wait_queue([&](const auto &){return !queue_account->snapshot().busy && menu.online_lobby_countdown()>0;});
        check(menu.online_lobby_countdown()<=10,"START GAME did not use the original published countdown");
        while(menu.online_lobby_countdown()>3) {
          const auto before=menu.online_lobby_countdown();menu.accept();
          wait_queue([&](const auto &){return !queue_account->snapshot().busy && menu.online_lobby_countdown()<before;});
        }
        wait_queue([&](const auto &){return menu.frame().screen==210;});
        check(menu.frame().screen==210,"Lobby countdown did not enter matchmaking at zero");
      };
      // TU1.5 0x25217B/0x252150: Cancel asks with dialog 0x8D (no remote
      // party members); Leave Matchmaking returns to pregame lobby14 pane2.
      auto leave_matchmaking=[&] {
        check(menu.frame().screen==210,"Leave requested outside screen210");
        menu.back();
        const auto &leave=content.dialogs.at(matchmaking_leave_dialog_local);
        check(menu.frame().screen==leave.screen && menu.frame().title==leave.title && menu.frame().entries.size()==2 &&
              menu.frame().entries[0].destination==-16,"Cancel did not open the TU1.5 leave-matchmaking dialog");
        menu.select(0);menu.accept();
        // The pregame lobby keeps the queue client connected but not queued.
        wait_queue([](const auto &s){return s.status==OnlineMatchmakingStatus::Ready || s.status==OnlineMatchmakingStatus::Idle;});
        check(menu.frame().screen==14,"Leave Matchmaking did not return to the pregame lobby");
        menu.return_online();
        check(menu.frame().screen==186,"ONLINE menu lost after leaving matchmaking");
      };
      menu.select(0); menu.accept();
      check(menu.frame().screen==14 && !menu.launch,"Quickmatch skipped the original pregame lobby");
      start_lobby_search();
      wait_queue([](const auto &s) { return s.status == OnlineMatchmakingStatus::Searching; });
      check(matchmaking->snapshot().playlist_id == "ranked" && menu.frame().screen == 210 && !menu.launch, "Quickmatch did not use server default");
      if(return_lobby_run) {
        check_return_online_lobby();
        wait_queue([](const auto &s){return s.playlist_id.empty() &&
            (s.status==OnlineMatchmakingStatus::Ready || s.status==OnlineMatchmakingStatus::Idle);});
        check(menu.frame().screen==14 && !menu.launch,"Abandoned queue publication replaced the returned lobby");
        menu.set_online_matchmaking({});menu.set_online_account({});
        return 0;
      }
      auto check_ranks=[&](std::vector<unsigned> pages) {
        const auto draw=painter.draw(menu,5);
        std::vector<unsigned> found;
        std::vector<Vertex> suppressed;
        std::vector<unsigned> icons;
        for(unsigned i=0;i<draw.vertices.size();i+=6) {
          const auto &material=mesh.materials.at(draw.vertices[i].material);
          const auto rank_bank=cache.is_mcc()?detail::og_matchmaking_layout::embedded_image_name("ui\\global_bitmaps\\rank_icons_sm"):
              std::string("ui\\global_bitmaps\\rank_icons_sm");
          if(material.name!=rank_bank) {
            suppressed.insert(suppressed.end(),draw.vertices.begin()+i,draw.vertices.begin()+i+6);
            continue;
          }
          icons.push_back(i);
          bool matched=false;
          const auto inspect=[&](const FrontendContent &owner) {
            for(const auto &[id,bank]:owner.images)if(bank.name==material.name)
            {
              check(bank.images.size()==50,"Authored rank artwork bank is incomplete");
              for(unsigned page=0;page<bank.images.size();++page) {
                const auto &image=bank.images[page];
                if(image.width==material.diffuse.width && image.height==material.diffuse.height &&
                    image.format==material.diffuse.format && image.pixels==material.diffuse.pixels) {
                  if(!matched)found.push_back(page);matched=true;return;
                }
              }
            }
          };
          inspect(native_layout);if(!matched)inspect(content);if(!matched && content.vista)inspect(*content.vista);
          check(matched,"Rank artwork is not an admitted source page");
        }
        std::sort(found.begin(),found.end());std::sort(pages.begin(),pages.end());
        if(found!=pages)throw FormatError("Native matchmaking rank pages "+nlohmann::json(found).dump()+
            " differ from authenticated playlist pages "+nlohmann::json(pages).dump());
        if(!icons.empty() && std::getenv("DISPLAY")) {
          const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
          check(scratch,"Rank GPU proof requires owned test scratch");
          Renderer renderer(capture_desktop->window(640,480,true),mesh,capture_validation);
          auto capture_rank=[&](const auto &vertices,const char *name) {
            const auto path=std::filesystem::path(scratch)/name;
            bool complete=false;
            for(unsigned attempt=0;attempt<4 && !complete;++attempt)
              complete=renderer.draw(Camera{},path,{},false,{},{},vertices);
            check(complete,"Rank GPU capture did not complete");
            std::ifstream file(path,std::ios::binary);std::string kind;unsigned w,h,max;
            file>>kind>>w>>h>>max;file.get();
            check(kind=="P6" && w==640 && h==480 && max==255,"Invalid rank framebuffer");
            std::vector<unsigned char> pixels(w*h*3);
            file.read(reinterpret_cast<char*>(pixels.data()),pixels.size());
            check(bool(file),"Incomplete rank framebuffer");return pixels;
          };
          const auto actual=capture_rank(draw.vertices,"rank.ppm"),absent=capture_rank(suppressed,"rank-hidden.ppm");
          for(auto first:icons) {
            const auto &a=draw.vertices[first],&b=draw.vertices[first+4];unsigned changed=0;
            for(int y=std::max(0,int(a.position.y));y<std::min(480,int(b.position.y)+1);++y)
              for(int x=std::max(0,int(a.position.x));x<std::min(640,int(b.position.x)+1);++x) {
                const auto at=(y*640+x)*3;
                changed+=!std::equal(actual.begin()+at,actual.begin()+at+3,absent.begin()+at);
              }
            check(changed>0,"Selected rank page produced no framebuffer pixels");
            std::cout<<"rank page="<<found.at(0)<<" changed_pixels="<<changed<<'\n';
          }
        }
      };
      if(rank_run) {
        check(menu.online_matchmaking_rank(0)==36,"Service level37 did not map to TU1.5 page36");
        check_ranks({36});
      }
      check(!has_logo(menu), "Title logo overlaps Quickmatch"); private_identity(menu); capture("quickmatch-searching");
      if(gathering_run) {
        wait_queue([](const auto &s){return s.status==OnlineMatchmakingStatus::Searching && s.gathering && s.gathering->players.size()==2;});
        check(menu.online_matchmaking_pane()==1 && menu.frame().entries.size()==8 &&
          menu.online_matchmaking_text(menu.frame(),0)=="Waiting for additional players...",
          "Actual gathering did not select the authored Vista waiting pane/title");
        check(menu.frame().entries[0].label.find("Pilot")!=std::string::npos &&
              menu.frame().entries[1].label=="Matched Player" &&
              menu.frame().entries[1].value==matchmaking_matched_player_row &&
              std::all_of(menu.frame().entries.begin()+2,menu.frame().entries.end(),[](const auto &e) {
                return e.label.empty() && e.value==matchmaking_empty_player_row;
              }),"Ranked actual gathering did not keep opponents anonymous and unfilled slots blank");
        check(menu.online_matchmaking_player_appearance(0)==menu.profile_appearance(),
          "Party A lost the selected profile's actual appearance");
        for(unsigned row=1;row<menu.frame().entries.size();++row) {
          check(!menu.online_matchmaking_rank(row),"A matched/blank slot fabricated a player's rank");
          check(!menu.online_matchmaking_player_appearance(row),"A matched/blank slot disclosed or invented a player's emblem");
        }
        // TU1.5 257A11/257A86 select player_in page1/page0 on the
        // source group bitmap1; placeholders have no speaker/emblem state.
        const auto placeholder_draw=painter.draw(menu,5);
        std::vector<unsigned> placeholder_pages;
        std::vector<float> placeholder_top;
        for(unsigned first=0;first<placeholder_draw.vertices.size();first+=6) {
          const auto &material=mesh.materials.at(placeholder_draw.vertices[first].material);
          const auto placeholder_bank=cache.is_mcc()?detail::og_matchmaking_layout::embedded_image_name("ui\\list_skins\\matchmaking\\player_in"):
              std::string("ui\\list_skins\\matchmaking\\player_in");
          if(material.name!=placeholder_bank)continue;
          placeholder_top.push_back(placeholder_draw.vertices[first].position.y);
          bool matched=false;
          const auto inspect=[&](const FrontendContent &owner) {
            for(const auto &[id,bank]:owner.images)if(bank.name==material.name)
              for(unsigned page=0;page<bank.images.size();++page) {
                const auto &image=bank.images[page];
                if(!matched && image.width==material.diffuse.width && image.height==material.diffuse.height &&
                    image.format==material.diffuse.format && image.pixels==material.diffuse.pixels) {
                  placeholder_pages.push_back(page);matched=true;
                }
              }
          };
          inspect(native_layout);if(!matched)inspect(content);if(!matched && content.vista)inspect(*content.vista);
          check(matched,"Matched/empty slot artwork is not an admitted source page");
        }
        std::sort(placeholder_pages.begin(),placeholder_pages.end());
        check(placeholder_pages==std::vector<unsigned>{0,0,0,0,0,0,1},
              "Actual ranked gathering did not draw six page0 blanks and one page1 matched slot");
        if(native_layout.original_matchmaking_layout){
          // Actual2BAFA4 unions all children, including the nested name group.
          // A largest-single-child-height approximation compresses every row.
          const auto &source_pane=native_layout.screen(210).panes.at(menu.online_matchmaking_progress_pane());
          const auto &skin=native_layout.skins.at(source_pane.lists.front().skin);
          const auto &group=skin.grids.front();
          const auto &name=native_layout.skin_groups.at(group.skin).texts.front();
          const auto &speaker=skin.bitmaps.front();
          const float top=name.bounds.top+group.position.y;
          const float bottom=speaker.position.y;
          check(top==28 && bottom==6,"Original matchmaking roster child bounds lost authenticated22-pixel union");
          std::sort(placeholder_top.begin(),placeholder_top.end());
          check(placeholder_top.size()==7,"Original roster omitted a placeholder before pitch verification");
          for(unsigned row=1;row<placeholder_top.size();++row)
            check(std::abs(placeholder_top[row]-placeholder_top[row-1]-(top-bottom))<.001f,
                "Actual original matchmaking rows do not advance by the complete child union");
        }
        if(rank_run)check_ranks({36});
        private_identity(menu);capture("quickmatch-waiting");
      }
      leave_matchmaking();
      menu.select(1); menu.accept();
      wait_queue([](const auto &s) { return s.status == OnlineMatchmakingStatus::Ready; });
      check(menu.frame().title == "CHOOSE MATCHMAKING PLAYLIST" && menu.frame().entries.size() == 4, "Optimatch did not load authenticated playlists");
      capture("playlists");
      for(unsigned row=2;row<4;++row) {
        menu.select(row);menu.accept();
        check(menu.frame().screen==219 && menu.online_matchmaking_text(menu.frame(),2).find("Clan membership required")!=std::string::npos,
          "Nonmember clan details lost the membership requirement");
        check(!has_logo(menu),"Title logo overlaps clan playlist details");private_identity(menu);
        capture(row==2?"minor-clan-playlist":"major-clan-playlist");
      }
      menu.select(1);
      const auto casual_catalog=matchmaking->snapshot().playlists;
      const auto casual=std::find_if(casual_catalog.begin(),casual_catalog.end(),[](const auto&p){return p.id=="casual";});
      check(casual!=casual_catalog.end() && !casual->ranked && casual->eligible && menu.frame().screen==219 &&
          menu.online_matchmaking_text(menu.frame(),1)==casual->name &&
          menu.online_matchmaking_text(menu.frame(),2)==casual->description,
          "Selected casual hopper lost its actual catalog name or description");
      check(menu.online_matchmaking_text(menu.frame(), 2).find("level") == std::string::npos, "Casual skill leaked into UI");
      capture("playlist-details"); menu.accept();
      check(menu.frame().screen==14 && !menu.launch,"Optimatch selection skipped the original pregame lobby");
      start_lobby_search();
      wait_queue([](const auto &s) { return s.status == OnlineMatchmakingStatus::CheckingConnections; });
      if(rank_run)check_ranks({});
      check(matchmaking->snapshot().playlist_id == "casual" && menu.frame().entries.size() == 1 && !menu.launch, "Attempting to join did not retain party A's roster");
      check(menu.online_matchmaking_pane()==2 && menu.online_matchmaking_text(menu.frame(),0)=="Attempting to join a game...",
        "Native admitted match did not bind Vista's join pane/title");
      check(menu.frame().entries[0].label.find("Pilot") != std::string::npos, "Party A's chosen name is missing from join roster");
      check(!has_logo(menu), "Title logo overlaps match roster"); private_identity(menu); capture("match-checking-connections");
      wait_queue([](const auto &s) { return s.status == OnlineMatchmakingStatus::Loading; });
      // B publishes one shared start clock after all peers become ready.
      // Observers receive its remaining time; their own UI clocks cannot
      // advance or restart that countdown.
      menu.update_online_matchmaking();
      check(!menu.launch && menu.online_matchmaking_ui()==MatchmakingProgressUi::start_match &&
            menu.online_matchmaking_progress_pane()==3 && menu.online_matchmaking_countdown()>0 &&
            menu.online_matchmaking_countdown()<=10 && matchmaking->snapshot().start_countdown_supported,
            "Ready match skipped the session-published TU1.5 start countdown");
      check(menu.frame().entries.size()==2 &&
            std::any_of(menu.frame().entries.begin(),menu.frame().entries.end(),[](const auto &e){return e.label=="Peer Pilot";}),
            "Countdown pane3 did not reveal game B's admitted roster");
      {const auto sounds=menu.take_ui_sounds();
       check(std::count(sounds.begin(),sounds.end(),FrontendUiSound::MatchmakingAdvance)==1,"Entering pane3 did not request UI sound12 once");}
      check(menu.online_matchmaking_text(menu.frame(),0).starts_with("Game starts in "+matchmaking_countdown_text(menu.online_matchmaking_countdown())+"\n"),
            ("Pane3 lost its countdown: "+menu.online_matchmaking_text(menu.frame(),0)).c_str());
      check(menu.online_matchmaking_text(menu.frame(),0).find("Slayer")!=std::string::npos &&
            menu.online_matchmaking_text(menu.frame(),0).find("on ")!=std::string::npos,"Pane3 lost its variant/map lines");
      if(detail::mcc_frontend_content(content)) {
        const auto map=std::find_if(content.multiplayer_maps.begin(),content.multiplayer_maps.end(),
            [](const auto&m){return m.scenario_path=="scenarios\\multi\\lockout\\lockout";});
        check(map!=content.multiplayer_maps.end() && menu.online_matchmaking_map_bitmap()==map->bitmap,
            "Matched Lockout preview lost its actual scenario-owned bitmap datum");
        const auto settled=painter.draw(menu,5,-1,menu.frame().pane_created_clock+2000);
        const auto alias="ui\\native_og_matchmaking\\map_previews\\"+map->scenario_path+"#og-matchmaking";
        bool rendered=false;
        for(const auto &vertex:settled.vertices) {
          const auto &material=mesh.materials.at(vertex.material);
          check(material.name!="ui\\global_bitmaps\\map_shots\\map_mp_zanzibar#og-matchmaking",
              "Matched pane3 still submitted its unbound authored Zanzibar preview");
          if(material.name==alias) {
            check(material.diffuse.width==220 && material.diffuse.height==207,
                "Actual matched map draw lost its authenticated original raster dimensions");rendered=true;
          }
        }
        check(rendered,"Actual matched pane3 draw did not submit its exact Lockout source raster alias");
      }
      private_identity(menu);capture("match-countdown");
      menu.back();
      check(menu.frame().screen==210 && menu.take_back_refused() && menu.take_ui_sounds()==std::vector{FrontendUiSound::Error} &&
            matchmaking->snapshot().status==OnlineMatchmakingStatus::Loading,
            "Found game accepted Cancel inside its one-minute lock");
      menu.ui_clock(queue_clock+=50000);menu.update_online_matchmaking();
      check(!menu.launch && menu.online_matchmaking_countdown()>0,
            "Advancing the local UI clock released a session whose countdown was still running");
      const auto countdown_end=std::chrono::steady_clock::now()+std::chrono::seconds(12);
      while(!menu.launch && std::chrono::steady_clock::now()<countdown_end) {
        menu.update_online_matchmaking();std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
      check(menu.launch && menu.launch->online_match && menu.launch->map=="lockout.map" &&
          menu.launch->multiplayer && menu.launch->local_players==1 && menu.launch->variant.name=="Slayer" &&
          menu.online_matchmaking_ui()==MatchmakingProgressUi::have_fun,
          "Native matchmaking did not hand its authenticated match into gameplay after the countdown");
      capture("match-loading");
      menu.launch.reset();
      menu.ui_clock(queue_clock+=60000);menu.update_online_matchmaking();
      check(!menu.launch,"A launched match was handed to gameplay twice");
      leave_matchmaking();
      menu.select(2); menu.accept();
      check(menu.frame().screen == 14 && queue_account->snapshot().party, "Custom Game failed to reopen the accepted party lobby");
      capture("online-lobby");
      check(menu.frame().entries.size()==2 && menu.frame().entries[1].destination==23,
            "Accepted party lobby lost the original two-button Game Setup route");
      menu.select(1);menu.accept();
      check(menu.frame().screen==23 && menu.frame().entries.size()==3 && menu.frame().entries[0].value==9 &&
            menu.frame().entries[1].value==6 && menu.frame().entries[2].value==7,
            "ONLINE Game Setup lost its original squad-settings choices");
      menu.select(0);menu.accept();
      wait_queue([](const auto &s) { return s.status == OnlineMatchmakingStatus::Ready; });
      check(menu.frame().screen == 237 && menu.frame().title == "CHOOSE MATCHMAKING PLAYLIST", "Native lobby Game Setup did not reach the original lobby playlist selector");
      menu.return_online();
      menu.select(0); menu.accept();
      start_lobby_search();
      wait_queue([](const auto &s) { return s.playlist_id == "casual" && s.match.has_value(); });
      check(!menu.launch, "Quickmatch launched before connection admission");
      {
        std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> fault(curl_easy_init(),curl_easy_cleanup);
        const auto url=matchmaking_url+"/fixture/fail-current-search";
        curl_easy_setopt(fault.get(),CURLOPT_URL,url.c_str());curl_easy_setopt(fault.get(),CURLOPT_PROXY,"");
        curl_easy_setopt(fault.get(),CURLOPT_POST,1L);curl_easy_setopt(fault.get(),CURLOPT_POSTFIELDS,"");
        curl_easy_setopt(fault.get(),CURLOPT_TIMEOUT_MS,3000L);curl_easy_setopt(fault.get(),CURLOPT_WRITEFUNCTION,discard);
        check(curl_easy_perform(fault.get())==CURLE_OK,"Could not deliver the deliberate room failure");
        long status{};curl_easy_getinfo(fault.get(),CURLINFO_RESPONSE_CODE,&status);
        check(status==200,"Room failure fixture rejected the current authenticated search");
      }
      wait_queue([&](const auto &){return menu.frame().title==content.dialogs.at(0xab).title &&
        menu.frame().description==content.dialogs.at(0xab).message;});
      check(!menu.launch && menu.online_matchmaking_countdown()==matchmaking_countdown_none,
        "Search failure retained launch/countdown authority");
      capture("matchmaking-search-failed");menu.back();
      check(menu.frame().screen==14,"Search failure did not return to the original pregame lobby");
      start_lobby_search();
      bool content_rejected{};
      menu.set_multiplayer_map_validator([&](const FrontendLaunch &){content_rejected=true;throw FormatError("Required cache content unavailable in admission control");});
      wait_queue([&](const auto &){return menu.frame().title==content.dialogs.at(0xac).title &&
        menu.frame().description==content.dialogs.at(0xac).message;});
      check(content_rejected && !menu.launch && menu.online_matchmaking_countdown()==matchmaking_countdown_none,
        "Actual content rejection did not clear launch/countdown authority");
      capture("matchmaking-missing-content");menu.set_multiplayer_map_validator({});menu.back();
      check(menu.frame().screen==14,"Missing content did not return to the original pregame lobby");
      start_lobby_search();
      wait_queue([](const auto &s){return s.playlist_id=="casual" && s.match.has_value();});
      check(!menu.launch,"Fresh retry bypassed connection/start admission");
      leave_matchmaking();
      std::cout<<"PASS real room failure -> original AB -> fresh pregame retry; actual cache-admission rejection -> original AC -> fresh pregame retry/cancel\n";
      check_return_online_lobby();
      menu.return_online();
      menu.set_online_matchmaking({});menu.set_online_account(account);menu.update_online_login();
    }
    if(party_run) {
      const auto discard_old_sounds=menu.take_ui_sounds();
      {
        AudioEngine mixer(false); // SDL device stays paused; render drives the real callback.
        std::set<std::uint32_t> decoded;
        for(const auto datum:content.sounds)if(datum!=0xffffffffu && decoded.insert(datum).second) {
          const auto clip=std::make_shared<PcmSound>(decode_sound(cache.load_sound(datum)));
          check(clip->frames()>0 && clip->rate>0,"Authored party/menu sound failed to decode");
          mixer.stop();mixer.play(clip,frontend_ui_sound_request_scale);
          const auto samples=mixer.render(clip->frames()*48000/clip->rate+1024);
          check(std::any_of(samples.begin(),samples.end(),[](float sample){return std::abs(sample)>0;}),
                "Authored party/menu cue produced silent native mixer output");
        }
        check(decoded.size()==10,"Authored menu sound bank lost a cue or its aliases");
      }
      auto verify_party_rows=[&](unsigned players) {
        const auto draw=painter.draw(menu,5,-1,10000);
        const auto &pane=native_layout.screen(14).panes.at(2);
        const auto &skin=native_layout.skin_groups.at(pane.grids.front().skin);
        const auto background=[&](std::uint32_t id) {
          const auto tag=detail::frontend_compiled_string_id(content,id);
          const auto b=std::find_if(skin.bitmaps.begin(),skin.bitmaps.end(),[&](const auto &b){return b.placeholder==tag;});
          check(b!=skin.bitmaps.end(),"Source player skin lost its background binding");
          return native_layout.images.at(b->datum).name;
        };
        const auto primary=background(0x1400069a);
        const auto box=cache.is_mcc()?detail::og_matchmaking_layout::embedded_image_name("ui\\list_skins\\matchmaking\\player_in"):
            std::string("ui\\list_skins\\matchmaking\\player_in");
        const auto blank=cache.is_mcc()?detail::og_matchmaking_layout::embedded_image_name("ui\\list_skins\\matchmaking\\item_background"):
            std::string("ui\\list_skins\\matchmaking\\item_background");
        unsigned emblems{},stripes{},empty{},boxes{},speakers{};
        std::set<unsigned> banks;
        const auto speaker=native_layout.images.at(pane.bitmaps.at(21).datum).name;
        for(unsigned i=0;i+5<draw.vertices.size();i+=6) {
          const auto &m=mesh.materials.at(draw.vertices[i].material);
          if(m.ui_emblem_pass){++emblems;banks.insert(draw.vertices[i].material);}
          stripes+=m.name==primary;empty+=m.name==blank;boxes+=m.name==box;speakers+=m.name==speaker;
        }
        check(emblems==players && banks.size()==players,"Party rows omitted or shared profile emblem banks");
        check(stripes==players && empty==0 && boxes==0,
              "Pregame party rows differ from the actual occupied-player count");
        check(speakers<=players,"An empty party slot drew a speaker icon");
        if(party_render && std::getenv("DISPLAY") && std::getenv("HALO2_TEST_SCRATCH")) {
          auto *window=capture_desktop->window(640,480,true);
          check(capture_desktop->apply_video(640,480,false).empty(),"Party render mode failed");
          if(!capture_renderer)capture_renderer=std::make_unique<Renderer>(window,mesh,capture_validation);
          else capture_renderer->resized();
          const auto path=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/
              ("party-roster-smoke-"+std::to_string(players)+".ppm");
          capture_renderer->update_material_constants(mesh.materials);
          bool captured{};
          for(unsigned attempt=0;attempt<3 && !captured;++attempt)
            captured=capture_renderer->draw(Camera{},path,{},false,{}, {},draw.vertices);
          check(captured,"Native Vulkan party roster capture failed");
          std::ifstream pixels(path,std::ios::binary);
          std::string magic;unsigned width{},height{},maximum{};
          pixels>>magic>>width>>height>>maximum;pixels.get();
          check(magic=="P6" && width==640 && height==480 && maximum==255,"Invalid native party capture");
          std::vector<unsigned char> actual(width*height*3);
          pixels.read(reinterpret_cast<char*>(actual.data()),actual.size());check(bool(pixels),"Incomplete party framebuffer");
          std::vector<Vertex> suppressed;
          for(unsigned i=0;i+5<draw.vertices.size();i+=6)
            if(!mesh.materials.at(draw.vertices[i].material).ui_emblem_pass)
              suppressed.insert(suppressed.end(),draw.vertices.begin()+i,draw.vertices.begin()+i+6);
          const auto control=path.parent_path()/"party-roster-without-emblems.ppm";
          captured=false;
          for(unsigned attempt=0;attempt<3 && !captured;++attempt)
            captured=capture_renderer->draw(Camera{},control,{},false,{}, {},suppressed);
          check(captured,"Party emblem framebuffer control failed");
          std::ifstream comparison(control,std::ios::binary);
          comparison>>magic>>width>>height>>maximum;comparison.get();
          std::vector<unsigned char> absent(actual.size());
          comparison.read(reinterpret_cast<char*>(absent.data()),absent.size());check(bool(comparison),"Incomplete emblem control");
          for(unsigned i=0;i+5<draw.vertices.size();i+=6) {
            if(!mesh.materials.at(draw.vertices[i].material).ui_emblem_pass)continue;
            const auto &a=draw.vertices[i],&b=draw.vertices[i+4];unsigned changed{};
            for(int y=std::max(0,int(a.position.y));y<std::min(480,int(b.position.y)+1);++y)
              for(int x=std::max(0,int(a.position.x));x<std::min(640,int(b.position.x)+1);++x) {
                const auto offset=(y*640+x)*3;
                changed+=!std::equal(actual.begin()+offset,actual.begin()+offset+3,absent.begin()+offset);
              }
            check(changed>0,"Party profile emblem submitted no visible framebuffer pixels");
          }
          std::cout<<"PASS native party roster pixels "<<path<<std::endl;
        }
      };
      unsigned party_wait{};
      auto wait_party=[&](auto predicate) {
        ++party_wait;
        const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(15);
        while(!predicate(account->snapshot()) && std::chrono::steady_clock::now()<until){menu.update_online_login();std::this_thread::sleep_for(std::chrono::milliseconds(5));}
        menu.update_online_login();
        if(!predicate(account->snapshot()))
          throw FormatError("Native party flow timed out at wait "+std::to_string(party_wait)+" on screen "+
                            std::to_string(menu.frame().screen)+": "+account->snapshot().party_message+" / "+account->snapshot().message);
      };
      auto choose=[&](const std::string &label) {
        const auto &entries=menu.frame().entries;
        const auto it=std::find_if(entries.begin(),entries.end(),[&](const auto &e){return e.label==label;});
        std::string present;for(const auto &e:entries)present+=" ["+e.label+"]";
        check(it!=entries.end(),("Party action missing: "+label+" on screen "+std::to_string(menu.frame().screen)+present).c_str());
        menu.select(unsigned(it-entries.begin()));menu.accept();
      };
      menu.select(2);menu.accept();
      wait_party([](const auto &s){return s.party && !s.busy;});
      check(menu.frame().screen==14 && account->snapshot().party->leader,"Create Party lost native leader lobby");
      check(menu.take_ui_sounds()==std::vector{FrontendUiSound::Advance},"Accepted party entry did not request its authored UI cue once");
      menu.update_online_login();
      check(menu.take_ui_sounds().empty(),"Unchanged party polling repeated the join sound");
      const auto party_draw=painter.draw(menu,5);
      check(std::any_of(party_draw.vertices.begin(),party_draw.vertices.end(),[&](const auto &v){
        return mesh.materials.at(v.material).ui_emblem_pass;
      }),"ONLINE party lobby omitted the actual local profile emblem");
      verify_party_rows(1);
      check(!has_logo(menu),"Title logo overlaps ONLINE party");capture("party-created");
      // The service retains this party across sign-ins. A profile that signs
      // in while still a member must land on ONLINE menu186 like any other
      // profile; Custom Game then re-enters the same leader lobby.
      menu.return_online();
      account=std::make_shared<OnlineAccount>(login,account_options);menu.set_online_account(account);
      wait_party([](const auto &s){return s.authenticated && s.party && !s.busy;});
      check(menu.frame().screen==186,"Retained service party skipped ONLINE menu186 into lobby14");
      check(menu.frame().entries.at(0).destination==210 && menu.frame().entries.at(1).destination==219,
            "Retained service party changed the ONLINE Quickmatch/Optimatch routes");
      menu.select(2);menu.accept();
      check(menu.frame().screen==14 && account->snapshot().party->leader,"Custom Game lost the retained leader lobby");
      (void)menu.take_ui_sounds();
      menu.auxiliary();capture("party-options");choose("Join Party Voice");
      const auto voice_until=std::chrono::steady_clock::now()+std::chrono::seconds(4);
      const auto party_voice_joined=[&] {
        menu.update_online_login();menu.auxiliary();
        // This actual action exists only when OnlineVoice::snapshot.joined
        // is true. Authored lobby status text0 is owned by countdown/errors;
        // it must not be replaced with a synthetic voice connection message.
        const bool joined=std::any_of(menu.frame().entries.begin(),menu.frame().entries.end(),
            [](const auto &entry){return entry.destination==-34 && entry.value==1;});
        menu.back();return joined;
      };
      bool joined=party_voice_joined();
      while(!joined && std::chrono::steady_clock::now()<voice_until){std::this_thread::sleep_for(std::chrono::milliseconds(5));joined=party_voice_joined();}
      check(joined,"Accepted party voice did not publish its actual joined-only microphone action");
      menu.voice_push_to_talk(true);capture("party-voice");menu.voice_push_to_talk(false);
      menu.auxiliary();choose("Mute Microphone");menu.auxiliary();capture("party-voice-options");choose("Unmute Microphone");
      menu.auxiliary();choose("Invite Player");
      check(menu.party_invite_picker() && menu.party_invite_tab()==0,"Invite Player did not open game friends tab");
      capture("party-invite-friends-tab");
      const auto tab_revision=menu.revision;
      check(menu.activate_pointer({FrontendPointerAction::InviteTab,1,tab_revision}) && menu.party_invite_tab()==1,
            "Clan tab header did not switch invite source");
      check(!menu.activate_pointer({FrontendPointerAction::InviteTab,0,tab_revision}),"Stale invite-tab geometry changed source");
      capture("party-invite-clan-tab");
      check(menu.switch_invite_tab(-1) && menu.party_invite_tab()==0,"Shoulder tab navigation did not return to game friends");
      choose("Invite by Friend Tag");
      check(menu.frame().screen==167 && menu.frame().title=="INVITE TO PARTY","Party invitation reused clan/name keyboard purpose");
      capture("party-invite-entry");menu.back();
      {
        const auto tags=std::getenv("H2_TEST_BLOCKED_FRIEND_TAGS");check(tags,"Missing owned blocked-player tags");
        for(const auto &tag:nlohmann::json::parse(tags)) {
          check(account->friend_command("search",tag.get<std::string>()),"Blocked-player fixture search denied");
          wait_party([](const auto &s){return s.friend_search.has_value() && !s.busy;});
          const auto target=account->snapshot().friend_search->id;
          check(account->friend_command("block",target),"Blocked-player fixture command denied");
          wait_party([&](const auto &s){return !s.busy && std::any_of(s.friends.begin(),s.friends.end(),[&](const auto &f){return f.id==target && f.state=="blocked";});});
        }
        menu.open(216,43);menu.select(2);
        const auto selected=account->snapshot().friends.at(unsigned(menu.frame().entries.at(2).value)).id;
        const auto revision=account->snapshot().revision;
        wait_party([&](const auto &s){return s.revision>revision;});
        const auto index=unsigned(menu.frame().entries.at(menu.frame().selected).value);
        check(menu.frame().entries.at(menu.frame().selected).destination==31 &&
              account->snapshot().friends.at(index).id==selected,
              "Blocked-player refresh selected another account with the same display name");
        menu.accept();
        check(menu.frame().context==43 && menu.frame().entries.front().label=="Unblock Player",
              "Blocked-player selection lost its account-specific action");
        menu.back();menu.back();
      }
      menu.open(26,0);choose("Find Player by Friend Tag");
      check(menu.frame().screen==167 && menu.frame().context==11,"Friend search did not open its keyboard");
      menu.select(44);menu.accept();
      const auto hash=std::find_if(menu.frame().entries.begin(),menu.frame().entries.end(),[](const auto &e){return e.label=="#";});
      check(hash!=menu.frame().entries.end(),
            "Controller keyboard cannot enter a friend tag's # separator");
      menu.select(unsigned(hash-menu.frame().entries.begin()));menu.accept();
      check(menu.keyboard_text=="#","Controller extra-symbol key did not enter the original # character");
      menu.erase_text();menu.select(44);menu.accept();
      const auto peer_tag=std::getenv("H2_TEST_PEER_FRIEND_TAG");check(peer_tag,"Missing owned peer friend tag");
      menu.text(peer_tag);menu.commit_text();
      wait_party([](const auto &s){return s.friend_search.has_value() && !s.busy;});
      check(menu.frame().screen==31 && menu.frame().context==46,"Friend search lost its result dialog");
      capture("party-friend-search");choose("Send Friend Request");
      wait_party([](const auto &s){return !s.busy && std::any_of(s.friends.begin(),s.friends.end(),[](const auto &f){return f.state=="friend";});});
      choose("Pilot");
      check(menu.frame().screen==31 && menu.frame().context==43,"Native friend selection lost options");
      capture("party-native-friend-options");choose("Invite To Party");
      wait_party([](const auto &s){return s.party && s.party->members.size()==2 && !s.busy;});
      check(menu.take_ui_sounds()==std::vector{FrontendUiSound::Advance},"Remote accepted member did not request one party join sound");
      check(menu.frame().screen==26 && menu.frame().context==2,"Invitation completion lost party members tab");
      check(account->snapshot().party->members[0].handle!=account->snapshot().party->members[1].handle &&
            account->snapshot().party->members[0].name==account->snapshot().party->members[1].name,"Duplicate chosen names changed party authority");
      private_identity(menu);capture("party-members");
      while(menu.frame().screen!=14)menu.back();
      wait_party([](const auto &s){return s.party && s.party->members.size()==2 && s.party->members[1].appearance.has_value();});
      check(account->snapshot().party->members[1].appearance->foreground==26 &&
            account->snapshot().party->members[1].appearance->colors[0]==8,
            "Remote party row lost the accepted member's saved emblem/color bytes");
      verify_party_rows(2);capture("party-two-profile-lobby");
      {
        // TU1.5 party search. The member's Leave Matchmaking (0x252150)
        // returns the party to pre-game and the leader's lobby shows 0xAD
        // (0x250804). Lobby START GAME (0x25106C) then searches the retained
        // hopper after its countdown, and the leader's own Leave Matchmaking
        // asks with 0x8C and shows no notice to its canceller.
        OnlineMatchmakingOptions party_queue;party_queue.endpoint=account_url;party_queue.allow_loopback_http=true;
        party_queue.compatibility="halo2-native-queue-v7";party_queue.maps={"lockout"};
        party_queue.poll_interval=std::chrono::milliseconds(25);
        auto queue=std::make_shared<OnlineMatchmaking>(login,party_queue);menu.set_online_matchmaking(queue);
        std::uint32_t party_clock=100000000;menu.ui_clock(party_clock);
        auto pump=[&](auto predicate,const char *failure) {
          const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(20);
          while(!predicate() && std::chrono::steady_clock::now()<until) {
            menu.update_online_login();menu.update_online_matchmaking();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
          }
          check(predicate(),failure);
        };
        auto dialog_shown=[&](unsigned id) {
          const auto &d=content.dialogs.at(id);const auto &f=menu.frame();
          return f.screen==d.screen && f.title==d.title && f.description==d.message;
        };
        auto start_party_countdown=[&] {
          const auto &entries=menu.frame().entries;
          const auto start=std::find_if(entries.begin(),entries.end(),[](const auto &e){return e.destination==-17;});
          check(menu.frame().screen==14 && start!=entries.end(),"Party lobby lost its source Start action");
          const auto &screen=content.screen(14);
          check(start->label==content.localized(screen,screen.panes.at(2).buttons.at(0).text.string),
                "Party Start label differs from its authored locale bank");
          menu.select(unsigned(start-entries.begin()));menu.accept();
        };
        menu.open(219);
        pump([&]{return std::any_of(menu.frame().entries.begin(),menu.frame().entries.end(),
                                    [](const auto &e){return e.label=="Party Search lab";});},"Party playlists did not load");
        choose("Party Search lab");menu.select(0);menu.accept();
        check(menu.frame().screen==14,"Party playlist skipped the original pregame lobby");
        pump([&]{const auto s=account->snapshot();return s.party && !s.busy &&
          std::all_of(s.party->members.begin(),s.party->members.end(),[](const auto &m){return m.ready_playlist=="party-search";});},
             "Every party member must consent to the playlist before START GAME");
        start_party_countdown();
        pump([&]{return menu.frame().screen==210;},"Shared initial party countdown did not reach zero");
        check(menu.frame().screen==210,"Initial party lobby countdown did not start search");
        pump([&]{return queue->snapshot().status==OnlineMatchmakingStatus::Searching;},"Party leader search did not start");
        pump([&]{return dialog_shown(0xad);},"Member Leave Matchmaking did not show the leader 0xAD");
        check(menu.stack().size()>=2 && menu.stack()[menu.stack().size()-2].screen==14,"0xAD was not shown over the pregame lobby");
        check(menu.frame().entries.size()==1,"0xAD is not a single-choice notice");
        menu.accept();check(menu.frame().screen==14,"0xAD OK did not return to the lobby");
        pump([&]{return queue->snapshot().status==OnlineMatchmakingStatus::Ready;},"A party-cancelled leader kept its queue intent");
        start_party_countdown();
        pump([&]{return !account->snapshot().busy && menu.online_lobby_countdown()>0;},"Shared countdown Start was not acknowledged");
        check(menu.online_lobby_countdown()<=matchmaking_lobby_countdown_initial &&
              menu.online_party_text(menu.frame(),0).find(matchmaking_lobby_countdown_text(menu.online_lobby_countdown()))!=std::string::npos,
              "Lobby START GAME did not show the 10 s countdown");
        const auto before_repeat=menu.online_lobby_countdown();start_party_countdown();
        pump([&]{return !account->snapshot().busy && menu.online_lobby_countdown()<before_repeat;},"Repeated shared Start did not shorten the countdown");
        (void)menu.take_ui_sounds();
        pump([&]{return menu.frame().screen==210;},"Shared party countdown did not search at its published zero");
        check(menu.frame().screen==210 && menu.online_lobby_countdown()<0,"Lobby countdown did not search at zero");
        {const auto sounds=menu.take_ui_sounds();
         check(std::count(sounds.begin(),sounds.end(),FrontendUiSound::PregameCountdown)==4,"Lobby countdown did not beep at stages 3..0");}
        pump([&]{return queue->snapshot().status==OnlineMatchmakingStatus::Searching;},"Lobby START GAME did not search the retained hopper");
        menu.back();
        check(dialog_shown(matchmaking_leave_dialog_party) && menu.frame().entries.at(0).destination==-16,
              "Party Leave Matchmaking did not ask with 0x8C");
        check(menu.frame().selected==1,"0x8C did not focus its authored Cancel choice");
        menu.select(0);menu.accept();check(menu.frame().screen==14,"Leave Matchmaking did not return the party to its lobby");
        pump([&]{return queue->snapshot().status==OnlineMatchmakingStatus::Ready;},"Leave Matchmaking kept the party ticket");
        for(unsigned i=0;i<20;++i){menu.update_online_login();menu.update_online_matchmaking();std::this_thread::sleep_for(std::chrono::milliseconds(10));}
        check(menu.frame().screen==14,"The canceller was shown its own party-cancel notice");
        // Closing the queue session withdraws every member's playlist consent
        // (a new party version); later commands must see that version.
        menu.set_online_matchmaking({});
        wait_party([](const auto &s){return s.party && !s.busy && std::all_of(s.party->members.begin(),s.party->members.end(),
                                                                          [](const auto &m){return m.ready_playlist.empty();});});
        // Back on lobby14 asks to leave it (0x8B); re-enter it as Custom Game.
        menu.return_online();menu.select(2);menu.accept();check(menu.frame().screen==14,"Custom Game lost the party lobby");
      }
      menu.players();
      if(!captures.empty()) {
        std::ofstream(captures/"request-full-party")<<"ready\n";
        for(unsigned i=0;i<14;++i) {
          check(account->party_command({"invite","layout_"+std::to_string(i)}),"Full party invitation was not submitted");
          wait_party([&](const auto &s){return s.party && s.party->members.size()==i+3 && !s.busy;});
        }
        wait_party([](const auto &s){return s.party && s.party->members.size()==16;});
        check(menu.frame().entries.size()==16,"Full accepted party roster truncated before scrolling");
        capture("party-full-members");menu.select(15);capture("party-members-scroll");
        while(menu.frame().screen!=14)menu.back();capture("party-full-lobby");
        auto click_party_prompt=[&](unsigned text,FrontendPointerAction action,unsigned destination) {
          painter.draw(menu,5,-1,10000);
          const auto &bounds=native_layout.screen(14).panes.at(2).texts.at(text).bounds;
          std::optional<FrontendPointerTarget> target;
          for(float y=240-bounds.top;y<240-bounds.bottom && !target;y+=1)
            for(float x=320+bounds.left;x<320+bounds.right && !target;x+=1)
              if(const auto hit=painter.pointer_hit(x,y);hit && hit->action==action)target=hit;
          check(target && menu.activate_pointer(*target) && menu.frame().screen==destination,
                "ONLINE party shortcut prompt cannot be clicked with a mouse");
          menu.back();
        };
        click_party_prompt(4,FrontendPointerAction::Players,26);
        click_party_prompt(5,FrontendPointerAction::Auxiliary,216);
        std::ofstream(captures/"full-party-complete")<<"done\n";
        wait_party([](const auto &s){return s.party && s.party->members.size()==2;});
        menu.players();
      }
      menu.select(1);menu.accept();capture("party-member-options");choose("Make Party Leader");
      wait_party([](const auto &s){return s.party && !s.party->leader && !s.busy;});
      while(menu.frame().screen!=14 && menu.stack().size()>1)menu.back();
      check(menu.frame().screen==14 && !menu.lobby_view()->input.host,"Leader transfer left local leader authority");
      capture("party-follower-lobby");
      menu.players();menu.select(0);menu.accept();choose("Leave Party");
      wait_party([](const auto &s){return !s.party && !s.busy && !s.party_invitations.empty();});capture("party-invitation");
      const auto notice=menu.invite_notice();check(notice && notice->can_accept && !notice->accepting,"Incoming real party invitation has no corner notice");
      painter.prompt_device(PromptDevice::Xbox);
      const auto resident_materials=mesh.materials.size();
      const auto corner=painter.draw_invite_notice(*notice,{1920,1080});
      check(!corner.vertices.empty() && mesh.materials.size()==resident_materials,"Incoming notification changed the resident GPU material table");
      bool project_buttons=false;
      for(const auto &v:corner.vertices) {
        check(v.position.x>=0 && v.position.x<=640 && v.position.y>=0 && v.position.y<=480 &&
              mesh.materials.at(v.material).ui_full_surface,"Widescreen corner notification escaped the full-screen viewport");
        project_buttons|=mesh.materials.at(v.material).name.starts_with("controller-prompts/xbox/");
      }
      check(project_buttons,"Corner invitation used bundled controller glyphs instead of project artwork");
      painter.draw(menu,5,-1,10000);
      std::optional<FrontendPointerTarget> popup_accept;
      for(float y=0;y<130 && !popup_accept;++y)for(float x=320;x<640 && !popup_accept;++x)
        if(const auto hit=painter.pointer_hit(x,y);hit && hit->action==FrontendPointerAction::InviteAccept)popup_accept=hit;
      check(popup_accept && menu.activate_pointer(*popup_accept),"Corner Accept button did not submit the real party invitation");
      check(!menu.act_invite_notice(true),"Repeated toast acceptance submitted another party mutation");
      wait_party([](const auto &s){return s.party && s.party->members.size()==2 && !s.party->leader && !s.busy;});
      check(menu.frame().screen==14,"Accepted party did not enter native lobby");capture("party-accepted-lobby");
      check(!menu.invite_notice(),"Accepted invitation remained visible in corner");
      menu.players();menu.select(1);menu.accept();choose("Leave Party");
      wait_party([](const auto &s){return !s.party && !s.busy;});
      menu.back();check(menu.frame().screen==186,"Party completion lost ONLINE parent");
      // Original2B55AE ->2B5D3B/2B5EF9 joins the selected same-title friend's
      // actual session. The service's real Open admission/context is required;
      // neither display names nor an invitation synthesize this eligibility.
      wait_party([](const auto &s){return !s.party && !s.busy && std::any_of(s.friends.begin(),s.friends.end(),
          [](const auto &f){return f.state=="friend" && f.party_joinable==true && !f.party_context.empty();});});
      const auto join_observation=account->snapshot();
      const auto join_target=std::find_if(join_observation.friends.begin(),join_observation.friends.end(),
          [](const auto &f){return f.party_joinable==true;});
      check(join_target!=join_observation.friends.end(),"Observed Open target disappeared before selection");
      const auto joined_context=join_target->party_context;
      menu.open(26,0);choose("Pilot");
      check(menu.frame().screen==31 && menu.frame().context==43,"Open friend join lost its actual friend selection");
      capture("party-open-friend-options");
      const auto join_serial=account->snapshot().party_command_serial;
      choose("Join Party");
      check(menu.frame().screen!=14 || (account->snapshot().party && account->snapshot().party_command_successful==true),
            "Join Party opened a lobby before actual accepted membership");
      wait_party([&](const auto &s){return s.party && !s.party->leader && s.party->members.size()==2 && !s.busy &&
          s.party_command_serial>join_serial && s.party_command_successful==true;});
      const auto accepted_join=account->snapshot();
      check(menu.frame().screen==14 && accepted_join.party && std::any_of(accepted_join.party->members.begin(),accepted_join.party->members.end(),
          [&](const auto &m){return m.handle==joined_context && m.leader;}),
            "Open friend join did not enter the exact acknowledged target party");
      capture("party-open-joined-lobby");
      menu.open(26,0);choose("Pilot");
      check(std::none_of(menu.frame().entries.begin(),menu.frame().entries.end(),[](const auto &e){return e.destination==-83 && e.value==8;}),
            "Accepted party bypassed its existing Leave requirement before another join");
      menu.back();menu.back();
      menu.players();menu.select(1);menu.accept();choose("Leave Party");
      wait_party([](const auto &s){return !s.party && !s.busy;});
      menu.back();check(menu.frame().screen==186,"Open join/leave lost ONLINE parent");
    }
    if (account && !party_run) {
      auto wait_account = [&](auto predicate) {
        const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(8);
        while(!predicate(account->snapshot()) && std::chrono::steady_clock::now()<until) { menu.update_online_login(); std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
        menu.update_online_login(); check(predicate(account->snapshot()),"Native clan flow timed out");
      };
      auto choose = [&](const std::string &label) {
        const auto &entries=menu.frame().entries;
        auto it=std::find_if(entries.begin(),entries.end(),[&](const auto &e){return e.label==label;});
        check(it!=entries.end(),"Clan action missing"); menu.select(unsigned(it-entries.begin())); menu.accept();
      };
      const auto click_prompt = [&](FrontendPointerAction action) {
        painter.draw(menu,5,-1,10000);
        std::optional<FrontendPointerTarget> target;
        for(float y=0;y<480 && !target;y+=1)for(float x=0;x<640 && !target;x+=1)
          if(const auto hit=painter.pointer_hit(x,y);hit && hit->action==action)target=hit;
        check(target && menu.activate_pointer(*target),"Full-screen clan prompt cannot be clicked");
      };
      menu.players(); menu.navigate(1,0);
      check(menu.frame().screen==26 && menu.frame().context==1,"Y menu did not open Clans tab");
      choose("Create Clan"); check(menu.frame().title=="CREATE CLAN", "Clan creation shows an invitation prompt"); capture("clan-name-entry"); menu.text("Native Clan 123"); menu.commit_text();
      wait_account([](const auto &s){return s.clan && !s.busy;});
      check(account->snapshot().clan->role==3,"Clan creator is not Overlord");
      check(menu.online_clan_text(menu.frame(),4)=="1 member" &&
            menu.online_clan_text(menu.frame(),6)=="VOICE OFF","Clan metadata has unclear count or voice labels");
      check(!has_logo(menu),"Halo 2 logo overlaps clan roster"); private_identity(menu); capture("clan-created");
      click_prompt(FrontendPointerAction::Accept);
      check(menu.frame().screen==31 && menu.frame().title=="Pilot","Full-screen clan Select prompt lost its selected member");
      click_prompt(FrontendPointerAction::Back);
      check(menu.frame().screen==26 && menu.frame().context==1,"Clan member Back prompt lost the roster");
      click_prompt(FrontendPointerAction::Back);
      check(menu.frame().screen==186,"Full-screen clan Close prompt did not return to ONLINE");
      menu.players();menu.navigate(1,0);
      click_prompt(FrontendPointerAction::Auxiliary);choose("Join Clan Voice");
      const auto voice_until=std::chrono::steady_clock::now()+std::chrono::seconds(4);
      while(menu.online_clan_text(menu.frame(),8).find("connected")==std::string::npos && std::chrono::steady_clock::now()<voice_until) {
        menu.update_online_login(); std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
      check(menu.online_clan_text(menu.frame(),8).find("connected")!=std::string::npos,"Native clan voice failed to connect with SDL dummy devices");
      check(menu.online_clan_text(menu.frame(),6)=="MIC ON","Clan microphone label lost its live state");
      menu.voice_push_to_talk(true); capture("clan-voice"); menu.voice_push_to_talk(false);
      click_prompt(FrontendPointerAction::Auxiliary);capture("clan-voice-options");choose("Mute Microphone");
      check(menu.online_clan_text(menu.frame(),8).find("microphone muted")!=std::string::npos,"Clan voice microphone mute missing");
      check(menu.online_clan_text(menu.frame(),6)=="MIC MUTED","Clan microphone mute label is ambiguous");
      menu.auxiliary(); capture("clan-options"); choose("Send Clan Invite"); capture("clan-invite-entry"); menu.text("peer_pilot"); menu.commit_text();
      wait_account([](const auto &s){return s.clan && s.clan->members.size()==2 && !s.busy;});
      capture("clan-roster");
      if(!captures.empty()) {
        std::ofstream(captures/"request-full-roster")<<"ready\n";
        wait_account([](const auto &s){return s.clan && s.clan->members.size()==100;});
        check(menu.frame().entries.size()==100,"Full clan roster truncated before scrolling");
        check(menu.online_clan_text(menu.frame(),4)=="100 members","Full clan member count lacks its label");
        private_identity(menu);capture("clan-full-roster");menu.select(99);
        const auto roster_revision=account->snapshot().revision;
        wait_account([&](const auto &s){return s.revision>roster_revision;});
        check(menu.frame().selected==99,"Clan poll moved selection to another member with the same screen name");
        check(account->snapshot().clan->members[99].presence==OnlineClanPresence::Offline,
              "Clan fixture without authenticated activity appeared online");
        capture("clan-roster-scroll");menu.select(0);
        std::ofstream(captures/"full-roster-complete")<<"done\n";
        wait_account([](const auto &s){return s.clan && s.clan->members.size()==2;});
      }
      // Click the far end of the second complete selection bar, where the
      // original name-only hit region had no target. The pointer projection
      // audit above covers the same hit at all six window aspect ratios.
      std::optional<FrontendPointerTarget> member_target;
      for(const auto [width,height]:viewports) {
        const auto canvas=frontend_canvas(width,height);
        const auto ui=painter.draw(menu,5,-1,10000,{width,height});
        audit_vista(menu,ui,{width,height});
        member_target.reset();
        const float right=(float(width)-canvas.left)/canvas.scale;
        for(float y=0;y<480 && !member_target;y+=1)
          if(const auto hit=painter.pointer_hit(right-24,y);hit &&
              hit->action==FrontendPointerAction::Entry && hit->index==1)
            member_target=hit;
        check(member_target.has_value(),"Extended clan row is not clickable at the surface's right edge");
      }
      check(member_target && menu.activate_pointer(*member_target) && menu.frame().title=="Peer Pilot",
            "Full-screen clan row does not respond across the full visible selection bar");
      capture("clan-member-options");choose("Mute Voice");
      check(menu.online_clan_column(menu.frame(),1,1).find("muted")!=std::string::npos,"Member voice mute missing");
      menu.select(1); menu.accept(); choose("Unmute Voice");
      menu.select(1); menu.accept(); choose("Change Clan Role"); menu.select(0); menu.accept();
      wait_account([](const auto &s){return s.clan && s.clan->members.back().role==0 && !s.busy;});
      menu.select(1); menu.accept(); choose("Change Clan Role"); capture("clan-roles"); menu.select(3); menu.accept();
      check(menu.frame().screen==33 && menu.frame().selected==1,"Overlord promotion lacks safe confirmation"); capture("clan-overlord-confirmation");
      menu.select(0); menu.accept();
      wait_account([](const auto &s){return s.clan && std::count_if(s.clan->members.begin(),s.clan->members.end(),[](const auto &m){return m.role==3;})==2 && !s.busy;});
      menu.auxiliary(); choose("Message Clan"); capture("clan-message-entry"); menu.text("Meet for Slayer"); menu.commit_text();
      wait_account([](const auto &s){return s.clan && !s.clan->messages.empty() && !s.busy;});
      menu.auxiliary(); choose("Read Clan Messages"); capture("clan-messages"); menu.back(); menu.back();
      menu.auxiliary(); choose("Leave Clan"); menu.select(0); menu.accept();
      wait_account([](const auto &s){return !s.clan && !s.busy && !s.invitations.empty();}); capture("clan-invitation");
      menu.select(1); menu.accept(); capture("clan-invite-options"); choose("Accept Clan Invite");
      wait_account([](const auto &s){return s.clan && s.clan->role==1 && !s.busy;}); capture("clan-joined");
      menu.auxiliary(); choose("Leave Clan"); menu.select(0); menu.accept();
      wait_account([](const auto &s){return !s.clan && !s.busy;});
      menu.back();
      check(menu.frame().screen==186,"Clan navigation lost ONLINE parent");
    }
    menu.select(3); menu.accept();
    check(menu.frame().screen == 20 && menu.frame().entries[0].label == "Continue as Pilot" && menu.frame().description == "Screen name: Pilot\nDiscord: verified_user", "Account screen does not distinguish screen name from Discord identity");
    check(!has_logo(menu), "Title logo overlaps the account panel");
    private_identity(menu);
    menu.select(1); menu.accept();
    if(menu.frame().screen!=167 || menu.keyboard_text!="Pilot")
      throw FormatError("Account name editor: screen="+std::to_string(menu.frame().screen)+
          " context="+std::to_string(menu.frame().context)+" text="+menu.keyboard_text);
    check(!has_logo(menu), "Title logo overlaps ONLINE name entry");
    menu.text("Custom Pilot"); menu.commit_text();
    check(menu.frame().screen == 20 && menu.profile_name() == "Custom Pilot" && menu.signed_in_profile() == "Custom Pilot", "Screen name edit did not update the selected player");
    if (account) {
      const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
      while(!menu.online_identity() && std::chrono::steady_clock::now()<until) { menu.update_online_login(); std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
    }
    check(menu.frame().entries[0].label == "Continue as Custom Pilot" && menu.online_identity()->username == "verified_user" && menu.online_identity()->id == "123456789012345678", "Screen name edit altered verified Discord identity");
    check(MenuState(content, preferences.path).profile_name() == "Custom Pilot", "Chosen screen name was not saved");
    menu.select(2);menu.accept();
    check(menu.frame().screen==167,"Service tag entry is not reachable");
    menu.text("a117");menu.commit_text();
    check(menu.frame().screen==20 && menu.profile_service_tag()=="A117","Service tag did not normalize separately from the player name");
    check(MenuState(content,preferences.path).profile_service_tag()=="A117","Service tag was not persisted");
    menu.select(2);menu.accept();menu.text("S002");menu.back();
    check(menu.profile_service_tag()=="A117","Cancelling service tag entry changed the profile");
    private_identity(menu);
    menu.select(1); menu.accept(); menu.text("Cancelled"); menu.back();
    check(menu.profile_name() == "Custom Pilot", "Cancelling name entry changed the screen name");
    menu.select(0); menu.accept();
    check(menu.frame().screen == 186 && menu.frame().description.starts_with("Custom Pilot | "), "ONLINE service retained the old screen name");
    menu.select(3); menu.accept();
    menu.select(3); menu.accept();
    check(!menu.online_identity() && menu.frame().screen == 20, "Sign-out retained Online access");
    menu.open(210);
    check(menu.frame().screen == 20, "Sign-out bypassed matchmaking gate");
    check(frontend_online_text("Xbox Live / XBOX LIVE / Xbox LIVE / xbox live") == "ONLINE / ONLINE / ONLINE / ONLINE", "Brand replacement missed uppercase ONLINE");
    // Offline modes remain reachable without Discord.
    MenuState offline(content); offline.open(6); offline.select(2); offline.accept();
    check(offline.frame().screen == 30 && !offline.online_identity(), "Split Screen requires Discord");
    for (const auto id : {212u, 238u, 239u, 240u, 241u}) {
      MenuState preview(content); preview.open(id);
      check(preview.frame().screen == id && !preview.online_identity(), "Offline Extras, error or fade screen requires Discord");
    }
    MenuState password(content); password.open(21);
    check(password.frame().screen == 20 && password.frame().entries[0].label == "Sign in with Discord", "Legacy password screen survived the service replacement");
    check((sampled.empty() && !cache.is_mcc()) || (sampled_online_font && sampled_online_bitmap), "ONLINE did not draw higher-resolution imported fonts and UI bitmaps");
    // The primary login changes provider without changing chosen Halo names,
    // private-ID treatment, Vista artwork, or widescreen menu bounds.
    const std::string device(43,'d');
    auto escape=[](const std::string &value) {auto *p=curl_easy_escape(nullptr,value.c_str(),int(value.size()));
      std::string out(p);curl_free(p);return out;};
    const std::string ns="http://specs.openid.net/auth/2.0";
    const auto steam_url="https://steamcommunity.com/openid/login?openid.ns="+escape(ns)+
      "&openid.mode=checkid_setup&openid.realm="+escape(std::string(default_online_origin)+"/")+
      "&openid.return_to="+escape(std::string(default_online_origin)+"/v1/auth/steam/callback/"+device)+
      "&openid.identity="+escape(ns+"/identifier_select")+"&openid.claimed_id="+escape(ns+"/identifier_select");
    auto steam=DiscordLogin::steam([&](const auto &url){return url==steam_url;},
      [&](const DiscordHttpRequest &r,std::stop_token) {
        using Json=nlohmann::json;
        if(r.url.ends_with("/start"))return DiscordHttpResponse{201,Json{{"device",device},{"url",steam_url},{"expires_in",300},{"interval",1}}.dump()};
        return DiscordHttpResponse{200,Json{{"state","signed_in"},{"token","h2s_"+std::string(43,'t')},{"expires_in",28800},
          {"identity",{{"id","76561197960287930"},{"username","Steam Persona"},{"display_name","Steam Persona"}}}}.dump()};
      });
    menu.set_online_account({});menu.set_discord_login(steam);menu.open(20);
    const auto restore_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(steam->snapshot().status==DiscordLoginStatus::Verifying && std::chrono::steady_clock::now()<restore_deadline) {
      menu.update_online_login();std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    menu.update_online_login();
    check(menu.frame().entries[0].label=="Sign in with Steam","Native ONLINE does not offer Steam sign-in");
    menu.accept();
    const auto steam_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(!menu.online_identity()&&std::chrono::steady_clock::now()<steam_deadline){menu.update_online_login();std::this_thread::sleep_for(std::chrono::milliseconds(5));}
    menu.update_online_login();
    check(menu.frame().screen==186,"Steam sign-in did not enter native ONLINE");
    menu.open(20);
    check(menu.online_identity()&&menu.profile_name()=="Custom Pilot"&&menu.frame().description=="Screen name: Custom Pilot\nSteam: Steam Persona",
          "Steam login lost chosen name or retained Discord account presentation");
    check(menu.frame().entries.back().label=="Sign out of Steam"&&!has_logo(menu),"Steam account action or panel overlap regressed");
    private_identity(menu);capture("steam-account");
    for(const auto &[width,height]:viewports)audit_vista(menu,painter.draw(menu,5,-1,10000,{width,height}),{width,height});
    menu.select(3);menu.accept();check(!menu.online_identity(),"Native Steam sign-out retained ONLINE identity");
    if (!sampled.empty() || cache.is_mcc()) std::cout << "Selected imported texture/font byte comparison passed on " << audited_panels.size() << " ONLINE panel/context pairs.\n";
    if (!sampled.empty() || cache.is_mcc()) std::cout << bounded_glyphs << " bounded HD text vertices; " << pointer_projections
        << " mouse target projections across " << std::size(viewports) << " aspect ratios.\n";
    std::cout << "PASS native Quickmatch/Optimatch/playlists when configured, ONLINE discovery/details/refresh/scroll/expiry/failure/recovery, private Discord identity, chosen name, unobstructed panels, cancellation, sign-out and offline access"
              << (cache.is_mcc() ? " using selected MCC-only assets and native MCC font prompts\n" : sampled.empty() ? " using original menu assets\n" : " using launcher-selected Vista HD fonts and UI bitmaps\n");
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}

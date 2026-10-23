#include "halo2/frontend.hpp"
#include "halo2/audio.hpp"
#include "halo2/frontend_online.hpp"
#include "halo2/online_service.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "ui/mcc_frontend_string_ids.hpp"
#include "ui/frontend_mcc_logical_layout.hpp"
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
    if (argc < 2) return 77;
    std::filesystem::path maps(argv[1]), sampled, selected_menu_assets, captures;
    std::string matchmaking_url, account_url, capture_prefix;
    unsigned callback_port=42103;
    bool party_run{}, gathering_run{}, party_render{}, capture_validation=true;
    std::shared_ptr<OnlineAccount> account;
    for (int i = 2; i < argc; ++i) {
      const std::string option(argv[i]);
      check(i + 1 < argc, "Missing ONLINE test option value");
      const std::filesystem::path value(argv[++i]);
      if (option == "--account-url") account_url = value.string();
      else if(option=="--party-url"){account_url=value.string();party_run=true;}
      else if(option=="--gathering-ui")gathering_run=value=="1";
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
    const auto logical_layout = detail::mcc_logical_layout::make_view(content);
    const auto &native_layout = logical_layout ? logical_layout->content : content;
    const auto selected=selected_menu_assets.empty()?sampled:selected_menu_assets;
    const auto ui_assets = load_frontend_ui_assets(content, selected);
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
        for(auto i=count;i<mesh.materials.size();++i)for(std::size_t j=0;j<count;++j) {
          const auto &a=mesh.materials[i].diffuse,&b=mesh.materials[j].diffuse;
          if(a.width==b.width && a.height==b.height && a.format==b.format && a.pixels==b.pixels) {
            vista_fonts[i]=vista_fonts[i] || vista_fonts[j];
            vista_bitmaps[i]=vista_bitmaps[i] || vista_bitmaps[j];
          }
        }
      }
      const auto surface=frontend_canvas(viewport[0],viewport[1]);
      const float surface_left=draw.full_surface_bounds?-surface.left/surface.scale:0;
      const float surface_top=draw.full_surface_bounds?-surface.top/surface.scale:0;
      const float surface_right=draw.full_surface_bounds?surface_left+surface.surface_width/surface.scale:640;
      const float surface_bottom=draw.full_surface_bounds?surface_top+surface.surface_height/surface.scale:480;
      bool font = false, bitmap = false;
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
          check(std::any_of(content.emblem_shaders.begin(),content.emblem_shaders.end(),[&](const auto &shader){
            return shader.second.name==name && shader.second.ui_emblem_pass;
          }),"ONLINE emblem does not belong to its selected native shader bank");
          check(material.forward_vertex==11 && material.forward_blend==1 &&
                material.animated_textures[0].size()==32 && material.animated_textures[1].size()==64 &&
                material.texture_frames[0]<32 && material.texture_frames[1]<64,
                "ONLINE emblem lost its source pass or bounded foreground/background pages");
          bitmap=true; // The verified emblem pass samples the imported bitmap banks.
        } else if (name.starts_with("font/")) {
          check(vista_fonts.at(v.material), "ONLINE text fell back from the selected imported font atlas");
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
          if (!vista_bitmaps.at(v.material)) missing.insert(name);
          bitmap = true;
        }
      }
      if(!missing.empty()) {
        std::string message="ONLINE bitmaps missing selected imported sampling:";
        for(const auto &name:missing)message+='\n'+name;
        throw std::runtime_error(message);
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
        const auto &pane=content.vista->screen(210).panes.at(state.online_matchmaking_pane());
        const auto &list=pane.lists.at(0);
        const auto &field=content.vista->skins.at(list.skin).texts.at(0);
        const float left=320+.4f*(list.position.x+field.bounds.left);
        const float right=320+.4f*(list.position.x+field.bounds.right);
        const float top=240-.4f*(list.position.y+field.bounds.top);
        const float bottom=240-.4f*(list.position.y+field.bounds.bottom);
        float ink_top=bottom,ink_bottom=top;
        for(const auto &v:draw.vertices) {
          if(!mesh.materials.at(v.material).name.starts_with("font/") ||
              v.color.x+v.color.y+v.color.z<.1f || v.position.x<left || v.position.x>right ||
              v.position.y<top || v.position.y>bottom)continue;
          ink_top=std::min(ink_top,v.position.y);ink_bottom=std::max(ink_bottom,v.position.y);
        }
        if(ink_bottom-ink_top<4)
          throw FormatError("Vista matchmaking roster clips the chosen screen name: ink="+
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
        const float top=240-grid.position.y-name->bounds.top;
        const float bottom=240-grid.position.y-name->bounds.bottom-
            float(lobby->party_names->size()-1)*grid.step.y;
        // Names now use source profile colors, including blue. Color alone
        // cannot distinguish a legitimate name from the old help overlay.
        check(state.online_party_text(state.frame(),47).empty(),
              "ONLINE party still binds the obsolete help field over player names");
        for(const unsigned helper:{4u,5u,6u}) {
          if(state.online_party_text(state.frame(),helper).empty())continue;
          const auto &bounds=pane.texts.at(helper).bounds;
          check(320+bounds.right<left || 240-bounds.top>bottom || 240-bounds.bottom<top,
                "ONLINE party helper bounds overlap roster names");
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
    if (!account_url.empty()) {
      OnlineAccountOptions account_options; account_options.endpoint=account_url; account_options.allow_loopback_http=true;
      account_options.enable_parties=party_run;
      account_options.poll_interval=captures.empty() ? std::chrono::milliseconds(250) : std::chrono::milliseconds(party_run?1500:1000);
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
      const bool pc_progress=id==210 && content.vista && content.vista->screens.contains(210);
      const auto first_pane=id==210 && !pc_progress?1u:0u;
      const auto pane_count=pc_progress?3u:id==210?2u:unsigned(content.screen(id).panes.size());
      for (unsigned pane=first_pane;pane<pane_count;++pane) {
        try {
          for(const auto [width,height]:viewports) {
            const auto draw=painter.draw(sample,5,int(pane),10000,{width,height});
            audit_vista(sample,draw,{width,height});
            if(cache.is_mcc() && pc_progress) {
              unsigned rings=0;
              const auto &source=content.vista->screen(210).panes.at(pane).scenes.at(0);
              for(const auto &scene:draw.scenes) {
                if(scene.definition->objects!=source.objects)continue;
                ++rings;const auto &a=scene.definition->viewport, &b=source.viewport;
                check(scene.vista_viewport && a.top==b.top && a.left==b.left &&
                    a.bottom==b.bottom && a.right==b.right,
                    "Matchmaking ring uses a different canvas/anchor from its PC progress widgets");
              }
              check(rings==1,"Matchmaking pane lost its one authored spinning ring");
            }
          }
        }
        catch(const std::exception &e) { throw std::runtime_error("ONLINE screen "+
            std::to_string(id)+" pane "+std::to_string(pane)+": "+e.what()); }
        if(id==210) {
          const auto &bounds=pc_progress?content.vista->header_bounds.at(3)
              :content.screen(id).panes.at(pane).texts.at(1).bounds;
          const float scale=pc_progress?.4f:1.f;
          std::optional<FrontendPointerTarget> cancel;
          for(float y=240-scale*bounds.top;y<240-scale*bounds.bottom;y+=1)
            for(float x=320+scale*bounds.left;x<320+scale*bounds.right;x+=1)
              if(const auto hit=painter.pointer_hit(x,y);hit && hit->action==FrontendPointerAction::Back)
                cancel=hit;
          check(cancel.has_value(),"ONLINE queue Cancel prompt cannot be clicked with a mouse");
          check(sample.activate_pointer(*cancel) && sample.frame().screen==186,
                "ONLINE queue mouse Cancel did not return to ONLINE");
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
    {
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
    menu.select(0); menu.accept();
    check(menu.frame().screen == 210 && !menu.launch, "Server discovery replaced the separate matchmaking route");
    menu.accept();
    check(menu.frame().screen == 210 && !menu.launch && menu.online_matchmaking_text(menu.frame(), 5).find("configured") != std::string::npos, "Unconfigured matchmaking lacks an honest status");
    check(!has_logo(menu), "Title logo overlaps matchmaking status");
    menu.back();
    if (!matchmaking_url.empty()) {
      OnlineMatchmakingOptions queue_options; queue_options.endpoint = matchmaking_url; queue_options.allow_loopback_http = true;
      queue_options.compatibility=gathering_run?"halo2-native-queue-v8":"halo2-native-queue-v7";
      queue_options.maps = {"lockout"}; queue_options.poll_interval = std::chrono::milliseconds(25);
      auto matchmaking = std::make_shared<OnlineMatchmaking>(login, queue_options);
      menu.set_online_matchmaking(matchmaking);
      unsigned queue_stage{};
      auto wait_queue = [&](auto predicate) {
        ++queue_stage;
        const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(8);
        while (!predicate(matchmaking->snapshot()) && std::chrono::steady_clock::now() < until) {
          menu.update_online_matchmaking(); std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        menu.update_online_matchmaking();
        if(!predicate(matchmaking->snapshot())) {
          const auto state=matchmaking->snapshot();
          throw FormatError("Native matchmaking stage "+std::to_string(queue_stage)+" timed out, status="+std::to_string(unsigned(state.status))+": "+state.message+" screen="+std::to_string(menu.frame().screen)+" "+menu.frame().description);
        }
      };
      menu.select(0); menu.accept();
      wait_queue([](const auto &s) { return s.status == OnlineMatchmakingStatus::Searching; });
      check(matchmaking->snapshot().playlist_id == "ranked" && menu.frame().screen == 210 && !menu.launch, "Quickmatch did not use server default");
      check(!has_logo(menu), "Title logo overlaps Quickmatch"); private_identity(menu); capture("quickmatch-searching");
      if(gathering_run) {
        wait_queue([](const auto &s){return s.status==OnlineMatchmakingStatus::Searching && s.gathering.has_value();});
        check(menu.online_matchmaking_pane()==1 && menu.frame().entries.size()==1 &&
          menu.online_matchmaking_text(menu.frame(),0)=="Waiting for additional players...",
          "Actual gathering did not select the authored Vista waiting pane/title");
        private_identity(menu);capture("quickmatch-waiting");
      }
      menu.back();
      wait_queue([](const auto &s) { return s.status == OnlineMatchmakingStatus::Idle; });
      menu.select(1); menu.accept();
      wait_queue([](const auto &s) { return s.status == OnlineMatchmakingStatus::Ready; });
      check(menu.frame().title == "ONLINE PLAYLISTS" && menu.frame().entries.size() == 4, "Optimatch did not load authenticated playlists");
      capture("playlists");
      for(unsigned row=2;row<4;++row) {
        menu.select(row);menu.accept();
        check(menu.frame().screen==237 && menu.online_matchmaking_text(menu.frame(),0).find("Clan membership required")!=std::string::npos,
          "Nonmember clan details lost the membership requirement");
        check(!has_logo(menu),"Title logo overlaps clan playlist details");private_identity(menu);
        capture(row==2?"minor-clan-playlist":"major-clan-playlist");menu.back();
      }
      menu.select(1); menu.accept();
      check(menu.frame().screen == 237 && menu.online_matchmaking_text(menu.frame(), 0).find("Casual") != std::string::npos, "Casual details missing");
      check(menu.online_matchmaking_text(menu.frame(), 0).find("level") == std::string::npos, "Casual skill leaked into UI");
      capture("playlist-details"); menu.accept();
      wait_queue([](const auto &s) { return s.status == OnlineMatchmakingStatus::CheckingConnections; });
      check(matchmaking->snapshot().playlist_id == "casual" && menu.frame().entries.size() == 2 && !menu.launch, "Selected playlist did not form a native roster");
      check(menu.online_matchmaking_pane()==2 && menu.online_matchmaking_text(menu.frame(),0)=="Attempting to join a game...",
        "Native admitted match did not bind Vista's join pane/title");
      check(menu.frame().entries[0].label.find("Pilot") != std::string::npos && menu.frame().entries[1].label.find("Pilot") != std::string::npos, "Chosen names missing from roster");
      check(!has_logo(menu), "Title logo overlaps match roster"); private_identity(menu); capture("match-checking-connections");
      wait_queue([](const auto &s) { return s.status == OnlineMatchmakingStatus::Loading; });
      check(menu.launch && menu.launch->online_match && menu.launch->map=="lockout.map" &&
          menu.launch->multiplayer && menu.launch->local_players==1 && menu.launch->variant.name=="Slayer",
          "Native matchmaking did not hand its authenticated match into gameplay");
      capture("match-loading");
      menu.launch.reset();
      menu.back(); menu.back(); menu.back();
      wait_queue([](const auto &s) { return s.status == OnlineMatchmakingStatus::Idle; });
      check(menu.frame().screen == 186, "Cancel failed to return to service menu");
      menu.select(2); menu.accept();
      check(menu.frame().screen == 14, "Create Party failed to open native lobby");
      capture("online-lobby");
      // This fixture deliberately has no account/party service. Its lobby
      // cannot pretend an accepted party exists; playlists remain accessible
      // through the independent Optimatch route.
      check(menu.frame().entries.front().label=="Create Party","Unavailable party service fabricated accepted membership");
      menu.back();menu.select(0);menu.accept();menu.select(1);menu.accept();menu.select(0);menu.accept();menu.select(1);menu.accept();
      wait_queue([](const auto &s) { return s.status == OnlineMatchmakingStatus::Ready; });
      check(menu.frame().screen == 219 && menu.frame().title == "ONLINE PLAYLISTS", "Native lobby Playlists route did not reach queue catalog");
      menu.back();menu.select(2);menu.accept();menu.back();menu.select(0);menu.accept();
      check(menu.frame().screen == 6, "Leaving ONLINE lobby did not return to main menu");
      menu.select(1); menu.accept();
      check(menu.frame().screen == 20, "ONLINE account route changed");
      menu.select(0); menu.accept();
      menu.select(0); menu.accept();
      wait_queue([](const auto &s) { return s.playlist_id == "casual" && s.match.has_value(); });
      // This second search is cancelled while connection probes are pending.
      check(!menu.launch, "Quickmatch launched before connection admission");
      menu.back();
      wait_queue([](const auto &s) { return s.status == OnlineMatchmakingStatus::Idle; });
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
        const std::string box="ui\\list_skins\\matchmaking\\player_in";
        const std::string blank="ui\\list_skins\\matchmaking\\item_background";
        unsigned emblems{},stripes{},empty{},boxes{},speakers{};
        std::set<unsigned> banks;
        const auto speaker=native_layout.images.at(pane.bitmaps.at(21).datum).name;
        for(unsigned i=0;i+5<draw.vertices.size();i+=6) {
          const auto &m=mesh.materials.at(draw.vertices[i].material);
          if(m.ui_emblem_pass){++emblems;banks.insert(draw.vertices[i].material);}
          stripes+=m.name==primary;empty+=m.name==blank;boxes+=m.name==box;speakers+=m.name==speaker;
        }
        check(emblems==players && banks.size()==players,"Party rows omitted or shared profile emblem banks");
        check(stripes==players && empty==16-players && boxes==16-players,
              "Party rows lost occupied stripes, empty bars or the separate empty-slot square");
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
      auto wait_party=[&](auto predicate) {
        const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(15);
        while(!predicate(account->snapshot()) && std::chrono::steady_clock::now()<until){menu.update_online_login();std::this_thread::sleep_for(std::chrono::milliseconds(5));}
        menu.update_online_login();
        if(!predicate(account->snapshot()))throw FormatError("Native party flow timed out: "+account->snapshot().party_message);
      };
      auto choose=[&](const std::string &label) {
        const auto &entries=menu.frame().entries;
        const auto it=std::find_if(entries.begin(),entries.end(),[&](const auto &e){return e.label==label;});
        check(it!=entries.end(),"Party action missing");menu.select(unsigned(it-entries.begin()));menu.accept();
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
      choose("Party Options");capture("party-options");choose("Join Party Voice");
      const auto voice_until=std::chrono::steady_clock::now()+std::chrono::seconds(4);
      while(menu.online_party_text(menu.frame(),0).find("connected")==std::string::npos && std::chrono::steady_clock::now()<voice_until){menu.update_online_login();std::this_thread::sleep_for(std::chrono::milliseconds(5));}
      check(menu.online_party_text(menu.frame(),0).find("connected")!=std::string::npos,"Accepted party voice failed to connect");
      menu.voice_push_to_talk(true);capture("party-voice");menu.voice_push_to_talk(false);
      choose("Party Options");choose("Mute Microphone");choose("Party Options");capture("party-voice-options");choose("Unmute Microphone");
      choose("Party Options");choose("Invite Player");
      check(menu.party_invite_picker() && menu.party_invite_tab()==0,"Invite Player did not open Steam friends tab");
      capture("party-invite-steam-tab");
      const auto tab_revision=menu.revision;
      check(menu.activate_pointer({FrontendPointerAction::InviteTab,1,tab_revision}) && menu.party_invite_tab()==1,
            "Clan tab header did not switch invite source");
      check(!menu.activate_pointer({FrontendPointerAction::InviteTab,0,tab_revision}),"Stale invite-tab geometry changed source");
      capture("party-invite-clan-tab");
      check(menu.switch_invite_tab(-1) && menu.party_invite_tab()==0,"Shoulder tab navigation did not return to Steam friends");
      choose("Search Discord Username");
      check(menu.frame().screen==167 && menu.frame().title=="INVITE TO PARTY","Party invitation reused clan/name keyboard purpose");
      capture("party-invite-entry");menu.back();
      const auto discord_friend=std::make_shared<FriendsFixture>();
      discord_friend->state.enabled=discord_friend->state.ready=true;
      discord_friend->state.friends.push_back({"private-local-friend","Discord display name","peer_pilot",DiscordFriendKind::Friend,DiscordFriendPresence::Online});
      menu.set_discord_social(discord_friend);menu.open(26,0);menu.select(0);menu.accept();
      check(menu.frame().screen==31 && menu.frame().context==43,"Discord friend selection lost options");
      capture("party-discord-friend-options");choose("Invite To Party");
      wait_party([](const auto &s){return s.party && s.party->members.size()==2 && !s.busy;});
      check(menu.take_ui_sounds()==std::vector{FrontendUiSound::Advance},"Remote accepted member did not request one party join sound");
      menu.set_discord_social({});
      check(menu.frame().screen==26 && menu.frame().context==2,"Invitation completion lost party members tab");
      check(account->snapshot().party->members[0].handle!=account->snapshot().party->members[1].handle &&
            account->snapshot().party->members[0].name==account->snapshot().party->members[1].name,"Duplicate chosen names changed party authority");
      private_identity(menu);capture("party-members");
      while(menu.frame().screen!=14)menu.back();
      wait_party([](const auto &s){return s.party && s.party->members.size()==2 && s.party->members[1].appearance.has_value();});
      check(account->snapshot().party->members[1].appearance->foreground==26 &&
            account->snapshot().party->members[1].appearance->colors[0]==8,
            "Remote party row lost the accepted member's saved emblem/color bytes");
      verify_party_rows(2);capture("party-two-profile-lobby");menu.players();
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

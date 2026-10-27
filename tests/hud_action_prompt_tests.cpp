#include "halo2/hud.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/math.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include "hud_action_prompt_display_fixture.hpp"
using namespace halo2;
namespace {
void require(bool ok,const char *message){if(!ok){std::cerr<<message<<'\n';throw FormatError(message);}}
}
int main(int argc,char **argv){try{
  if(argc>=3 && std::string_view(argv[1])=="--display") {
    require(argc==3 || argc==4,"Supply --display maps [UI assets]");
    return action_prompt_display_test(argv[2],argc==4?std::filesystem::path(argv[3]):std::filesystem::path{});
  }
  require(argc==2 || argc==3,"Supply original maps [Vista UI pack]");
  const bool vista=argc==3;
  const std::string primary=vista?"\xee\x84\xb2" "E":"[E]",secondary=vista?"\xee\x84\xb2" "X":"[X]";
  const std::string pad_primary=vista?"\xee\x84\x82":"[X]",pad_secondary=vista?"\xee\x84\x83":"[Y]";
  const auto maps=std::filesystem::path(argv[1]);
  action_prompt_display_test(maps,vista?std::filesystem::path(argv[2]):std::filesystem::path{});
  Cache cache(maps/"zanzibar.map");
  auto content=cache.load_content(0,true);
  auto rules=slayer_rules({"Action prompt regression",0,{{89,5}}});
  rules.weapon=cache.slayer_starting_weapon(5);rules.score_limit=rules.time_ticks=0;
  // Controlled clear-world pose, actual map item, Match, snapshot and HUD.
  // This tests prompt delivery independently of the unfinished retail mover.
  content.collision.clear();
  MultiplayerMatch match(content,0,cache.spawns(),rules);match.configure_weapon_pickups();
  const auto id=match.join("Prompt owner");std::array<Controls,multiplayer_capacity> input{};
  for(unsigned tick=0;tick<30;++tick)match.step(input);
  const auto item=std::find_if(match.weapon_pickups().begin(),match.weapon_pickups().end(),[&](const auto &p){
    return content.weapons.at(p.definition).name.find("needler")!=std::string::npos;
  });
  require(item!=match.weapon_pickups().end(),"Zanzibar Needler fixture missing");
  auto &unit=const_cast<Unit&>(match.unit(id));unit.body.position=item->position;unit.body.velocity={};
  match.step(input);
  auto imported=cache.load_hud(content.player.datum,rules.weapon);
  imported.widgets.clear(); // Isolate the separately owned action message.
  Mesh mesh;MultiplayerHud hud(std::move(imported),maps/"fonts",mesh,4,{},std::nullopt,
      vista?std::filesystem::path(argv[2]):std::filesystem::path{});
  const auto item_handle=item->handle;
  auto state=match.state();
  require(state.players[id].action_prompt.target.object==item_handle,"Idle prompt selected a different weapon owner");
  require(hud.action_text(state,id,0)=="Hold "+primary+" to pick up \xee\x84\x9b\r\nHold "+secondary+" to dual-wield",
      "Needler did not retain its authored pickup/dual text and weapon icon");
  const auto vertices=hud.draw(state,id,0,1,640,480,0,0);
  require(!vertices.empty(),"Nearby Needler did not draw an idle pickup button prompt");
  require(mesh.materials.at(vertices.front().material).name=="action-message/font6","Full-screen prompt used the wrong source font");
  std::array<RetailFont,2> vista_sources;
  if(vista)for(unsigned f=0;f<2;++f){
    vista_sources[f]=load_retail_font(hud_menu_asset_directory(std::filesystem::path(argv[2]))/"fonts"/(f?"conduit-12":"conduit-13"));
    apply_vista_prompt_sampling(vista_sources[f],std::filesystem::path(argv[2]),f?5:6);
    const auto &raw=vista_sources[f];
    const auto name=f?"action-message/vista-buttons/font5":"action-message/vista-buttons/font6";
    const auto material=std::find_if(mesh.materials.begin(),mesh.materials.end(),[&](const auto &m){return m.name==name;});
    require(material!=mesh.materials.end() && material->diffuse.width==raw.atlas.width &&
        material->diffuse.height==raw.atlas.height && material->diffuse.pixels==raw.atlas.pixels,
        "HUD button texture differs from the selected control artwork");
    hud.prompt_gamepad(f,false);
    const auto draw=hud.draw(state,id,f,f?2:1,640,480,0,0);
    const auto vertex=std::find_if(draw.begin(),draw.end(),[&](const auto &v){return v.material==unsigned(material-mesh.materials.begin());});
    const auto &keycap=raw.glyphs.at(0xe132);
    const auto keycap_uv=retail_font_sample_uvs(raw,0xe132,0,0,keycap.width,keycap.height);
    require(vertex!=draw.end() && std::abs(vertex->uv.x-keycap_uv[0])<1e-6f &&
        std::abs(vertex->uv.y-keycap_uv[1])<1e-6f,
        "Keyboard prompt did not sample Vista keyboard_button_background0");
  }
  hud.prompt_keyboard_buttons(0,{"F","Space"});hud.prompt_gamepad(0,false);
  require(hud.action_text(state,id,0)=="Hold "+(vista?std::string("\xee\x84\xb2" "F"):std::string("[F]"))+
      " to pick up \xee\x84\x9b\r\nHold "+(vista?std::string("\xee\x84\xb2" "Space"):std::string("[Space]"))+" to dual-wield",
      "Remapped pickup buttons retained E/X labels");
  if(vista) {
    const auto draw=hud.draw(state,id,0,1,1280,800,0,0);const auto &raw=vista_sources[0];bool wider=false;
    for(unsigned c=0xe133;c<=0xe135;++c) {
      const auto &g=raw.glyphs.at(c);const auto uv=retail_font_sample_uvs(raw,c,0,0,g.width,g.height);
      for(const auto &v:draw)if(mesh.materials.at(v.material).name=="action-message/vista-buttons/font6" &&
          std::abs(v.uv.x-uv[0])<1e-6f && std::abs(v.uv.y-uv[1])<1e-6f)wider=true;
    }
    require(wider,"Long keyboard binding did not use an actual wider Vista background");
  }
  hud.prompt_keyboard_buttons(0,{"E","X"});
  for(unsigned count=1;count<=4;++count)for(unsigned view=0;view<count;++view){
    hud.prompt_gamepad(view,true);
    require(hud.action_text(state,id,view)=="Hold "+pad_primary+" to pick up \xee\x84\x9b\r\nHold "+pad_secondary+" to dual-wield",
        "Controller prompt did not match actual X/Y gameplay bindings");
    for(const auto size:std::array<Vec2,3>{{{640,480},{1920,1080},{1280,800}}}){
      const auto draw=hud.draw(state,id,view,count,unsigned(size.x),unsigned(size.y),0,0);
      require(!draw.empty(),"Split/wide view lost its prompt");
      const auto expected=count==1?"action-message/font6":"action-message/font5";
      require(mesh.materials.at(draw.front().material).name==expected,"Split prompt used the wrong source font");
      if(vista){
        const auto &raw=vista_sources[count>1];
        std::vector<Vertex> buttons;
        for(const auto &v:draw)if(mesh.materials.at(v.material).name.starts_with("action-message/vista-buttons/"))buttons.push_back(v);
        require(buttons.size()==12,"Controller message did not draw exactly two textured button glyphs");
        for(unsigned n=0;n<2;++n){
          const auto &g=raw.glyphs.at(n?0xe103:0xe102);const auto &v=buttons[n*6];
          const auto uv=retail_font_sample_uvs(raw,n?0xe103:0xe102,0,0,g.width,g.height);
          require(std::abs(v.uv.x-uv[0])<1e-6f && std::abs(v.uv.y-uv[1])<1e-6f,
              "Controller prompt did not sample the selected Vista X/Y texture");
        }
        for(const auto &v:buttons)require(v.color.x==1 && v.color.y==1 && v.color.z==1,
            "HUD tint replaced the Vista button's native color");
      }
      const auto viewport=hud_viewport(count,view,size.x*3>size.y*4);
      for(const auto &v:draw)require(std::isfinite(v.position.x) && std::isfinite(v.position.y) &&
          v.position.x>=viewport.outer.left && v.position.x<=viewport.outer.right &&
          v.position.y>=viewport.outer.top && v.position.y<=viewport.outer.bottom,
          "Action text escaped its own viewport");
    }
  }
  hud.prompt_gamepad(0,false);
  auto hidden=state;hidden.players[id].dead=true;
  require(hud.draw(hidden,id,0,1,640,480,0,0).empty(),"Dead player retained an action prompt");
  hidden=state;hidden.finished=true;
  require(hud.action_text(hidden,id,0).empty(),"Finished match retained an action prompt");
  unit.body.position={1000,1000,1000};
  require(hud.action_text(match.state(),id,0).empty(),"Out-of-range weapon retained an action prompt");
  unit.body.position=item->position;unit.body.velocity={};
  input[id].pickup=true;input[id].pickup_milliseconds=232;match.step(input);
  require(match.unit(id).weapon.source_datum!=item_handle,"Prompt pickup ignored the source hold threshold");
  input[id].pickup_milliseconds=233;match.step(input);
  require(match.unit(id).weapon.source_datum==item_handle,"Displayed pickup button did not pick up the Needler");
  require(hud.action_text(match.state(),id,0).empty(),"Taken Needler retained a stale pickup prompt");
  input[id]={};match.step(input);
  const auto second=std::find_if(match.weapon_pickups().begin(),match.weapon_pickups().end(),[&](const auto &p){
    return p.weapon && p.definition==state.players[id].action_prompt.definition;
  });
  require(second!=match.weapon_pickups().end(),"Second actual Needler missing");
  unit.body.position=second->position;unit.body.velocity={};
  unit.weapon.reserve=content.weapons.at(unit.weapon.definition).reserve;
  unit.weapon.smg.reserve=unit.weapon.reserve;
  require(hud.action_text(match.state(),id,0)=="Hold "+secondary+" to dual-wield \xee\x84\x9b",
      "Full-ammo Needler advertised an unavailable primary pickup");
  const auto rifle=std::find_if(match.weapon_pickups().begin(),match.weapon_pickups().end(),[&](const auto &p){
    return p.weapon && content.weapons.at(p.definition).name.find("battle_rifle")!=std::string::npos;
  });
  require(rifle!=match.weapon_pickups().end(),"Actual battle rifle swap fixture missing");
  const auto rifle_handle=rifle->handle;
  unit.body.position=rifle->position;unit.body.velocity={};
  require(match.action_prompt(id).target.index==2 && hud.action_text(match.state(),id,0).starts_with("Hold "+primary+" to swap for "),
      "Full inventory did not advertise the actual swap action");
  input[id].pickup=true;input[id].pickup_milliseconds=233;match.step(input);
  require(match.unit(id).weapon.source_datum==rifle_handle,"Displayed swap action did not replace the selected weapon");
  input[id]={};match.step(input);
  // Actual static keypad owner, source model bound and missing-use-marker
  // node0 basis. The controlled player eye faces its front within reach.
  std::uint32_t keypad=null_datum;Vec3 forward{},center{};
  for(const auto &[handle,owner]:match.simulation().devices().owners()){
    const auto &definition=*content.devices[owner.placement];
    if(!definition.control)continue;
    require(!definition.control_nodes.empty(),"Zanzibar keypad has no authored target node");
    const auto node=compose_transform(owner.root,definition.control_nodes[0]);
    center=source_shadow_sphere(definition.control_bounds,node,owner.root.scale).center;
    forward=node.basis[0];keypad=handle;break;
  }
  require(keypad!=null_datum,"Zanzibar keypad owner missing");
  unit.body.position=center+forward*.5f-Vec3{0,0,unit.definition.camera_height};unit.body.velocity={};
  unit.yaw=std::atan2(-forward.y,-forward.x);unit.pitch=std::asin(-forward.z);
  auto terminal=match.state();
  require(terminal.players[id].action_prompt.target.object==keypad,"Looking at Zanzibar keypad did not select it");
  require(hud.action_text(terminal,id,0)=="Hold "+primary+" to open the gate","Zanzibar lost its authored gate action text");
  unit.yaw+=3.14159265f;
  require(hud.action_text(match.state(),id,0).empty(),"Looking away retained the keypad prompt");
  unit.yaw-=3.14159265f;
  // Exercise the actual host snapshot route used by remote HUDs.
  LanHost host(match,0,0x151110);LanClient peer(host.join_address(),0x151110,1);
  std::array<Controls,1> neutral{};
  for(unsigned attempt=0;attempt<64 && !peer.ready();++attempt){peer.send(neutral);host.receive(input);host.broadcast();peer.receive();}
  require(peer.ready(),"Prompt LAN peer did not join");
  require(peer.state().players[id].action_prompt.target==terminal.players[id].action_prompt.target &&
      hud.action_text(peer.state(),id,0)=="Hold "+primary+" to open the gate","LAN snapshot lost authoritative action text");
  input={};input[id].yaw=unit.yaw;input[id].pitch=unit.pitch;
  input[id].pickup=true;input[id].pickup_milliseconds=232;match.step(input);
  require(match.simulation().devices().groups()[0].value==0,"Keypad ignored the source hold threshold");
  input[id].pickup_milliseconds=233;match.step(input);
  require(match.simulation().devices().groups()[0].value==1,"Displayed interact button did not open the gate");
  require(hud.action_text(match.state(),id,0).empty(),"Used one-shot gate retained an interaction prompt");
  host.broadcast();peer.receive();
  require(hud.action_text(peer.state(),id,0).empty(),"LAN HUD retained the used gate prompt");
  std::cout<<"PASS authored Needler/keypad text, idle contexts, available actions, source hold threshold, "
      "keyboard/controller labels, death/range/aim/use suppression, full/split/wide bounds and LAN publication\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

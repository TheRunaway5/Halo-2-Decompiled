#include "halo2/hud.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <sstream>
#include <stdexcept>
using namespace halo2;
namespace {
void check(bool yes,const std::string &message){if(!yes)throw std::runtime_error(message);}
Texture solid(unsigned rgb,unsigned alpha) {
  Texture t;t.width=t.height=32;t.format=11;t.pixels.resize(32*32*4);
  for(unsigned n=0;n<32*32;++n) {
    for(unsigned c=0;c<3;++c)t.pixels[n*4+c]=std::byte(rgb);
    t.pixels[n*4+3]=std::byte(alpha);
  }
  return t;
}
void passes() {
  MotionSensorArtwork art{solid(0,128),solid(255,128),solid(255,0),solid(255,0)};
  MotionSensorRenderPlan plan;plan.visible=true;plan.pulse=1;
  plan.blips.push_back({{32,32},.5f,1,{1,0,0},false});
  const auto image=source_motion_sensor_composite(plan,art,{0,0,0});
  const auto at=(32*64+32)*4;
  check(image.pixels[at]==std::byte{32} && image.pixels[at+1]==std::byte{0} &&
        image.pixels[at+3]==std::byte{0},"Original RGB-only radar passes wrote alpha and darkened the HUD");
  check(image.pixels[(32*64+35)*4]==std::byte{0},"Original blip half extent intensity*2 changed");
  // Unchanged254A67/254A7A sets the sweep sampler's ARGB border to
  //46000000. Its nonzero alpha retains a contact outside the expanding
  //sweep texture; transparent black incorrectly erases that contact.
  art={solid(0,0),solid(255,255),solid(255,0),solid(255,0)};
  plan.pulse=8;plan.blips={{{8,32},1,1,{1,0,0},false}};
  const auto border=source_motion_sensor_composite(plan,art,{0,0,0});
  check(border.pixels[(32*64+8)*4]==std::byte{70},
        "Original sweep border alpha erased a dot outside the expanding pulse");
}
void live(const std::filesystem::path &maps) {
  Cache cache(maps/"lockout.map");const auto level=cache.load_content(0,true);
  auto content=cache.load_hud(level.player_representations.at(3).biped.datum,cache.slayer_starting_weapon());
  check(content.sensor.range==8 && content.sensor.minimum_speed>.029f &&
        content.sensor.minimum_speed<.031f && content.sensor.sneak_speed>.89f,
        "Motion sensor did not use actual hudg/player-information fields");
  std::erase_if(content.widgets,[](const auto &w){return w.name!="motion_tracker_background";});
  // Actual artwork, every30Hz phase of the original2.1-second sweep. The
  // intermediate target is cleared to zero and all three passes write RGB
  // only (unchanged254200/2548F0 color-write state00010101).
  for(unsigned tick=0;tick<63;++tick) {
    const auto plan=source_motion_sensor_render_plan({},0,0,{0,0},{0,0},tick,tick_seconds);
    const auto page=source_motion_sensor_composite(plan,content.sensor.artwork,{206.f/255,143.f/255,222.f/255});
    for(unsigned at=3;at<page.pixels.size();at+=4)
      check(page.pixels[at]==std::byte{},"Radar pulse wrote opacity into its RGB-only target");
    for(unsigned pixel:{0u,63u,63u*64,64u*64-1})for(unsigned c=0;c<4;++c)
      check(page.pixels[pixel*4+c]==std::byte{},"Radar pulse leaked into the square target corners");
  }
  Mesh mesh;MultiplayerHud hud(content,maps/"fonts",mesh,4);
  MultiplayerState state;
  for(unsigned n=0;n<4;++n) {
    auto &p=state.players[n];p.connected=true;p.shield=p.health=1;
    p.source_player=0x10000+n;p.source_unit=0x20000+n;p.source_membership=n;
    p.position={float(n)*2,0,0};p.yaw=0;
    if(n%2==0){p.source_configuration=SourcePlayerConfiguration{};(*p.source_configuration)[0x44]=1;}
  }
  auto image=[&](unsigned view) -> const Texture & {
    const auto material=hud.motion_sensor_material(view);
    check(material.has_value(),"Live radar has no authored composition material");
    return mesh.materials.at(*material).diffuse;
  };
  auto update=[&] {
    ++state.tick;
    for(unsigned view=0;view<4;++view)hud.update_motion_sensor(state,view,view);
  };
  update();const auto first=image(0).pixels;
  for(unsigned n=0;n<30;++n)update();
  const auto idle=image(0).pixels;
  check(first!=idle,"Live radar does not pulse with source ticks");
  hud.update_motion_sensor(state,0,0);
  check(idle==image(0).pixels,"Rendering the same tick advanced the radar trail");
  // Compare simultaneous source ticks with/without a genuinely moving enemy.
  auto changed=[&](Vec3 velocity,std::uint8_t action,Vec3 position,bool dead=false,
                   std::uint8_t grenade_phase=0) {
    Mesh a,b;MultiplayerHud absent(content,maps/"fonts",a,1),present(content,maps/"fonts",b,1);
    auto baseline=state,contacts=state;
    for(unsigned n=2;n<4;++n)baseline.players[n].connected=contacts.players[n].connected=false;
    baseline.players[1].velocity={};baseline.players[1].motion_sensor_control=0;
    contacts.players[1].velocity=velocity;contacts.players[1].motion_sensor_control=action;
    contacts.players[1].grenade_action.source[0xc]=std::byte{grenade_phase};
    contacts.players[1].position=position;contacts.players[1].dead=dead;
    for(unsigned tick=31;tick<41;++tick) {
      baseline.tick=contacts.tick=tick;
      absent.update_motion_sensor(baseline,0,0);present.update_motion_sensor(contacts,0,0);
    }
    const auto &x=a.materials.at(*absent.motion_sensor_material(0)).diffuse.pixels,
               &y=b.materials.at(*present.motion_sensor_material(0)).diffuse.pixels;
    return x!=y;
  };
  check(changed({2,0,0},0,{2,0,0}),"Moving nearby player never appears on live radar");
  check(!changed({.8f,0,0},0,{2,0,0}),"Sneaking player leaked onto radar below original speed threshold");
  check(changed({},1,{2,0,0}) && changed({},0x20,{2,0,0}),"Either stationary weapon trigger must reveal radar contacts");
  check(changed({},0,{2,0,0},false,1) && changed({},0,{2,0,0},false,2) &&
        !changed({},0,{2,0,0},false,3),
        "Radar did not consume the original retained grenade phases1/2/3");
  check(!changed({2,0,0},0,{8,0,0}) && !changed({2,0,0},0,{2,0,0},true),
        "Out-of-range or released dead unit appeared on radar");
  state.players[1].velocity={2,0,0};
  for(unsigned n=0;n<10;++n)update();
  check(image(0).pixels!=image(1).pixels,"Independent local radar views share their contacts or palette");
  state.players[1].velocity={};
  for(unsigned n=0;n<10;++n)update();
  Mesh clean_mesh;MultiplayerHud clean(content,maps/"fonts",clean_mesh,1);
  clean.update_motion_sensor(state,0,0);
  check(image(0).pixels==clean_mesh.materials.at(*clean.motion_sensor_material(0)).diffuse.pixels,
        "Stopped player did not fade out after original ten-sample history");
  for(unsigned players=1;players<=4;++players)for(unsigned view=0;view<players;++view) {
    const auto vertices=hud.draw(state,view,view,players,1280,720,25,14400);
    check(std::count_if(vertices.begin(),vertices.end(),[&](const auto &v){
      return v.material==*hud.motion_sensor_material(view);})==6,"Split view did not submit radar composition");
    state.players[view].source_hud_zoom_mask=0x400;
    const auto zoomed=hud.draw(state,view,view,players,1280,720,25,14400);
    check(std::none_of(zoomed.begin(),zoomed.end(),[&](const auto &v){
      return v.material==*hud.motion_sensor_material(view);}),"Binocular zoom did not suppress radar");
    state.players[view].source_hud_zoom_mask=0x40;
  }
}
void original_capture(const std::filesystem::path &maps,const std::filesystem::path &capture) {
  // Original game in xemu, retained capture20, sweep9328 then mask9335.
  // The UV edge words are actual original vertex data, not a native clock.
  Cache cache(maps/"lockout.map");const auto level=cache.load_content(0,true);
  const auto content=cache.load_hud(level.player_representations.at(2).biped.datum,cache.slayer_starting_weapon());
  auto read=[](const std::filesystem::path &p) {
    std::ifstream in(p,std::ios::binary|std::ios::ate);
    if(!in)throw std::runtime_error("Original radar capture unavailable: "+p.string());
    std::vector<char> b(std::size_t(in.tellg()));in.seekg(0);in.read(b.data(),b.size());return b;
  };
  MotionSensorRenderPlan plan;plan.visible=true;
  Vec3 hud_color{117.f/255,186.f/255,1};
  const bool contacts=std::filesystem::exists(capture/"original-plan.tsv");
  if(contacts) {
    std::ifstream in(capture/"original-plan.tsv");unsigned count;
    in>>plan.pulse>>hud_color.x>>hud_color.y>>hud_color.z>>count;
    check(count<=160,"Original capture contact count changed");
    for(unsigned n=0;n<count;++n) {
      MotionSensorDrawBlip b;
      in>>b.point.x>>b.point.y>>b.scale>>b.intensity>>b.color.x>>b.color.y>>b.color.z>>b.custom;
      plan.blips.push_back(b);
    }
    check(bool(in),"Original capture plan incomplete");
  } else {
    const auto uv=read(capture/"9328-vertex-buffer-1.bin");check(uv.size()>=8,"Original sweep UVs missing");
    float left,right;std::memcpy(&left,uv.data(),4);std::memcpy(&right,uv.data()+4,4);
    plan.pulse=left-right;
  }
  const auto page=source_motion_sensor_composite(plan,content.sensor.artwork,hud_color);
  {std::ofstream out(capture/"native-sweep.rgba",std::ios::binary);
   out.write(reinterpret_cast<const char *>(page.pixels.data()),page.pixels.size());}
  const auto expected=read(capture/(contacts?"original-target.rgba":"9335-after.bin"));
  check(expected.size()==page.pixels.size(),"Original radar target size changed");
  unsigned different=0,maximum=0,total=0;
  for(unsigned p=0;p<64*64;++p)for(unsigned c=0;c<4;++c) {
    // Original captured target is B8G8R8A8; host decoded page is RGBA.
    const int a=std::to_integer<unsigned>(page.pixels[p*4+c]);
    const int b=static_cast<unsigned char>(expected[p*4+(contacts?c:c==0?2:c==2?0:c)]);
    const unsigned error=unsigned(std::abs(a-b));maximum=std::max(maximum,error);total+=error;different+=error!=0;
  }
  std::cout<<"Original xemu radar comparison: "<<different<<" differing channels, maximum error "<<maximum
           <<"/255, total error "<<total<<"; pulse="<<plan.pulse<<'\n';
  check(maximum==0,"Original captured sweep pixels differ");
}
void original_viewports(const std::filesystem::path &maps,const std::filesystem::path &fixture) {
  Cache cache(maps/"lockout.map");const auto level=cache.load_content(0,true);
  std::ifstream in(fixture);check(bool(in),"Original radar vertex fixture unavailable");
  std::string line;unsigned cases=0,additional_video_cases=0;
  while(std::getline(in,line)) {
    if(line.empty() || line.front()=='#')continue;
    unsigned flags,players,view,appearance;float left,top,right,bottom;
    std::istringstream row(line);row>>flags>>players>>view>>appearance>>left>>top>>right>>bottom;
    check(bool(row),"Original radar vertex fixture incomplete");
    // Keep the user's current HUD safe area. Original flags8/9 select a
    // different32/24-pixel overscan profile; it is captured for reference,
    // but no desktop HUD option selects that profile today.
    if(flags&8){++additional_video_cases;continue;}
    const bool wide=flags&1;const unsigned width=wide?1280:640,height=wide?720:480;
    const auto cell=hud_viewport(players,view,wide).outer;
    const float scale=std::min(width/640.f,height/480.f);
    auto content=cache.load_hud(level.player_representations.at(appearance?3:2).biped.datum,
                                cache.slayer_starting_weapon());
    std::erase_if(content.widgets,[](const auto &w){return w.name!="motion_tracker_background";});
    Mesh mesh;MultiplayerHud hud(content,maps/"fonts",mesh,players);
    MultiplayerState state;state.tick=40;auto &p=state.players[view];p.connected=true;
    p.source_unit=0x20000+view;p.source_player=0x10000+view;p.source_membership=view;
    if(appearance){p.source_configuration=SourcePlayerConfiguration{};(*p.source_configuration)[0x44]=1;}
    hud.update_motion_sensor(state,view,view);
    const auto vertices=hud.draw(state,view,view,players,width,height,25,14400);
    std::vector<Vertex> sensor;
    for(const auto &v:vertices)if(v.material==*hud.motion_sensor_material(view))sensor.push_back(v);
    check(sensor.size()==6,"Original radar fixture did not produce one quad");
    // Current native widescreen layout expands each outer cell while keeping
    // original pixel margins and uniform artwork scale. Compare that explicit
    // transform of the original vertices, including the spanning third view.
    auto x=[&](float value){return cell.left*width/640+(value-cell.left)*scale;};
    auto y=[&](float value){return cell.top*height/480+(value-cell.top)*scale;};
    const std::array<Vec2,6> expected{{{x(left),y(top)},{x(right),y(top)},
      {x(left),y(bottom)},{x(right),y(top)},{x(right),y(bottom)},{x(left),y(bottom)}}};
    for(unsigned n=0;n<6;++n) {
      const float actual_x=sensor[n].position.x*width/640,actual_y=sensor[n].position.y*height/480;
      check(std::abs(actual_x-expected[n].x)<.001f && std::abs(actual_y-expected[n].y)<.001f,
          "Radar vertex differs from Xbox: flags="+std::to_string(flags)+" players="+std::to_string(players)+
          " view="+std::to_string(view)+" vertex="+std::to_string(n)+" actual="+std::to_string(actual_x)+","+
          std::to_string(actual_y)+" expected="+std::to_string(expected[n].x)+","+std::to_string(expected[n].y));
      check(actual_x>=cell.left*width/640 && actual_x<=cell.right*width/640 &&
            actual_y>=cell.top*height/480 && actual_y<=cell.bottom*height/480,
          "Radar quad crosses its local viewport");
    }
    ++cases;
  }
  check(cases==40 && additional_video_cases==40,"Original radar fixture lost a video/species/local-view case");
  std::cout<<cases<<" original Xbox radar quads matched across every current local cell and aspect; "
           <<additional_video_cases<<" additional overscan-profile cases retained as reference\n";
}
}
int main(int argc,char **argv){try{
  check(argc>=2 && argc<=4,"Supply original maps and optional capture directory or --viewports FIXTURE");passes();live(argv[1]);
  if(argc==3)original_capture(argv[1],argv[2]);
  if(argc==4){check(std::string(argv[2])=="--viewports","Unknown radar fixture option");original_viewports(argv[1],argv[3]);}
  std::cout<<"Live radar pulse, contacts, sneak, both triggers, grenade phases, strict range, trail, zoom and four independent views passed\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

#include "halo2/vista_gameplay_fade.hpp"
#include "halo2/source_player_lives.hpp"
#include <bit>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;using J=nlohmann::json;
namespace {
void require(bool v,const char* why){if(!v)throw std::runtime_error(why);}
VistaGameplayFadeState state(const J& q){VistaGameplayFadeState s;s.phase_counter=q.value("counter",0);s.opacity_bits=q.value("opacity",std::array<std::uint32_t,4>{});s.activation_mask=q.value("mask",0);s.rendered_frames=q.value("frames",std::array<std::uint8_t,4>{});return s;}
J record(const VistaGameplayFadeState& s){return J{{"counter",s.phase_counter},{"opacity",s.opacity_bits},{"mask",s.activation_mask},{"frames",s.rendered_frames}};}
}
int main(int argc,char** argv){try{
 require(argc==3,"Original gameplay fade and owner fixture arguments");std::ifstream file(argv[1]);require(bool(file),"Open original gameplay fade fixture");std::string line;unsigned ticks{},renders{},n{};
 while(std::getline(file,line)){
  ++n;const auto r=J::parse(line);const auto& q=r.at("input");auto s=state(r.at("before"));const VistaLoadingFloatingPoint fp{r.at("cw"),r.at("mxcsr")};
  if(r.at("kind")=="tick"){
   VistaGameplayFadeTick in;in.engine_present=q.value("engine",1);in.playback=q.value("mode",1)==4;in.phase=q.value("phase",2);in.pending_phase=q.value("pending",2);in.tick_rate=q.value("rate",60);in.tick=q.value("tick",1000u);in.round_start_tick=q.value("start",0u);J calls=J::array();
   const auto users=q.value("users",J{{"0",J::object()}});
   for(auto it=users.begin();it!=users.end();++it){const auto u=std::stoul(it.key());const auto p=it.value();VistaGameplayFadePlayer player;player.player_present=p.value("player",1);player.unit_present=p.value("unit",1);player.respawn_countdown=p.value("respawn",0);player.active_player=[&,u,p]{calls.push_back({"active",u});return bool(p.value("active",0));};in.users[u]=player;}
   vista_gameplay_fade_tick(s,in,fp);
   if(record(s)!=r.at("after") || calls!=r.at("calls")){std::cerr<<record(s)<<" expected "<<r.at("after")<<" calls "<<calls<<" expected "<<r.at("calls")<<"\n";throw std::runtime_error("Original gameplay fade tick mismatch at"+std::to_string(n));}
   ++ticks;
  }else{
   VistaGameplayFadeRender in;in.engine_present=q.value("engine",1);in.cubemap_capture_in_progress=q.value("blocked",0);in.user=q.value("user",0u);in.unresolved_world_geometry_count=q.value("windows",1);in.tick_rate=q.value("rate",60);
   const auto color=vista_gameplay_fade_render(s,in,fp);J colors=J::array();if(color)colors.push_back(*color);
   if(record(s)!=r.at("after") || colors!=r.at("colors"))throw std::runtime_error("Original gameplay fade render mismatch at"+std::to_string(n));
   ++renders;
  }
 }
 require(ticks>4000 && renders>1000,"Original gameplay fade fixture coverage");VistaGameplayFadeState activated;vista_gameplay_fade_activate(activated,2);require(activated.opacity_bits[2]==0x3f800000 && activated.activation_mask==4,"Original user activation stores");
 std::ifstream owners_file(argv[2]);require(bool(owners_file),"Open original gameplay fade owners fixture");const auto owners=J::parse(owners_file);
 for(const auto& q:owners.at("players")){
   const auto& p=q.at("native");
   require(source_player_respawn_allowed(q.at("limit"),p.at("flags"),p.at("membership"),p.at("unit"),p.at("lives"))==bool(q.at("result").get<unsigned>()),"Native player eligibility differs from original PC46C54F");
 }
 for(const auto& q:owners.at("guards")){
   VistaGameplayFadeState s;s.opacity_bits[0]=0x3f800000;s.activation_mask=1;const auto before=s;
   VistaGameplayFadeRender in;in.engine_present=true;in.cubemap_capture_in_progress=q.at("taking").get<unsigned>() && q.at("cubemap").get<unsigned>();in.tick_rate=60;in.user=0;
   const auto color=vista_gameplay_fade_render(s,in,{0x27f,0x1f80});
   if(q.at("result")==1)require(!color && s==before,"Original cubemap guard must suppress all fade writes");
   else require(color==0xff000000 && !s.activation_mask,"Idle capture permits original render branch");
 }
 for(const auto& q:owners.at("counts")){
   VistaGameplayFadeState s;s.opacity_bits[0]=0x3f800000;s.activation_mask=1;VistaGameplayFadeRender in;in.engine_present=true;in.user=0;in.tick_rate=60;in.unresolved_world_geometry_count=std::bit_cast<std::int32_t>(q.at("after").get<std::uint32_t>());
   require(vista_gameplay_fade_render(s,in,{0x27f,0x1f80})==0xff000000,"Geometry cache count color");
   require(s.activation_mask==(in.unresolved_world_geometry_count>0?1:0),"Original signed geometry cache pending guard");
 }
 require(owners.at("players").size()==1024,"Original PC/native eligibility corpus coverage");
 std::cout<<ticks<<" original tick projections, "<<renders<<" original black-quad renders, "<<owners.at("players").size()<<" PC/native player queries, "<<owners.at("guards").size()<<" cubemap guards, "<<owners.at("counts").size()<<" geometry count tails; passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

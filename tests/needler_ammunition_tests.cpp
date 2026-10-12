#include "halo2/fp_registration.hpp"
#include "halo2/first_person_ik.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
void check(bool b,const char *why){if(!b)throw FormatError(why);}
int main(int argc,char **argv)try {
  check(argc==3,"Supply MAP ORIGINAL_FIXTURE");Cache cache(argv[1]);
  const auto binding=cache.load_first_person_binding(0xe34b01d6,2);
  check(binding.graph==0xe3c50250 && binding.ammunition[0] && binding.ammunition[1],"Needler ammunition overlay missing from actual binding");
  for(unsigned i=0;i<2;++i)check(binding.ammunition[i]->source_index==i && binding.ammunition[i]->animation_type==2,"Wrong authored single/dual ammunition selection");
  std::ifstream f(argv[2]);check(bool(f),"Original ammunition fixture missing");std::string line;unsigned cases{};
  while(std::getline(f,line)) {
    if(line.empty() || line[0]=='#')continue;
    unsigned dual;int loaded,reserve,elapsed;std::uint32_t activity,expected;std::string hex;
    std::istringstream row(line);check(bool(row>>dual>>loaded>>reserve>>elapsed>>std::hex>>activity>>expected>>hex),"Truncated original ammunition fixture");
    const auto &clip=*binding.ammunition.at(dual);const auto count=clip.nodes.size();check(hex.size()==count*64,"Original local pose extent");
    auto frame=first_person_ammunition_frame(loaded,std::bit_cast<std::int16_t>(std::uint16_t(loaded+reserve)),30,elapsed,2,activity,clip.frame_count);
    check(std::bit_cast<std::uint32_t>(frame)==expected,"Original ammunition frame mismatch");
    std::vector<BonePose> locals(count);
    for(unsigned i=0;i<count;++i){locals[i].translation={7+i*.125f,8+i*.25f,9+i*.5f};locals[i].scale=2+i*.03125f;}
    first_person_ammunition_overlay(locals,clip,frame);
    for(unsigned i=0;i<count;++i) {
      const auto &p=locals[i];const std::array<float,8> values{p.rotation[0],p.rotation[1],p.rotation[2],p.rotation[3],p.translation.x,p.translation.y,p.translation.z,p.scale};
      for(unsigned j=0;j<8;++j){const auto at=i*64+j*8;std::uint32_t word{};
        for(unsigned k=0;k<4;++k)word|=std::stoul(hex.substr(at+k*2,2),nullptr,16)<<(k*8);
        if(std::bit_cast<std::uint32_t>(values[j])!=word){std::cerr<<"case="<<cases<<" dual="<<dual<<" loaded="<<loaded<<" frame="<<frame<<" node="<<i<<" component="<<j<<" actual="<<std::hex<<std::bit_cast<std::uint32_t>(values[j])<<" expected="<<word<<std::dec<<'\n';throw FormatError("Original partial ammunition pose mismatch");}
      }
    }
    ++cases;
  }
  check(cases==470,"Original ammunition fixture coverage changed");
  // The same partial channel must affect live shared weapon/arms posing.
  const auto weapon=cache.load_model(binding.weapon_model),arms=cache.load_model(binding.arms_model);
  const auto idle=cache.load_animation(binding.graph,binding.idle_index);const auto ik=cache.load_first_person_ik(binding.graph,binding.weapon_model,binding.arms_model);
  const auto &ammo=*binding.ammunition[0];const Transform root;
  auto full=pose_first_person_models_world_frame(weapon,arms,idle,0,{},root,ik,0,false,nullptr,&ammo,30);
  auto empty=pose_first_person_models_world_frame(weapon,arms,idle,0,{},root,ik,0,false,nullptr,&ammo,0);
  check(full.weapon.size()==empty.weapon.size() && full.weapon_nodes.size()==weapon.bones.size(),"Overlay lost model nodes/geometry");
  unsigned changed{};for(unsigned i=0;i<full.weapon.size();++i)if(full.weapon[i].position.x!=empty.weapon[i].position.x || full.weapon[i].position.y!=empty.weapon[i].position.y || full.weapon[i].position.z!=empty.weapon[i].position.z)++changed;
  check(changed>0,"Ammo channel never moved visible needle geometry");
  std::cout<<"PASS "<<cases<<" exact original single/dual ammunition selectors and 55-node partial poses; full/empty changed vertices="<<changed<<'\n';
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

#include "halo2/gameplay_fov.hpp"
#include "halo2/pc_mouse_look.hpp"
#include "halo2/unit_scope_camera.hpp"
#include "halo2/unit_zoom.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

using namespace halo2;
namespace {
unsigned samples{};
void check(bool value,const std::string &reason) {
  if(!value)throw std::runtime_error(reason);
}
float real(const nlohmann::json &bits) {
  return std::bit_cast<float>(std::uint32_t(std::stoul(bits.get<std::string>(),nullptr,16)));
}
nlohmann::json load(const char *path) {
  std::ifstream input(path);check(bool(input),"Missing original scope fixture");
  return nlohmann::json::parse(input);
}
void screen_motion(float current,float unzoomed) {
  for(float baseline:{.0025f,.003f})for(bool invert:{false,true}) {
    PcMouseLook mouse(baseline);mouse.projection(current,unzoomed);
    PcKeyboardSettings settings;settings.invert=invert;settings.sensitivity=3;
    float yaw{},pitch{};mouse.motion(1,-1,settings,yaw,pitch);
    const double expected=double(baseline)/std::tan(double(unzoomed)*.5);
    const double actual=-double(yaw)/std::tan(double(current)*.5);
    check(std::abs(actual-expected)<expected*3e-6,"Authored scope loses fine mouse control");
    check(pitch==-yaw*(invert?-1.f:1.f),"Scoped pitch gain differs from yaw");
    ++samples;
  }
}
}
int main(int argc,char **argv)try {
  check(argc==3 || argc==4,"Supply original optic execution report, camera constants and optional report path");
  const auto optics=load(argv[1]),constants=load(argv[2]);
  check(optics.at("original_images_equal")==true && optics.at("images").size()==2,
      "Optic fixture lacks both original executable results");
  check(optics.at("images").at(1).at("sha256")==
      "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
      "Optic authority must be the original TU1.5 executable");
  const auto &f=constants.at("fields_in_native_struct_order");
  const auto operand=[&](unsigned i) {
    const auto s=f.at(i).at("bits").get<std::string>();
    std::uint64_t bits{};
    for(unsigned n=0;n<s.size()/2;++n)bits|=std::stoull(s.substr(n*2,2),nullptr,16)<<(n*8);
    return bits;
  };
  const auto r=[&](unsigned i){return std::bit_cast<float>(std::uint32_t(operand(i)));};
  const UnitScopeCameraConstants c{r(0),r(1),r(2),r(3),r(4),r(5),r(6),r(7),r(8),r(9),r(10),
      r(11),r(12),r(13),r(14),r(15),r(16),r(17),r(18),r(19),r(20),r(21),r(22),
      std::bit_cast<double>(operand(23))};
  const auto &b=optics.at("binocular");
  const BinocularZoomDefinition binocular{b.at("levels"),real(b.at("minimum_bits")),real(b.at("maximum_bits"))};
  std::map<std::string,ZoomWeaponDefinition> weapons;
  std::map<std::string,unsigned> magazines;
  unsigned cases{},levels{};
  for(const auto &row:optics.at("records")) {
    const auto name=row.at("weapon").get<std::string>();
    const ZoomWeaponDefinition definition{std::uint32_t(std::stoul(row.at("flags_bits").get<std::string>(),nullptr,16)),
        row.at("levels"),real(row.at("minimum_bits")),real(row.at("maximum_bits"))};
    const std::vector<std::int16_t> modes(row.at("magazine_count").get<unsigned>(),0);
    const ZoomWeaponView weapon{definition,modes};
    const auto value=source_zoom_magnification(row.at("selector"),&weapon,binocular);
    check(value && std::bit_cast<std::uint32_t>(*value)==
        std::bit_cast<std::uint32_t>(real(row.at("original_magnification_bits"))),name+" differs from TU1.5 XMM0");
    weapons.emplace(name,definition);magazines.emplace(name,modes.size());++cases;
  }
  const std::set<std::string> expected{
      "objects\\weapons\\rifle\\battle_rifle\\battle_rifle",
      "objects\\weapons\\rifle\\covenant_carbine\\covenant_carbine",
      "objects\\weapons\\rifle\\sniper_rifle\\sniper_rifle",
      "objects\\weapons\\rifle\\beam_rifle\\beam_rifle",
      "objects\\weapons\\support_high\\rocket_launcher\\rocket_launcher",
      "objects\\weapons\\support_high\\flak_cannon\\flak_cannon"};
  std::set<std::string> actual;for(const auto &[name,w]:weapons){actual.insert(name);levels+=w.levels;}
  check(actual==expected && cases==20 && levels==8,"Incomplete original optic matrix");
  // The no-optic entry exercises the authored matg binocular fallback too.
  const auto &smg=optics.at("binocular_weapon");
  weapons.emplace("binocular",ZoomWeaponDefinition{
      std::uint32_t(std::stoul(smg.at("flags_bits").get<std::string>(),nullptr,16)),
      smg.at("levels"),real(smg.at("minimum_bits")),real(smg.at("maximum_bits"))});
  magazines.emplace("binocular",smg.at("magazine_count"));
  check(optics.at("camera_fields").size()==4,"Missing authored campaign/multiplayer camera fields");
  for(const auto &[name,definition]:weapons) {
    std::vector<std::int16_t> modes(magazines.at(name),0);
    const ZoomWeaponView weapon{definition,modes};
    const int count=definition.levels?definition.levels:binocular.levels;
    UnitZoomState unit;std::int16_t selector=-1;
    const UnitZoomContext admitted{0xffffffffu,0,{&weapon,nullptr}};
    for(int level=0;level<count;++level) {
      selector=source_zoom_next(selector,&weapon,binocular);check(selector==level,name+" selector order");
      source_zoom_request(unit,selector);source_zoom_update(unit,admitted,true);
      check(source_zoom_hud_mask(source_zoom_hud_status(unit.current,&weapon))==
          ((level?0x100:0x80)|(definition.levels?0:0x400)),name+" HUD zoom conditions");
    }
    check(source_zoom_next(selector,&weapon,binocular)==-1,name+" fails to cycle out");
    for(std::int16_t mode=1;!modes.empty() && mode<=6;++mode) {
      modes[0]=mode;check(!source_zoom_admitted(admitted),name+" admits an active magazine");
      source_zoom_update(unit,admitted,true);check(unit.current==-1,name+" retains scoped unit during magazine action");
    }
    if(!modes.empty())modes[0]=0;
    source_zoom_reset(unit);
    check(unit.current==-1 && unit.requested==-1 && !unit.motion &&
        source_zoom_hud_mask(source_zoom_hud_status(unit.current,&weapon))==0x40,name+" reset/HUD conditions");
    // Compose the verified original scalar camera with real authored optics.
    // This is component evidence; it supplies no live unit or render result.
    for(const auto &camera:optics.at("camera_fields"))for(unsigned fov:{70u,78u,120u})
    for(float aspect:{2.f/3.f,4.f/3.f,16.f/9.f,32.f/9.f})for(float hz:{30.f,60.f,144.f}) {
      const float authored=real(camera.at("fov_bits")),wide=gameplay_field_of_view(fov);
      const float wide_projection=gameplay_camera_angles(wide,wide,aspect).projection_angle;
      auto observer=source_scope_camera_initial(c);UnitScopeCameraProducer producer{};
      for(int step=-1;step<=count;++step) {
        const int selected=step==count?-1:step;
        const auto mag=source_zoom_magnification(std::int16_t(selected),&weapon,binocular);
        check(bool(mag),name+" lost authored magnification");
        const float target=source_scope_camera_fov(authored,*mag,c);
        for(unsigned frame=0;frame<90;++frame) {
          UnitScopeCameraPacket packet{1,0,0,0,0};
          source_scope_camera_target(producer,packet,target,c);
          source_scope_camera_register(observer,packet);source_scope_camera_adopt(observer,packet,c);
          check(!source_scope_camera_motion(observer,source_scope_camera_dt(1.f/hz,c),c),
              name+" requires an unavailable coupled camera owner");
          const float current=source_scope_camera_render_fov(observer,c);
          const float horizontal=gameplay_observer_field_of_view(current,authored,fov);
          screen_motion(gameplay_camera_angles(horizontal,wide,aspect).projection_angle,wide_projection);
        }
        check(observer.current==target*c.packet_multiplier,name+" transition failed to reach original lens");
      }
    }
    std::cout<<"PASS "<<name<<"\n";
  }
  std::cout<<cases<<" exact original magnifications, "<<levels<<" authored optic levels plus binoculars, "
      <<samples<<" camera/mouse samples passed\n";
  if(argc==4) {
    std::ofstream report(argv[3]);check(bool(report),"Cannot write scope regression report");
    report<<nlohmann::json{{"passed",true},{"original_magnification_cases",cases},
        {"authored_optics",expected},{"authored_levels",levels},{"binocular_fallback",true},
        {"camera_mouse_samples",samples},{"component_validation_only",true},
        {"original_images",optics.at("images")}}.dump(2)<<'\n';
  }
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

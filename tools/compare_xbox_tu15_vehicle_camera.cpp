// Compare the port's complete projection bank to an unchanged TU1.5 scene.
// Pose and FOV inputs come from the original completed observer, never from
// the reference projection output. This validates the native presentation
// boundary; independent director replays validate its producer.
#include "halo2/gameplay_fov.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/original_data.hpp"
#include "halo2/observer_motion.hpp"
#include <fstream>
#include <iostream>
#include <cstring>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
std::vector<std::byte> read(const std::filesystem::path &p,std::size_t n) {
 std::ifstream f(p,std::ios::binary);if(!f)throw FormatError("Missing original camera bank: "+p.string());
 std::vector<std::byte> b(n);f.read(reinterpret_cast<char*>(b.data()),n);
 if(std::size_t(f.gcount())!=n)throw FormatError("Truncated original camera bank");return b;
}
template<class T> T at(const std::vector<std::byte>&b,unsigned p) {
 if(p+sizeof(T)>b.size())throw FormatError("Camera field outside bank");
 T v;std::memcpy(&v,b.data()+p,sizeof(v));return v;
}
void authenticate(const std::filesystem::path &path) {
 std::ifstream input(path);nlohmann::json state;input>>state;
 if(state.at("reference")!="OG Xbox TU1.5" || !state.at("whole_live_text_matches_original").get<bool>() ||
    state.at("text_sha256")!="d9c6fd7e7475862b1f19c2da80d7f2f6dc41da53619991002e9546e5f9e629fe" ||
    !state.at("guest_code_patches").empty() || !state.at("guest_memory_patches").empty())
   throw FormatError("Unauthenticated original scene");
}
}
int main(int argc,char **argv)try {
 if(argc!=3 && argc!=4)throw FormatError("Supply ORIGINAL_STATE_DIRECTORY OUTPUT_JSON [TU15_XBE for motion]");
 const std::filesystem::path base=argv[1];
 if(std::filesystem::exists(base/"motion.json") || std::filesystem::exists(base/"adopt.json")) {
   const bool adopt=std::filesystem::exists(base/"adopt.json");
   authenticate(base/(adopt?"adopt.json":"motion.json"));
   if(argc!=4)throw FormatError("Original motion comparison requires its authenticated TU1.5 XBE");
   const auto original=OriginalData::load_xbe(argv[3]);
   if(original->image!=OriginalData::Image::TitleUpdate15 || !original->observer_constants)
     throw FormatError("Motion operands require the owned TU1.5 image");
   const auto before=read(base/"before.bin",0x358),after=read(base/"after.bin",0x358);
   SourceObserverState observer;std::memcpy(observer.data(),before.data(),before.size());
   const auto dt=at<float>(read(base/"dt.bin",4),0);
   DeathCameraPacket packet{};
   if(adopt) {
     const auto input=read(base/"packet-before.bin",sizeof(packet));std::memcpy(packet.data(),input.data(),input.size());
     source_observer_adopt(observer,packet,*original->observer_constants);
   } else source_observer_motion(observer,dt,*original->observer_constants);
   nlohmann::json differences=nlohmann::json::array();
   for(unsigned i=0;i<observer.size();++i)if(observer[i]!=at<std::uint32_t>(after,4*i))
     differences.push_back({{"word",i},{"native",observer[i]},{"original",at<std::uint32_t>(after,4*i)}});
   if(adopt) {
     const auto expected=read(base/"packet-after.bin",sizeof(packet));
     for(unsigned i=0;i<packet.size();++i)if(packet[i]!=at<std::uint32_t>(expected,4*i))
       differences.push_back({{"packet_word",i},{"native",packet[i]},{"original",at<std::uint32_t>(expected,4*i)}});
   }
   nlohmann::json report={{"reference",adopt?"Unchanged Xbox TU1.5 observer1723D0 in xemu":"Unchanged Xbox TU1.5 observer172CF0 in xemu"},
     {"source_state",base.string()},{"source_dt",dt},{"observer_words",observer.size()},
     {"packet_words",adopt?packet.size():0},
     {"physical_xbox_capture",false},{"differences",differences},{"passed",differences.empty()}};
   std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
   return report["passed"].get<bool>()?0:1;
 }

 const auto observer=read(base/"observers.bin",4*0x358),header=read(base/"next-view-header.bin",0x298),reference=read(base/"camera.bin",0xc0);
 std::ifstream metadata(base/"state.json");nlohmann::json state;metadata>>state;
 authenticate(base/"state.json");
 if(state.at("reference")!="OG Xbox TU1.5" || !state.at("whole_live_text_matches_original").get<bool>() ||
     !state.at("guest_code_patches").empty() || !state.at("guest_memory_patches").empty())
   throw FormatError("Unauthenticated original scene");
 const auto lens=vehicle_camera_angles(at<float>(observer,0xf0),1.2217304706573486f,70);
 SourceFrustumProjectionInput projection;
 projection.projection_angle=lens.projection_angle;
 constexpr unsigned camera=0x18;
 for(unsigned i=0;i<4;++i) {
  projection.outer[i]=at<std::int16_t>(header,camera+0x30+i*2);
  projection.safe[i]=at<std::int16_t>(header,camera+0x38+i*2);
 }
 projection.rectangle_override_enabled=at<std::uint8_t>(header,camera+0x58)!=0;
 projection.rectangle_center=at<Vec2>(header,camera+0x5c);
 projection.rectangle_scale=at<float>(header,camera+0x64);
 projection.screen_offset_enabled=at<std::uint8_t>(header,camera+0x68)!=0;
 projection.screen_offset=at<Vec2>(header,camera+0x6c);
 projection.widescreen=at<std::uint8_t>(read(base/"video-widescreen.bin",1),0)!=0;
 if(projection.widescreen)throw FormatError("This scene comparison requires its captured standard-aspect context");
 SourceViewProjectionInput view;
 view.position=at<Vec3>(observer,0xb8);view.forward=at<Vec3>(observer,0xd8);view.up=at<Vec3>(observer,0xe4);
 view.near_distance=at<float>(header,camera+0x40);view.far_distance=at<float>(header,camera+0x44);
 view.oblique_near_plane=at<ShaderVector>(header,camera+0x48);
 view.projection_program=source_frustum_projection(projection).projection_program;
 const auto bank=source_view_projection(view);
 const auto actual=std::bit_cast<std::array<std::uint32_t,48>>(bank);
 nlohmann::json differences=nlohmann::json::array();
 for(unsigned i=0;i<48;++i)if(actual[i]!=at<std::uint32_t>(reference,4*i))
  differences.push_back({{"word",i},{"native",actual[i]},{"original",at<std::uint32_t>(reference,4*i)}});
 const bool lens_pass=std::bit_cast<std::uint32_t>(lens.projection_angle)==at<std::uint32_t>(header,camera+0x28) &&
   std::bit_cast<std::uint32_t>(lens.lighting_distance_scale)==at<std::uint32_t>(header,camera+0x2c);
 nlohmann::json report={{"reference","OG Xbox TU1.5 unchanged original execution in xemu"},
  {"physical_xbox_capture",false},{"source_state",base.string()},{"lens_exact",lens_pass},
  {"projection_bank_words",48},{"differences",differences},{"passed",lens_pass && differences.empty()},
  {"scope","Completed observer to complete native view/projection; does not replace independent director or framebuffer pixel validation"}};
 std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
 return report["passed"].get<bool>()?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

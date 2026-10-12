#include "halo2/source_transparent_queue.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool pass,const char* text){if(!pass)throw std::runtime_error(text);}
std::vector<std::uint8_t> bytes(const std::string& hex){std::vector<std::uint8_t> out;for(unsigned i=0;i<hex.size();i+=2)out.push_back(std::stoul(hex.substr(i,2),nullptr,16));return out;}
std::uint32_t datum(const Json& text){return std::stoul(text.get<std::string>(),nullptr,16);}
}
int main(int argc,char** argv)try {
 check(argc==2,"usage: king_queue_submission_tests original-report");std::ifstream file(argv[1]);Json report;file>>report;
 const auto geometry=bytes(report.at("geometry_hex"));std::array<float,3> center;std::memcpy(center.data(),geometry.data()+0x164,12);
 unsigned cases{};
 for(const auto& [name,version]:report.at("results").items())for(unsigned n=0;n<report.at("inputs").size();++n) {
  const auto& input=report.at("inputs")[n];const auto& expected=version.at("cases")[n];
  const unsigned count=input.at("prior_count");SourceTransparentQueue queue(true);SourceTransparentCommand* seed{};
  for(unsigned j=0;j<=std::min(count,1023u);++j)seed=queue.allocate();
  queue.reset_world();for(unsigned j=0;j<count;++j)check(queue.allocate(),"Declared queue prefix failed");
  const auto initial=bytes(expected.at("seed_hex"));std::memcpy(seed,initial.data(),112);
  SourceTransparentShaderInput p;p.selected_template_layers=0x8000;p.shader=datum(report.at("shader"));
  p.context=0xffffffffu;p.callback=datum(version.at("entries").at("submission_dispatch"));
  p.shader_priority_flags=input.at("shader_flags").is_null()?version.at("original_shader_flags").get<std::uint16_t>():input.at("shader_flags").get<std::uint16_t>();
  p.center={center[0],center[1],center[2]};const auto& camera=input.at("camera");
  p.camera_position={camera[0],camera[1],camera[2]};p.camera_forward={camera[3],camera[4],camera[5]};
  const int markers=input.at("marker_count");if(markers>=4 && !(markers&1))queue.append_shader(p);
  const auto want=bytes(expected.at("record_hex"));
  check(!std::memcmp(seed,want.data(),112),"Native shader producer differs from whole original112-byte record");
  check(queue.count()==expected.at("count_after").get<unsigned>() && queue.overflow_notice_pending()==expected.at("overflow_notice_after").get<unsigned>(),"Native shader producer changed count/overflow behavior");++cases;
 }
 check(cases==135,"Incomplete original shader-submission corpus");
 SourceTransparentQueue queue;SourceTransparentShaderInput p;p.selected_template_layers=0x8000;p.view_type=1;
 check(!queue.append_shader(p) && queue.count()==0,"Source view-type gate lost");
 p.view_type=0;p.selected_template_layers=0;check(!queue.append_shader(p) && queue.count()==0,"Source layer gate lost");
 bool rejected=false;p.selected_template_layers=0x8002;try{queue.append_shader(p);}catch(const std::invalid_argument&){rejected=true;}
 check(rejected && queue.count()==0,"Missing immediate service silently admitted");
 std::cout<<cases<<" whole retail/TU1.1/TU1.5 world submissions match every persistent command byte, count and overflow flag\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

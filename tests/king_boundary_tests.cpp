#include "halo2/source_king_boundary.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool pass,const std::string& why){if(!pass)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string& hex) {
  check(!(hex.size()&1),"Odd original byte input");std::vector<std::byte> out;
  for(unsigned n=0;n<hex.size();n+=2)out.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));
  return out;
}
std::uint32_t word(const std::vector<std::byte>& data,unsigned at) {
  std::uint32_t out{};for(unsigned i=0;i<4;++i)out|=std::to_integer<std::uint32_t>(data.at(at+i))<<(8*i);return out;
}
template<std::size_t N>std::array<float,N> values(const Json& words) {
  check(words.size()==N,"Original vertex attribute has wrong component count");std::array<float,N> out;
  for(unsigned i=0;i<N;++i)out[i]=std::bit_cast<float>(words[i].get<std::uint32_t>());
  return out;
}
template<std::size_t N>void exact(const std::array<float,N>& actual,const Json& original,const std::string& what) {
  for(unsigned i=0;i<N;++i)check(std::bit_cast<std::uint32_t>(actual[i])==original[i].get<std::uint32_t>(),what+" component "+std::to_string(i));
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply original CPU corridor emission report");std::ifstream file(argv[1]);Json report;file>>report;
  check(report.at("instruction_patches").empty() && report.at("supplied_returns").empty(),"Original vertex proof replaces original instructions/services");
  check(std::endian::native==std::endian::little,"Original binary state requires little endian host");
  const auto raw=bytes(report.at("geometry_hex"));check(raw.size()==sizeof(SourceKingState),"Original hill state size differs");
  SourceKingState state;std::memcpy(&state,raw.data(),raw.size());unsigned cases{},vertices{},words{};
  for(const auto& row:report.at("cases"))try {
    const auto& input=row.at("input");const auto camera=values<3>(input.at("camera_bits"));
    const auto rgb_bytes=bytes(input.at("rgb_hex"));std::array<float,3> rgb;
    for(unsigned i=0;i<3;++i)rgb[i]=std::bit_cast<float>(word(rgb_bytes,i*4));
    const auto clock=bytes(row.at("initial").at("clock30_hex"));
    const auto rate=std::bit_cast<std::int16_t>(std::uint16_t(std::to_integer<unsigned>(clock.at(2))|std::to_integer<unsigned>(clock.at(3))<<8));
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned short cw_before;asm volatile("fnstcw %0":"=m"(cw_before));const auto mx_before=_mm_getcsr();
#endif
    const auto result=source_king_boundary(state,camera,rgb,input.at("tick"),rate,
      {input.at("requested_fpcw"),input.at("requested_mxcsr")});check(bool(result),"Visible original emission became absent");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned short cw_after;asm volatile("fnstcw %0":"=m"(cw_after));
    check(cw_before==cw_after && mx_before==_mm_getcsr(),"Boundary changed caller floating point controls/status");
#endif
    check(row.at("declared_post_preparation").at("whole_helper_claim")==false &&
      row.at("declared_post_preparation").at("preparation_success_claim")==false,"Original corridor was relabeled a complete draw");
    const auto frame=bytes(row.at("prefix").at("frame68_hex"));
    check(std::bit_cast<std::uint32_t>(result->inner)==word(frame,0x14) &&
      std::bit_cast<std::uint32_t>(result->outer)==word(frame,0x10),"Executed original distance locals differ");
    check(result->repeats==row.at("fistps").back().at("output_signed"),"Actual original FISTP differs");
    check(row.at("vertices").size()==result->strip.size(),"Original closed strip count differs");
    for(unsigned i=0;i<result->strip.size();++i) {
      const auto& native=result->strip[i];const auto& original=row.at("vertices")[i];
      exact(native.position,original.at("position0_bits"),"position0 vertex "+std::to_string(i));
      exact(native.uv,original.at("uv1_bits"),"UV1 vertex "+std::to_string(i));
      exact(native.rgba,original.at("rgba11_bits"),"RGBA11 vertex "+std::to_string(i));++vertices;words+=10;
    }
    ++cases;
  }catch(const std::exception& error){throw std::runtime_error("Emission case "+std::to_string(cases)+": "+error.what());}
  check(cases==22 && vertices==1452,"Original emission corpus coverage changed");
  const std::array<float,3> far{float(double(state.center[0])+double(state.radius)*10),state.center[1],state.center[2]};
  check(!source_king_boundary(state,far,{1,1,1},0,30),"Original far-view no-draw branch emitted geometry");
  auto invalid=state;invalid.marker_count=3;
  check(!source_king_boundary(invalid,{}, {},0,0),"Invalid callback gate read absent geometry/clock");
  bool rejected=false;try{(void)source_king_boundary(state,state.center,{1,1,1},0,0);}catch(const std::invalid_argument&){rejected=true;}
  check(rejected,"Missing source clock became successful geometry");
  std::cout<<cases<<" original CPU emission corridors matched "<<vertices<<" vertices / "<<words<<" binary32 position, UV and RGBA words\n";
  std::cout<<"LIMIT declared post-preparation corridor; graphics preparation, SDK restore, rendered pixels and normal King admission remain separate\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

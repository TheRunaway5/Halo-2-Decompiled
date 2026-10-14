#include "halo2/source_light_volume_draw.hpp"
#include "halo2/source_transparent_queue.hpp"
#include "halo2/original_data.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)throw FormatError("Supply original light-volume attachment fixture");
  std::ifstream input(argv[1]);if(!input)return 77;
  std::string line;unsigned cases{},admitted{},full{},other_views{};
  for(;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::array<std::uint32_t,82> v{};
    for(auto &w:v)if(!(row>>std::hex>>w))throw FormatError("Truncated original attachment record");
    const auto root=std::bit_cast<Transform>(std::array<std::uint32_t,13>{v[5],v[6],v[7],v[8],v[9],v[10],v[11],v[12],v[13],v[14],v[15],v[16],v[17]});
    const Vec3 eye{std::bit_cast<float>(v[18]),std::bit_cast<float>(v[19]),std::bit_cast<float>(v[20])};
    const Vec3 forward{std::bit_cast<float>(v[21]),std::bit_cast<float>(v[22]),std::bit_cast<float>(v[23])};
    SourceTransparentQueue queue(v[4]!=0);
    for(unsigned n=0;n<v[3];++n)(void)queue.allocate();
    auto &retained=const_cast<SourceTransparentCommand &>(queue.storage()[std::min(v[3],1023u)]);
    std::array<std::uint32_t,28> before{};std::copy_n(v.begin()+24,28,before.begin());
    retained=std::bit_cast<SourceTransparentCommand>(before);
    if(v[2]==0)if(auto *command=queue.allocate()) {
      // Both actual tags have no model, marker0, flag20 clear, no opacity
      // function/fade flag and color-selector0. Original whole41980 proves
      // this completed fallback matrix, literal one and literal white path.
      const auto state=source_light_volume_attachment_state(root,false,1.f,{1,1,1});
      source_light_volume_command(*command,0x10000u,v[1]==0xe44802d3u?0xe44902d4u:0xe4980323u,state,eye,forward);
      ++admitted;
    }
    std::array<std::uint32_t,28> expected{};std::copy_n(v.begin()+54,28,expected.begin());
    if(queue.count()!=v[52] || queue.overflow_notice_pending()!=bool(v[53]) ||
        std::bit_cast<std::array<std::uint32_t,28>>(retained)!=expected) {
      std::cerr<<"image="<<v[0]<<" datum="<<std::hex<<v[1]<<" mode="<<v[2]<<" prefix="<<v[3]<<'\n';
      throw FormatError("Native no-model attachment command differs from unchanged original owner");
    }
    ++cases;if(v[2])++other_views;else if(v[3]==1024)++full;
  }
  if(cases!=768 || admitted!=192 || full!=64 || other_views!=512)
    throw FormatError("Original no-model attachment coverage incomplete");
  std::cout<<"PASS "<<cases<<" original whole attachment/queue cases, "<<admitted
      <<" exact112-byte records, view/overflow/retained plane bytes exact\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

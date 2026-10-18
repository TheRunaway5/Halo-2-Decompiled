#include "halo2/source_campaign_dof_recipe_tu15.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
using namespace halo2;using Json=nlohmann::json;
namespace {
void check(bool v,const char *s){if(!v)throw FormatError(s);}
std::vector<unsigned> program(const std::string &hex){check(hex.size()==480,"Incomplete original DoF pixel program");std::vector<unsigned> v;for(unsigned p=0;p<480;p+=8){unsigned word{};for(unsigned n=0;n<4;++n)word|=unsigned(std::stoul(hex.substr(p+n*2,2),nullptr,16))<<(n*8);v.push_back(word);}return v;}
}
int main(int argc,char **argv)try {
 if(argc!=2)throw FormatError("Supply optional original DoF filter JSON");if(!std::filesystem::is_regular_file(argv[1]))return 77;Json r;std::ifstream(argv[1])>>r;
 check(r.at("status")=="PASS_UNCHANGED_ORIGINAL_DOF_FILTER_COMPOSITE_CPU_RECIPE"&&r.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&r.at("actual_pow_executed").get<bool>(),"Present filter oracle lacks original/source CRT authentication");
 unsigned cases{},commands{},draws{},programs{};
 for(const auto &c:r.at("cases")){
  const auto args=c.at("arguments").get<std::array<unsigned,3>>();const auto v=source_campaign_dof_recipe_tu15(std::bit_cast<float>(args[0]),std::bit_cast<float>(args[1]),std::bit_cast<float>(args[2]),3);const auto &events=c.at("events");check(v.size()==events.size(),"Native DoF command count differs from original21A60");
  for(unsigned i=0;i<v.size();++i){const auto &a=v[i];const auto &b=events[i];check(a.source_entry==std::stoul(b.at("entry").get<std::string>(),nullptr,16)&&a.source_callsite==std::stoul(b.at("callsite").get<std::string>(),nullptr,16),"Native DoF command owner/order differs");std::vector<unsigned> expected;SourceCampaignDofCommandKindTu15 kind;
   if(b.contains("program")){kind=SourceCampaignDofCommandKindTu15::program;expected=program(b.at("program"));++programs;}
   else if(b.contains("arguments")){kind=SourceCampaignDofCommandKindTu15::draw;expected=b.at("arguments").get<std::vector<unsigned>>();++draws;}
   else if(b.contains("target")){kind=SourceCampaignDofCommandKindTu15::texture;expected={b.at("target"),b.at("stage")};}
   else if(a.source_entry==0x15760){kind=SourceCampaignDofCommandKindTu15::texture_state;expected={b.at("stage"),b.at("state")};}
   else{kind=SourceCampaignDofCommandKindTu15::render_state;expected={b.at("state"),b.at("value")};}
   if(a.kind!=kind||a.words!=expected){std::cerr<<"Mismatch case "<<cases<<" command "<<i<<" owner "<<std::hex<<a.source_callsite<<std::dec<<'\n';for(unsigned j=0;j<a.words.size()&&j<expected.size();++j)if(a.words[j]!=expected[j])std::cerr<<j<<' '<<std::hex<<a.words[j]<<' '<<expected[j]<<std::dec<<'\n';throw FormatError("Native exact DoF program/draw/sampler/render-state words differ from unchanged original");}++commands;
  }++cases;
 }
 check(cases>=10&&draws>=60&&programs>=50,"DoF original filter controls incomplete");
 check(source_campaign_dof_recipe_tu15(-1.f,-1.f,2.f,19).empty(),"Original disabled DoF filter produced commands");
 const auto independent=source_campaign_dof_recipe_tu15(0,1,5,19);bool target19{};for(const auto &c:independent)if(c.kind==SourceCampaignDofCommandKindTu15::draw&&c.words[0]==19)target19=true;check(target19,"DoF current-target ownership was replaced with a fixed target");
 bool rejected{};try{(void)source_campaign_dof_recipe_tu15(INFINITY,0,1,3);}catch(const FormatError&){rejected=true;}check(rejected,"DoF nonadvancing positive infinite loop was admitted");
 std::cout<<"PASS "<<cases<<" whole original21A60/21180/20F10/CRT-pow recipes: "<<commands<<" ordered graphics commands, "<<programs<<" exact60word program commits, "<<draws<<" exact11word draw packets and current-target independence\nLIMIT value/ABI recipe only; GPU texture dimensions/quad coordinates, near/far render targets, source resource readiness, framebuffer and resolution parity remain open\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

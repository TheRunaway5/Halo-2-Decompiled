#include "halo2/source_shadow_lighting.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace {
using namespace halo2;
constexpr auto none=0xffffffffu;
void check(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
std::vector<std::uint32_t> words(const std::string& text){std::istringstream in(text);std::vector<std::uint32_t> out;std::string value;
    while(in>>value){std::size_t used;const auto v=std::stoul(value,&used,16);check(used==value.size()&&v<=none,"invalid lighting fixture word");out.push_back(std::uint32_t(v));}return out;}
std::array<std::uint32_t,5> execute(const std::vector<std::uint32_t>& row){
    std::size_t at=0;auto take=[&](){check(at<row.size(),"truncated lighting fixture");return row[at++];};
    const auto version=take();check(version==1||version==2,"unknown lighting fixture version");const auto identity=take(),token=take(),count=take(),valid_count=take();check(count<=2048&&valid_count<=256,"lighting fixture count outside source bank");
    if(version==2)check(count==202,"retained lighting case lacks complete original object bank");
    std::vector<SourceShadowLightingOwner> owners;for(unsigned i=0;i<count;++i){const auto slot=take(),parent=take(),flags=take();check(slot<2048,"lighting fixture owner slot outside source table");owners.push_back({std::uint16_t(slot),parent,flags});}
    std::array<std::optional<std::uint8_t>,256> valid{};
    for(unsigned i=0;i<valid_count;++i){const auto slot=take(),byte=take();check(slot<256&&byte<=255&&!valid[slot],"lighting fixture validity byte invalid/duplicated");valid[slot]=std::uint8_t(byte);}
    check(at==row.size(),"extra lighting fixture input");std::uint32_t calls=0,called=none;
    const auto result=source_shadow_lighting_selection(owners,identity,true,[&](std::uint32_t owner){++calls;called=owner;return token;},valid);
    return {std::uint32_t(result.storage),result.owner,result.token,calls,called};
}
void controls(){
    std::vector<SourceShadowLightingOwner> owners{{0x77,none,0}};
    std::array<std::optional<std::uint8_t>,256> valid{};valid[9]=1;
    std::function<std::uint32_t(std::uint32_t)> acquire=[](auto){return 0xe16c0009u;};
    unsigned rejects=0;
    auto reject=[&](const auto& o,std::uint32_t id,bool def,const auto& callback,const auto& bytes){bool error=false;try{(void)source_shadow_lighting_selection(o,id,def,callback,bytes);}catch(const FormatError&){error=true;}check(error,"missing shadow lighting input accepted");++rejects;};
    reject(owners,none,true,acquire,valid);reject(owners,0xe0000140u,true,acquire,valid);
    auto o=owners;o[0].object_flags.reset();reject(o,0xe2e60077u,true,acquire,valid);
    o=owners;o[0].parent.reset();reject(o,0xe2e60077u,true,acquire,valid);
    o=owners;o[0].parent=0x12340077u;reject(o,0xe2e60077u,true,acquire,valid);
    o=owners;o[0].parent=0x12340140u;reject(o,0xe2e60077u,true,acquire,valid);
    o=owners;o.push_back(o.front());reject(o,0xe2e60077u,true,acquire,valid);
    o=owners;o[0].slot=2048;reject(o,0xe2e60077u,true,acquire,valid);
    reject(owners,0xe2e60077u,true,std::function<std::uint32_t(std::uint32_t)>{},valid);
    auto bytes=valid;bytes[9].reset();reject(owners,0xe2e60077u,true,acquire,bytes);
    reject(owners,0xe2e60077u,true,std::function<std::uint32_t(std::uint32_t)>{[](auto){return 0xe0000100u;}},valid);
    o=owners;o[0].object_flags=0x01000000u;reject(o,0xe2e60077u,false,acquire,valid);
    unsigned calls=0;
    auto unexpected=[&](std::uint32_t){++calls;return none;};
    o[0].parent.reset();const auto early=source_shadow_lighting_selection(o,0xabcd0077u,true,unexpected,{});
    check(early.storage==SourceShadowLightingStorage::default_bank&&early.owner==0xabcd0077u&&calls==0,"current default flag read parent or cache producer");
    o={{0x77,0x22220078u,0},{0x78,0x33330079u,0x01000000u},{0x79,none,0}};
    auto actual=[&](std::uint32_t owner){check(owner==0x33330079u,"cache producer owner is not complete root");++calls;return 0xfffe0009u;};
    const auto root=source_shadow_lighting_selection(o,0x11110077u,false,actual,valid);
    check(root.storage==SourceShadowLightingStorage::owner_cache&&root.owner==0x33330079u&&root.token==0xfffe0009u&&calls==1,"intermediate default flag or handle salt changed root lookup");
    o[1].object_flags.reset();check(source_shadow_lighting_selection(o,0x11110077u,false,actual,valid).storage==SourceShadowLightingStorage::owner_cache,"BAF80 read intermediate flags");
    const auto absent=source_shadow_lighting_selection(owners,0xe2e60077u,false,unexpected,{});check(absent.storage==SourceShadowLightingStorage::absent,"NONE cache token required validity bank");
    bytes=valid;bytes[9]=0;check(source_shadow_lighting_selection(owners,0xe2e60077u,false,acquire,bytes).storage==SourceShadowLightingStorage::absent,"zero valid byte admitted");
    bytes[9]=255;check(source_shadow_lighting_selection(owners,0x12340077u,false,acquire,bytes).storage==SourceShadowLightingStorage::owner_cache,"noncanonical nonzero valid byte rejected");
    std::cout<<rejects<<" unresolved controls; exact default/complete-root/salt/valid-byte branches pass\n";
}
}
int main(int argc,char** argv){try{
    if(argc==2&&std::string_view(argv[1])=="--probe"){std::string line;while(std::getline(std::cin,line)){if(line.empty())continue;const auto result=execute(words(line));for(unsigned i=0;i<result.size();++i)std::cout<<(i?" ":"")<<std::hex<<result[i];std::cout<<'\n';}return 0;}
    check(argc==1,"expected optional --probe");controls();
    std::ifstream file(std::filesystem::path(HALO2_SOURCE_DIR)/"tests/fixtures/retail_source_shadow_lighting.tsv");check(bool(file),"original shadow lighting fixture missing");
    std::string line;std::size_t cases=0,retained=0;std::array<bool,3> kinds{};
    while(std::getline(file,line)){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');check(split!=std::string::npos,"lighting fixture lacks expected words");const auto expected=words(line.substr(split+1));check(expected.size()==5,"lighting fixture output size");const auto input=words(line.substr(0,split));const auto actual=execute(input);retained+=input.front()==2;
        for(unsigned i=0;i<5;++i)check(actual[i]==expected[i],"original shadow lighting mismatch case "+std::to_string(cases));check(actual[0]<3,"lighting storage kind outside source branches");kinds[actual[0]]=true;++cases;}
    check(cases==2090&&retained==42&&kinds==std::array<bool,3>{true,true,true},"lighting fixture controlled/retained coverage incomplete");
    std::cout<<cases<<" original3DC90/BAF80 cases, "<<cases*5<<" selection/callback words exact; explicit3DDD0 validity inputs\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

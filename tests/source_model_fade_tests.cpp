#include "halo2/source_model_fade.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {
using namespace halo2;
void check(bool value,const std::string &message){if(!value)throw std::runtime_error(message);}
std::vector<std::uint32_t> words(const std::string &text){std::istringstream input(text);std::vector<std::uint32_t> result;std::string word;
    while(input>>word){std::size_t n;const auto v=std::stoul(word,&n,16);check(n==word.size()&&v<=0xfffffffful,"invalid model fade fixture word");result.push_back(std::uint32_t(v));}return result;}
SourceModelFadeBytes execute(const std::vector<std::uint32_t> &row){
    std::size_t at=0;auto take=[&](){check(at<row.size(),"truncated source model fade fixture");return row[at++];};
    check(take()==1,"unknown model fade fixture version");const auto no=take(),nd=take(),nm=take();
    check(no<=64&&nd<=64&&nm<=64,"model fade fixture count exceeds bound");const auto identity=take(),modifier=take();
    check(modifier<=255,"model fade fixture modifier out of range");SourceModelFadeInput input{identity,std::bit_cast<float>(take()),std::uint8_t(modifier)};
    std::vector<SourceModelFadeOwner> owners;std::vector<SourceModelFadeDefinition> definitions;std::vector<SourceModelFadeModel> models;
    for(unsigned i=0;i<no;++i){SourceModelFadeOwner o;o.identity=take();o.flags=take();o.parent=take();o.definition=take();o.radius=std::bit_cast<float>(take());owners.push_back(o);}
    for(unsigned i=0;i<nd;++i){SourceModelFadeDefinition d;d.identity=take();d.model=take();definitions.push_back(d);}
    for(unsigned i=0;i<nm;++i){SourceModelFadeModel m;m.identity=take();m.fade_end=std::bit_cast<float>(take());m.fade_start=std::bit_cast<float>(take());
        for(auto &cutoff:m.render_cutoffs)cutoff=std::bit_cast<float>(take());const auto count=take();check(count<=65535,"model fade fixture LOD count out of range");m.render_lod_count=std::uint16_t(count);models.push_back(m);}
    check(at==row.size(),"extra source model fade input");return source_model_fade(owners,definitions,models,input);
}
void unresolved_controls(){
    constexpr auto none=0xffffffffu;
    std::vector<SourceModelFadeOwner> owners{{0xe2ff0090u,0,none,0xe0000020u,.44f}};
    std::vector<SourceModelFadeDefinition> definitions{{0xe0000020u,0xe0000021u}};
    std::vector<SourceModelFadeModel> models{{0xe0000021u,40.f,30.f,{10.f,5.f,2.8f,1.8f,0.f},0}};
    SourceModelFadeInput input{owners[0].identity,3.0f,0};
    auto rejected=[&](const auto &o,const auto &d,const auto &m,const auto &in,const char *label){bool failure=false;
        try{(void)source_model_fade(o,d,m,in);}catch(const FormatError&){failure=true;}check(failure,std::string("unresolved model fade accepted: ")+label);};
    auto absent=input;absent.view_modifier.reset();rejected(owners,definitions,models,absent,"modifier");
    absent=input;absent.draw_metric.reset();rejected(owners,definitions,models,absent,"metric");
    absent=input;absent.draw_metric=std::numeric_limits<float>::infinity();rejected(owners,definitions,models,absent,"nonfinite metric");
    for(unsigned f=0;f<4;++f){auto o=owners;if(f==0)o[0].flags.reset();if(f==1)o[0].parent.reset();if(f==2)o[0].definition.reset();if(f==3)o[0].radius.reset();rejected(o,definitions,models,input,"owner field");}
    auto o=owners;o[0].parent=0xe0000140u;rejected(o,definitions,models,input,"missing parent");
    o=owners;o[0].parent=o[0].identity;rejected(o,definitions,models,input,"parent cycle");
    o=owners;o.push_back(o[0]);rejected(o,definitions,models,input,"duplicate owner");
    o=owners;o[0].radius=-.44f;rejected(o,definitions,models,input,"negative radius");
    auto d=definitions;d[0].model.reset();rejected(owners,d,models,input,"model sentinel unresolved");
    d.clear();rejected(owners,d,models,input,"definition missing");
    auto m=models;m.clear();rejected(owners,definitions,m,input,"model metadata missing");
    m=models;m[0].render_lod_count.reset();rejected(owners,definitions,m,input,"LOD count");
    m=models;m[0].render_cutoffs[4].reset();rejected(owners,definitions,m,input,"selected cutoff");
    m=models;m[0].fade_end.reset();rejected(owners,definitions,m,input,"fade end");
    m=models;m[0].fade_start.reset();auto inside=input;inside.draw_metric=35.f;rejected(owners,definitions,m,inside,"fade start");
    m=models;m[0].fade_end=std::numeric_limits<float>::quiet_NaN();rejected(owners,definitions,m,input,"nonfinite model field");
    // Source early branches must not demand fields they do not read.
    o=owners;o[0].flags=1u<<19;o[0].parent.reset();o[0].definition.reset();o[0].radius.reset();absent=input;absent.draw_metric.reset();
    check(source_model_fade(o,{}, {},absent)==SourceModelFadeBytes{255,255},"force branch looked up unread metadata");
    o[0].flags=0;absent.view_modifier=0x80;check(source_model_fade(o,{}, {},absent)==SourceModelFadeBytes{255,255},"nonboolean view modifier not preserved");
    o=owners;o[0].parent.reset();o[0].radius.reset();d=definitions;d[0].model=none;absent.view_modifier=0;
    check(source_model_fade(o,d,{},absent)==SourceModelFadeBytes{},"modelNONE read parent/metric/radius");
    std::cout<<"19 unresolved-input controls and3 unread-field early controls pass\n";
}
}
int main(int argc,char **argv){try{
    if(argc==2&&std::string_view(argv[1])=="--probe"){std::string line;while(std::getline(std::cin,line)){if(line.empty())continue;const auto value=execute(words(line));std::cout<<std::hex<<unsigned(value.first)<<' '<<unsigned(value.second)<<'\n';}return 0;}
    check(argc==1,"expected optional --probe");unresolved_controls();
    std::ifstream file(std::filesystem::path(HALO2_SOURCE_DIR)/"tests/fixtures/retail_source_model_fade.tsv");check(bool(file),"original model fade fixture missing");
    std::string line;std::size_t cases=0;bool fractional_first=false,fractional_second=false;
    while(std::getline(file,line)){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');check(split!=std::string::npos,"model fade fixture lacks original outputs");
        const auto row=words(line.substr(0,split)),expected=words(line.substr(split+1));check(expected.size()==2&&expected[0]<=255&&expected[1]<=255,"invalid original model fade bytes");
        const auto actual=execute(row);check(actual==SourceModelFadeBytes{std::uint8_t(expected[0]),std::uint8_t(expected[1])},"original model fade mismatch case"+std::to_string(cases));
        fractional_first|=actual.first>0&&actual.first<255;fractional_second|=actual.second>0&&actual.second<255;++cases;
    }
    check(cases>=1000&&fractional_first&&fractional_second,"source fade fixture has empty/interpolation-free coverage");
    std::cout<<cases<<" original complete4BAF0 cases,"<<cases*2<<" fade bytes exact; actual BAF80 roots and322B0C conversion\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

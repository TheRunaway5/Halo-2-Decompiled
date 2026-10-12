#include "halo2/source_shadow_view.hpp"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace {
using namespace halo2;using Words=std::vector<std::uint32_t>;
void require(bool ok,const char* message){if(!ok)throw FormatError(message);}
Words evaluate(const Words& words){
    if(words.at(0)==0){
        require(words.size()==128,"source shadow volume input length");
        std::array<std::uint32_t,111> raw_camera{};std::copy_n(words.begin()+1,111,raw_camera.begin());
        std::array<std::uint32_t,16> raw_box{};std::copy_n(words.begin()+112,16,raw_box.begin());
        const auto value=source_shadow_view_volume(std::bit_cast<SourceFrustumCamera>(raw_camera),
            std::bit_cast<SourceShadowViewBox>(raw_box));
        Words out{unsigned(value.admitted)};
        const auto bounds=std::bit_cast<std::array<std::uint32_t,6>>(value.projected_bounds);
        const auto endpoints=std::bit_cast<std::array<std::uint32_t,72>>(value.world_endpoints);
        out.insert(out.end(),bounds.begin(),bounds.end());out.insert(out.end(),endpoints.begin(),endpoints.end());
        return out;
    }
    require(words.at(0)==1 && words.size()==38,"source secondary shadow input length");
    const auto f=[&](unsigned i){return std::bit_cast<float>(words.at(i));};
    SourceShadowSecondaryProjection p;std::copy_n(words.begin()+7,31,p.words.begin());
    const auto value=source_shadow_secondary_camera(p,{f(1),f(2),f(3)},{f(4),f(5),f(6)});
    const auto camera=std::bit_cast<std::array<std::uint32_t,111>>(value.camera);
    Words out{1,unsigned(value.plane_inconsistent)};
    out.insert(out.end(),camera.begin(),camera.end());return out;
}
Words parse(std::string text){std::istringstream in(text);Words result;std::uint32_t v;while(in>>std::hex>>v)result.push_back(v);return result;}
void print(const Words& words){for(unsigned i=0;i<words.size();++i)std::cout<<(i?" ":"")<<std::hex<<words[i];std::cout<<'\n';}
}
int main(int argc,char**argv){
    try{
        if(argc==3&&std::string_view(argv[1])=="--point-cameras"){
            std::ifstream fixture(argv[2],std::ios::binary);require(bool(fixture),"missing original point camera fixture");
            const auto word=[&](){std::uint32_t v;fixture.read(reinterpret_cast<char*>(&v),4);require(bool(fixture),"truncated point camera fixture");return v;};
            const auto scalar=[&](){return std::bit_cast<float>(word());};
            require(word()==0x31434548,"invalid point camera fixture");const auto count=word();
            for(unsigned n=0;n<count;++n){
                const Vec3 position{scalar(),scalar(),scalar()};const float radius=scalar();
                const auto cameras=source_effect_light_cameras(position,radius);
                for(unsigned face=0;face<cameras.size();++face){
                    const auto actual=std::bit_cast<std::array<std::uint32_t,111>>(cameras[face].camera);
                    for(unsigned i=0;i<actual.size();++i){const auto expected=word();
                        if(actual[i]!=expected){std::cerr<<"case "<<n<<" face "<<face<<" word "<<i<<" actual "<<std::hex<<actual[i]<<" expected "<<expected<<'\n';
                            throw FormatError("source point camera differs from original words");}
                    }
                }
            }
            std::cout<<"PASS "<<count<<" whole-original point light calls / "<<count*6<<" cameras / "<<count*666<<" exact words\n";
            return 0;
        }
        if(argc==2&&std::string_view(argv[1])=="--probe"){
            std::string line;while(std::getline(std::cin,line))if(!line.empty())print(evaluate(parse(line)));return 0;
        }
        const auto path=argc==2?std::filesystem::path(argv[1]):std::filesystem::path(HALO2_SOURCE_DIR)/"tests/fixtures/retail_source_shadow_view.tsv";
        std::ifstream fixture(path);if(!fixture){std::cerr<<"Source shadow view fixture unavailable\n";return 77;}
        std::size_t cases=0,checked=0,volume=0,cameras=0,admitted=0,clipped=0;
        Words live_camera_input;
        std::string line;while(std::getline(fixture,line)){
            if(line.empty()||line[0]=='#')continue;
            const auto split=line.find('|');require(split!=line.npos,"source shadow view fixture separator");
            const auto input=parse(line.substr(0,split)),expected=parse(line.substr(split+1));
            const auto actual=evaluate(input);
            if(actual!=expected){
                for(unsigned i=0;i<std::min(actual.size(),expected.size());++i)if(actual[i]!=expected[i]){
                    std::cerr<<"case "<<cases<<" word "<<i<<" actual "<<std::hex<<actual[i]<<" expected "<<expected[i]<<'\n';break;}
                throw FormatError("source shadow view differs from original words");
            }
            ++cases;checked+=expected.size();
            if(input[0]==0){++volume;admitted+=actual[0]!=0;clipped+=actual[0]==0;}
            else {++cameras;if(live_camera_input.empty())live_camera_input=input;}
        }
        require(cases>=4000&&volume>=2000&&cameras>=2000&&admitted>0&&clipped>0,"source shadow view fixture lacks branch coverage");
        //Point-light cube mode must not be silently replaced with one camera.
        auto unsupported=live_camera_input;unsupported[7]=0;
        bool rejected=false;try{evaluate(unsupported);}catch(const FormatError&){rejected=true;}
        require(rejected,"unproved source point-light branch accepted");
        auto bad=live_camera_input;bad[1]=0x7fc00000u;rejected=false;
        try{evaluate(bad);}catch(const FormatError&){rejected=true;}
        require(rejected,"unresolved source light input accepted");
        std::cout<<std::dec<<"Source shadow volume/secondary camera: "<<cases<<" cases, "<<checked
            <<" exact words; volume "<<volume<<" ("<<admitted<<" admitted / "<<clipped<<" rejected), cameras "<<cameras<<"\n";
        return 0;
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}

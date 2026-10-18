#include "halo2/source_campaign_environment_view_tu15.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <xmmintrin.h>
using namespace halo2;
using Json=nlohmann::json;
int main(int argc,char **argv)try {
    if(argc!=2)throw FormatError("Supply original TU1.5 fog extern fixture");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Json fixture;{std::ifstream f(argv[1]);f>>fixture;}
    if(fixture.at("status")!="PASS_UNCHANGED_ORIGINAL_FOG_EXTERNS" ||
       fixture.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" ||
       !fixture.at("code_patches").empty() || !fixture.at("service_returns").empty())
        throw FormatError("Original fog authority differs");
    if(fixture.at("cases").size()!=87 ||
       fixture.at("scalar_indices")!=Json{3,24,25,26,27,28,29,30,31,32,33,34,35,36} ||
       fixture.at("color_indices")!=Json{25,26,27,28,29,30,31,32})
        throw FormatError("Present original fog fixture has incomplete consumer coverage");
    std::fesetround(FE_TONEAREST);_mm_setcsr(0x1f80);
    unsigned failed{},observations{},banks{};
    for(const auto &c:fixture.at("cases")) {
        SourceCampaignEnvironmentFrameTu15 frame{};
        const auto hex=c.at("bytes_hex").get<std::string>();
        if(hex.size()!=frame.bytes.size()*2)throw FormatError("Incomplete source bank");
        for(unsigned i=0;i<frame.bytes.size();++i)
            frame.bytes[i]=std::byte(std::stoul(hex.substr(i*2,2),nullptr,16));
        const auto sample=source_campaign_environment_sample_tu15(frame,BspAtmosphere{},-1);
        const auto values=bsp_fog_externs(sample);
        const auto compare=[&](float value,const Json &bits,const char *kind,unsigned slot,unsigned component) {
            ++observations;const auto expected=std::stoul(bits.get<std::string>(),nullptr,16);
            const auto actual=std::bit_cast<std::uint32_t>(value);
            if(actual!=expected) {
                ++failed;
                std::cerr<<c.at("name").get<std::string>()<<' '<<kind<<' '<<slot<<'/'<<component
                    <<" expected "<<std::hex<<expected<<" got "<<actual<<std::dec<<'\n';
            }
        };
        for(const auto &i:fixture.at("scalar_indices")) {
            const auto index=i.get<unsigned>();
            compare(values.scalars.at(index),c.at("scalars").at(std::to_string(index)).at("bits")[0],"scalar",index,0);
        }
        for(const auto &i:fixture.at("color_indices")) {
            const auto index=i.get<unsigned>();
            for(unsigned k=0;k<3;++k)
                compare(values.colors.at(index)[k],c.at("colors").at(std::to_string(index)).at("bits")[k],"RGB",index,k);
        }
        ++banks;
    }
    std::cout<<(failed?"FAIL":"PASS")<<' '<<banks<<" complete source banks, "<<observations
        <<" unchanged-original scalar/RGB observations, "<<failed<<" mismatches\n";
    return failed?1:0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

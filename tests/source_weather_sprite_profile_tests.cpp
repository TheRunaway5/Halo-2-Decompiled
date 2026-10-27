#include "halo2/source_weather_sprite_profile.hpp"
#include "halo2/texture.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <array>
#include <fstream>
#include <iostream>

namespace {
using namespace halo2;
using Json=nlohmann::json;
void need(bool ok,const char* why){if(!ok)throw FormatError(why);}
std::vector<std::byte> unhex(const std::string& text) {
    need(text.size()%2==0,"Incomplete weather sprite fixture hex");
    std::vector<std::byte> out;
    for(std::size_t i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return out;
}
std::string sha(std::span<const std::byte> raw) {
    std::array<unsigned char,32> bytes{};unsigned size{};
    need(EVP_Digest(raw.data(),raw.size(),bytes.data(),&size,EVP_sha256(),nullptr)==1&&size==bytes.size(),"Weather sprite hash failed");
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto byte:bytes){out+=digits[byte>>4];out+=digits[byte&15];}return out;
}
}
int main(int argc,char** argv){try {
    need(argc==2,"Literal weather sprite resource fixture required");
    std::ifstream in(argv[1]);need(bool(in),"Missing weather sprite resource fixture");const auto fixture=Json::parse(in);
    need(fixture.at("schema")=="H2_WEATHER_SPRITE_RESOURCE_FIXTURE_V1"&&fixture.at("cases").size()==2,"Changed weather source-resource census");
    unsigned resources{},mips{},rejections{};std::vector<std::byte> original;
    for(const auto& source:fixture.at("cases")) {
        const auto& d=source.at("descriptor");
        const auto raw=unhex(source.at("raw_resource_hex"));
        const unsigned width=d.at("width"),height=d.at("height"),format=d.at("format"),flags=d.at("flags");
        const unsigned additional=source.at("mcc").get<bool>()?d.at("declared_mips").get<unsigned>():
            retail_mip_count(width,height,1,format,flags&16,d.at("declared_mips"));
        need(width==64&&height==64&&additional==4&&d.at("depth")==1&&d.at("type")==0,"Actual flood-spore source metadata changed");
        need(source_weather_sprite_resource_profile(width,height,additional+1,format,1,0),"Actual source sprite profile rejected");
        const auto image=prepare_texture(Bytes(raw),width,height,format,flags&8,additional);
        need(source_weather_sprite_resource_profile(image.width,image.height,unsigned(image.mips.size()),image.format,image.layers,image.depth),"Decoded actual sprite resource rejected");
        need(image.pixels.size()==5456&&sha(image.pixels)=="c5d137fbdb1b369f583f807f89ca580deac087933502583a639a5ac92232ca52","Complete original flood-spore mip bytes changed");
        if(original.empty())original=image.pixels;else need(original==image.pixels,"Original/prepared sprite mip payloads differ");
        std::size_t offset{};
        for(unsigned i=0;i<5;++i) {
            const auto& mip=image.mips.at(i);const auto side=64u>>i,bytes=(side/4)*(side/4)*16;
            need(mip.width==side&&mip.height==side&&mip.depth==1&&mip.offset==offset&&mip.size==bytes,"Actual source mip layout differs");
            const auto pixels=std::span<const std::byte>(image.pixels).subspan(mip.offset,mip.size);
            need(sha(pixels)==source.at("mip_sha256").at(i).get<std::string>(),"Complete source mip payload changed");
            offset+=bytes;++mips;
        }
        bool failed=false;
        try{prepare_texture(Bytes(std::span(raw).first(5455)),64,64,16,false,4);}catch(const FormatError&){failed=true;}
        need(failed,"Truncated source mip bank admitted");++rejections;++resources;
    }
    need(source_weather_sprite_resource_profile(32,32,4,16,1,0),"Existing snow sprite profile changed");
    for(unsigned width:{0u,1u,4u,8u,16u,31u,32u,33u,63u,64u,65u,128u})
        for(unsigned height:{0u,1u,4u,8u,16u,31u,32u,33u,63u,64u,65u,128u})
            for(unsigned count=0;count<9;++count) {
                if((width==32&&height==32&&count==4)||(width==64&&height==64&&count==5))continue;
                need(!source_weather_sprite_resource_profile(width,height,count,16,1,0),"Unproved weather dimensions/mip count admitted");++rejections;
            }
    for(unsigned side:{32u,64u})for(unsigned kind=0;kind<6;++kind) {
        const auto count=side==32?4u:5u;
        need(!source_weather_sprite_resource_profile(side,side,count,kind<3?(kind==0?14u:kind==1?15u:11u):16u,
            kind==3?0u:kind==4?6u:1u,kind==5?1u:0u),"Unproved weather format/layer/volume admitted");++rejections;
    }
    std::cout<<resources<<" original/prepared source resources; "<<mips<<" exact mip payloads; "<<rejections
        <<" malformed resource/profile rejects. Data component only; no GPU draw claim\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

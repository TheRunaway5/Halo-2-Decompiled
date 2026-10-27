#include "halo2/texture.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <array>
#include <fstream>
#include <iostream>

namespace {
using namespace halo2;
void need(bool ok,const char* why){if(!ok)throw FormatError(why);}
std::vector<std::byte> read(const std::filesystem::path& path) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    need(bool(in)&&in.tellg()>0&&in.tellg()<=2*1024*1024,"Missing or excessive original water resource component input");
    std::vector<std::byte> bytes(std::size_t(in.tellg()));in.seekg(0);
    need(bool(in.read(reinterpret_cast<char*>(bytes.data()),bytes.size())),"Incomplete water resource input");return bytes;
}
std::string sha(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> digest{};unsigned size{};
    need(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&size,EVP_sha256(),nullptr)==1&&size==digest.size(),"Water resource hash failed");
    constexpr char hex[]="0123456789abcdef";std::string out;
    for(auto byte:digest){out+=hex[byte>>4];out+=hex[byte&15];}return out;
}
}
int main(int argc,char** argv){try {
    need(argc==2,"Original water owner extraction fixture required");
    std::ifstream in(argv[1]);need(bool(in),"Missing original water owner profile");const auto profile=nlohmann::json::parse(in);
    need(profile.at("schema")=="H2_MCC_NATIVE_WATER_BEACH_OWNER_V1"&&profile.at("original").at("frame_count")==17,
        "Changed finite original water source profile");
    const auto folder=std::filesystem::path(argv[1]).parent_path();
    const auto original=read(folder/"original-morton.rg8"),projected=read(folder/"projected-linear.rg8");
    need(original.size()==731136&&projected.size()==731136,"Incomplete original seventeen-frame resource bank");
    const auto& storage=profile.at("storage_projection");
    need(sha(original)==storage.at("original_morton_pool_sha256").get<std::string>()&&
         sha(projected)==storage.at("projected_linear_pool_sha256").get<std::string>(),"Complete original/projected resource pools differ");
    unsigned frames{},mips{},controls{};std::size_t decoded{};
    for(const auto& frame:profile.at("original").at("frames")) {
        const auto index=frame.at("frame").get<unsigned>();need(index==frames,"Original frame order changed");
        const auto source=std::span(original).subspan(index*43008,43008),linear=std::span(projected).subspan(index*43008,43008);
        need(sha(source)==frame.at("original_morton_sha256").get<std::string>()&&
             sha(linear)==frame.at("linear_rg8_sha256").get<std::string>(),"Complete frame resource changed");
        need(retail_mip_count(128,128,1,23,false,2)==2,"Original mip selection differs");
        const auto before=prepare_texture(Bytes(source),128,128,23,true,2),after=prepare_texture(Bytes(linear),128,128,23,false,2);
        need(before.width==128&&before.height==128&&before.format==11&&before.layers==1&&!before.depth&&
             before.pixels==after.pixels&&before.mips.size()==3&&after.mips.size()==3&&
             sha(after.pixels)==frame.at("decoded_rgba8_sha256").get<std::string>(),"Native source/projected decoded frame differs");
        for(unsigned level=0;level<3;++level) {
            const auto& expected=frame.at("mips").at(level);const auto& a=before.mips[level];const auto& b=after.mips[level];
            need(a.width==b.width&&a.height==b.height&&a.offset==b.offset&&a.size==b.size&&a.depth==b.depth,
                "Complete source mip layout changed");
            need(sha(std::span<const std::byte>(after.pixels).subspan(b.offset,b.size))==expected.at("decoded_rgba8_sha256").get<std::string>(),
                "Complete source decoded mip payload changed");++mips;
        }
        auto changed=std::vector<std::byte>(linear.begin(),linear.end());changed[0]^=std::byte{1};
        const auto damaged=prepare_texture(Bytes(changed),128,128,23,false,2);
        need(sha(damaged.pixels)!=frame.at("decoded_rgba8_sha256").get<std::string>(),"Source resource mutation escaped complete identity check");
        ++controls;decoded+=after.pixels.size();++frames;
    }
    need(frames==17&&mips==51&&decoded==1462272,"Incomplete original resource proof");
    std::cout<<frames<<" original water frames, "<<mips<<" exact native-decoded mips, "<<decoded<<" RGBA bytes; "<<controls
        <<" source-data mutation controls. Storage component only; no prepared cache publication or GPU claim\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

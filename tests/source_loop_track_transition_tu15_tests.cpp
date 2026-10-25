#include "halo2/source_loop_track_transition_tu15.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <sstream>
#include <vector>
int main(int argc,char **argv)try {
    if(argc!=2)throw std::runtime_error("Supply independent original transition fixture");
    std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
    if(fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw std::runtime_error("Original transition authority differs");
    std::vector<unsigned char> bytes;bytes.reserve(401408u*36u);unsigned cases{};
    for(unsigned mask=0;mask<128;++mask) {
      std::array<std::uint32_t,7> sounds;
      for(unsigned n=0;n<7;++n)sounds[n]=mask&(1u<<n)?0x12340000u+n:0xffffffffu;
      for(unsigned flags=0;flags<32;++flags)for(unsigned current=0;current<7;++current)
      for(unsigned pending=0;pending<7;++pending)for(unsigned alternate=0;alternate<2;++alternate) {
        const auto result=halo2::source_loop_track_transition_tu15(sounds,flags,current,pending,alternate);
        for(const auto value:{mask,flags,current,pending,alternate,result.traversed?0x22121bu:0x22123bu,
            result.sound,result.impulse,unsigned(result.pending)})
          for(unsigned n=0;n<4;++n)bytes.push_back(value>>(n*8));
        ++cases;
      }
    }
    unsigned char hash[SHA256_DIGEST_LENGTH];SHA256(bytes.data(),bytes.size(),hash);
    std::ostringstream encoded;encoded<<std::hex<<std::setfill('0');for(auto byte:hash)encoded<<std::setw(2)<<unsigned(byte);
    if(cases!=fixture.at("cases").get<unsigned>() || encoded.str()!=fixture.at("results_sha256").get<std::string>())
      throw std::runtime_error("Native track transition differs from independently executed original CPU slice: "+encoded.str());
    std::cout<<"PASS "<<cases<<" original normal/alternate track transition decisions\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

#include "halo2/native_tu15_shadow_view_320a0.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <nlohmann/json.hpp>
namespace {
using namespace halo2;using J=nlohmann::json;
struct Memory:SourcePhysicsContactMemory {
    struct Bank {std::uint32_t address;std::vector<std::byte> bytes;};std::vector<Bank> banks;
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        for(const auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+out.size()<=b.bytes.size()){
            std::copy_n(b.bytes.begin()+at-b.address,out.size(),out.begin());return;}
        throw std::runtime_error("Unmapped shadow view constant");
    }
    void write(std::uint32_t,std::span<const std::byte>)override {throw std::runtime_error("Shadow view wrote memory");}
};
J run(const J& row) {
    Memory m;
    for(const auto& bank:row.at("banks")) {
        const auto s=bank.at("before").get<std::string>();std::vector<std::byte> bytes;
        for(std::size_t n=0;n<s.size();n+=2)bytes.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));
        m.banks.push_back({bank.at("address"),std::move(bytes)});
    }
    const auto c=row.at("camera").get<std::array<std::uint32_t,111>>();
    const auto b=row.at("box").get<std::array<std::uint32_t,16>>();const auto& controls=row.at("before_fp");
    NativeTu15PhysicsFrameFp fp{controls[0],controls[1],controls[2]};
    const auto result=native_tu15_shadow_view_volume_320a0(m,std::bit_cast<SourceFrustumCamera>(c),std::bit_cast<SourceShadowViewBox>(b),fp);
    return {{"admitted",result.admitted},{"bounds",std::bit_cast<std::array<std::uint32_t,6>>(result.projected_bounds)},
        {"endpoints",std::bit_cast<std::array<std::uint32_t,72>>(result.world_endpoints)},{"fp",{fp.control_word,fp.status,fp.mxcsr}}};
}
}
int main()try {
    for(std::string line;std::getline(std::cin,line);)if(!line.empty())std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

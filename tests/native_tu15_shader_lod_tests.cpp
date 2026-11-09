#include "halo2/native_tu15_shader_lod.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <nlohmann/json.hpp>
namespace {
using namespace halo2;using J=nlohmann::json;
struct Memory:SourcePhysicsContactMemory {
    struct Bank{std::uint32_t at;std::vector<std::byte> bytes;};std::vector<Bank> banks;
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        for(const auto& b:banks)if(at>=b.at&&std::uint64_t(at-b.at)+out.size()<=b.bytes.size()) {
            std::copy_n(b.bytes.begin()+at-b.at,out.size(),out.begin());return;
        }
        throw std::out_of_range("Unmapped native1BF30 input at "+std::to_string(at));
    }
    void write(std::uint32_t,std::span<const std::byte>)override {throw std::runtime_error("Native1BF30 has no non-output store owner");}
};
J run(const J& row) {
    Memory m;for(const auto& b:row.at("banks")) {
        const auto text=b.at("before").get<std::string>();std::vector<std::byte> bytes;
        for(std::size_t i=0;i<text.size();i+=2)bytes.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
        m.banks.push_back({b.at("address"),std::move(bytes)});
    }
    const auto& f=row.at("before_fp");NativeTu15PhysicsFrameFp fp{f[0],f[1],f[2]};
    J result;
    try {
        const auto out=native_tu15_shader_lod_1bf30(m,row.at("gp")[0],row.at("matrix"),row.at("source"),fp);
        result=J{{"rejected",false},{"index",out.index},{"accepted",out.accepted},{"stores",out.index_stores}};
    }catch(const std::invalid_argument&){result=J{{"rejected",true}};}
    result["fp"]=J::array({fp.control_word,fp.status,fp.mxcsr});return result;
}
}
int main()try {
    std::string line;while(std::getline(std::cin,line))std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

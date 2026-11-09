#include "halo2/native_tu15_command_part_prepare.hpp"
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>
namespace {
using namespace halo2;using J=nlohmann::json;
std::vector<std::byte> decode(const std::string& text) {
    std::vector<std::byte> out;
    for(std::size_t i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return out;
}
std::string hex(std::span<const std::byte> bytes) {
    std::string out;constexpr char digits[]="0123456789abcdef";
    for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4];out+=digits[v&15];}return out;
}
struct Memory:SourcePhysicsContactMemory {
    struct Bank{std::uint32_t at;std::vector<std::byte> bytes;};std::vector<Bank> banks;
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        for(const auto& b:banks)if(at>=b.at&&std::uint64_t(at-b.at)+out.size()<=b.bytes.size()) {
            std::copy_n(b.bytes.begin()+at-b.at,out.size(),out.begin());return;
        }
        throw std::runtime_error("Unmapped native45CD0 read at "+std::to_string(at));
    }
    void write(std::uint32_t,std::span<const std::byte>)override {throw std::runtime_error("45CD0 wrote outside its typed packet");}
};
J run(const J& row) {
    Memory m;for(const auto& b:row.at("banks"))m.banks.push_back({b.at("address"),decode(b.at("before"))});
    std::array<std::byte,44> initial{};const auto packet=row.at("packet").get<std::uint32_t>();m.read(packet,initial);
    const auto& f=row.at("before_fp");NativeTu15PhysicsFrameFp fp{f[0],f[1],f[2]};
    const auto out=native_tu15_command_part_prepare_45cd0(m,row.at("gp")[1],row.at("matrix"),row.at("source"),initial,fp);
    J writes=J::array();for(const auto& s:out.stores) {
        std::array<std::byte,4> bytes{};for(unsigned i=0;i<s.bytes;++i)bytes[i]=std::byte(s.value>>(8*i));
        writes.push_back(J{{"instruction",s.instruction},{"address",packet+s.offset},{"written",hex(std::span(bytes).first(s.bytes))}});
    }
    return J{{"accepted",out.accepted},{"packet",hex(out.bytes)},{"writes",writes},{"fp",J::array({fp.control_word,fp.status,fp.mxcsr})}};
}
}
int main()try {
    std::string line;while(std::getline(std::cin,line))std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

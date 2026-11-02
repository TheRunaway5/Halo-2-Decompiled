#include "halo2/native_tu15_render_script_flags.hpp"
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace {
using namespace halo2;using J=nlohmann::json;
std::vector<std::byte> decode(const std::string& s) {
    std::vector<std::byte> out;
    for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));
    return out;
}
std::string hex(std::span<const std::byte> b) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;
}
struct Memory:SourcePhysicsContactMemory {
    struct Bank {std::uint32_t address;std::vector<std::byte> bytes;};
    std::vector<Bank> banks;mutable J reads=J::array();J writes=J::array();
    int fail_read=-1;bool fail_write{},fail_after{};mutable int read_count{};
    std::span<std::byte> bytes(std::uint32_t p,std::size_t n) {
        for(auto& b:banks)if(p>=b.address&&std::uint64_t(p-b.address)+n<=b.bytes.size())return std::span(b.bytes).subspan(p-b.address,n);
        throw std::runtime_error("Unmapped flag operand");
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        if(read_count++==fail_read)throw std::runtime_error("Declared read refusal");
        const auto b=const_cast<Memory*>(this)->bytes(p,out.size());std::ranges::copy(b,out.begin());
        reads.push_back(J::array({p,out.size()}));
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        if(fail_write)throw std::runtime_error("Declared write refusal");
        std::ranges::copy(in,bytes(p,in.size()).begin());writes.push_back(J::array({p,hex(in)}));
        if(fail_after)throw std::runtime_error("Declared committed observer refusal");
    }
};
J run(const J& row) {
    Memory memory;
    for(const auto& b:row.at("banks"))memory.banks.push_back({b.at("address"),decode(b.at("before"))});
    memory.fail_read=row.value("fail_read",-1);memory.fail_write=row.value("fail_write",false);memory.fail_after=row.value("fail_after",false);
    bool completed=true;
    try {
        const auto publisher=[&](auto p,auto bytes){memory.write(p,bytes);};
        const NativeTu15RenderFlagPublisher write=row.value("publisher",false)?publisher:NativeTu15RenderFlagPublisher{};
        if(row.at("kind")=="warthog")native_tu15_warthog_chaingun_light_enable(memory,row.at("value"),write);
        else native_tu15_object_cinematic_visibility(memory,row.at("object"),row.at("value"),write);
    }catch(const std::runtime_error&){completed=false;}
    J banks=J::array();for(const auto& b:memory.banks)banks.push_back({{"address",b.address},{"after",hex(b.bytes)}});
    return {{"completed",completed},{"reads",memory.reads},{"writes",memory.writes},{"banks",banks}};
}
}
int main()try {
    for(std::string line;std::getline(std::cin,line);)if(!line.empty())std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

#include "halo2/source_first_person_light_pose_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
std::vector<std::byte> bytes(const std::string& hex) {
    std::vector<std::byte> out(hex.size()/2);
    for(unsigned n=0;n<out.size();++n)out[n]=std::byte(std::stoul(hex.substr(n*2,2),nullptr,16));
    return out;
}
struct Memory final:SourcePhysicsContactMemory {
    std::array<std::byte,0x20000> data{};
    std::vector<std::byte> constants;
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        if(at>=0x1000000u && std::uint64_t(at)+out.size()<=0x1020000u)
            std::copy_n(data.begin()+at-0x1000000u,out.size(),out.begin());
        else if(at>=0x46a000u && std::uint64_t(at)+out.size()<=0x46a000u+constants.size())
            std::copy_n(constants.begin()+at-0x46a000u,out.size(),out.begin());
        else throw std::runtime_error("FP light up read an undeclared operand");
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        if(at<0x1000000u || std::uint64_t(at)+in.size()>0x1020000u)
            throw std::runtime_error("FP light up wrote outside its declared caller memory");
        std::ranges::copy(in,data.begin()+at-0x1000000u);
    }
};
}
int main(int argc,char**argv)try {
    if(argc!=2)throw std::runtime_error("Supply independent complete31B30 fixture");
    std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
    if(fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw std::runtime_error("FP light pose authority changed");
    if(!source_first_person_light_pose_tu15_owns_entry(0x31b30))throw std::runtime_error("Original FP light up owner is absent");
    Memory memory;memory.constants=bytes(fixture.at("original_constant_page"));
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner=[&](auto& m,const auto& call,auto& abi,const auto& observer) {
        if(call.target!=0x30bd0u)throw std::runtime_error("FP light up reached an unqualified child");
        source_run_physics_live_support_tu15(m,call.target,abi,{0x27f,0x1f80},services,observer);
    };
    unsigned index=0;
    for(const auto& row:fixture.at("cases")) {
        memory.data.fill(std::byte{});const auto input=bytes(row.at("input"));
        memory.write(0x1001000u,std::span(input).first(12));memory.write(0x1002000u,std::span(input).subspan(12));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[0]=0x1001000u;abi.gp[1]=0x1002000u;abi.gp[4]=0x101e000u;abi.eflags=2;
        source_run_first_person_light_pose_tu15(memory,0x31b30u,abi,{0x27f,0x1f80},services);
        std::array<std::byte,12> actual{},forward{};memory.read(0x1001000u,actual);memory.read(0x1002000u,forward);
        if(!std::ranges::equal(actual,bytes(row.at("output"))) || !std::ranges::equal(forward,bytes(row.at("forward"))))
            throw std::runtime_error("FP light up differs from whole original CPU case"+std::to_string(index));
        if(abi.gp[4]!=0x101e004u)throw std::runtime_error("FP light up lost its original caller stack");
        ++index;
    }
    std::cout<<"PASS "<<index<<" complete original FP light up/normalize cases\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

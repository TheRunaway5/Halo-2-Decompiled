#include "halo2/source_player_departure_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &hex){
    std::vector<std::byte> result;for(std::size_t n=0;n<hex.size();n+=2)
        result.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));return result;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply the original TU1.5 session-row cleanup fixture");std::ifstream file(argv[1]);if(!file)return 77;
    const auto fixture=nlohmann::json::parse(file);unsigned cases{};
    check(fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Session cleanup fixture has a different executable");
    for(const auto &c:fixture.at("cases")){
        SourceRuntimeMemory memory;
        for(const auto &bank:c.at("banks"))memory.mount(bank.at("address"),bytes(bank.at("initial")),
            SourceRuntimeMemory::Access::ReadWrite,"declared original session-row cleanup caller storage");
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=c.at("entry_gp").get<decltype(abi.gp)>();abi.eflags=0x202;
        std::vector<std::uint32_t> trace;SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc){trace.push_back(pc);};
        services.required_owner=[&](auto &m,const auto &call,auto &state,const auto &observe){
            if(!source_player_departure_tu15_owns_entry(call.target))throw std::runtime_error("Session cleanup reached an unowned callee");
            source_run_player_departure_tu15(m,call.target,state,{0x27f,0x1f80},services,observe);
        };
        source_run_player_departure_tu15(memory,fixture.at("entry"),abi,{0x27f,0x1f80},services);
        check(abi.gp==c.at("exit_gp").get<decltype(abi.gp)>(),"Original session cleanup register outputs differ");
        check((abi.eflags&0xcd5u)==(c.at("eflags").get<unsigned>()&0xcd5u),"Original session cleanup flags differ");
        check(trace==c.at("instructions").get<decltype(trace)>(),"Original session cleanup instruction path differs");
        for(const auto &bank:c.at("banks"))check(std::ranges::equal(memory.view(bank.at("address"),bank.at("final").get<std::string>().size()/2),bytes(bank.at("final"))),
            "Original session cleanup caller/session/row memory differs");
        ++cases;
    }
    std::cout<<"PASS untouched TU1.5 session-row departure cleanup: "<<cases<<" instruction/register/flag/complete-memory cases\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

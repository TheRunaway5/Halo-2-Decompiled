#include "halo2/source_shadow_array_swap_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::vector<std::byte> unhex(const std::string &text){std::vector<std::byte> result;for(std::size_t n=0;n<text.size();n+=2)result.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return result;}
struct Memory final:SourcePhysicsContactMemory {
    struct Bank {std::uint32_t address;std::vector<std::byte> bytes;};std::vector<Bank> banks;
    const Bank &bank(std::uint32_t at,std::size_t bytes)const {
        for(const auto &b:banks)if(at>=b.address && std::uint64_t(at)+bytes<=std::uint64_t(b.address)+b.bytes.size())return b;
        throw std::runtime_error("Shadow swap accessed unowned fixture memory");
    }
    void read(std::uint32_t at,std::span<std::byte> output)const override {
        const auto &b=bank(at,output.size());std::copy_n(b.bytes.begin()+(at-b.address),output.size(),output.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte> input)override {
        const auto &b=bank(at,input.size());auto &bytes=const_cast<Bank&>(b).bytes;
        std::copy(input.begin(),input.end(),bytes.begin()+(at-b.address));
    }
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply original TU1.5 shadow array-swap fixture");
    check(source_shadow_array_swap_tu15_owns_entry(0x166c30),"Missing native original166C30 shadow swap owner");
    check(!source_shadow_array_swap_tu15_owns_entry(0x166c31),"Unowned interior PC was admitted as a callable entry");
    std::ifstream stream(argv[1]);const auto fixture=nlohmann::json::parse(stream);
    check(fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Shadow swap requires original TU1.5 authority");
    unsigned count{};
    for(const auto &c:fixture.at("cases")) {
        Memory memory;for(const auto &row:c.at("initial_banks"))memory.banks.push_back({row.at("address"),unhex(row.at("hex"))});
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=c.at("entry_gp").get<decltype(abi.gp)>();abi.eflags=0x202;
        abi.xmm=c.at("entry_xmm").get<decltype(abi.xmm)>();
        std::vector<std::uint32_t> trace;SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc){trace.push_back(pc);};
        services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Leaf swap reached an external owner");};
        source_run_shadow_array_swap_tu15(memory,0x166c30,abi,{0x27f,0x1f80},services);
        check(abi.gp==c.at("exit_gp").get<decltype(abi.gp)>(),"Original shadow swap GP/stack output differs");
        check((abi.eflags&0xcd5u)==(c.at("eflags").get<unsigned>()&0xcd5u),"Original shadow swap flags differ");
        check(abi.xmm==c.at("entry_xmm").get<decltype(abi.xmm)>(),"Integer shadow swap changed SSE registers");
        check(trace==c.at("instructions").get<decltype(trace)>(),"Original shadow swap instruction sequence differs");
        for(const auto &row:c.at("final_banks")) {
            const auto expected=unhex(row.at("hex"));std::vector<std::byte> actual(expected.size());memory.read(row.at("address"),actual);
            check(actual==expected,"Original shadow swap memory/store order differs");
        }
        ++count;
    }
    std::cout<<"PASS original TU1.5 shadow array swap: "<<count<<" GP/SSE/flag/instruction/memory cases\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

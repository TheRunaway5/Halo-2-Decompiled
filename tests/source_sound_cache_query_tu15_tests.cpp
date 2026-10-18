#include "halo2/source_sound_cache_query_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using J = nlohmann::json;
namespace {
struct Memory final : SourcePhysicsContactMemory {
    std::vector<std::byte> bytes;
    std::uint32_t cr3, cr4;
    Memory(const char *path, const J &cpu) : cr3(cpu["control_registers"]["CR3"]), cr4(cpu["control_registers"]["CR4"]) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if(!file || file.tellg()!=67108864)throw std::runtime_error("Actual original RAM required");
        bytes.resize(67108864);file.seekg(0);file.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
        if(!file)throw std::runtime_error("Truncated original RAM");
    }
    std::uint32_t word(std::uint32_t p) const {
        if(std::uint64_t(p)+4>bytes.size())throw std::out_of_range("Page table escapes RAM");
        std::uint32_t result;std::memcpy(&result,bytes.data()+p,4);return result;
    }
    std::uint32_t physical(std::uint32_t p) const {
        const auto directory=word((cr3&0xfffff000u)+(p>>22)*4u);
        if(!(directory&1))throw std::out_of_range("Original PDE absent");
        if(directory&128){if(!(cr4&16))throw std::runtime_error("Original PSE absent");return (directory&0xffc00000u)+(p&0x3fffffu);}
        const auto table=word((directory&0xfffff000u)+((p>>12)&1023u)*4u);
        if(!(table&1))throw std::out_of_range("Original PTE absent");
        return (table&0xfffff000u)+(p&4095u);
    }
    void read(std::uint32_t p,std::span<std::byte> result) const override {
        while(!result.empty()){
            const auto at=physical(p);const auto n=std::min<std::size_t>(result.size(),4096-(p&4095));
            if(std::uint64_t(at)+n>bytes.size())throw std::out_of_range("Original read escapes RAM");
            std::memcpy(result.data(),bytes.data()+at,n);p+=std::uint32_t(n);result=result.subspan(n);
        }
    }
    void write(std::uint32_t,std::span<const std::byte>) override {throw std::runtime_error("Read-only query performed a source write");}
    void input(std::uint32_t p,const std::string &hex) {
        if(hex.size()%2)throw std::invalid_argument("Odd fixture byte extent");
        for(std::size_t i=0;i<hex.size();i+=2){const auto at=physical(p++);if(at>=bytes.size())throw std::out_of_range("Fixture input escapes RAM");bytes[at]=std::byte(std::stoul(hex.substr(i,2),nullptr,16));}
    }
};
}
int main(int argc,char **argv)try {
    if(argc!=4)throw std::invalid_argument("Expected declared fixtures, original entry RAM and result JSON");
    J input;std::ifstream(argv[1])>>input;
    Memory initial(argv[2],input["cpu"]);J records=J::array();
    for(const auto &fixture:input["fixtures"]) {
        auto memory=initial;
        for(const auto &patch:fixture["declared_input_bytes"])memory.input(patch["address"],patch["hex"]);
        const auto &cpu=input["cpu"];
        SourcePhysicsWorldRuntimeAbi state{};state.gp=cpu["gp"];state.xmm=cpu["xmm"];
        state.eflags=cpu["eflags"];state.fs_base=cpu["fs_base"];state.x87_status=cpu["x87_status"];
        SourcePhysicsFactoryFloatingPoint fp{cpu["x87_control"],cpu["mxcsr"]};state.floating_point=fp;
        J trace=J::array();SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc){trace.push_back(pc);};
        services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Query entered an unexpected external owner");};
        source_run_sound_cache_query_tu15(memory,0x1296e0u,state,fp,services);
        records.push_back({{"name",fixture["name"]},{"gp",state.gp},{"xmm",state.xmm},
            {"eflags",state.eflags},{"trace",trace},{"x87_status",state.x87_status},
            {"x87_control",state.floating_point->x87_control_word},{"mxcsr",state.floating_point->mxcsr}});
    }
    std::ofstream(argv[3])<<records.dump()<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

#include "halo2/source_sound_binding_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using J = nlohmann::json;
namespace {
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;
    for(const auto byte:bytes){const auto n=std::to_integer<unsigned>(byte);result+=digits[n>>4];result+=digits[n&15];}
    return result;
}
struct Memory final : SourcePhysicsContactMemory {
    std::vector<std::byte> bytes;
    std::uint32_t cr3, cr4, instruction{};
    J writes=J::array();
    Memory(const char *path,const J &cpu):cr3(cpu["control_registers"]["CR3"]),cr4(cpu["control_registers"]["CR4"]) {
        std::ifstream file(path,std::ios::binary|std::ios::ate);
        if(!file || file.tellg()!=67108864)throw std::runtime_error("Actual original RAM required");
        bytes.resize(67108864);file.seekg(0);file.read(reinterpret_cast<char *>(bytes.data()),bytes.size());
        if(!file)throw std::runtime_error("Truncated original RAM");
    }
    std::uint32_t word(std::uint32_t p) const {
        if(std::uint64_t(p)+4>bytes.size())throw std::out_of_range("Page table escapes RAM");
        std::uint32_t result;std::memcpy(&result,bytes.data()+p,4);return result;
    }
    std::pair<std::uint32_t,bool> physical(std::uint32_t p) const {
        const auto directory=word((cr3&0xfffff000u)+(p>>22)*4u);
        if(!(directory&1))throw std::out_of_range("Original PDE absent");
        if(directory&128){if(!(cr4&16))throw std::runtime_error("Original PSE absent");return {(directory&0xffc00000u)+(p&0x3fffffu),bool(directory&2)};}
        const auto table=word((directory&0xfffff000u)+((p>>12)&1023u)*4u);
        if(!(table&1))throw std::out_of_range("Original PTE absent");
        return {(table&0xfffff000u)+(p&4095u),bool(table&directory&2)};
    }
    void read(std::uint32_t p,std::span<std::byte> result) const override {
        while(!result.empty()){
            const auto [at,w]=physical(p);const auto n=std::min<std::size_t>(result.size(),4096-(p&4095));
            if(std::uint64_t(at)+n>bytes.size())throw std::out_of_range("Original read escapes RAM");
            std::memcpy(result.data(),bytes.data()+at,n);p+=std::uint32_t(n);result=result.subspan(n);
        }
    }
    void write(std::uint32_t p,std::span<const std::byte> input) override {
        while(!input.empty()){
            const auto [at,w]=physical(p);const auto n=std::min<std::size_t>(input.size(),4096-(p&4095));
            if(!w || std::uint64_t(at)+n>bytes.size())throw std::out_of_range("Original destination is unavailable");
            writes.push_back({{"instruction",instruction},{"address",p},{"before",hex(std::span(bytes).subspan(at,n))},{"written",hex(input.first(n))}});
            std::memcpy(bytes.data()+at,input.data(),n);p+=std::uint32_t(n);input=input.subspan(n);
        }
    }
    void input(std::uint32_t p,const std::string &raw) {
        if(raw.size()%2)throw std::invalid_argument("Odd fixture byte extent");
        for(std::size_t i=0;i<raw.size();i+=2){const auto [at,w]=physical(p++);if(at>=bytes.size())throw std::out_of_range("Fixture input escapes RAM");bytes[at]=std::byte(std::stoul(raw.substr(i,2),nullptr,16));}
    }
};
}
int main(int argc,char **argv)try {
    if(argc!=4)throw std::invalid_argument("Expected declared fixtures, original entry RAM and result JSON");
    J input;std::ifstream(argv[1])>>input;Memory initial(argv[2],input["cpu"]);J records=J::array();
    for(const auto &fixture:input["fixtures"]) {
        auto memory=initial;
        for(const auto &patch:fixture["declared_input_bytes"])memory.input(patch["address"],patch["hex"]);
        const auto &cpu=fixture["cpu"];
        SourcePhysicsWorldRuntimeAbi state{};state.gp=cpu["gp"];state.xmm=cpu["xmm"];
        state.eflags=cpu["eflags"];state.fs_base=cpu["fs_base"];state.x87_status=cpu["x87_status"];
        SourcePhysicsFactoryFloatingPoint fp{cpu["x87_control"],cpu["mxcsr"]};state.floating_point=fp;
        J trace=J::array();SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc){memory.instruction=pc;trace.push_back(pc);};
        services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Listener selection entered an unowned callee");};
        bool returned=false;std::string error;
        try {source_run_sound_binding_tu15(memory,fixture["entry"].get<std::uint32_t>(),state,fp,services);returned=true;}
        catch(const std::exception &failure){error=failure.what();}
        records.push_back({{"name",fixture["name"]},{"returned",returned},{"error",error},
            {"gp",state.gp},{"xmm",state.xmm},{"eflags",state.eflags},{"trace",trace},{"writes",memory.writes},
            {"x87_depth",state.x87_depth},{"x87_status",state.x87_status},{"x87_control",state.floating_point->x87_control_word},{"mxcsr",state.floating_point->mxcsr}});
    }
    std::ofstream(argv[3])<<records.dump()<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

#include "halo2/native_tu15_shader_lod.hpp"
#include "../src/rendering/world/native_tu15_command_arithmetic.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <nlohmann/json.hpp>
namespace {
using namespace halo2;using J=nlohmann::json;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
void verify_scope_refusals() {
    using Policy=detail::CommandArithmeticPolicy;
    constexpr std::array invalid{
        NativeTu15PhysicsFrameFp{0x377e,0x3925,0x1f80},  // unmasked x87
        NativeTu15PhysicsFrameFp{0x077f,0x69c1,0x1f80},  // x87 exception pending
        NativeTu15PhysicsFrameFp{0x077f,0x6941,0x1f00},  // unmasked SSE
        NativeTu15PhysicsFrameFp{0x077f,0x6941,0x11f80}  // reserved MXCSR
    };
    for(const auto &expected:invalid)for(const auto policy:{Policy::Full,Policy::RetainedSse}) {
        auto fp=expected;
        bool refused=false;
        const auto host=_mm_getcsr();
        try {detail::CommandArithmeticScope scope(fp,policy);}
        catch(const std::invalid_argument&) {refused=true;}
        if(!refused || fp.control_word!=expected.control_word || fp.status!=expected.status ||
            fp.mxcsr!=expected.mxcsr || _mm_getcsr()!=host)
            throw std::runtime_error("Command scope did not preserve its original FP refusal");
    }
}
#endif

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
J run(const J& row,detail::CommandArithmeticPolicy policy) {
    Memory m;for(const auto& b:row.at("banks")) {
        const auto text=b.at("before").get<std::string>();std::vector<std::byte> bytes;
        for(std::size_t i=0;i<text.size();i+=2)bytes.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
        m.banks.push_back({b.at("address"),std::move(bytes)});
    }
    const auto& f=row.at("before_fp");NativeTu15PhysicsFrameFp fp{f[0],f[1],f[2]};
    J result;
    try {
        const auto out=detail::command_shader_lod(m,row.at("gp")[0],row.at("matrix"),row.at("source"),fp,policy);
        result=J{{"rejected",false},{"index",out.index},{"accepted",out.accepted},{"stores",out.index_stores}};
    }catch(const std::invalid_argument&){result=J{{"rejected",true}};}
    result["fp"]=J::array({fp.control_word,fp.status,fp.mxcsr});return result;
}
}
int main()try {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    verify_scope_refusals();
#endif
    using Policy=detail::CommandArithmeticPolicy;
    std::string line;
    while(std::getline(std::cin,line)) {
        const auto row=J::parse(line);
        const auto expected=run(row,Policy::Full);
        const auto actual=detail::reference_command_x87_scope()?expected:run(row,Policy::RetainedSse);
        if(actual!=expected)throw std::runtime_error("SSE-only 1BF30 differs from its full x87 scope");
        std::cout<<actual.dump()<<'\n'<<std::flush;
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

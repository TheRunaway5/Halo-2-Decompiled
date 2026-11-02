#include "halo2/native_tu15_shadow_basis_32340.hpp"
#include <cstring>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::uint32_t> words;
    void read(std::uint32_t p,std::span<std::byte> b)const override {
        if(b.size()!=4)throw std::runtime_error("Unexpected shadow constant width");
        const auto v=words.at(p);std::memcpy(b.data(),&v,4);
    }
    void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("Shadow value kernel wrote input memory");}
};
struct Host {
    alignas(16)std::array<std::byte,108> saved{};unsigned mxcsr;
    Host():mxcsr(_mm_getcsr()) {
        asm volatile("fnsave %0":"=m"(saved)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        _mm_setcsr(0x9fc0);const long double first=.25L,second=-1.75L;
        asm volatile("fldt %0\n\tfldt %1"::"m"(first),"m"(second):"memory","st","st(1)");
    }
    void verify() {
        std::uint16_t cw,sw;asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
        if(cw!=0x37f||sw!=0x3000||_mm_getcsr()!=0x9fc0)throw std::runtime_error("Shadow kernel leaked its FP environment");
        long double a{},b{};asm volatile("fstpt %0\n\tfstpt %1":"=m"(a),"=m"(b)::"memory","st","st(1)");
        const long double first=.25L,second=-1.75L;
        if(std::memcmp(&a,&second,10)||std::memcmp(&b,&first,10))throw std::runtime_error("Shadow kernel changed live host x87 values");
    }
    ~Host(){asm volatile("frstor %0"::"m"(saved):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mxcsr);}
};
J run(const J& row) {
    Memory memory;for(const auto& word:row.at("constants"))memory.words.emplace(word[0],word[1]);
    const NativeTu15ShadowBasisInput input{row.at("center"),row.at("direction"),row.at("transverse"),row.at("axial")};
    NativeTu15PhysicsFrameFp fp{row.at("cw"),row.at("status"),row.at("mxcsr")};
    auto gate_fp=fp;NativeTu15ShadowGateResult gates;
    {
        Host host;gates=native_tu15_shadow_gates_32340(memory,
            {row.at("owner_fade"),row.at("history_fade"),row.at("cached_opacity"),row.at("radius")},gate_fp);host.verify();
    }
    J stores=J::array();NativeTu15ShadowBasisResult basis;
    {
        Host host;basis=native_tu15_shadow_basis_32340(memory,input,fp,[&](auto site,auto p,auto v){stores.push_back(J::array({site,p,v}));});host.verify();
    }
    J result{{"direction",basis.direction},{"up",basis.up},{"near",basis.near_offset},{"far",basis.far_offset},{"width",basis.width},
        {"direction_normalized",basis.direction_normalized},{"up_normalized",basis.up_normalized},{"second_basis",basis.second_basis},
        {"stores",stores},{"fp",J::array({fp.control_word,fp.status,fp.mxcsr})},{"host_fp_and_stack_restored",true},
        {"gates",{{"fade",gates.fade},{"opacity",gates.opacity},{"minimum_radius",gates.minimum_radius},
            {"cached_positive",gates.cached_positive},{"admitted",gates.admitted},{"fp",J::array({gate_fp.control_word,gate_fp.status,gate_fp.mxcsr})}}}};
    if(row.contains("projection")) {
        NativeTu15ShadowProjectionTail tail;
        {Host host;tail=native_tu15_shadow_projection_tail_32340(memory,input.center,basis,row.at("projection"),fp);host.verify();}
        result["tail"]={{"left",tail.left},{"volume_center",tail.volume_center},{"half_extent",tail.half_extent},
            {"virtual_eye",tail.virtual_eye},{"virtual_distance",tail.virtual_distance},{"query_radius",tail.query_radius}};
        result["tail_fp"]=J::array({fp.control_word,fp.status,fp.mxcsr});
    }
    return result;
}
}
int main()try {
    for(std::string line;std::getline(std::cin,line);)if(!line.empty())std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

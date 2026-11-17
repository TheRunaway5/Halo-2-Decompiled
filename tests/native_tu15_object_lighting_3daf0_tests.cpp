#include "halo2/native_tu15_object_lighting_3daf0.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <nlohmann/json.hpp>
// Native side of tools/validate_native_tu15_object_lighting_3daf0.py: one
// JSON row per line in; the final banks, EAX and FP words out. The D3D90
// owner is the original's unavailable-bank prefix (D3D90..D3DDC); a case
// whose banks pass those gates belongs to the live query owner instead.
namespace {
using namespace halo2;using J=nlohmann::json;
struct Memory:SourcePhysicsContactMemory {
    struct Bank {std::uint32_t address;std::vector<std::byte> bytes;};std::vector<Bank> banks;
    Bank& find(std::uint32_t at,std::size_t n) {
        for(auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+n<=b.bytes.size())return b;
        throw std::runtime_error("Unmapped native 3DAF0 access at "+std::to_string(at));
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        auto& b=const_cast<Memory*>(this)->find(at,out.size());std::copy_n(b.bytes.begin()+(at-b.address),out.size(),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        auto& b=find(at,in.size());std::copy(in.begin(),in.end(),b.bytes.begin()+(at-b.address));
    }
    std::uint32_t word(std::uint32_t at)const {std::array<std::byte,4> b{};read(at,b);std::uint32_t v{};
        for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;}
};
std::string hex(std::span<const std::byte> bytes) {
    static constexpr char digits[]="0123456789abcdef";std::string s;s.reserve(bytes.size()*2);
    for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);s+=digits[v>>4];s+=digits[v&15u];}return s;
}
std::vector<std::byte> bytes(const std::string& s) {
    std::vector<std::byte> b(s.size()/2);
    for(std::size_t n=0;n<b.size();++n)b[n]=std::byte(std::stoul(s.substr(2*n,2),nullptr,16));
    return b;
}
J run(const J& row,NativeTu15ObjectLightingStats& stats) {
    Memory m;
    for(const auto& b:row.at("banks"))m.banks.push_back({b.at("address").get<std::uint32_t>(),bytes(b.at("before"))});
    const auto control=row.at("before_fp");
    NativeTu15PhysicsFrameFp fp{control[0].get<std::uint16_t>(),control[1].get<std::uint16_t>(),control[2].get<std::uint32_t>()};
    NativeTu15ObjectLightingServices services;
    services.query=[&m](std::uint32_t,std::uint32_t,std::uint32_t)->bool {
        // D3D90..D3DDC: no lightmap bank, BSP or matching group: AL=1, no store.
        const auto lightmaps=m.word(0x4ecc14u),structure=m.word(0x4ecc18u);
        if(!lightmaps || std::bit_cast<std::int32_t>(m.word(lightmaps+0x80u))<=0 || !structure)return true;
        const auto group=m.word(lightmaps+0x84u);
        if(m.word(group+0x1cu)==0xffffffffu || m.word(group+4u)!=m.word(structure+8u))return true;
        throw std::runtime_error("Physical case reaches the live D3D90 query owner");
    };
    const auto entry=row.at("entry").get<std::uint32_t>();
    const auto gp=row.at("gp").get<std::array<std::uint32_t,8>>();
    std::uint32_t eax=0;
    if(entry==0x3daf0u)eax=native_tu15_object_lighting_3daf0(m,gp[0],row.at("matrix").get<std::uint32_t>(),fp,services,&stats);
    else throw std::runtime_error("Unsupported native 3DAF0 test entry");
    J out;out["eax"]=eax;out["fp"]=J::array({fp.control_word,fp.status,fp.mxcsr});
    J banks=J::array();for(const auto& b:m.banks)banks.push_back({{"address",b.address},{"after",hex(b.bytes)}});
    out["banks"]=banks;return out;
}
}
int main() {
    try {
        NativeTu15ObjectLightingStats stats;std::string line;
        while(std::getline(std::cin,line)) {
            if(line.empty())continue;
            auto out=run(J::parse(line),stats);
            const auto& s=stats;
            out["coverage"]=J::array({s.calls,s.default_banks,s.probe_banks,s.roots,s.children,s.uncached_children,
                s.acquisitions,s.allocations,s.evictions,s.updates,s.unchanged,s.frozen,s.due,s.near_skips,s.queries,
                s.query_hits,s.interpolations,s.rotations,s.snaps,s.conversions,s.conversion_fallbacks,s.color_publications});
            stats={};
            std::cout<<out.dump()<<'\n'<<std::flush;
        }
        return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

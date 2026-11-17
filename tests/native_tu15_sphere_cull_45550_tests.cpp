#include "halo2/native_tu15_sphere_cull_45550.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <nlohmann/json.hpp>
// Native side of tools/validate_native_tu15_sphere_cull_45550.py: one JSON
// row per line in, the final non-stack banks and FP words out.
namespace {
using namespace halo2;using J=nlohmann::json;
struct Memory:SourcePhysicsContactMemory {
    struct Bank {std::uint32_t address;std::vector<std::byte> bytes;};std::vector<Bank> banks;
    Bank& find(std::uint32_t at,std::size_t n) {
        for(auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+n<=b.bytes.size())return b;
        throw std::runtime_error("Unmapped native45550 access at "+std::to_string(at));
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
J run(const J& row) {
    Memory m;
    for(const auto& bank:row.at("banks")) {
        const auto s=bank.at("before").get<std::string>();std::vector<std::byte> bytes(s.size()/2);
        for(std::size_t i=0;i<bytes.size();++i)bytes[i]=std::byte(std::stoul(s.substr(2*i,2),nullptr,16));
        m.banks.push_back({bank.at("address"),std::move(bytes)});
    }
    const auto& c=row.at("before_fp");NativeTu15PhysicsFrameFp fp{c[0],c[1],c[2]};
    const auto entry=row.at("entry").get<std::uint32_t>();
    J coverage=J::array();
    if(entry==0x45550u) {
        const auto unit=row.at("gp")[1].get<std::uint32_t>(),point=row.at("matrix").get<std::uint32_t>();
        NativeTu15SphereCullServices services;
        services.query_2e7c40=[](auto,const auto&,auto&,auto&){throw std::runtime_error("Unexpected 2E7C40 query");};
        NativeTu15SphereCullCoverage seen;
        native_tu15_sphere_cull_45550(m,unit,{m.word(point),m.word(point+4u),m.word(point+8u)},
            row.at("source").get<std::uint32_t>(),fp,services,&seen);
        coverage={seen.calls,seen.no_geometry,seen.queries,seen.parts,seen.already,seen.unbounded,seen.bsp_parts,
            seen.no_object,seen.same_node,seen.no_nodes,seen.transforms,seen.hits,seen.misses};
    } else if(entry==0x458c0u) {
        const auto& args=row.at("stack_arguments");
        const auto list=[&](std::uint32_t header,bool pairs,std::uint32_t& count) {
            const auto data=m.word(header),stored=m.word(header+4u);
            count=std::bit_cast<std::int32_t>(stored)>0?stored:0u;
            std::vector<std::uint32_t> rows(count+(pairs&&(count&1u)));
            for(std::uint32_t n=0;n<rows.size();++n)rows[n]=m.word(data+4u*n);
            return rows;
        };
        const bool ranges=(row.at("source").get<std::uint32_t>()&255u)!=0;
        std::uint32_t first_count{},second_count{};
        const auto first=list(args[0],ranges,first_count),second=list(args[1],false,second_count);
        native_tu15_sphere_mask_458c0(m,row.at("matrix").get<std::uint32_t>(),ranges,first,first_count,second);
    } else throw std::runtime_error("Unknown native sphere-cull entry");
    J banks=J::array();
    for(const auto& b:m.banks)banks.push_back(J{{"address",b.address},{"after",hex(b.bytes)}});
    return {{"banks",banks},{"fp",{fp.control_word,fp.status,fp.mxcsr}},{"coverage",coverage}};
}
}
int main()try {
    for(std::string line;std::getline(std::cin,line);)if(!line.empty())std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

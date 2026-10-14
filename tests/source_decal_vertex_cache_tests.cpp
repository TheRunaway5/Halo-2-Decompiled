#include "halo2/source_decal_vertex_cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cmath>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=2)throw std::runtime_error("Supply completed source decal cache fixture");
    std::ifstream input(argv[1]);if(!input)return 77;
    nlohmann::json fixture;input>>fixture;
    SourceRuntimeMemory memory;
    for(const auto &region:fixture.at("regions")) {
        std::vector<std::byte> bytes;
        if(region.contains("file")) {
            const auto path=std::filesystem::path(argv[1]).parent_path()/region.at("file").get<std::string>();
            std::ifstream f(path,std::ios::binary);if(!f)return 77;
            const std::vector<char> raw{std::istreambuf_iterator<char>(f),{}};
            for(auto b:raw)bytes.push_back(std::byte(static_cast<unsigned char>(b)));
        } else {
            const auto value=region.at("value").get<std::uint32_t>();
            for(unsigned i=0;i<4;++i)bytes.push_back(std::byte(value>>(i*8u)));
        }
        memory.mount(region.at("address"),bytes,SourceRuntimeMemory::Access::ReadWrite,"Completed original cache fixture");
    }
    const auto result=source_decal_vertex_cache_tu15(memory);
    std::size_t quads=0;for(const auto &record:result) {
        if(record.layer!=1 || record.tint!=0xffffffffu)
            throw std::runtime_error("Original lit decal metadata changed");
        quads+=record.quads.size();
    }
    if(result.size()!=fixture.at("records") || quads!=fixture.at("quads") || result.empty())
        throw std::runtime_error("Incomplete original source decal cache export");
    const auto &first=result.front().quads.front();
    const std::array<std::array<float,3>,4> projection{{{0,0,1},{1,0,1},{1,1,1},{0,1,1}}};
    for(unsigned i=0;i<4;++i)if(first[i].projection!=projection[i])
        throw std::runtime_error("Original packed UV/brightness corners changed");
    if(std::abs(first[0].position.x+4.095351696014404f)>1.e-7f ||
        std::abs(first[2].position.z-3.1336231231689453f)>1.e-7f)
        throw std::runtime_error("Original clipped world vertices changed");
    const auto word=[&](auto p){auto b=memory.view(p,4);std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(b[i])<<(i*8);return v;};
    const auto pool=word(0x4f7220u),record=word(pool+0x44u);
    const auto rejected=[&](std::uint32_t p,std::byte value) {
        const auto before=memory.view(p,1)[0];const std::array b{value};memory.write(p,b);
        bool failed=false;try{(void)source_decal_vertex_cache_tu15(memory);}catch(const std::runtime_error&){failed=true;}
        const std::array restore{before};memory.write(p,restore);
        if(!failed)throw std::runtime_error("Invalid source cache range/identity was admitted");
    };
    rejected(record+6u,std::byte{0}); // Full cache salt, not the slot ordinal.
    rejected(record+0x2au,std::byte{0xff}); // Quads cannot exceed retained pages.
    const auto again=source_decal_vertex_cache_tu15(memory);
    if(again.size()!=result.size() || again.front().quads.front()[0].packed_projection!=first[0].packed_projection)
        throw std::runtime_error("Read-only cache export mutated source geometry");
    std::cout<<"PASS "<<result.size()<<" original decal records, "<<quads<<" clipped quads, UV/brightness, stale salt and range rejection\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

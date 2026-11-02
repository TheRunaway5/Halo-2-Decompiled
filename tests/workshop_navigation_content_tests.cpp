#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void need(bool value,const char* why){if(!value)throw FormatError(why);}
struct Patch {
    std::byte* at;
    std::vector<std::byte> original;
    Patch(std::span<const std::byte> raw,std::size_t offset,unsigned width,std::uint32_t value)
        :at(const_cast<std::byte*>(raw.data())+offset),original(at,at+width){
        for(unsigned n=0;n<width;++n)at[n]=std::byte(value>>(8*n));
    }
    ~Patch(){std::copy(original.begin(),original.end(),at);}
};
}
int main(int argc,char** argv)try{
    if(argc!=6)throw FormatError("Supply MAP SHARED_MAPS PREPARED_ROOT EXPECTATION REPORT");
    nlohmann::json expected;std::ifstream input(argv[4]);input>>expected;
    Cache cache(argv[1],argv[2],{},argv[3]);
    const auto index=expected.at("bsp").get<std::size_t>();
    const auto content=cache.load_bsp_content(index);
    need(content.navigation.size()==expected.at("sectors").get<std::size_t>(),
        "Navigation reader changed the authored sector count");
    std::size_t polygons{},portals{};
    for(const auto& sector:content.navigation){
        polygons+=!sector.polygon.empty();portals+=sector.portals.size();
        for(const auto& portal:sector.portals)
            need(portal.sector<content.navigation.size(),"Navigation portal lost its authored sector");
    }
    need(polygons==expected.at("closed_polygons").get<std::size_t>(),
        "Navigation reader dropped an authored active polygon");
    const auto sample=expected.at("sample_sector").get<std::size_t>();
    const auto& polygon=content.navigation.at(sample).polygon;
    const auto& points=expected.at("polygon");
    need(polygon.size()==points.size(),"High-link sector polygon was truncated");
    for(std::size_t n=0;n<polygon.size();++n){
        const auto& bits=points[n].at("bits");const auto& p=polygon[n];
        need(std::bit_cast<std::uint32_t>(p.x)==bits[0].get<std::uint32_t>()&&
             std::bit_cast<std::uint32_t>(p.y)==bits[1].get<std::uint32_t>()&&
             std::bit_cast<std::uint32_t>(p.z)==bits[2].get<std::uint32_t>(),
             "High-link polygon changed authored vertex bits or winding");
    }
    // Corrupt only this Cache's private decoded view. Each guard must refuse
    // the malformed graph and restore the exact bytes before the next case.
    const auto raw=cache.source_runtime_cache_bytes();const auto& bsp=cache.bsps().at(index);
    const Bytes meta(raw.subspan(bsp.file_offset,bsp.size));
    const TagAddressSpace address(meta,bsp.memory_address);
    const auto navigation=address.block(16+0xc4,0x74);need(navigation.count==1,"Expected one authored navigation bank");
    const auto nav=navigation.offset;
    const auto sectors=address.block(nav,8),links=address.block(nav+8,16),vertices=address.block(nav+0x28,12);
    need(links.count==expected.at("links").get<std::size_t>()&&vertices.count==expected.at("vertices").get<std::size_t>(),
        "Prepared navigation bank differs from its authored fixture");
    const auto first=meta.u32(sectors.offset+sample*8+4);
    const auto edge=links.offset+std::size_t(first)*16;
    need(first>32767,"Regression no longer exercises an unsigned high link");
    const bool left=meta.u16(edge+12)==sample;
    unsigned rejected{};
    const auto reject=[&](std::size_t at,unsigned width,std::uint32_t value){
        const auto before=std::vector(raw.begin(),raw.end());bool refused=false;
        {Patch patch(raw,bsp.file_offset+at,width,value);
            try{(void)cache.load_bsp_content(index);}catch(const FormatError&){refused=true;}}
        need(refused,"Unsigned navigation handling swallowed a malformed graph");
        need(std::equal(before.begin(),before.end(),raw.begin()),"Navigation mutation control changed retained cache bytes");
        ++rejected;
    };
    reject(sectors.offset+sample*8+4,4,links.count);
    reject(edge+(left?8:10),2,links.count);
    reject(edge+(left?12:14),2,0xffff);
    reject(edge+(left?0:2),2,vertices.count);
    nlohmann::json report{{"passed",true},{"bsp",index},{"sectors",content.navigation.size()},
        {"closed_polygons",polygons},{"directed_portals",portals},{"sample_sector",sample},
        {"sample_vertex_bits_and_winding_unchanged",true},{"malformed_graphs_rejected",rejected},
        {"scope","Actual prepared Workshop BSP content only; no AI, gameplay or rendering acceptance"}};
    std::ofstream(argv[5])<<report.dump(2)<<'\n';
    std::cout<<"PASS actual Workshop navigation: BSP="<<index<<" sectors="<<content.navigation.size()
        <<" polygons="<<polygons<<" rejected="<<rejected<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

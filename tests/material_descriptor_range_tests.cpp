#include "rendering/renderer/material_descriptor_range.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>
using Bank=std::map<std::pair<unsigned,unsigned>,std::uint64_t>;
using Entry=std::pair<std::pair<unsigned,unsigned>,std::uint64_t>;
std::vector<Entry> reference(const Bank& bank,unsigned material) {
    std::vector<Entry> out;
    for(const auto& [key,descriptor]:bank)if(key.first==material)out.emplace_back(key,descriptor);
    return out;
}
void check() {
    Bank bank;
    for(unsigned material=0;material<127;material+=3)
        for(unsigned lightmap:{0u,1u,7u,71u,std::numeric_limits<unsigned>::max()})
            bank[{material,lightmap}]=(std::uint64_t(material)<<32)|lightmap;
    bank[{std::numeric_limits<unsigned>::max(),0}]=31;
    bank[{std::numeric_limits<unsigned>::max(),std::numeric_limits<unsigned>::max()}]=47;
    unsigned cases{};
    const auto compare=[&](unsigned material) {
        std::vector<Entry> actual;
        for(const auto& [key,descriptor]:halo2::material_descriptor_range(bank,material))actual.emplace_back(key,descriptor);
        if(actual!=reference(bank,material))throw std::runtime_error("material descriptor membership/order differs from complete scan");
        ++cases;
    };
    for(unsigned material=0;material<132;++material)compare(material);
    compare(std::numeric_limits<unsigned>::max());compare(std::numeric_limits<unsigned>::max()-1);
    bank.clear();compare(0);compare(std::numeric_limits<unsigned>::max());
    std::cout<<cases<<" exact descriptor membership/order cases passed\n";
}
std::uint64_t visit(const Bank& bank,unsigned selected,bool complete_scan) {
    std::uint64_t checksum=0;
    const auto consume=[&](const auto& entry){checksum=(checksum*0x100000001b3ull)^entry.second;};
    for(unsigned i=0;i<selected;++i) {
        const auto material=(i*37u)%unsigned(bank.size()/4);
        if(complete_scan) {for(const auto& entry:bank)if(entry.first.first==material)consume(entry);}
        else for(const auto& entry:halo2::material_descriptor_range(bank,material))consume(entry);
    }
    return checksum;
}
int main(int argc,char** argv)try {
    check();
    if(argc==2 && std::string_view(argv[1])=="--benchmark") {
        using Clock=std::chrono::steady_clock;
        for(unsigned materials:{512u,2048u,8192u}) {
            Bank bank;for(unsigned i=0;i<materials;++i)for(unsigned j=0;j<4;++j)bank[{i,j}]=(std::uint64_t(i)<<32)|j;
            const auto selected=std::min(materials,1024u);
            std::array<std::vector<double>,2> times;
            std::uint64_t checksum{};
            for(unsigned trial=0;trial<7;++trial)for(unsigned n=0;n<2;++n) {
                const auto mode=(n+trial)%2;
                const auto start=Clock::now();const auto result=visit(bank,selected,mode==0);
                const auto ms=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
                times[mode].push_back(ms);if(trial || n) {if(result!=checksum)throw std::runtime_error("benchmark changed descriptor order/contents");}
                checksum=result;
            }
            for(auto& values:times)std::sort(values.begin(),values.end());
            std::cout<<"{\"materials\":"<<materials<<",\"descriptors\":"<<bank.size()<<",\"selected\":"<<selected
                <<",\"scan_median_ms\":"<<times[0][3]<<",\"range_median_ms\":"<<times[1][3]<<",\"checksum\":"<<checksum<<"}\n";
        }
    }
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

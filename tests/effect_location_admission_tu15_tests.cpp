#include "halo2/source_effect_location_admission.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

int main(int argc,char **argv)try {
    if(argc!=2)throw std::runtime_error("Supply the unchanged TU1.5 WORLD location admission fixture");
    std::ifstream input(argv[1],std::ios::binary);
    if(!input)throw std::runtime_error("Cannot open original location admission fixture");
    const std::vector<unsigned char> expected(std::istreambuf_iterator<char>(input),{});
    if(expected.size()!=65536)throw std::runtime_error("Original location domain is incomplete");
    unsigned admitted{};
    for(unsigned node=0;node<expected.size();++node) {
        if(expected[node]>1 || halo2::source_effect_world_location_admitted_tu15(std::uint16_t(node))!=bool(expected[node]))
            throw std::runtime_error("WORLD location admission differs from original at node "+std::to_string(node));
        admitted+=expected[node];
    }
    std::cout<<"PASS "<<expected.size()<<" unchanged TU1.5 location admissions; "<<admitted<<" WORLD/unbound nodes\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

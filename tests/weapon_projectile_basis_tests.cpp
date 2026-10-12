#include "halo2/native_weapon_projectile.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
int main(int argc,char **argv)try {
    if(argc!=2)return 2;
    std::ifstream in(argv[1]);if(!in)return 77;
    std::string line;unsigned count{};
    while(std::getline(in,line)) {
        if(line.empty()||line[0]=='#')continue;
        std::istringstream row(line);std::array<std::uint32_t,6> words{};
        for(auto &w:words)if(!(row>>std::hex>>w))throw std::runtime_error("Invalid original basis row");
        const auto up=halo2::source_weapon_projectile_up({std::bit_cast<float>(words[0]),
            std::bit_cast<float>(words[1]),std::bit_cast<float>(words[2])});
        const std::array actual{std::bit_cast<std::uint32_t>(up.x),std::bit_cast<std::uint32_t>(up.y),std::bit_cast<std::uint32_t>(up.z)};
        for(unsigned i=0;i<3;++i)if(actual[i]!=words[i+3])
            throw std::runtime_error("Original projectile basis differs at row "+std::to_string(count));
        ++count;
    }
    if(count<100)throw std::runtime_error("Original basis cases absent");
    std::cout<<"PASS "<<count<<" original projectile up-basis rows, exact float words\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

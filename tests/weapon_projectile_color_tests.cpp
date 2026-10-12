#include "halo2/native_weapon_projectile.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
int main(int argc,char **argv)try {
    if(argc!=2)return 2;
    std::ifstream input(argv[1]);if(!input)return 77;
    std::string line;unsigned count{};
    while(std::getline(input,line)) {
        if(line.empty()||line[0]=='#')continue;
        std::istringstream row(line);std::array<std::uint32_t,16> w;
        for(auto &v:w)if(!(row>>std::hex>>v))throw std::runtime_error("Invalid original color row");
        const auto f=[&](unsigned i){return std::bit_cast<float>(w[i]);};
        const float amount=halo2::source_projectile_acceleration_range(f(0),f(1),f(2),f(3),f(4),w[5]);
        const auto color=halo2::source_projectile_rgb_color({f(6),f(7),f(8)},{f(9),f(10),f(11)},amount);
        const std::array actual{std::bit_cast<std::uint32_t>(amount),std::bit_cast<std::uint32_t>(color.x),
            std::bit_cast<std::uint32_t>(color.y),std::bit_cast<std::uint32_t>(color.z)};
        for(unsigned i=0;i<4;++i)if(actual[i]!=w[i+12])
            throw std::runtime_error("Original projectile color differs at row "+std::to_string(count));
        ++count;
    }
    if(count<100)throw std::runtime_error("Original color cases absent");
    std::cout<<"PASS "<<count<<" original acceleration/color cases, exact float words\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

#include "halo2/source_bsp_accelerator.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <iostream>
#include <nlohmann/json.hpp>
int main()try {
    using namespace halo2;using J=nlohmann::json;
    for(std::string line;std::getline(std::cin,line);)if(!line.empty()) {
        const auto row=J::parse(line);const auto text=row.at("tree").get<std::string>();std::vector<std::byte> tree;
        for(std::size_t i=0;i<text.size();i+=2)tree.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
        std::vector<ShaderVector> planes;for(const auto& p:row.at("planes"))planes.push_back(std::bit_cast<ShaderVector>(p.get<std::array<std::uint32_t,4>>()));
        const auto& controls=row.at("before_fp");NativeTu15PhysicsFrameFp fp{controls[0],controls[1],controls[2]};
        std::vector<std::uint32_t> direct,ranges;bool complete;
        {detail::NativeTu15ArithmeticEnvironment environment(fp);
            complete=source_bsp_accelerator_query0(tree,planes,direct,ranges).complete;}
        std::cout<<J{{"complete",complete},{"direct",direct},{"ranges",ranges},{"fp",{fp.control_word,fp.status,fp.mxcsr}}}.dump()<<'\n'<<std::flush;
    }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

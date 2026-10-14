#include "halo2/source_contrail_state.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=2)throw std::runtime_error("Supply original contrail state fixture");
    std::ifstream file(argv[1]);if(!file)return 77;
    const auto fixture=nlohmann::json::parse(file);unsigned count{};
    for(const auto &row:fixture.at("cases")) {
        const auto p=source_contrail_pixel_program(row.at("blend"),row.at("properties"),row.at("declaration"));
        if(p.words!=row.at("pixel").get<decltype(p.words)>())
            throw std::runtime_error("Original contrail pixel program differs: "+row.dump());
        ++count;
    }
    std::cout<<"PASS original52D30 contrail CPU pixel stores: "<<count<<" cases; GPU resource services remain separate\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

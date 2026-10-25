#include "halo2/source_loop_track_control_tu15.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

int main(int argc,char**argv)try {
    if(argc!=2)throw std::runtime_error("Supply the independent original loop-control fixture");
    std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
    if(fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw std::runtime_error("Loop-control authority differs");
    unsigned failures{},index{};
    const auto real=[](const auto &row,const char*key){return std::bit_cast<float>(row.at(key).template get<std::uint32_t>());};
    for(const auto &row:fixture.at("layers")) {
        const auto out=halo2::source_loop_pitch_layer_tu15(row.at("interval").get<std::array<std::int16_t,5>>(),
            real(row,"pitch_bits"),row.at("layered").get<bool>());
        if(out.active!=row.at("active").get<bool>() || std::bit_cast<std::uint32_t>(out.gain_db)!=row.at("gain_bits").get<std::uint32_t>()) {
            if(failures++<8)std::cerr<<"Layer case "<<index<<" expected "<<row.dump()<<" actual gain="
                <<std::bit_cast<std::uint32_t>(out.gain_db)<<" active="<<out.active<<'\n';
        }
        ++index;
    }
    index=0;
    for(const auto &row:fixture.at("slew")) {
        const auto out=halo2::source_loop_track_gain_control_tu15(real(row,"current_bits"),real(row,"target_bits"),
            real(row,"rate_bits"),real(row,"elapsed_bits"));
        if(std::bit_cast<std::uint32_t>(out)!=row.at("result_bits").get<std::uint32_t>()) {
            if(failures++<8)std::cerr<<"Slew case "<<index<<" expected "<<row.dump()
                <<" actual="<<std::bit_cast<std::uint32_t>(out)<<'\n';
        }
        ++index;
    }
    if(failures)throw std::runtime_error(std::to_string(failures)+" native loop-control results differ from original CPU execution");
    std::cout<<"PASS "<<fixture.at("layers").size()<<" layer decisions and "<<index<<" gain-control updates\n";
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}

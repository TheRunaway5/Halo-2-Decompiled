#include "halo2/source_weather.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace halo2;
namespace {
using Input=std::array<std::uint64_t,19>;
using Output=std::array<std::uint32_t,13>;
float real(std::uint64_t word){return std::bit_cast<float>(std::uint32_t(word));}
SourceWeatherState state(const Input &in,unsigned first){
    return {std::bit_cast<std::int32_t>(std::uint32_t(in[first])),real(in[first+1]),
            real(in[first+2]),std::bit_cast<std::int32_t>(std::uint32_t(in[first+3])),real(in[first+4])};
}
Output execute(const Input &in){
    auto current=state(in,1);
    SourceWeatherRecord record{state(in,7),std::uint32_t(in[12])};
    SourceWeatherClock clock{real(in[13]),real(in[14])};
    auto *retained=in[6]?&record:nullptr;
    switch(in[0]){
    case 0:source_weather_start(current,real(in[16]),retained);break;
    case 1:source_weather_stop(current,real(in[16]),retained);break;
    case 2:source_weather_change(current,real(in[16]),real(in[17]),retained);break;
    case 3:source_weather_advance(current,real(in[16]),retained);break;
    case 4:source_weather_view_tick(current,clock,std::bit_cast<double>(in[15]),
                                  std::int16_t(in[18]),retained);break;
    default:throw std::runtime_error("unknown weather fixture operation");
    }
    const auto word=[](float value){return std::bit_cast<std::uint32_t>(value);};
    return {std::uint32_t(current.stopped),word(current.current),word(current.target),
            std::uint32_t(current.transition),word(current.rate),
            std::uint32_t(record.state.stopped),word(record.state.current),word(record.state.target),
            std::uint32_t(record.state.transition),word(record.state.rate),record.marker,
            word(clock.previous),word(clock.elapsed)};
}
}
int main(int argc,char **argv){try{
    const bool probe=argc==2 && std::string(argv[1])=="--probe";
    std::ifstream fixtures;
    if(!probe){if(argc!=2)throw std::runtime_error("weather source fixture path required");fixtures.open(argv[1]);
        if(!fixtures)throw std::runtime_error("weather source fixtures unavailable");}
    auto &stream=probe?std::cin:fixtures;
    std::string line;unsigned cases=0;
    while(std::getline(stream,line)){
        if(line.empty() || line.front()=='#')continue;
        std::istringstream row(line);row>>std::hex;Input input{};
        for(auto &value:input)if(!(row>>value))throw std::runtime_error("incomplete weather fixture input");
        const auto actual=execute(input);
        if(probe){for(const auto value:actual)std::cout<<std::hex<<value<<' ';std::cout<<'\n';}
        else for(unsigned field=0;field<actual.size();++field){
            std::uint32_t expected{};if(!(row>>expected))throw std::runtime_error("incomplete weather fixture output");
            if(actual[field]!=expected)throw std::runtime_error("weather source case "+std::to_string(cases)+
                " field "+std::to_string(field)+" differs");
        }
        std::string trailing;if(row>>trailing)throw std::runtime_error("weather fixture trailing data");
        ++cases;
    }
    if(!cases)throw std::runtime_error("weather fixture contains no source cases");
    if(!probe)std::cout<<cases<<" original weather state/record/first-view clock cases matched\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

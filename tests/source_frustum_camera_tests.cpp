#include "halo2/source_frustum_camera.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using Words=std::vector<std::uint32_t>;
Words evaluate(const Words& words) {
    const auto real=[&](unsigned i){return std::bit_cast<float>(words.at(i));};
    Words result;
    if(words.at(0)==0) {
        if(words.size()!=16)throw std::runtime_error("invalid constructor input length");
        const auto camera=halo2::source_frustum_camera({{real(1),real(2),real(3)},
            {real(4),real(5),real(6)},{real(7),real(8),real(9)},real(10),real(11),
            {real(12),real(13),real(14),real(15)}});
        const auto bank=std::bit_cast<std::array<std::uint32_t,0x1bc/4>>(camera);
        result.assign(bank.begin(),bank.end());
        auto frustum=camera.full_frustum;bool diagnostic=false;
        halo2::source_frustum_rectangle(camera,{real(12),real(13),real(14),real(15)},0,frustum,&diagnostic);
        result.push_back(diagnostic);
    } else if(words.at(0)==1) {
        if(words.size()!=183)throw std::runtime_error("invalid rectangle input length");
        std::array<std::uint32_t,111> bank{};
        std::copy_n(words.begin()+1,111,bank.begin());
        const auto camera=std::bit_cast<halo2::SourceFrustumCamera>(bank);
        std::array<std::uint32_t,66> previous{};
        std::copy_n(words.begin()+117,66,previous.begin());
        auto out=std::bit_cast<halo2::SourceCameraFrustum>(previous);bool diagnostic=false;
        const bool success=halo2::source_frustum_rectangle(camera,
            {real(112),real(113),real(114),real(115)},words[116],out,&diagnostic);
        result={success,diagnostic};
        const auto actual=std::bit_cast<std::array<std::uint32_t,66>>(out);
        result.insert(result.end(),actual.begin(),actual.end());
    } else if(words.at(0)==2||words.at(0)==3) {
        const bool complete=words[0]==3;
        if(words.size()!=(complete?38u:27u))throw std::runtime_error("invalid source projection input length");
        halo2::SourceFrustumProjectionInput input;
        input.projection_angle=real(1);
        for(unsigned i=0;i<4;++i){input.outer[i]=std::int16_t(words[2+i]);input.safe[i]=std::int16_t(words[6+i]);}
        input.rectangle_override_enabled=words[10];input.rectangle_center={real(11),real(12)};input.rectangle_scale=real(13);
        input.screen_offset_enabled=words[14];input.screen_offset={real(15),real(16)};
        input.widescreen=words[17];input.layout_state_present=words[18];input.layout_state_flag=std::uint8_t(words[19]);
        input.layout_state_index=std::int16_t(words[20]);input.layout_state_mode=std::int32_t(words[21]);
        input.auxiliary_state_present=words[22];input.auxiliary_state_flag=std::uint8_t(words[23]);
        input.scenario_present=words[24];input.scenario_type=std::int16_t(words[25]);input.use_projection_bounds=words[26];
        const auto projection=halo2::source_frustum_projection(input);
        for(auto value:projection.projection_bounds)result.push_back(std::bit_cast<std::uint32_t>(value));
        for(auto value:projection.projection_program)result.push_back(std::bit_cast<std::uint32_t>(value));
        result.push_back(projection.view_modifier);
        if(complete) {
            const auto camera=halo2::source_frustum_camera({{real(27),real(28),real(29)},
                {real(30),real(31),real(32)},{real(33),real(34),real(35)},real(36),real(37),projection.clipping_rectangle()});
            const auto bank=std::bit_cast<std::array<std::uint32_t,111>>(camera);
            result.insert(result.end(),bank.begin(),bank.end());bool diagnostic=false;auto frustum=camera.full_frustum;
            halo2::source_frustum_rectangle(camera,projection.clipping_rectangle(),0,frustum,&diagnostic);
            result.push_back(diagnostic);
        }
    } else throw std::runtime_error("invalid source frustum mode");
    return result;
}
}
int main(int argc,char**argv){try{
    const bool probe=argc==2 && std::string(argv[1])=="--probe";
    if(argc!=2)throw std::runtime_error("source frustum fixture required");
    std::ifstream file;if(!probe){file.open(argv[1]);if(!file)throw std::runtime_error("source frustum fixture unavailable");}
    std::istream& stream=probe?std::cin:file;
    std::string line;unsigned cases=0;
    while(std::getline(stream,line)) {
        if(line.empty()||line.front()=='#')continue;
        std::istringstream row(line);row>>std::hex;unsigned count{};
        if(!(row>>count))throw std::runtime_error("source frustum missing input count");
        Words input(count);for(auto& value:input)if(!(row>>value))throw std::runtime_error("source frustum missing input");
        const auto output=evaluate(input);
        if(probe){for(auto value:output)std::cout<<std::hex<<value<<' ';std::cout<<'\n';}
        else {
            for(unsigned i=0;i<output.size();++i){std::uint32_t expected;
                if(!(row>>expected)||expected!=output[i])throw std::runtime_error("source frustum differs case "+std::to_string(cases)+" field "+std::to_string(i));}
            std::string trailing;if(row>>trailing)throw std::runtime_error("source frustum trailing fixture fields");
        }
        ++cases;
    }
    if(!cases)throw std::runtime_error("no source frustum cases");
    if(!probe)std::cout<<cases<<" complete original camera/frustum banks matched\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

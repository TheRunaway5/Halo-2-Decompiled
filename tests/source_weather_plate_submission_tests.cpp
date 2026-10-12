#include "halo2/source_weather_plate_submission.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <limits>

using namespace halo2;
int main(int argc,char **argv){try{
    if(argc==2&&std::string(argv[1])=="--controls") {
        std::array<std::byte,0x3a8> plate{};
        const auto put=[&](unsigned at,float v){std::memcpy(plate.data()+at,&v,4);};
        put(0x80,10);put(0x84,30);
        SourceWeatherPlateSubmissionInput in{plate,0,{0,0,480,640},{.015625f,1024},{.015625f,1024}};
        const auto baseline=source_weather_plate_submission(in);
        const auto reject=[](const auto &bad){bool failed=false;try{(void)source_weather_plate_submission(bad);}catch(const std::invalid_argument&){failed=true;}
            if(!failed)throw std::runtime_error("unsupported plate submission admitted");};
        unsigned checks=0;
        for(int slot:{-1,4}){auto bad=in;bad.slot=slot;reject(bad);++checks;}
        auto bad=in;bad.plate=std::span(plate).first(plate.size()-1);reject(bad);++checks;
        bad=in;bad.rectangle={481,0,480,640};reject(bad);++checks;
        bad=in;bad.rectangle={0,641,480,640};reject(bad);++checks;
        for(unsigned at:{0x48u,0x80u,0x84u,0x98u,0xecu,0x11cu,0x120u,0x134u,0x148u,0x14cu}) {
            auto changed=plate;const float nan=std::numeric_limits<float>::quiet_NaN();std::memcpy(changed.data()+at,&nan,4);
            bad=in;bad.plate=changed;reject(bad);++checks;
        }
        bad=in;bad.source_fpcw=0x037f;reject(bad);++checks;
        bad=in;bad.source_mxcsr=0x9f80;reject(bad);++checks;
        const auto after=source_weather_plate_submission(in);
        if(after.caller.compositor.words!=baseline.caller.compositor.words||after.scroll!=baseline.scroll)
            throw std::runtime_error("rejected plate inputs mutated their retained owner");
        std::cout<<checks<<" plate submission guards passed\n";return 0;
    }
    std::ifstream fixture;std::istream *stream=&std::cin;
    if(argc==2){fixture.open(argv[1]);if(!fixture)throw std::runtime_error("plate submission fixture missing");stream=&fixture;}
    else if(argc!=1)throw std::runtime_error("optional plate submission fixture path required");
    std::string line;unsigned count=0;
    while(std::getline(*stream,line)) {
        if(line.empty()||line[0]=='#')continue;
        std::istringstream row(line);row>>std::hex;
        std::array<std::uint32_t,243> input{};
        for(auto &word:input)if(!(row>>word))throw std::runtime_error("short plate submission input");
        std::array<std::byte,0x3a8> plate;std::memcpy(plate.data(),input.data(),plate.size());
        SourceWeatherPlateSubmissionInput in;in.plate=plate;in.slot=std::int16_t(input[234]);
        std::memcpy(in.rectangle.data(),input.data()+235,8);
        for(unsigned i=0;i<2;++i){in.selection[i]=std::bit_cast<float>(input[237+i]);in.projection[i]=std::bit_cast<float>(input[239+i]);}
        in.source_fpcw=input[241];in.source_mxcsr=input[242];
        const auto actual=source_weather_plate_submission(in);
        std::array<std::uint32_t,168> output{};unsigned at=0;
        output[at++]=actual.result.program.has_value();output[at++]=unsigned(actual.result.profile);
        output[at++]=actual.result.depth?std::bit_cast<std::uint32_t>(*actual.result.depth):0;
        if(actual.result.program)std::copy(actual.result.program->words.begin(),actual.result.program->words.end(),output.begin()+at);at+=60;
        std::copy(actual.caller.compositor.words.begin(),actual.caller.compositor.words.end(),output.begin()+at);at+=13;
        for(const auto &scroll:actual.scroll)for(float v:scroll)output[at++]=std::bit_cast<std::uint32_t>(v);
        for(const auto &vertex:actual.vertices)for(const auto &reg:vertex.registers)for(float v:reg)output[at++]=std::bit_cast<std::uint32_t>(v);
        if(at!=output.size())throw std::runtime_error("plate submission output size");
        if(argc==2) {
            char split;row>>split;if(split!='|')throw std::runtime_error("plate submission fixture separator");
            for(auto actual_word:output){std::uint32_t expected;if(!(row>>expected)||expected!=actual_word)
                throw std::runtime_error("original plate submission mismatch in case "+std::to_string(count));}
        }else {std::cout<<std::hex;for(auto word:output)std::cout<<word<<' ';std::cout<<'\n';}++count;
    }
    if(!count)throw std::runtime_error("plate submission cases required");
    if(argc==2)std::cout<<count<<" original plate submissions / "<<count*168<<" words passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

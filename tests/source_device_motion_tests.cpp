#include "halo2/source_device_motion.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
std::vector<unsigned> run(const std::vector<unsigned>&r){
    if(r.size()!=19)throw FormatError("device motion row must have 19 words");
    const auto f=[&](unsigned i){return std::bit_cast<float>(r[i]);};
    SourceDeviceMotionDefinition d{r[0],f(1),f(2),f(3),f(4),f(5),f(6),f(7),{0x101,0x102,0x103,0x104},0x105};
    SourceDeviceMotionState s{f(8),f(9),f(10),f(11),r[12],std::bit_cast<std::int16_t>(std::uint16_t(r[13]))};
    const auto out=source_device_motion_step(d,s,r[16]&1?std::optional<float>(f(14)):std::nullopt,
        r[16]&2?std::optional<float>(f(15)):std::nullopt,f(17),r[18]);
    if(out.effects.size()>1)throw FormatError("source device emitted multiple motion effects");
    return {std::bit_cast<unsigned>(s.position),std::bit_cast<unsigned>(s.position_velocity),
        std::bit_cast<unsigned>(s.power),std::bit_cast<unsigned>(s.power_velocity),s.flags,
        std::uint16_t(s.delay_ticks),unsigned(out.updated),unsigned(out.position_dirty)+unsigned(out.power_dirty),
        unsigned(out.effects.size()),out.effects.empty()?~0u:out.effects[0]};
}
}
int main(int argc,char**argv){try{
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
    std::ifstream fixture;
    if(!probe){if(argc!=3 || std::string_view(argv[1])!="--fixture")throw FormatError("expected --probe or --fixture PATH");
        fixture.open(argv[2]);if(!fixture)throw FormatError("missing device motion fixture");}
    auto& in=probe?std::cin:fixture;std::string line;unsigned count=0;
    while(std::getline(in,line)){
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);std::vector<unsigned> words;unsigned word;
        while(row>>std::hex>>word)words.push_back(word);
        if(!row.eof() || words.size()!=(probe?19:29))throw FormatError("invalid device motion fixture row");
        const std::vector<unsigned> inputs(words.begin(),words.begin()+19);const auto out=run(inputs);
        if(probe){for(auto v:out)std::cout<<std::hex<<v<<' ';std::cout<<'\n';}
        else if(!std::equal(out.begin(),out.end(),words.begin()+19))
            throw FormatError("original device motion mismatch at case "+std::to_string(count));
        ++count;
    }
    if(!probe){if(count<1000)throw FormatError("device motion fixture lost coverage");std::cout<<count<<" original device motion cases passed\n";}
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

#include "halo2/source_bloom_context.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
Words parse(const std::string &line) {
    std::istringstream in(line);Words out;std::uint32_t word;
    while(in>>std::hex>>word)out.push_back(word);
    return out;
}
Words solve(const Words &input) {
    if(input.size()!=9 || input[0]>2 || input[1]>65535)throw FormatError("bloom context fixture input");
    std::optional<SourceBloomScreenContext> context;
    Words out;
    if(input[0]==1) {
        context=source_player_bloom_screen_context(std::bit_cast<std::int16_t>(std::uint16_t(input[1])));
        out.assign(context->words.begin(),context->words.end());
    } else if(input[0]==2) {
        context.emplace();context->words[4]=input[2];
    }
    BspFogSample fog;
    for(unsigned i=0;i<4;++i)fog.bloom[i]=std::bit_cast<float>(input[i+5]);
    const auto frame=bsp_bloom_frame(fog,std::bit_cast<float>(input[3]),std::bit_cast<float>(input[4]),
        source_bloom_screen_threshold_factor(context?&*context:nullptr));
    for(float value:{frame.threshold,frame.strength,frame.gamma})out.push_back(std::bit_cast<std::uint32_t>(value));
    return out;
}
}
int main(int argc,char **argv) {try {
    const bool probe=argc==3 && std::string(argv[1])=="--probe";
    const auto path=probe?argv[2]:HALO2_SOURCE_DIR "/tests/fixtures/retail_source_bloom_context.tsv";
    std::ifstream fixture(path);if(!fixture)throw FormatError("original bloom context fixture missing");
    unsigned cases=0,normal=0,absent=0,controlled=0;std::size_t checked=0;std::string line;
    while(std::getline(fixture,line)) {
        if(line.empty()||line[0]=='#')continue;
        const auto split=line.find('|');if(split==std::string::npos)throw FormatError("bloom context fixture separator");
        const auto input=parse(line.substr(0,split)),expected=parse(line.substr(split+1)),actual=solve(input);
        if(actual!=expected) {
            for(std::size_t i=0;i<std::min(actual.size(),expected.size());++i)if(actual[i]!=expected[i])
                std::cerr<<"case "<<cases<<", word "<<i<<": "<<std::hex<<actual[i]<<" versus "<<expected[i]<<std::dec<<'\n';
            throw FormatError("normal player bloom differs from untouched source");
        }
        ++cases;checked+=actual.size();normal+=input[0]==1;absent+=input[0]==0;controlled+=input[0]==2;
        if(probe) {for(auto word:actual)std::cout<<std::hex<<word<<' ';std::cout<<'\n';}
    }
    if(cases!=526 || normal!=178 || absent!=178 || controlled!=170 || checked!=3180)
        throw FormatError("bloom source coverage changed");
    // Independent retained original20 GPU threshold8941 C6 is 172/255,
    // alpha32/255. The old absent-context path is the negative control.
    const auto context=source_player_bloom_screen_context(0);
    const auto player=bsp_bloom_frame({},.5f,.5f,source_bloom_screen_threshold_factor(&context));
    const auto null=bsp_bloom_frame({},.5f,.5f,source_bloom_screen_threshold_factor(nullptr));
    if(player.programs.threshold.words[13]!=0x20acacacu || null.programs.threshold.words[13]!=0x20e5e5e5u ||
       player.programs.composite.words[10]!=null.programs.composite.words[10] ||
       player.programs.blur.words!=null.programs.blur.words)
        throw FormatError("actual captured threshold or independent null control changed");
    if(!probe)std::cout<<cases<<" unchanged source cases, "<<checked<<" exact words; player172/null229 threshold controls passed\n";
    return 0;
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}}

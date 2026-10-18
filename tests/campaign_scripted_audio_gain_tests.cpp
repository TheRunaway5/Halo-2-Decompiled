#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/sound_variation.hpp"
#include "halo2/sound_driver_parameters.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <set>
#include <span>
using namespace halo2;
namespace {
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;
    for(auto byte:bytes){const auto v=std::to_integer<unsigned>(byte);result+=digits[v>>4];result+=digits[v&15];}
    return result;
}
}
int main(int argc,char **argv)try {
    if(argc!=4)throw FormatError("Supply TU15_XBE OPENING_MAP ORIGINAL_GAIN_TSV");
    for(int i=1;i<4;++i)if(!std::filesystem::is_regular_file(argv[i]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    if(!cache.original_data()->sound_driver_parameter_constants)throw FormatError("Original driver constants absent");
    std::ifstream input(argv[3]);std::set<std::uint32_t> cues;
    std::uint32_t sound,control;int volume;std::string raw,playback,cls,scale;
    unsigned admitted{};
    while(input>>std::hex>>sound>>control>>std::dec>>volume>>raw>>playback>>cls>>scale) {
        if(!cues.insert(sound).second)throw FormatError("Repeated opening gain cue");
        const auto definition=cache.load_sound_variation(sound);
        if(hex(definition.sound)!=raw || hex(definition.playback)!=playback ||
            hex(definition.sound_class)!=cls || hex(definition.scale)!=scale)
            throw FormatError("Opening sound/playback/class/scale bytes differ from original fixture");
        if(std::bit_cast<std::uint32_t>(source_sound_control_gain(definition,1.f))!=control)
            throw FormatError("Opening caller gain differs from unchanged Xbox CPU bits");
        SoundLocalRandom random(0x12345678u);
        const auto gain=source_sound_gain(definition,random)+source_sound_control_gain(definition,1.f);
        auto range=imported_sound_range_state(definition);
        const auto selected=source_sound_permutation(definition,range,random);
        const auto permutation=std::bit_cast<std::int8_t>(definition.permutations.at(selected)[4]);
        const auto actual=source_sound_driver_volume(std::bit_cast<std::uint32_t>(gain+float(permutation)),
            *cache.original_data()->sound_driver_parameter_constants,{0x027f,0x1f80});
        if(actual!=volume)throw FormatError("Opening native gain kernels differ from unchanged Xbox packet/driver output");
        if(gain>-64.f)++admitted;
    }
    if(!input.eof() || cues.size()!=49 || admitted!=47)throw FormatError("Incomplete original opening gain coverage");
    std::cout<<"PASS all49 exact sound/playback/class/scale banks and caller gain bits; native gain/driver kernels match original packet CPU,47 above admission floor/2 silent\n";
    std::cout<<"LIMIT centered control1 with neutral class fades; no PCM decoding, live listener/DSP/mix or audible synchronization proof\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

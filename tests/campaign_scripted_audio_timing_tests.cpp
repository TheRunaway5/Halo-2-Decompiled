#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/scripts.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=4)throw FormatError("Supply TU15_XBE OPENING_MAP ORIGINAL_TIMING_TSV");
    for(int i=1;i!=4;++i)if(!std::filesystem::is_regular_file(argv[i]))return 77;
    struct Expected {unsigned tick;std::uint32_t sound;};
    std::ifstream input(argv[3]);std::map<unsigned,Expected> expected;
    unsigned script,tick;std::uint32_t sound;
    while(input>>std::dec>>script>>tick>>std::hex>>sound)
        if(!expected.emplace(script,Expected{tick,sound}).second)
            throw FormatError("Repeated original cue thread");
    if(expected.size()!=49)throw FormatError("Incomplete original opening timing fixture");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const auto content=cache.load_content(0,false);
    std::map<std::uint32_t,unsigned> actual;
    ScriptVm vm(content.scripts,[&](const auto &name,const auto &args) {
        if(name=="sound_impulse_start") {
            if(args.size()!=3 || !actual.emplace(args[0].bits,tick).second)
                throw FormatError("Opening cue duplicated or malformed");
        }
        return ScriptValue{};
    },[](int index,unsigned){return std::uint32_t(index+1);});
    for(const auto &[index,cue]:expected) {
        if(index>=content.scripts.scripts.size() || content.scripts.scripts[index].type!=1)
            throw FormatError("Original fixture does not refer to a dormant authored cue");
        vm.start(content.scripts.scripts[index].name);
    }
    for(tick=0;tick<=750;++tick)vm.tick(tick);
    if(!vm.diagnostics().empty())throw FormatError(vm.diagnostics().front());
    if(actual.size()!=49)throw FormatError("Native script VM omitted an opening cue");
    for(const auto &[index,cue]:expected) {
        const auto found=actual.find(cue.sound);
        if(found==actual.end() || found->second!=cue.tick)
            throw FormatError("Original wake-relative audio request timing differs: "+content.scripts.scripts[index].name);
    }
    if(actual.at(0xea13089eu)-actual.at(0xea12089du)!=211)
        throw FormatError("Prophet dialogue request gap differs from original Xbox");
    std::cout<<"PASS all 49 authored cue request ticks match unchanged TU1.5 HS runner; Prophet request gap=211 ticks\n";
    std::cout<<"LIMIT wake-relative script timing; full scene wake ordering and SDL delivery are separate\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

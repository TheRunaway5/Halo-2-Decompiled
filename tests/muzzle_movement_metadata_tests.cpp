#include "halo2/muzzle_effects.hpp"
#include <algorithm>
#include <array>
#include <iostream>

int main(int argc,char **argv) {try {
    using namespace halo2;
    if(argc!=3)throw FormatError("Supply original disc and map-pack directories");
    const std::array names{"ascension","beavercreek","burial_mounds","coagulation",
        "colossus","cyclotron","foundation","headlong","lockout","midship",
        "waterworks","zanzibar","backwash","containment","deltatap","dune",
        "elongation","gemini","triplicate","turf","warlock"};
    unsigned maps=0,aliases=0;
    for(const auto name:names) {
        auto path=std::filesystem::path(argv[1])/(std::string(name)+".map");
        if(!std::filesystem::exists(path))path=std::filesystem::path(argv[2])/path.filename();
        Cache cache(path);
        const auto effect=std::find_if(cache.tags().begin(),cache.tags().end(),[](const Tag &tag){
            return tag.group=="effe" && tag.name=="effects\\objects\\weapons\\rifle\\smg\\fire_bullet";
        });
        if(effect==cache.tags().end())throw FormatError("Missing actual stock muzzle effect");
        const auto definition=cache.load_muzzle_effect(effect->datum);
        if(definition.systems.empty())throw FormatError("Actual stock muzzle systems were omitted");
        for(const auto &tag:cache.tags())if(tag.group=="pmov" && tag.address && !tag.size &&
                tag.name=="effects\\particle_physics\\fire_burst") {
            const auto owner=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const Tag &candidate){
                return candidate.group==tag.group && candidate.address==tag.address && candidate.size>=16;
            });
            if(owner==cache.tags().end())throw FormatError("Zero-size original movement alias has no literal resident owner");
            ++aliases;
        }
        ++maps;
    }
    if(!aliases)throw FormatError("Actual zero-size movement aliases were not exercised");
    std::cout<<maps<<" stock SMG definitions passed; "<<aliases<<" literal zero-size fire-burst aliases exercised\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}

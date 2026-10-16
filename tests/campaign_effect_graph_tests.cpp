#include "halo2/world_effects.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_script_effect.hpp"
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=4)throw FormatError("Supply opening MAP SHARED_MAPS TU15_XBE");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::is_regular_file(argv[3]))return 77;
    const auto original=OriginalData::load_xbe(argv[3]);
    if(original->image!=OriginalData::Image::TitleUpdate15)
        throw FormatError("Opening graph test requires its TU1.5 original-data bank");
    Cache cache(argv[1],argv[2],original);
    const auto content=cache.load_content(0,false);
    const auto &named=content.object_names.at(26);
    const auto object=std::ranges::find_if(content.objects,[&](const auto &placed) {
        return placed.type==named.type && placed.placement==named.placement;
    });
    if(named.name!="halo_exploding" || object==content.objects.end() || object->definition!=0xe201008cu)
        throw FormatError("Authored opening explosion owner differs");
    const auto model=cache.load_initial_model_definition(object->definition);
    const auto markers=source_model_markers(cache.load_source_tag_metadata(model.render_model),0x130008fdu);
    if(markers.size()!=1 || markers[0].region!=-1 || markers[0].permutation!=-1 || markers[0].node!=0)
        throw FormatError("Actual opening explosion marker requires different selection inputs");
    CollisionWorld collision(content.collision);Mesh mesh;
    NativeWorldEffects effects(cache,mesh,collision,0x12345678);
    for(const auto id:{0xea03088eu,0xea070892u}) {
        // Component admission remains available to callers explicitly owning
        // separate source parts. It must not imply complete graph admission.
        effects.preload_attached(id);
        bool missing=false;
        try {effects.preload_complete(id);}
        catch(const FormatError &error) {
            const auto message=std::string(error.what());
            missing=message.find("lens")!=std::string::npos || message.find("Lens")!=std::string::npos;
            if(missing)std::cout<<"EXPECTED "<<std::hex<<id<<std::dec<<' '<<message<<'\n';
            else throw;
        }
        if(!missing)throw FormatError("Opening lens graph was incorrectly admitted as complete");
    }
    // Independent real particle-only graph remains complete. Choose by actual
    // reference so a guessed replacement graph cannot satisfy this control.
    const auto debris=cache.query_tag({.handle=TagHandle{0xe24d00d8u}});
    if(!debris || debris->name!="effects\\cinematics\\01\\halo_venting_debris")
        throw FormatError("Actual particle-only opening control differs");
    effects.preload_complete(debris->datum);
    std::cout<<"PASS actual opening marker; both partial graphs retain explicit lens boundaries\n";
    std::cout<<"LIMIT no effect allocation, moving animation, RNG, GPU/audio consumers or complete opening playback\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

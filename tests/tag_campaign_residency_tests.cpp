#include "halo2/residency.hpp"
#include "halo2/model_selection.hpp"
#include <iostream>

int main(int argc,char **argv) try {
    using namespace halo2;
    if(argc!=3)throw FormatError("Supply CAMPAIGN_MAP SHARED_DIRECTORY");
    Cache cache(argv[1],argv[2]);
    const auto *biped=cache.query_tag({.group=tag_groups::biped,.require_address=true});
    if(!biped)throw FormatError("Campaign fixture has no authored biped");
    const auto definition=cache.load_initial_model_definition(biped->datum);
    if(!cache.find_tag({definition.render_model},tag_groups::render_model))
        throw FormatError("Authored biped has no render model");
    NativeResidency residency(cache);
    residency.model(definition.model,-1);
    const auto bitmaps=residency.bitmap_count();
    if(residency.model_count()!=1 || !bitmaps)
        throw FormatError("Campaign hlmt prediction did not retain its render resources");
    residency.model(definition.render_model,-1);
    residency.model(biped->datum,-1);
    residency.model(null_datum,-1);
    if(residency.model_count()!=1 || residency.bitmap_count()!=bitmaps)
        throw FormatError("Model aliases allocated duplicate residency banks");
    bool rejected{};
    try{residency.model(definition.model^0x10000u,-1);}
    catch(const FormatError &){rejected=true;}
    if(!rejected || residency.model_count()!=1 || residency.bitmap_count()!=bitmaps)
        throw FormatError("Stale model salt changed residency");
    std::cout<<"PASS campaign model residency: object="<<biped->datum
             <<" hlmt="<<definition.model<<" mode="<<definition.render_model
             <<" models="<<residency.model_count()<<" bitmaps="<<bitmaps<<'\n';
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

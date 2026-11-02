#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include <algorithm>
#include <iostream>
using namespace halo2;
int main(int argc,char**argv)try {
    if(argc!=2)throw FormatError("Supply prepared MCC maps");
    if(!std::filesystem::is_regular_file(std::filesystem::path(argv[1])/"shared.map"))return 77;
    struct Case {const char*target;const char*donor;const char*bitmap;};
    for(const auto&test:{Case{"03a_oldmombasa","shared","scenarios\\bitmaps\\detail_maps\\metal\\scratches"},
        Case{"06a_sentinelwalls","00a_introduction","objects\\characters\\prophet\\bitmaps\\holo_static_scanlines"},
        Case{"06a_sentinelwalls","00a_introduction","objects\\characters\\prophet\\bitmaps\\holo_static_noise"},
        Case{"derelict","01a_tutorial","objects\\bitmaps\\default\\grid_256x256"}}) {
        const Cache target(std::filesystem::path(argv[1])/(std::string(test.target)+".map"));
        const Cache donor(std::filesystem::path(argv[1])/(std::string(test.donor)+".map"));
        const auto*a=target.query_tag({.group=tag_groups::bitmap,.name=test.bitmap,.require_address=true});
        const auto*b=donor.query_tag({.group=tag_groups::bitmap,.name=test.bitmap,.require_address=true});
        if(!a || !b || target.bitmap_count(a->datum)!=donor.bitmap_count(b->datum))throw FormatError("Missing complete shader bitmap owner");
        const auto actual=target.load_bitmap(a->datum,0),expected=donor.load_bitmap(b->datum,0);
        if(actual.width!=expected.width || actual.height!=expected.height || actual.depth!=expected.depth ||
           actual.layers!=expected.layers || actual.format!=expected.format || actual.pixels!=expected.pixels || actual.mips.size()!=expected.mips.size())
            throw FormatError("Shader bitmap owner changed a complete mip/face bank");
        for(unsigned n=0;n<actual.mips.size();++n) {
            const auto&x=actual.mips[n];const auto&y=expected.mips[n];
            if(x.width!=y.width || x.height!=y.height || x.depth!=y.depth || x.offset!=y.offset || x.size!=y.size)
                throw FormatError("Shader bitmap owner changed its mip layout");
        }
        std::cout<<"PASS "<<test.target<<" "<<test.bitmap<<" ("<<actual.mips.size()<<" exact mips/faces)\n";
    }
    unsigned maps=0,materials=0,bindings=0;
    for(const auto&entry:std::filesystem::directory_iterator(argv[1]))if(entry.path().extension()==".map") {
        const Cache cache(entry.path());++maps;
        for(const auto&tag:cache.tags())if(tag.group=="shad" && cache.is_preserved_native_shader(tag)) {
            const auto metadata=cache.load_material_metadata(tag.datum);++materials;
            for(const auto&pass:metadata->passes)for(const auto&binding:pass.textures) {
                if(binding.external || binding.parameter==255)continue;
                const auto remap=std::ranges::find_if(pass.bitmap_remaps,
                    [&](const auto&value){return (value.destination&255)==binding.parameter;});
                if(remap==pass.bitmap_remaps.end() || remap->source>=metadata->bitmaps.size())
                    throw FormatError("Bound bitmap remap outside native shader: "+tag.name);
                const auto&image=metadata->bitmaps[remap->source];
                if(image.datum==null_datum)continue;
                try {
                    if(image.index>=cache.bitmap_count(image.datum))throw FormatError("Bitmap frame outside bank");
                }catch(const FormatError&e) {
                    throw FormatError(entry.path().stem().string()+" / "+tag.name+": "+e.what());
                }
                ++bindings;
            }
        }
    }
    if(maps<40 || materials<15000 || !bindings)throw FormatError("Native shader bitmap census was not exercised");
    std::cout<<"PASS "<<maps<<" maps / "<<materials<<" native materials / "<<bindings<<" bound bitmap owners\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

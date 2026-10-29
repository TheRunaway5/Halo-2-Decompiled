#include "halo2/particle_model.hpp"
#include "halo2/particle_render_inputs.hpp"
#include "halo2/particle_draw_resource_validation.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <iostream>

using namespace halo2;
int main(int argc,char** argv)try {
    if(argc!=2 || !*argv[1] || !std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());
    if(!cache.is_mcc())throw FormatError("Particle model child validation requires prepared MCC content");
    unsigned models{},systems{};
    for(const auto& tag:cache.select_tags({.group=TagGroup::from_string("PRTM"),.require_address=true})) {
        const auto metadata=cache.load_source_tag_metadata(tag.datum);
        const Bytes root(metadata.bytes_at(metadata.source_address(),0xe0));
        const auto count=root.u32(0x78);if(!count)continue;
        const auto model=load_particle_model(cache,tag.datum);validate_particle_draw_resource(model);++models;
        const Bytes children(metadata.bytes_at(root.u32(0x7c),std::size_t(count)*0x38));
        for(unsigned system=0;system<count;++system) {
            const auto definition=cache.load_particle_system_render_definition(tag.datum,0xffffffffu,system);
            if(definition.particle_datum!=children.u32(system*0x38+4) || definition.emitters.empty())
                throw FormatError("Model child dispatch lost its actual authored particle/emitter owner");
            validate_particle_draw_resource(definition);++systems;
        }
    }
    if(!models || !systems)throw FormatError("Prepared MCC map did not exercise actual model children");
    std::cout<<"PASS "<<models<<" actual model meshes with "<<systems<<" independently authored child systems\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

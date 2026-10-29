#include "halo2/particle_draw_resource_validation.hpp"
#include <iostream>
#include <limits>

using namespace halo2;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
ParticleSpriteDefinition sprite() {
    ParticleSpriteDefinition result;
    for(auto& property:result.properties)property.modifier_input=-1;
    return result;
}
}
int main()try {
    const auto base=sprite();const SourceParticleSpriteContext context{-1,1,true};
    validate_particle_draw_resource(base,context);
    auto packed_color=base;packed_color.properties[0].curve.resize(20,std::byte{255});
    packed_color.properties[0].curve[0]=std::byte{1};packed_color.properties[0].curve[1]=std::byte{0x10};
    validate_particle_draw_resource(packed_color,context); // ARGB bytes are not scalar float endpoints.
    unsigned rejected{};
    const auto rejects=[&](const auto& edit) {
        auto definition=base;auto selected=context;edit(definition,selected);bool failed{};
        try{validate_particle_draw_resource(definition,selected);}
        catch(const ParticleResourceError&){failed=true;}
        check(failed,"Deferred malformed sprite inputs passed resource admission");++rejected;
    };
    rejects([](auto& d,auto&){d.properties[0].input=17;});
    rejects([](auto& d,auto&){d.properties[1].range=-1;});
    rejects([](auto& d,auto&){d.properties[2].modifier_input=17;});
    rejects([](auto& d,auto&){d.properties[3].curve={std::byte{1}};});
    rejects([](auto& d,auto& c){d.bitmap_type=3;d.flags=0x10; c.selected_sequence=0;});
    rejects([](auto& d,auto& c){d.bitmap_type=3;d.sequences={{{{0,0,0,1},{.5f,.5f}}}};c.selected_sequence=0;});
    rejects([](auto& d,auto& c){d.bitmap_type=3;d.sequences.resize(1);c.selected_sequence=1;});
    rejects([](auto&,auto& c){c.width=std::numeric_limits<float>::quiet_NaN();});
    // Direct geometry, unused frame properties and empty sequences have valid
    // authored behavior. An absent bitmap emits no sprite and needs no atlas.
    auto unused=base;unused.properties[4].input=17;
    validate_particle_draw_resource(unused,context);
    unused.bitmap_type=3;unused.sequences.resize(1);
    validate_particle_draw_resource(unused,{0,1,true});
    unused.flags=0x10000;unused.sequences[0].push_back({});
    validate_particle_draw_resource(unused,{0,1,true});
    unused.properties[0].input=17;
    validate_particle_draw_resource(unused,{-1,1,false});
    ParticleModelDefinition model;model.orientation=1;
    for(auto& property:model.properties)property.modifier_input=-1;
    model.vertices.resize(1);model.indices={0,0,0};model.spans={{{0,3}}};
    validate_particle_draw_resource(model);
    for(unsigned index=0;index<4;++index) {
        auto bad=model;bad.properties[index].input=17;bool failed{};
        try{validate_particle_draw_resource(bad);}catch(const ParticleResourceError&){failed=true;}
        check(failed,"Deferred malformed model property passed resource admission");++rejected;
    }
    std::cout<<"PASS static particle draw resource admission and "<<rejected<<" deferred failure controls\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

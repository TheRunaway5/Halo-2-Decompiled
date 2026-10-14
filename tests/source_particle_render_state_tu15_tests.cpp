#include "halo2/source_particle_render_state_tu15.hpp"
#include "halo2/source_particle_sprite_context.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>

using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply BACKWASH_MAP ORIGINAL_RENDER_INPUT_FIXTURE");
    if(!std::filesystem::exists(argv[1]))return 77;
    std::ifstream input(argv[2]);const auto fixture=nlohmann::json::parse(input);
    Cache cache(argv[1]);SourceRuntimeMemory memory;
    for(const auto &bank:fixture.at("banks")) {
        const auto raw=bank.at("hex").get<std::string>();std::vector<std::byte> bytes;
        for(std::size_t i=0;i<raw.size();i+=2)
            bytes.push_back(std::byte(std::stoul(raw.substr(i,2),nullptr,16)));
        memory.mount(bank.at("address").get<std::uint32_t>(),bytes,
            SourceRuntimeMemory::Access::ReadOnly,"original source particle fixture");
    }
    const auto saved=memory.checkpoint();unsigned count{};
    const auto vector=[](const auto &v){return Vec3{v.at(0).template get<float>(),
        v.at(1).template get<float>(),v.at(2).template get<float>()};};
    const auto &camera=fixture.at("camera");
    const SourceParticleRenderCameraTu15 view{vector(camera.at("position")),
        vector(camera.at("forward")),vector(camera.at("up")),{}};
    const auto bits=[](float value){return std::bit_cast<std::uint32_t>(value);};
    for(const auto &row:fixture.at("cases")) {
        const auto system=row.at("system").get<std::uint32_t>();
        const auto location=row.at("location").get<std::uint32_t>();
        const auto read=source_particle_render_state_tu15(cache,memory,system,location,view);
        const auto sprite=cache.load_particle_sprite(read.definition.particle_datum);
        ParticleRenderValueCache values;values.system_identity=system;values.location_identity=location;
        const auto mask=sprite.required_values&0x107f0u;
        if(mask!=row.at("shared_mask"))throw FormatError("Original shared-value request differs");
        complete_particle_values(values.values,mask,read.state.sources);
        std::size_t cursor{};
        for_each_particle_render_input(read.definition,read.state,values,*cache.shader_function_tables(),
            [&](auto particle,const ParticleSpriteInput &in,ParticleValueCache &computed,const ParticleValueSources &) {
                const auto &expected=row.at("callbacks").at(cursor++);
                const auto equal=[&](const auto &actual,const char *field) {
                    if(nlohmann::json(actual)!=expected.at(field))
                        throw FormatError(std::string("Original1784B0 input differs: ")+field);
                };
                equal(particle,"particle");equal(computed.evaluated,"evaluated");
                std::vector<std::uint32_t> slots;for(float v:computed.values)slots.push_back(bits(v));
                equal(slots,"values");equal(bits(in.scale),"scale");
                equal(std::array{bits(in.position.x),bits(in.position.y),bits(in.position.z)},"position");
                equal(std::array{bits(in.direction.x),bits(in.direction.y),bits(in.direction.z)},"direction");
                equal(std::array{bits(in.color[0]),bits(in.color[1]),bits(in.color[2]),bits(in.color[3])},"color");
                ++count;
            });
        if(cursor!=row.at("callbacks").size())throw FormatError("Original callback count differs");
        bool stale{};
        try{(void)source_particle_render_state_tu15(cache,memory,system^0x10000u,location,view);}
        catch(const FormatError&){stale=true;}
        if(!stale)throw FormatError("Stale source particle system was admitted");
        const auto other=fixture.at("cases").at(1-(system&65535u)).at("location").get<std::uint32_t>();
        bool foreign{};
        try{(void)source_particle_render_state_tu15(cache,memory,system,other,view);}
        catch(const FormatError&){foreign=true;}
        if(!foreign)throw FormatError("Foreign source particle location was admitted");
    }
    if(count!=14 || memory.checkpoint()!=saved)throw FormatError("Incomplete or mutating particle readback");
    std::cout<<"PASS14 original Backwash particle render inputs, two source systems; stale/foreign rejection and unchanged pools\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

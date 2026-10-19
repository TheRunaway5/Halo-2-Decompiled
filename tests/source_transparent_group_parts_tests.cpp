#include "halo2/source_transparent_group_parts.hpp"
#include "halo2/menu_world_source_geometry.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
unsigned rejected;
template<class F>void rejects(F &&fn) {
    try{fn();}catch(const FormatError &){++rejected;return;}
    throw FormatError("changed group command ownership was admitted");
}
}
int main(int argc,char **argv)try {
    require(argc==3,"Supply original map and native group fixture");
    if(!std::filesystem::exists(argv[1]))return 77;
    Cache cache(argv[1]);std::ifstream file(argv[2]);const auto fixture=nlohmann::json::parse(file);
    require(fixture.at("schema")=="opening-native-group-command-join-v1","group fixture schema changed");
    const auto &g=fixture.at("group");SourceLiveTransparentGroupTu15 group;
    group.section=g.at("section");group.flags=g.at("flags");group.first_command=g.at("first_command");
    group.end_command=g.at("end_command");group.layers=g.at("layers");group.shader_clock=g.at("shader_clock");
    group.queue_command.callback=0x423b0u;const std::array payload{group.section,group.flags};
    std::memcpy(group.queue_command.payload.data(),payload.data(),sizeof(payload));
    for(const auto &c:g.at("commands"))group.commands.push_back({c.get<std::array<std::uint32_t,6>>()});
    for(const auto &layer:g.at("ordered_layers"))group.ordered_layers.push_back({layer.at("layer"),
        layer.at("descriptors").get<std::vector<std::array<std::uint32_t,6>>>()});
    std::vector<SourceLiveModelBaseDrawTu15> units;
    for(const auto &row:fixture.at("units")) {
        auto &u=units.emplace_back();u.unit.words=row.at("words").get<std::array<std::uint32_t,8>>();
        u.ordinal=row.at("ordinal");u.object=row.at("object");u.definition=row.at("definition");
        u.override_slot=row.at("override_slot");u.shader_override=row.at("shader_override");u.lighting_class=u.unit.words[0]>>29;
        // Actual sorted native commands require resident source geometry.
        // Only immutable part/material bytes are consumed by this join; no
        // palette, WORLD pose, GPU allocation or visibility is manufactured.
        const auto geometry=load_menu_world_source_model_section(cache,u.unit.words[1],u.unit.words[3]>>9&511u);
        u.geometry.geometry.resident=true;
        for(const auto &part:geometry.parts()) {
            u.geometry.geometry.parts.push_back(part.bytes);
            if(part.material<0)continue;
            if(unsigned(part.material)>=u.geometry.materials.size())u.geometry.materials.resize(unsigned(part.material)+1);
            u.geometry.materials[unsigned(part.material)].shader=part.shader;
        }
    }
    SourceTransparentGroupPartBindings resolver(cache);const auto bound=resolver.resolve(group,units);
    require(bound.ordered_layers.size()==6 && bound.owners==std::vector<std::uint32_t>{3805216863u,3805413474u},
        "group lost Regret/throne ownership or an admitted empty layer");
    unsigned body=0,overrides=0;
    for(unsigned l=0;l<bound.ordered_layers.size();++l) {
        const auto &layer=bound.ordered_layers[l];const auto &original=group.ordered_layers[l];
        require(layer.layer==original.layer && layer.parts.size()==original.descriptors.size(),"group dropped or reordered a descriptor");
        for(unsigned n=0;n<layer.parts.size();++n) {
            const auto &part=layer.parts[n];const auto &d=original.descriptors[n];const auto &u=units.at(part.base_unit);
            require(part.sorted_ordinal==n && part.command_ordinal==(d[0]&65535u) &&
                part.pass->implementation_address()==d[4],"bound group descriptor identity changed");
            if(layer.layer==21){++overrides;require(u.override_slot!=255 && part.pass->shader()==0xe87106fcu,"hologram lost actual override");}
            else {++body;require(u.override_slot==255,"ordinary group part became an override");}
        }
    }
    require(body==17 && overrides==11,"actual native body/hologram schedule is incomplete");
    const auto again=resolver.resolve(group,units);
    require(again.ordered_layers.back().parts.front().pass==bound.ordered_layers.back().parts.front().pass,
        "immutable selected pass was rebuilt between identical views");
    rejects([&]{auto bad=group;bad.queue_command.callback=0x423c0u;(void)resolver.resolve(bad,units);});
    rejects([&]{auto bad=group;bad.queue_command.payload[0]^=1;(void)resolver.resolve(bad,units);});
    rejects([&]{auto bad=group;std::swap(bad.ordered_layers[0],bad.ordered_layers[1]);(void)resolver.resolve(bad,units);});
    rejects([&]{auto bad=group;bad.ordered_layers.back().descriptors.front()[0]^=1u<<16;(void)resolver.resolve(bad,units);});
    rejects([&]{auto bad=group;bad.ordered_layers.back().descriptors.front()[4]^=4;(void)resolver.resolve(bad,units);});
    rejects([&]{auto bad=group;bad.ordered_layers.back().descriptors.front()[2]^=1;(void)resolver.resolve(bad,units);});
    rejects([&]{auto bad=group;bad.commands.front().words[2]^=1;(void)resolver.resolve(bad,units);});
    rejects([&]{auto bad=units;bad.front().shader_override^=1;(void)resolver.resolve(group,bad);});
    rejects([&]{auto bad=units;bad.front().geometry.geometry.resident=false;(void)resolver.resolve(group,bad);});
    rejects([&]{auto bad=units;bad.back().ordinal=bad.front().ordinal;(void)resolver.resolve(group,bad);});
    // Declared post-cull bank controls test the join's stable-field boundary;
    // these are not a capture of44710 or a GPU resource-readiness substitute.
    auto native=std::make_shared<SourceLiveTransparentGroupGeometryTu15>();native->units.resize(153);
    for(const auto &u:units) {
        native->units[u.ordinal]=u.unit;
        native->units[u.ordinal].words[0]^=0x40u;
        native->units[u.ordinal].words[5]=0x556e18u+0x25cu;
    }
    auto current=group;current.geometry=native;
    require(resolver.resolve(current,units).ordered_layers.back().parts.size()==11,
        "post-cull mask/admission fields invalidated immutable group ownership");
    for(const unsigned word:{1u,2u,3u,4u})rejects([&] {
        auto changed=std::make_shared<SourceLiveTransparentGroupGeometryTu15>(*native);
        changed->units[units.front().ordinal].words[word]^=1;
        auto bad=current;bad.geometry=changed;(void)resolver.resolve(bad,units);
    });
    // Losing an entire admitted layer is the same silent consumer gap as
    // losing every hologram descriptor. Validate completeness as well as
    // the identities of whichever records happened to survive.
    for(const unsigned dropped:{21u,1u,3u,7u,16u,15u}) {
        auto bad=group;
        std::erase_if(bad.ordered_layers,[&](const auto &layer){return layer.layer==dropped;});
        bool failed=false;try{(void)resolver.resolve(bad,units);}catch(const FormatError &){failed=true;}
        require(failed,"transparent group silently accepts a dropped admitted layer");++rejected;
    }
    std::cout<<"PASS actual Regret/throne group: 17 body + 11 override descriptors, retained empty layer, "
        <<rejected<<" changed-owner/order controls; metadata join only, no live GPU claim\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

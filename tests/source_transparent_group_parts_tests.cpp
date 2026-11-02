#include "halo2/source_transparent_group_parts.hpp"
#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/source_hologram_group.hpp"
#include "halo2/source_bsp_ranges.hpp"
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
    std::vector<std::uint32_t> definitions;
    for(const auto &unit:units)definitions.push_back(unit.definition);
    std::ranges::sort(definitions);definitions.erase(std::unique(definitions.begin(),definitions.end()),definitions.end());
    Mesh hologram_residency;SourceHologramGroupBindings holograms(cache,hologram_residency,definitions);
    require(hologram_residency.materials.size()==1 &&
        hologram_residency.materials.front().source_shader_datum==0xe87106fcu &&
        hologram_residency.materials.front().forward_blend==1,
        "Regret/throne definition preload lost its authored HLMT hologram override");
    SourceHologramGroupDraw submission;submission.cache=&cache;submission.command=group.queue_command;
    submission.owners=bound.owners;submission.shader_clock=group.shader_clock;
    const std::array<std::array<float,12>,1> diagnostic_palette{{{1,0,0,0,0,1,0,0,0,0,1,0}}};
    // This isolated range handoff uses retained unit flags with an explicitly
    // declared identity palette. It does not stand in for a live WORLD pose.
    auto unfiltered=group;auto ranges=std::make_shared<SourceLiveTransparentGroupGeometryTu15>();
    ranges->units.resize(153);auto diagnostic_units=units;
    for(auto &unit:diagnostic_units) {
        ranges->units[unit.ordinal]=unit.unit;
        unit.palette_rows.assign(diagnostic_palette.begin(),diagnostic_palette.end());
    }
    unfiltered.geometry=ranges;
    const auto planned=holograms.draw(std::span{&unfiltered,1},std::span{&bound,1},diagnostic_units,
        {},{}, {0,16777215,.01f,1024.f},{1,1},{640,480});
    require(planned.size()==1 && planned.front().parts.size()==28,
        "unfiltered original group lost its exact body/override range handoff");
    for(const auto &part:planned.front().parts)if(part.layer!=21)
        require(part.body_vertices>0,"unfiltered body part lost its nondegenerate original triangle count");
    {
        auto filtered=unfiltered;auto masked=std::make_shared<SourceLiveTransparentGroupGeometryTu15>(*ranges);
        const auto &body_part=planned.front().parts.front();const auto &base=diagnostic_units.at(bound.ordered_layers.front().parts.front().base_unit);
        const auto metadata=load_source_model_range_geometry(cache,base.unit.words[1],body_part.section);
        require(metadata.subpart_count>0,"filtered body control lost its actual source subpart bank");
        masked->arena_address=0x100000u;masked->units[body_part.base_ordinal].words[0]|=0x40u;
        masked->units[body_part.base_ordinal].words[5]=masked->arena_address;filtered.geometry=masked;
        bool failed=false;
        try{(void)holograms.draw(std::span{&filtered,1},std::span{&bound,1},diagnostic_units,
            {},{}, {0,16777215,.01f,1024.f},{1,1},{640,480});}
        catch(const FormatError &error){failed=std::string_view(error.what())=="filtered hologram body part requires its native CPU range consumer";}
        require(failed,"filtered native body range silently became an unfiltered whole-part draw");++rejected;
    }
    for(const auto &layer:bound.ordered_layers)for(const auto &part:layer.parts) {
        const auto &unit=units.at(part.base_unit);
        const auto &command=group.commands.at(part.command_ordinal-group.first_command).words;
        auto &selected=submission.parts.emplace_back();selected.layer=layer.layer;selected.command=part.command_ordinal;
        selected.sorted=part.sorted_ordinal;selected.base_ordinal=unit.ordinal;selected.object=unit.object;
        selected.section=unit.unit.words[3]>>9&511u;selected.part=command[3]&255u;selected.pass=part.pass;
        selected.body_vertices=3; // Explicit command-ledger diagnostic count, not a pose/range proof.
        if(layer.layer==21) {
            const auto geometry=std::make_shared<const MenuWorldSourceSection>(
                load_menu_world_source_model_section(cache,unit.unit.words[1],selected.section));
            const auto topology=(unit.unit.words[0]&0x200u) && !(geometry->parts().at(selected.part).flags&8u)?
                MenuWorldSourceTopology::triangle_strip:MenuWorldSourceTopology::triangle_list;
            selected.hologram_ranges.push_back(menu_world_source_draw(geometry,selected.part,
                topology,diagnostic_palette));
        }
    }
    std::vector<SourceHologramGroupSubmission> ledger{{SourceHologramGroupSubmission::Kind::scene_copy,0}};
    for(unsigned d=0;d<submission.parts.size();++d) {
        const auto &part=submission.parts[d];
        std::vector<std::uint16_t> ordered;
        if(part.layer==21)for(const auto &range:part.hologram_ranges)
            for(std::size_t triangle=0;triangle<range.triangle_count();++triangle) {
                const auto indices=range.triangle(triangle);
                ordered.insert(ordered.end(),indices.begin(),indices.end());
            }
        ledger.push_back({SourceHologramGroupSubmission::Kind::part,0,d,part.layer,0,
            part.layer==21?unsigned(ordered.size()):3u,part.object,part.pass->shader(),1,2});
        if(part.layer==21){ledger.back().indexed=true;ledger.back().index_buffer=3;}
    }
    validate_source_hologram_group_submissions(std::span{&submission,1},ledger);
    rejects([&]{validate_source_hologram_group_submissions(std::span{&submission,1},{});});
    rejects([&]{auto bad=ledger;bad.erase(bad.begin());validate_source_hologram_group_submissions(std::span{&submission,1},bad);});
    rejects([&]{auto bad=ledger;std::swap(bad[0],bad[1]);validate_source_hologram_group_submissions(std::span{&submission,1},bad);});
    rejects([&]{auto bad=ledger;std::erase_if(bad,[](const auto &e){return e.layer==21;});
        validate_source_hologram_group_submissions(std::span{&submission,1},bad);});
    rejects([&]{auto bad=ledger;bad.back().descriptor_set=0;validate_source_hologram_group_submissions(std::span{&submission,1},bad);});
    rejects([&]{auto bad=ledger;bad.back().index_count^=1;validate_source_hologram_group_submissions(std::span{&submission,1},bad);});
    require(submission.parts.at(17).hologram_ranges.front().index_range().count==785 &&
        ledger.at(18).index_count==2349,"actual Regret strip no longer distinguishes785 source indices from2349 submitted list indices");
    rejects([&]{auto bad=ledger;bad.at(18).index_count=785;
        validate_source_hologram_group_submissions(std::span{&submission,1},bad);});
    rejects([&]{auto bad=ledger;bad.back().first_index=1;validate_source_hologram_group_submissions(std::span{&submission,1},bad);});
    rejects([&]{auto bad=ledger;bad.back().vertex_offset=1;validate_source_hologram_group_submissions(std::span{&submission,1},bad);});
    rejects([&]{auto bad=ledger;bad.back().index_buffer=0;validate_source_hologram_group_submissions(std::span{&submission,1},bad);});
    rejects([&]{auto bad=ledger;bad.at(1).index_count=6;validate_source_hologram_group_submissions(std::span{&submission,1},bad);});
    rejects([&]{auto bad=ledger;std::swap(bad[1],bad[2]);validate_source_hologram_group_submissions(std::span{&submission,1},bad);});
    rejects([&]{auto bad=ledger;bad.push_back(bad.back());validate_source_hologram_group_submissions(std::span{&submission,1},bad);});
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

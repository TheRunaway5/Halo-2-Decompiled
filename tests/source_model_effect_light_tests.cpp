#include "halo2/source_effect_light_opaque.hpp"
#include "halo2/source_bsp_admission.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <bit>
#include <cstring>
using namespace halo2;
using Json=nlohmann::json;
namespace {void require(bool b,const char* why){if(!b)throw FormatError(why);}}
int main(int argc,char**argv)try{
    require(argc==5,"Supply MAP ORIGINAL_SELECTION_JSON ORIGINAL_RANGE_FIXTURE ORIGINAL_COMMAND_JSON");
    auto cache=std::make_shared<Cache>(argv[1]);std::ifstream file(argv[2]);if(!file)return 77;
    Json input;file>>input;require(input.at("schema")=="tu15-full-light-selection-v1","Unexpected original selection schema");
    SourceTagMetadataStore store(cache,{"shad","stem","spas","vrtx"});
    const auto bsp=cache->load_source_bsp_admission(0);
    std::vector<SourceCullDrawUnit> units;
    for(const auto& row:input.at("units"))units.push_back({row.get<std::array<std::uint32_t,8>>()});
    SourcePartCommandBank commands;commands.header=input.at("header").get<std::array<std::uint32_t,6>>();
    for(const auto& row:input.at("commands")){
        auto values=row.get<std::array<std::uint32_t,6>>();
        require(values[0]==0x4cbe0 && values[1]==0x4d0a0,"Unresolved original sphere callback");
        values[0]=0x4cbf0;values[1]=0x4d0b0;commands.storage.push_back({values});
    }
    struct Geometry {SourceBspRangeGeometry ranges;SourceOpaqueGeometryMetadata metadata;};
    using Key=std::tuple<unsigned,std::uint32_t,unsigned>;
    std::map<Key,Geometry> geometries;unsigned models{};
    const auto key=[&](const SourceCullDrawUnit& unit){
        if(unit.words[1]!=0xffffffffu)return Key{2,unit.words[1],unit.words[3]>>9&511u};
        if(unit.words[0]&0x1000u)return Key{1,0,unsigned(bsp.instances.at(unit.words[3]>>18&2047u).definition)};
        return Key{0,0,unit.words[3]>>9&511u};
    };
    const auto geometry=[&](unsigned ordinal)->Geometry&{
        const auto selected=key(units.at(ordinal));auto found=geometries.find(selected);
        if(found==geometries.end()){
            const auto [kind,model,section]=selected;Geometry bank;
            if(kind==2){
                const auto raw=load_menu_world_source_model_section(*cache,model,section);
                bank.ranges=source_model_range_geometry(raw);
                bank.metadata=load_source_opaque_model_geometry_metadata(*cache,model,section);++models;
            }else{
                const auto category=kind?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster;
                bank.ranges=load_source_bsp_range_geometry(*cache,0,category,section);
                bank.metadata=load_source_opaque_bsp_geometry_metadata(*cache,0,category,section);
            }
            found=geometries.emplace(selected,std::move(bank)).first;
        }
        return found->second;
    };
    std::ifstream command_file(argv[4]);if(!command_file)return 77;Json projected;command_file>>projected;
    require(projected.at("schema")=="tu15-light-model-commands-v1","Unexpected original command schema");
    std::vector<SourceCullDrawUnit> command_units;
    for(const auto& row:projected.at("units"))command_units.push_back({row.get<std::array<std::uint32_t,8>>()});
    SourceBspPartCommandContext context;
    context.mode=projected.at("mode");context.flag_mask=projected.at("flag_mask");context.group=projected.at("group");
    context.main_prefix=projected.at("prefix");context.draw_count=projected.at("count");context.mask_arena_address=projected.at("arena_address");
    context.player_multiplier=std::bit_cast<float>(projected.at("player_multiplier").get<std::uint32_t>());
    context.view_modifier=projected.at("view_modifier").get<unsigned>()!=0;
    for(unsigned i=0;i<7;++i)context.lod_view.multipliers[i]=std::bit_cast<float>(projected.at("lod_view")[i].get<std::uint32_t>());
    const auto coverage=projected.at("coverage").get<std::vector<std::uint8_t>>();
    const auto slots=projected.at("override_slots").get<std::vector<std::uint8_t>>();
    const auto overrides=projected.at("overrides").get<std::vector<std::uint32_t>>();
    context.coverage=coverage;context.override_slots=slots;context.overrides=overrides;
    SourceBspRangeArena command_arena;command_arena.words=projected.at("arena").get<std::array<std::uint32_t,512>>();
    SourceBspPartCommandWorld world;
    std::vector<SourceBspCommandGeometry> clusters(bsp.clusters.size()),definitions(bsp.definitions.size());
    std::vector<std::int16_t> instances;for(const auto& instance:bsp.instances)instances.push_back(instance.definition);
    for(const auto& unit:command_units)if(unit.words[1]==0xffffffffu){
        const auto [kind,unused,section]=key(unit);auto& target=kind?definitions.at(section):clusters.at(section);
        if(!target.resident){
            const auto raw=load_source_bsp_range_geometry(*cache,0,kind?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster,section);
            target.parts=raw.parts;target.resident=true;
        }
    }
    world.clusters=clusters;world.definitions=definitions;world.instance_definitions=instances;
    const auto source_bsp=cache->load_source_bsp_metadata(0);const Bytes bsp_root(source_bsp.bytes_at(source_bsp.source_address(),source_bsp.size()));
    std::vector<SourceBspCommandMaterial> materials;
    for(unsigned i=0;i<bsp_root.u32(16+0xa4);++i){const Bytes row(source_bsp.bytes_at(bsp_root.u32(16+0xa8)+i*32,32));
        materials.push_back({row.u32(12),std::to_integer<std::uint8_t>(row.slice(28,1)[0])});}
    world.materials=materials;world.bsp_selector=projected.at("bsp_selector");
    const auto hex=[](const std::string& value){std::vector<std::byte> out;require(value.size()%2==0,"Invalid original hex backing");
        for(unsigned i=0;i<value.size();i+=2)out.push_back(std::byte(std::stoul(value.substr(i,2),nullptr,16)));return out;};
    const auto visibility=hex(projected.at("visibility"));world.material_visibility=visibility;
    const auto read=[&](std::uint32_t at,std::size_t n){return Bytes(cache->read_tag_bytes(at,n));};
    std::map<std::uint32_t,SourceBspCommandShader> shaders;
    world.shader=[&](std::uint32_t id)->const SourceBspCommandShader&{
        auto found=shaders.find(id);if(found==shaders.end()){
            SourceBspCommandShader value;value.lod=cache->load_source_shader_lod_definition(id);
            const auto shader=cache->load_source_tag_metadata(id);const auto root=read(shader.source_address(),0x28);
            const auto stem=cache->load_source_tag_metadata(read(root.u32(0x24),4).u32(0));const auto post=read(read(stem.source_address(),0x60).u32(0x5c),8);
            for(unsigned i=0;i<post.u32(0);++i)value.layers.push_back(read(post.u32(4)+i*10,10).u32(2));
            found=shaders.emplace(id,std::move(value)).first;
        }return found->second;
    };
    // Geometry resources are shared; current palettes belong to each unit's
    // cache handle. Repeated sections must not alias another object's pose.
    std::map<std::array<std::uint32_t,8>,SourceModelCommandGeometry> model_commands;
    for(const auto& model:projected.at("models")){
        const auto id=model.at("model").get<std::uint32_t>(),section=model.at("section").get<unsigned>();
        SourceModelCommandGeometry value;value.geometry.resident=true;value.geometry.parts=load_source_model_range_geometry(*cache,id,section).parts;
        const auto tag=cache->load_source_tag_metadata(id);const Bytes root(tag.bytes_at(tag.source_address(),0x68));
        for(unsigned i=0;i<root.u32(0x60);++i){const Bytes row(tag.bytes_at(root.u32(0x64)+i*32,32));value.materials.push_back({row.u32(12),255});}
        value.model_flags=root.u16(4);
        const Bytes section_metadata(tag.bytes_at(root.u32(0x28)+section*92,92));
        value.rigid_node=std::bit_cast<std::int16_t>(section_metadata.u16(0x2c));
        require(value.model_flags==model.at("model_flags").get<std::uint16_t>() &&
            value.rigid_node==model.at("rigid_node").get<std::int16_t>(),"Original model center metadata differs");
        for(const auto& range:model.at("regions"))value.palette.regions.push_back({range[0].get<std::uint16_t>(),range[1].get<std::uint16_t>()});
        for(const auto& matrix:model.at("matrices")){std::array<float,12> row;for(unsigned i=0;i<12;++i)row[i]=std::bit_cast<float>(matrix[i].get<std::uint32_t>());value.palette.matrices.push_back(row);}
        model_commands.emplace(command_units.at(model.at("unit").get<unsigned>()).words,std::move(value));
    }
    world.model=[&](const SourceCullDrawUnit& unit)->const SourceModelCommandGeometry&{return model_commands.at(unit.words);};
    SourceBspPartCommandState native_commands;
    for(unsigned i=0;i<3;++i){auto& bank=native_commands.banks[i];bank.header=projected.at("before")[i].at("header").get<std::array<std::uint32_t,6>>();
        for(const auto& row:projected.at("before")[i].at("storage"))bank.storage.push_back({row.get<std::array<std::uint32_t,6>>()});}
    const auto special=hex(projected.at("special_before"));require(special.size()==0x406,"Original special bank size differs");
    std::memcpy(native_commands.special_units.data(),special.data(),1024);std::memcpy(&native_commands.special_count,special.data()+0x404,2);
    source_bsp_append_part_commands(native_commands,command_units,command_arena,world,context);
    const auto special_after=hex(projected.at("special_after"));
    require(special_after.size()==0x406 && std::memcmp(native_commands.special_units.data(),special_after.data(),1024)==0 &&
        std::memcmp(&native_commands.special_count,special_after.data()+0x404,2)==0 &&
        std::memcmp(special.data()+0x400,special_after.data()+0x400,4)==0,"Original combined special-unit bank differs");
    for(unsigned i=0;i<3;++i){const auto& bank=native_commands.banks[i];const auto& original=projected.at("after")[i];
        require(bank.header==original.at("header").get<std::array<std::uint32_t,6>>(),"Original combined part-command header differs");
        for(unsigned n=0;n<bank.storage.size();++n){auto expected=original.at("storage")[n].get<std::array<std::uint32_t,6>>();
            if(i!=0 && n<bank.header[1]){if(expected[0]==0x4cbe0)expected[0]=0x4cbf0;if(expected[1]==0x4d0a0)expected[1]=0x4d0b0;}
            require(bank.storage[n].words==expected,"Original combined part-command payload differs");}
    }
    const auto original_calls=projected.at("calls").get<std::vector<std::uint32_t>>();
    require(!original_calls.empty() && original_calls.front()==0x44ac0,"Original combined command call trace is absent");
    if(native_commands.calls!=original_calls){
        unsigned first=0;while(first<native_commands.calls.size() && first<original_calls.size() && native_commands.calls[first]==original_calls[first])++first;
        std::cerr<<"Part-command calls native="<<native_commands.calls.size()<<" original="<<original_calls.size()<<" first difference="<<first<<'\n';
        for(unsigned n=first>5?first-5:0;n<std::min<unsigned>(first+10,std::max(native_commands.calls.size(),original_calls.size()));++n)
            std::cerr<<n<<" native="<<std::hex<<(n<native_commands.calls.size()?native_commands.calls[n]:0)<<" original="<<(n<original_calls.size()?original_calls[n]:0)<<std::dec<<'\n';
        throw FormatError("Original combined part-command call order differs");
    }
    require(input.at("retained_records_are_zero").get<bool>(),"Missing original retained output seed");
    std::array<SourceOpaqueSortRecord,1024> records{};
    const auto count=source_effect_light_opaque_select(commands,units,store.providers(),[&](unsigned ordinal){
        const auto& bank=geometry(ordinal);return SourceOpaqueGeometrySelection{bank.metadata,bank.ranges.parts};
    },records);
    require(count==input.at("records").size(),"Combined original model/BSP light selection count differs");
    for(unsigned i=0;i<count;++i)require(records[i].words==input.at("records")[i].get<std::array<std::uint32_t,6>>(),
        "Combined original model/BSP light sort record differs");
    SourceBspRangeArena arena;arena.words=input.at("arena").get<std::array<std::uint32_t,512>>();
    const auto arena_address=input.at("arena_address").get<std::uint32_t>();
    const auto ranges=source_effect_light_ordered_ranges(commands,units,std::span(records).first(count),arena,
        arena_address,[&](unsigned,unsigned ordinal,unsigned){return SourceOpaqueRangeGeometry{&geometry(ordinal).ranges,true};});
    std::ifstream fixture(argv[3],std::ios::binary);if(!fixture)return 77;
    const std::string raw((std::istreambuf_iterator<char>(fixture)),{});
    const Bytes all(std::as_bytes(std::span(raw.data(),raw.size())));std::size_t cursor=0;
    const auto word=[&](){const auto value=all.u32(cursor);cursor+=4;return value;};
    require(word()==0x474e524c && word()==1,"Unexpected original range schema");
    require(word()==ranges.size(),"Combined original light range record count differs");
    unsigned draws{},indices{},model_draws{};
    for(const auto& range:ranges){
        const auto command=word(),unit=word(),vertex=word(),kind=word(),section=word();
        require(command==range.command && unit==range.unit && vertex==range.vertex_class,"Original light range ordering differs");
        require(Key{kind,kind==2?units.at(unit).words[1]:0,section}==key(units.at(unit)),"Original light geometry owner differs");
        for(unsigned i=0;i<8;++i)require(word()==units.at(unit).words[i],"Original selected unit changed");
        const auto part=word();(void)word();(void)word();require(word()==arena_address,"Original mask arena changed");
        for(unsigned i=0;i<512;++i)require(word()==arena.words[i],"Original mask backing differs");
        const auto topology=word(),filtered=word(),returned=word(),n=word();
        require(part==range.part && topology==unsigned(range.selected.topology) && bool(filtered)==range.selected.filtered &&
            n==range.selected.ranges.size(),"Original model/BSP range selection differs");
        unsigned selected_indices{};
        for(const auto& selection:range.selected.ranges){
            const auto primitive=word(),first=word(),length=word();
            require(primitive==(topology?6u:5u) && first==selection.first && length==selection.count,"Original indexed submission differs");
            const auto [category,model,ordinal]=key(units.at(unit));
            const auto actual=category==2?load_menu_world_source_model_section(*cache,model,ordinal):
                load_menu_world_source_bsp_section(*cache,0,category?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster,ordinal);
            for(unsigned i=0;i<length;++i)require(word()==actual.indices()[first+i],"Original selected index value differs");
            ++draws;selected_indices+=length;if(kind==2)++model_draws;
        }
        require(returned==selected_indices,"Original indexed return count differs");indices+=selected_indices;
    }
    require(cursor==all.size() && models && model_draws,"Original model coverage incomplete");
    std::cout<<"PASS "<<count<<" combined original model/BSP sort records, "<<draws<<" indexed draws / "<<indices
        <<" indices; model draws="<<model_draws<<"; collector and material binding are separate owners\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

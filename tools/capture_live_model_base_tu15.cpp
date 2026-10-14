#include "capture_live_model_common.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_object_draw_collector.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_prt.hpp"
#include "halo2/model.hpp"
#include <cstring>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=5 && argc!=6)throw FormatError("Supply TU15_XBE MAP MODEL_FRAGMENT NEW_OUTPUT [valid|empty|invalid]");
    const std::string control=argc==6?argv[5]:"valid";
    if(control!="valid" && control!="empty" && control!="invalid")throw FormatError("unknown declared SH control");
    const std::filesystem::path out=argv[4];
    if(std::filesystem::exists(out))throw FormatError("model base output exists");
    std::filesystem::create_directories(out);Cache cache(argv[2]);
    const auto tag=std::ranges::find_if(cache.tags(),[&](const auto &t){return t.group=="mode" && t.name.find(argv[3])!=std::string::npos;});
    if(tag==cache.tags().end())throw FormatError("requested source model absent");
    PhysicsLiveWorldTu15 world(cache,argv[1],out/"savegame");
    world.configure_session(source_game_variant_default(0),1,30,0x12345678);
    world.preload_render_model(tag->datum);world.initialize_map();
    const auto placement=original_model_placement(world,tag->datum);
    const auto warmup=world.create_scenario_object(placement);
    if(warmup==null_datum)throw FormatError("source constructor declined warmup owner");
    world.begin_model_frame();world.begin_model_view();
    (void)world.build_world_model(warmup,0,1,false,0,0);
    const auto object=warmup;
    if(control!="valid") {
        const auto word=[&](std::uint32_t p){return Bytes(world.memory().view(p,4)).u32(0);};
        const auto lightmap=word(0x4ecc14u);
        if(!lightmap || !word(lightmap+0x80u))throw FormatError("SH control requires a real active lightmap group");
        const auto group=word(lightmap+0x84u),address=group+(control=="empty"?0x38u:0x1cu);
        const std::uint32_t value=control=="empty"?0u:0xffffffffu;
        world.memory().write(address,{reinterpret_cast<const std::byte*>(&value),sizeof(value)});
    }
    const auto dump=[&](const char *name) {
        std::ofstream file(out/name,std::ios::binary);
        world.memory().visit_committed([&](auto address,auto bytes) {
            const auto count=std::uint32_t(bytes.size());file.write(reinterpret_cast<const char*>(&address),4);
            file.write(reinterpret_cast<const char*>(&count),4);file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
        });
        if(!file)throw FormatError("model base memory export failed");
    };
    world.begin_model_frame();world.begin_model_view();dump("before.bin");
    SourceCullDrawUnit absent;absent.words.fill(0xdeadbeefu);
    if(source_live_model_prt_bank_tu15(world,absent,*cache.load_source_prt_definition(tag->datum)))
        throw FormatError("empty original PRT cache supplied a draw bank");
    const auto collector=world.build_world_model(object,0,1,false,0,0);dump("after.bin");
    std::ofstream(out/"call.json")<<nlohmann::json{{"owner",object},{"collector",collector.address},
        {"view",0},{"players",1},{"modifier",false},{"metric",0},{"lighting_metric",0},{"declared_sh_control",control},
        {"scope","Original-placement base collector after actual resource warmup, explicit distance; no rendered-frame claim"}}.dump(2)<<'\n';
    const auto count=Bytes(world.memory().view(0x515ec8,4)).u32(0);
    if(count==0)throw FormatError("selected original base collector did not populate PRT");
    // Read-only24020 binding controls use fields from the real completed
    // collector. Other unit fields remain unconsumed by that original owner.
    const auto word=[&](std::uint32_t p){return Bytes(world.memory().view(p,4)).u32(0);};
    const Bytes c(collector.bytes);const auto mode=c.u32(0x5cu),context=c.u32(0x48u);
    const auto body=word(word(word(0x4ecbd0u)+0x44u)+(object&65535u)*12u+8u);
    const Bytes b(world.memory().view(body+0x114u,4));std::vector<Transform> nodes(b.u16(0)/52u);
    std::memcpy(nodes.data(),world.memory().view(body+b.u16(2),nodes.size()*52u).data(),nodes.size()*52u);
    std::vector<unsigned> sections;std::vector<unsigned> ordinals;
    for(unsigned n=0;n<c.u32(0x18u);++n) {
        const auto section=std::to_integer<unsigned>(c.slice(0x75u+n,1).front());
        if(section!=255){sections.push_back(section);ordinals.push_back(n);}
    }
    auto lighting=cache.load_source_prt_lighting();
    if(control=="empty")lighting.records.clear();else if(control=="invalid")lighting.structure_matches=false;
    const auto definition=*cache.load_source_prt_definition(mode);
    const auto incident_bytes=world.memory().view(0x4c51d0u+92u,12);Vec3 incident;std::memcpy(&incident,incident_bytes.data(),12);
    const auto expected=source_prt_section_banks(cache,cache.load_model(mode,false),definition,c.u16(0x168u),sections,nodes,lighting,incident);
    unsigned checked=0;
    for(unsigned n=0;n<expected.size();++n) {
        SourceCullDrawUnit unit;unit.words.fill(0xdeadbeefu);
        unit.words[1]=mode;unit.words[3]=(sections[n]<<9)|ordinals[n];unit.words[4]=context;
        const auto actual=source_live_model_prt_bank_tu15(world,unit,definition);
        if(!actual || actual->constants.size()!=expected[n].bank->constants.size())throw FormatError("original PRT readback lost its bank extent");
        for(unsigned i=0;i<actual->constants.size()*4u;++i) {
            if(i==3u || (i>=8u && (i-8u)%16u==3u))continue;
            if(std::bit_cast<std::uint32_t>(actual->constants[i/4u][i%4u])!=
                std::bit_cast<std::uint32_t>(expected[n].bank->constants[i/4u][i%4u]))
                throw FormatError("original completed PRT readback differs from independently projected bank");
            ++checked;
        }
    }
    std::cout<<"Native TU1.5 PRT base: "<<tag->name<<" / "<<count<<" original owner records\n";
    std::cout<<"Completed original PRT readback: "<<checked<<" bit-exact consumed words\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

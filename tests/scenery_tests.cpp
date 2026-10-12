#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/scenery.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>

namespace {
using namespace halo2;
void require(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
struct Counts {unsigned placements{},baked{},dynamic{},unsupported{},excluded{},policy_dynamic{},probes{};};
Counts verify(const std::filesystem::path& path) {
    const Cache cache(path);
    const auto content=cache.load_content(0,true);
    const auto lighting=cache.load_bsp_lighting(0);
    const auto probes=cache.load_scenery_lightprobes(0);
    std::ifstream stream(path,std::ios::binary|std::ios::ate);
    require(bool(stream),"scenery fixture missing");
    std::vector<std::byte> bytes(static_cast<std::size_t>(stream.tellg()));
    stream.seekg(0);stream.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    require(bool(stream),"scenery fixture truncated");
    const Bytes raw(bytes);
    const auto index=raw.u32(16);
    const auto table=std::int64_t(raw.u32(index+8))-(std::int64_t(raw.u32(index))-index-32);
    const auto magic=std::int64_t(raw.u32(std::size_t(table)+8))-(index+raw.u32(20));
    const auto scenario=std::find_if(cache.tags().begin(),cache.tags().end(),
        [&](const auto& t){return t.datum==raw.u32(index+12);});
    require(scenario!=cache.tags().end(),"original scenario tag missing");
    const auto root=std::size_t(std::int64_t(scenario->address)-magic);
    const auto placed=std::size_t(std::int64_t(raw.u32(root+0x54))-magic);
    std::map<std::pair<unsigned,unsigned>,RenderModel> models;
    const auto& bsp=cache.bsps().at(0);
    const auto bsp_root=std::size_t(bsp.file_offset)+16;
    require(probes.size()==raw.u32(bsp_root+0x22c),"authored object lightprobe count changed");
    if(!probes.empty()) {
        const auto source=std::size_t(bsp.file_offset)+std::size_t(std::int64_t(raw.u32(bsp_root+0x230))-bsp.memory_address);
        for(std::size_t i=0;i<probes.size();++i) {
            const auto at=source+i*0x5c;const auto& probe=probes[i];
            require(probe.unique_id==raw.u32(at) && probe.origin_bsp==std::int16_t(raw.u16(at+4)) &&
                probe.type==std::int8_t(std::to_integer<unsigned>(raw.slice(at+6,1)[0])) &&
                probe.source==std::int8_t(std::to_integer<unsigned>(raw.slice(at+7,1)[0])),"authored lightprobe identity changed");
            const auto values=probe.lighting.source_values();
            for(unsigned component=0;component<20;++component)
                require(std::bit_cast<std::uint32_t>(values[component])==raw.u32(at+8+component*4),"authored lightprobe lighting float changed");
            require(probe.sh_index==std::int16_t(raw.u16(at+0x58)) && probe.unknown==std::int16_t(raw.u16(at+0x5a)),
                "authored lightprobe packed metadata changed");
        }
    }
    Counts result;
    for(const auto& object:content.objects)if(object.type==6) {
        ++result.placements;
        const auto definition=cache.load_scenery_definition(object.definition);
        require(bool(definition),"original scenery definition missing");
        const auto source=placed+std::size_t(object.placement)*0x5c;
        require(object.valid_multiplayer_games==raw.u16(source+0x5a) &&
            object.lightmapping_policy==std::int16_t(raw.u16(source+0x4e)),
            "authored scenery placement policy changed");
        const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),
            [&](const auto& value){return value.datum==object.definition;});
        require(tag!=cache.tags().end() && tag->group=="scen","original scenery palette class changed");
        const auto tag_root=std::size_t(std::int64_t(tag->address)-magic);
        require(definition->object_flags==raw.u16(tag_root+2) &&
            definition->lightmapping_policy==std::int16_t(raw.u16(tag_root+0xc0)) &&
            definition->model==raw.u32(tag_root+0x38),"authored scenery definition policy changed");
        if(definition->lightmapping_policy==2)++result.policy_dynamic;
        if(!scenery_automatic_creation_allowed(object,2)) {++result.excluded;continue;}
        if(scenery_lightprobe(probes,object,*definition))++result.probes;
        const auto key=std::pair{object.definition,object.variant};
        auto found=models.find(key);
        if(found==models.end())found=models.emplace(key,
            cache.load_model(object.definition,false,object.variant)).first;
        const auto selection=scenery_lighting_implementation(lighting,found->second,object,*definition);
        if(selection==SceneryLightingImplementation::per_vertex)++result.baked;
        else if(selection==SceneryLightingImplementation::dynamic)++result.dynamic;
        else ++result.unsupported;
    }
    require(result.placements==raw.u32(root+0x50),"scenery placement inventory incomplete");
    require(!result.unsupported,"original multiplayer scenery lighting bank unresolved");
    std::cout<<cache.name()<<": "<<result.baked<<" baked + "<<result.dynamic
        <<" dynamic Slayer scenery ("<<result.probes<<" source lightprobes), "
        <<result.excluded<<" excluded by authored creation gates\n";
    return result;
}
} // namespace

int main(int argc,char** argv) {
    try {
        using namespace halo2;
        if(argc==2 && std::string_view(argv[1])=="--gate") {
            int type,engine;unsigned flags,games;
            while(std::cin>>type>>flags>>games>>engine) {
                ScenarioObject object;object.type=type;object.flags=flags;
                object.valid_multiplayer_games=std::uint16_t(games);object.definition=1;
                std::cout<<scenery_multiplayer_game_allowed(object,engine)<<' '
                    <<scenery_automatic_creation_allowed(object,engine)<<'\n';
            }
            return 0;
        }
        if(argc==2 && std::string_view(argv[1])=="--probe") {
            unsigned id,flags,count;int origin,type,source;
            while(std::cin>>id>>origin>>type>>source>>flags>>count) {
                ScenarioObject object;object.unique_id=id;object.origin_bsp=origin;object.type=type;object.source=source;
                SceneryDefinition definition;definition.object_flags=std::uint16_t(flags);
                std::vector<SceneryLightprobe> probes(count);
                for(auto& probe:probes) {
                    int o,t,s;std::cin>>probe.unique_id>>o>>t>>s;probe.origin_bsp=o;probe.type=t;probe.source=s;
                }
                const auto found=scenery_lightprobe(probes,object,definition);
                std::cout<<(found?std::ptrdiff_t(found-probes.data()):-1)<<'\n';
            }
            return 0;
        }
        if(argc==2 && std::string_view(argv[1])=="--selector") {
            unsigned match,bitmap,policy,record,resident;
            while(std::cin>>match>>bitmap>>policy>>record>>resident) {
                BspLighting bank;bank.structure_matches=match;bank.bitmap_datum=bitmap?0:0xffffffffu;
                RenderModel model;model.checksum=1;
                ScenarioObject object;object.type=6;object.source=1;object.unique_id=1;
                SceneryDefinition definition;definition.lightmapping_policy=std::int16_t(policy);
                if(record) {
                    BspLightingScenery data;data.unique_id=1;data.type=6;data.source=1;
                    data.checksum=1;data.reference.bucket=0;bank.scenery.push_back(data);
                    bank.buckets.resize(1);
                    if(resident){bank.buckets[0].incident.push_back(0);bank.buckets[0].radiance.push_back({0,0,0});}
                }
                std::cout<<int(scenery_lighting_implementation(bank,model,object,definition))<<'\n';
            }
            return 0;
        }
        require(argc==2 || argc==3,"expected original maps directory and optional map-pack directory");
        Counts total;
        const auto add=[&](const std::filesystem::path& path) {
            const auto count=verify(path);total.placements+=count.placements;total.baked+=count.baked;
            total.dynamic+=count.dynamic;total.excluded+=count.excluded;total.policy_dynamic+=count.policy_dynamic;
            total.probes+=count.probes;
        };
        constexpr const char* retail[]{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar"};
        for(const auto name:retail)add(std::filesystem::path(argv[1])/(std::string(name)+".map"));
        const auto pack=argc==3?std::filesystem::path(argv[2]):
            std::filesystem::path(argv[1]).parent_path().parent_path()/"map-pack/maps";
        unsigned maps=12;
        if(argc==3 || std::filesystem::exists(pack)) {
            constexpr const char* extra[]{"backwash","containment","deltatap","dune","elongation","gemini","triplicate","turf","warlock"};
            for(const auto name:extra)add(pack/(std::string(name)+".map"));
            maps+=std::size(extra);
            require(total.placements==410 && total.baked==200 && total.dynamic==78 &&
                total.excluded==132 && total.policy_dynamic==58,"all-map original scenery policy inventory changed");
            require(total.probes==12,"authored scenery massive sample identity inventory changed");
        }
        std::cout<<total.placements<<" authored scenery placements verified across "<<maps<<" multiplayer maps\n";
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';return 1;
    }
}

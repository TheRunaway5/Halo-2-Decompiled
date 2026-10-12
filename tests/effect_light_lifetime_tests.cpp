#include "halo2/effect_light_lifetime.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>

using namespace halo2;
int main(int argc,char** argv) {try {
    if(argc!=3)throw FormatError("Supply original Lockout map and light lifetime fixture");
    std::ifstream file(argv[2],std::ios::binary);
    const auto read=[&](void* p,std::size_t n) {file.read(static_cast<char*>(p),n);if(!file)throw FormatError("Truncated light lifetime fixture");};
    const auto word=[&]() {std::uint32_t v;read(&v,4);return v;};
    const auto scalar=[&]() {return std::bit_cast<float>(word());};
    const auto vector=[&]() {return Vec3{scalar(),scalar(),scalar()};};
    if(word()!=0x32464c4c)throw FormatError("Invalid original light lifetime fixture");
    const auto cases=word(),clusters=word();const auto selected=std::int16_t(word()),rate=std::int16_t(word());
    const auto definition_count=word();std::array<std::uint8_t,1024> curve;read(curve.data(),curve.size());
    std::map<std::uint32_t,EffectLightDefinition> definitions;
    for(unsigned i=0;i<definition_count;++i) {const auto id=word();std::array<std::byte,228> raw;read(raw.data(),raw.size());definitions.emplace(id,source_effect_light_definition(raw));}
    Cache cache(argv[1]);const auto locations=cache.load_bsp_atmosphere(0);
    const auto memberships=cache.load_object_light_spatial_bsp(0);const auto terrain=cache.load_raw_terrain_collision(0);
    if(cases!=5 || memberships.clusters.size()!=clusters || terrain.bsps.size()!=1)
        throw FormatError("Original light lifetime map/coverage mismatch");
    std::uint64_t compared=0;unsigned checkpoints=0,births=0,failed_births=0,failures=0;
    for(unsigned n=0;n<cases;++n) {
        const bool debug=word();const auto generation=std::uint16_t(word());
        for(unsigned i=0;i<definition_count;++i) {const auto id=word();std::array<std::byte,228> raw;read(raw.data(),raw.size());definitions.at(id)=source_effect_light_definition(raw);}
        auto pool=std::make_unique<SourceEffectLightDatumPool>();read(pool->records.data(),pool->records.size()*sizeof(pool->records[0]));
        auto spatial=std::make_unique<SourceEffectLightSpatialPool>(clusters);pool->activate(debug);pool->next_generation=generation;spatial->activate(debug);
        const auto operations=word();std::vector<std::uint32_t> expected;
        for(unsigned step=0;step<operations;++step) {
            const auto kind=word(),tick=word(),epoch=word(),enabled=word(),id=word(),tag=word(),result=word();
            const auto control=scalar();const auto position=vector(),forward=vector(),up=vector();
            const auto size=word(),deltas=word();if(size>65536 || deltas>size)throw FormatError("Excessive original lifetime state");
            expected.resize(size);
            for(unsigned i=0;i<deltas;++i) {const auto at=word(),value=word();if(at>=size)throw FormatError("Original lifetime delta outside state");expected[at]=value;}
            const SourceEffectLightLifetimeWorld world{locations,memberships,terrain.bsps[0],curve,tick,epoch,rate,selected,bool(enabled)};
            std::uint32_t returned=0xffffffffu;
            if(kind==1) {returned=source_standalone_effect_light_create(*pool,*spatial,definitions.at(tag),tag,control,position,forward,up,world);++births;failed_births+=returned==0xffffffffu;}
            else if(kind==2) {pool->record(id)[0xcc/4]=std::bit_cast<std::uint32_t>(control);source_standalone_effect_light_update(*pool,*spatial,definitions.at(pool->record(id)[1]),id,world);}
            else if(kind==3)source_standalone_effect_light_delete(*pool,*spatial,id);
            else if(kind==4)source_standalone_effect_lights_advance(*pool,*spatial,[&](auto definition)->const EffectLightDefinition& {return definitions.at(definition);},world);
            else if(kind!=0)throw FormatError("Unknown original light lifetime operation");
            const auto exact=[&](std::uint32_t got,std::uint32_t original,std::size_t at) {
                ++compared;if(got!=original) {if(failures<24)std::cerr<<"case="<<n<<" step="<<step<<" kind="<<kind<<" word="<<at<<" native="<<std::hex<<got<<" original="<<original<<std::dec<<'\n';++failures;}
            };
            exact(returned,result,0xffffffffu);
            std::vector<std::uint32_t> actual{pool->cursor,pool->highwater,pool->active_count,pool->next_generation};
            actual.insert(actual.end(),pool->occupied.begin(),pool->occupied.end());
            for(const auto& r:pool->records)actual.insert(actual.end(),r.begin(),r.end());
            actual.insert(actual.end(),spatial->heads.begin(),spatial->heads.end());
            for(const auto* p:{&spatial->entries,&spatial->references}) {
                for(auto v:std::array{p->cursor,p->highwater,p->active_count,std::uint32_t(p->next_generation)})actual.push_back(v);
                actual.insert(actual.end(),p->occupied.begin(),p->occupied.end());
                for(const auto& r:p->records)actual.insert(actual.end(),r.begin(),r.begin()+3);
            }
            if(actual.size()!=expected.size())throw FormatError("Original light lifetime state extent mismatch");
            for(unsigned i=0;i<actual.size();++i)exact(actual[i],expected[i],i);
            ++checkpoints;
        }
    }
    if(checkpoints<700 || births<400 || failed_births<2 || file.peek()!=std::char_traits<char>::eof())throw FormatError("Original light lifetime coverage incomplete");
    if(failures)throw FormatError("Original light lifetime differs in "+std::to_string(failures)+" words");
    std::cout<<"PASS "<<checkpoints<<" whole original lifetime checkpoints, "<<births<<" birth attempts, "<<failed_births<<" full-pool rejections, "<<compared<<" exact complete record/pool words\n";
    return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}}

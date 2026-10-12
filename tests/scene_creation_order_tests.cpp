#include "halo2/object_initialization.hpp"
#include "halo2/scene.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
struct Fixture {
    int phase{},bsp{};
    std::vector<ScenarioObject> objects;
    std::vector<SourceScenarioObjectIdentity> existing;
    std::vector<SourceScenarioCreationKey> keys;
};
Fixture parse(std::string_view line) {
    std::istringstream input{std::string(line)};Fixture out;unsigned count;
    input>>out.phase>>out.bsp>>count;require(count==32,"source mixed object bank size differs");
    for(unsigned i=0;i<count;++i) {
        ScenarioObject object;unsigned manual,damageable;
        input>>object.type>>object.placement>>object.name>>object.flags>>object.source
             >>object.origin_bsp>>object.bsp_policy>>manual>>damageable
             >>object.definition>>object.unique_id;
        object.manual_bsps=std::uint16_t(manual);object.damageable=damageable!=0;
        out.objects.push_back(object);
    }
    input>>count;require(count<=32,"source mixed registry fixture is excessive");
    for(unsigned i=0;i<count;++i) {
        SourceScenarioObjectIdentity object;
        input>>object.type>>object.placement>>object.source>>object.unique_id>>object.origin_bsp;
        out.existing.push_back(object);
    }
    input>>count;require(count<=32,"source mixed attempt fixture is excessive");
    for(unsigned i=0;i<count;++i) {
        SourceScenarioCreationKey key;input>>key.type>>key.placement;out.keys.push_back(key);
    }
    require(bool(input),"short original mixed creation fixture");
    std::string extra;require(!(input>>extra),"extra original mixed creation fixture field");
    return out;
}
SourceScenarioCreationKey key(const ScenarioObject& object) {
    return {object.type,object.placement};
}
LevelContent content(const Fixture& f) {
    LevelContent result;result.objects=f.objects;
    for(const auto& object:f.objects)if(object.name>=0 &&
        std::size_t(object.name)>=result.object_names.size())
        result.object_names.resize(std::size_t(object.name)+1);
    for(const auto& object:f.objects)if(object.name>=0)
        result.object_names[object.name]={"literal source fixture",object.type,object.placement};
    return result;
}
void native_initial(const Fixture& f) {
    require(f.phase==0 && f.keys.size()==25,"source initial mixed attempt coverage differs");
    std::vector<SourceScenarioCreationKey> old_content_order;
    for(const auto& object:f.objects)if(std::find(f.keys.begin(),f.keys.end(),key(object))!=f.keys.end()) {
        // These independently executed source attempts all pass the current
        // native final NONE/flag64 gate. No original factory success is asserted.
        require(object.definition!=null_datum && !(object.flags&64),
                "source mixed initial fixture requires a separate native admission control");
        old_content_order.push_back(key(object));
    }
    require(old_content_order!=f.keys,"old content-order negative control did not differ");
    const auto bank=content(f);SceneObjects scene(bank,std::size_t(f.bsp));
    require(scene.objects().size()==f.keys.size(),"native initial allocation count differs from admitted attempts");
    unsigned deleted=0;
    for(std::size_t i=0;i<f.keys.size();++i) {
        const auto& object=scene.objects()[i];
        require(key(bank.objects.at(object.placement))==f.keys[i],
                "native initial pool allocation order differs from literal source attempts");
        require(object.created_tick==0,"native cold lifetime tick differs");
        if(object.handle==null_datum)++deleted;
        else require(scene.get(object.handle).placement==object.placement,
                     "native live full-generation identity does not resolve its source placement");
    }
    // BSP0 legitimately removes three initial unnamed manual-policy SCEN.
    // Their native slots retain placement records; no later constructor reuses
    // them in this literal fixture. This checks allocation order through adoption.
    require(deleted==3,"source manual-BSP native lifetime control differs");
}
void native_missing_bsp(const Fixture& f) {
    require(f.phase==1 && f.existing.empty(),"expected empty original BSP registry fixture");
    std::vector<SourceScenarioCreationKey> old_content_order;
    for(const auto& object:f.objects)if(std::find(f.keys.begin(),f.keys.end(),key(object))!=f.keys.end())
        old_content_order.push_back(key(object));
    require(old_content_order!=f.keys,"old BSP content-order negative control did not differ");
    const auto bank=content(f);SceneObjects scene(bank,0);
    scene.destroy_all();scene.switch_bsp(std::size_t(f.bsp));
    std::vector<SourceScenarioCreationKey> actual;
    for(const auto& object:scene.objects())if(object.handle!=null_datum) {
        actual.push_back(key(bank.objects.at(object.placement)));
        require(scene.get(object.handle).placement==object.placement,
                "native recreated generation does not resolve source placement");
    }
    require(actual==f.keys,"native missing-BSP creation order differs from literal source attempts");
}
}
int main(int argc,char** argv) {try {
    require(argc==2,"scene creation order test requires its original fixture path");
    std::ifstream input(argv[1]);require(bool(input),"missing original scene creation order fixture");
    std::vector<Fixture> fixtures;std::string line;
    while(std::getline(input,line)) {
        if(line.empty() || line.front()=='#')continue;
        auto f=parse(line);
        const auto actual=f.phase==0?source_scenario_initial_creation_attempts(f.objects,false,f.existing):
            source_scenario_bsp_creation_attempts(f.objects,f.bsp,f.existing);
        require(actual==f.keys,"source mixed construction predicate/order differs from original attempted keys");
        fixtures.push_back(std::move(f));
    }
    require(input.eof() && fixtures.size()==13,"incomplete original mixed scene fixture bank");
    native_initial(fixtures.front());
    unsigned bsp_controls=0;
    for(const auto& f:fixtures)if(f.phase==1 && f.existing.empty()) {
        native_missing_bsp(f);++bsp_controls;
    }
    require(bsp_controls==3,"original mixed BSP context coverage differs");
    std::cout<<"13 original mixed attempt fixtures passed; native25 initial allocations and3 missing-BSP orders matched; old-order controls differ\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

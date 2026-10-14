#include "halo2/content.hpp"
#include "halo2/prop_sandbox.hpp"
#include <iostream>
#include <map>
#include <set>

// Actual campaign placements must share immutable tag definitions while
// retaining independent lifetime, transform and vitality owners.
int main(int argc,char **argv)try {
    using namespace halo2;
    if(argc!=3)throw FormatError("Supply MAP SHARED");
    Cache cache(argv[1],argv[2]);const auto content=cache.load_content(0);
    using Key=std::pair<std::uint32_t,std::uint32_t>;
    std::map<Key,const PropSandboxDefinition*> props;
    std::map<Key,const StaticSceneryCollisionDefinition*> scenery;
    unsigned repeated_props{},repeated_scenery{};
    for(unsigned i=0;i<content.objects.size();++i) {
        const auto &object=content.objects[i];const Key key{object.definition,object.variant};
        if(const auto &definition=content.prop_sandbox[i]) {
            const auto [at,inserted]=props.emplace(key,definition.get());
            if(!inserted) {
                if(at->second!=definition.get())throw FormatError("Repeated campaign prop retains duplicated immutable tag data");
                ++repeated_props;
            }
        }
        if(const auto &definition=content.static_scenery_collision[i]) {
            const auto [at,inserted]=scenery.emplace(key,definition.get());
            if(!inserted) {
                if(at->second!=definition.get())throw FormatError("Repeated campaign scenery retains duplicated immutable tag data");
                ++repeated_scenery;
            }
        }
    }
    if(!repeated_props || !repeated_scenery)throw FormatError("Campaign fixture needs repeated prop and scenery definitions");
    SceneObjects scene(content,0);PropSandbox sandbox(content);sandbox.synchronize(scene);
    std::set<std::uint32_t> handles;
    for(const auto &state:sandbox.states()) {
        if(!handles.insert(state.handle).second)throw FormatError("Campaign placements lost their independent object generations");
        const auto &object=scene.get(state.handle);
        if(object.placement!=state.placement)throw FormatError("Campaign prop state lost its placement owner");
    }
    std::cout<<"PASS campaign definitions: unique_props="<<props.size()<<" repeated_props="<<repeated_props
        <<" unique_scenery="<<scenery.size()<<" repeated_scenery="<<repeated_scenery
        <<" independent_prop_owners="<<handles.size()<<'\n';return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

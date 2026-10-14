#include "halo2/source_map_effects.hpp"
#include "halo2/scene.hpp"
#include "halo2/model.hpp"
#include "halo2/math.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/original_data.hpp"
#include <iostream>
using namespace halo2;
int main(int argc,char **argv) {try {
    if(argc<4)throw FormatError("Supply DISC_MAPS DLC_MAPS UPDATE_XBE [MAP]");
    auto original=OriginalData::load_xbe(argv[3]);unsigned maps=0,prepared=0,spawned=0;std::size_t particle_peaks=0;
    const auto audit=[&](const std::filesystem::path &path) {
        Cache cache(path,argv[1],original);const auto content=cache.load_content(0,true);
        CollisionWorld collision(content.collision);Mesh mesh;
        NativeWorldEffects effects(cache,mesh,collision,0x12345678);
        SourceMapEffects attachments(cache,content.objects,effects);
        SceneObjects scene(content,0,SourceSceneGameContext{1,2});
        std::vector<std::vector<Transform>> banks;std::vector<std::vector<std::uint8_t>> permutations;
        struct Input {SourceSceneEffectOwner owner;std::size_t bank{};};std::vector<Input> inputs;
        for(const auto &object:scene.objects())if(object.handle!=null_datum) {
            const auto &placed=content.objects.at(object.placement);
            if(placed.type!=6 && placed.type!=7 && placed.type!=11)continue;
            const auto def=cache.load_initial_model_definition(placed.definition);if(def.render_model==null_datum)continue;
            const auto model=cache.load_model(def.render_model,false);
            const auto basis=authored_basis(placed.rotation);
            banks.push_back(pose_model_nodes_world(model,source_object_transform(object.position,basis.forward,basis.up,object.scale,false)));
            const auto selection=cache.load_initial_model_selection(placed.definition,placed.variant);
            if(!selection.resolved()){banks.pop_back();continue;}
            permutations.push_back(selection.permutations);
            inputs.push_back({{object.placement,object.handle,object.created_tick,{},{}},banks.size()-1});
        }
        std::vector<SourceSceneEffectOwner> owners;
        for(auto &in:inputs){in.owner.nodes=banks[in.bank];in.owner.permutations=permutations[in.bank];owners.push_back(in.owner);}
        std::size_t peak{};MuzzleEffectView view{};view.width=640;view.height=480;view.forward={1,0,0};view.up={0,0,1};view.orientation={1,0,0,0,1,0,0,0,1};
        for(unsigned tick=0;tick<180;++tick) {
            attachments.synchronize(owners,tick);const auto births=attachments.stats().spawned;
            attachments.synchronize(owners,tick);
            if(attachments.stats().spawned!=births)throw FormatError("Repeated ownership replayed attachment births");
            effects.advance(tick,1.f/30.f);peak=std::max(peak,effects.diagnostics().particles);
            const auto first=effects.draw(view,0,1.f/30.f);const auto second=effects.draw(view,0,1.f/30.f);
            if(first.size()!=second.size())throw FormatError("Second view changed map effect simulation");
            for(unsigned n=0;n<first.size();++n)if(first[n].vertex.attributes!=second[n].vertex.attributes)
                throw FormatError("Second view changed retained map particles");
            (void)effects.take_sounds();
        }
        const auto state=attachments.stats();
        if(path.stem()=="foundation" && (state.attachments!=4 || state.prepared!=4 || state.spawned!=4 || peak==0))
            throw FormatError("Foundation actual four HVAC attachments did not produce particles");
        for(const auto &gap:state.unavailable)std::cout<<"BOUNDARY "<<path.stem().string()<<' '<<gap<<'\n';
        //Complete lifetime removal must retire attached particles immediately.
        attachments.synchronize({},180);
        if(effects.diagnostics().effects || effects.diagnostics().particles)
            throw FormatError("Removed map generations retained their attached particles");
        for(auto &owner:owners){owner.object+=0x10000u;owner.created_tick=180;}
        attachments.synchronize(owners,180);effects.advance(180,1.f/30.f);
        if(attachments.stats().spawned!=state.spawned*2)throw FormatError("New map generations lost or duplicated their births");
        prepared+=state.prepared;spawned+=state.spawned;particle_peaks+=peak;++maps;
        std::cout<<"MAP "<<path.stem().string()<<" authored="<<state.attachments<<" prepared="<<state.prepared<<" spawned="<<state.spawned<<" peak_particles="<<peak<<'\n';
    };
    if(argc==5){const auto name=std::string(argv[4]);auto path=std::filesystem::path(argv[1])/(name+".map");if(!std::filesystem::exists(path))path=std::filesystem::path(argv[2])/(name+".map");audit(path);}
    else {
        for(auto name:{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar"})audit(std::filesystem::path(argv[1])/(std::string(name)+".map"));
        for(auto name:{"containment","warlock","turf","deltatap","triplicate","gemini","elongation","dune","backwash"})audit(std::filesystem::path(argv[2])/(std::string(name)+".map"));
    }
    std::cout<<"PASS maps="<<maps<<" prepared="<<prepared<<" spawned="<<spawned<<" summed_particle_peaks="<<particle_peaks<<"; explicit bind WORLD inputs, full native lifetimes, per-view invariance and recreation\n";
    std::cout<<"LIMIT declared bind WORLD inputs do not establish current original animation, physics, spatial scheduling, named functions or all network history\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

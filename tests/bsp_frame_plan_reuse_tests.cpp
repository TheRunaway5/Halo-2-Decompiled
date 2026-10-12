#include "halo2/source_bsp_resident_plan.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>

using namespace halo2;
namespace {
SourceBspDrawKey key(const Vertex &v) {
    return {v.material,v.lightmap,v.bsp_cluster,v.source_section,v.source_part};
}
void require(bool condition,const char *message) {
    if(!condition)throw FormatError(message);
}
// Independent original encounter-order scatter oracle. It does not call merge
// or reuse its run catalogue, count computation, or storage selection.
void compare(const SourceBspResidentPlan &resident,SourceBspMergedDrawPlan &retained,
    std::span<const SourceBspPartBinding> bindings,std::span<const Vertex> objects,
    unsigned materials,unsigned lightmaps,unsigned dynamic_first,unsigned logical_first,bool borrow_objects) {
    auto original=resident.expand(bindings);
    original.insert(original.end(),objects.begin(),objects.end());
    std::map<SourceBspDrawKey,std::vector<Vertex>> grouped;
    for(const auto &vertex:original)grouped[key(vertex)].push_back(vertex);
    resident.merge_into(retained,bindings,objects,materials,lightmaps,dynamic_first,logical_first,borrow_objects);
    require(retained.draws.size()==grouped.size() && retained.descriptors.size()==grouped.size(),
        "retained BSP plan changed ordered draw/descriptor counts");
    if(borrow_objects && !objects.empty())require(retained.borrowed_object_vertices>0,
        "borrowed BSP fixture did not exercise direct object ranges");
    if(borrow_objects && objects.size()>9)require(!retained.borrowed_runs.empty(),
        "borrowed BSP fixture did not exercise separated source ranges");
    require(retained.logical_vertices==original.size(),"retained BSP plan changed logical vertex count");
    unsigned index{},logical=logical_first;
    for(const auto &[expected_key,expected]:grouped) {
        const auto &draw=retained.draws[index];const auto &descriptor=retained.descriptors[index++];
        require(draw.key==expected_key && draw.count==expected.size() && draw.logical_first==logical,
            "retained BSP plan changed ordered key/count/logical offset");
        require(descriptor.material==expected_key.material && descriptor.lightmap==expected_key.lightmap,
            "retained BSP plan changed ordered descriptor request");
        logical+=draw.count;
        std::vector<Vertex> gathered;std::span<const Vertex> bytes;
        if(draw.storage==SourceBspVertexStorage::resident)
            bytes=std::span{resident.resident_vertices()}.subspan(draw.first,draw.count);
        else if(draw.storage==SourceBspVertexStorage::objects)bytes=objects.subspan(draw.first,draw.count);
        else if(draw.storage==SourceBspVertexStorage::object_runs) {
            const auto runs=std::span{retained.borrowed_runs}.subspan(draw.first,draw.run_count);
            for(const auto &run:runs) {
                const auto part=objects.subspan(run.first,run.count);
                gathered.insert(gathered.end(),part.begin(),part.end());
            }
            require(gathered.size()==draw.count,"borrowed BSP run list changed vertex count");bytes=gathered;
        }
        else {
            require(draw.first>=dynamic_first,"retained BSP token precedes current dynamic prefix");
            bytes=std::span{retained.dynamic_vertices}.subspan(draw.first-dynamic_first,draw.count);
        }
        for(unsigned i=0;i<bytes.size();++i) {
            auto actual=bytes[i];
            // Material is a draw binding, not an exposed GPU vertex attribute.
            if(draw.storage==SourceBspVertexStorage::resident)actual.material=draw.key.material;
            require(std::memcmp(&actual,&expected[i],sizeof(Vertex))==0,
                "retained BSP plan changed full vertex bytes or encounter order");
        }
    }
}
}
int main(int argc,char **argv) {
    try {
        if(argc!=3)throw FormatError("Supply MAP SHARED");
        if(!std::filesystem::exists(argv[1]))return 77;
        const Cache cache(argv[1],argv[2]);auto mesh=cache.load_bsp(0);
        const SourceBspResidentPlan resident(cache,mesh,0);
        require(!resident.parts().empty(),"BSP reuse fixture has no authored parts");
        const unsigned materials=unsigned(mesh.materials.size())+7,lightmaps=unsigned(mesh.lightmaps.size())+1;
        std::vector<SourceBspPartBinding> bindings(resident.parts().size());
        SourceBspMergedDrawPlan retained;
        for(unsigned frame=0;frame<64;++frame) {
            for(unsigned p=0;p<bindings.size();++p) {
                const auto &part=resident.parts()[p];
                bindings[p]={part.original.material,{}};
                if((frame+p)%7==0 || frame%16==15)bindings[p].material.reset();
                else {
                    if(frame%3==1)bindings[p].material=(part.original.material+frame)%materials;
                    if(frame%4==2)bindings[p].stipple=(p+frame)%32;
                }
            }
            const auto base=resident.expand(bindings);std::vector<Vertex> objects;
            if(frame%16!=15)for(unsigned i=0;i<std::min<std::size_t>(base.size(),333);++i) {
                auto v=base[i];
                // Include exact BSP-key collisions and separated ordinary
                // object runs; preserve unusual metadata in the full record.
                if(i%3){v.bsp_cluster=-1;v.source_section=0xffffffffu;v.source_part=0xffffffffu;}
                v.position.x+=float(frame)*.125f;objects.push_back(v);
            }
            if(!base.empty())for(unsigned i=0;i<9;++i) {
                auto v=base[i%base.size()];v.material=materials-1;v.lightmap=0;
                v.bsp_cluster=-1;v.source_section=123;v.source_part=456;objects.push_back(v);
            }
            if(frame%2)std::reverse(objects.begin(),objects.end());
            for(bool borrow:{false,true})compare(resident,retained,bindings,objects,materials,lightmaps,frame*113,frame*271,borrow);
        }
        auto bad=bindings;bad[0].material=materials;bool rejected{};
        try{resident.merge_into(retained,bad,{},materials,lightmaps);}catch(const FormatError&){rejected=true;}
        require(rejected,"retained BSP plan accepted invalid current material");
        for(unsigned p=0;p<bindings.size();++p)bindings[p]={resident.parts()[p].original.material,{}};
        compare(resident,retained,bindings,{},materials,lightmaps,91,117,true);
        std::cout<<"PASS 129 retained/borrowed BSP frames against original scatter oracle, including empty/repopulated outputs, changed bindings/stipple, object collisions and invalid input recovery\n";
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}

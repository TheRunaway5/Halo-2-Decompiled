#include "halo2/source_machine_scene.hpp"
#include "halo2/source_device.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include "halo2/scene.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using namespace halo2;
namespace {
void require(bool yes,const char *message){if(!yes)throw FormatError(message);}
struct Record {unsigned ordinal{},definition{},flags{};int power{},position{};std::array<unsigned,11> result{};};
void gemini_lifetimes(const Cache &cache,const LevelContent &content,SourceMachineScene &machines,SceneObjects &objects,Mesh &mesh) {
    const auto d=cache.load_source_portal_mask_definition(0);const auto packets=machines.portal_owners();
    std::vector<SourcePortalOwnerState> owners;for(const auto &p:packets)owners.push_back(p.owner);
    SourcePortalMaskState state;state.initialize(d,owners);
    for(auto w:state.words)require(w==0xffffffffu,"Gemini initial native mask skipped original cold state");
    unsigned assigned=0;for(const auto &o:owners)if(o.count)++assigned;
    require(assigned==6,"Gemini portal bridge lost one of six actual assigned owners");
    machines.update_pose(1,1,mesh);const auto next=machines.portal_owners();
    for(const auto &p:next){const auto o=std::find_if(owners.begin(),owners.end(),[&](const auto &o){return o.handle==p.owner.handle;});
        require(o!=owners.end(),"same machine tick changed lifetime");state.step(d,*o,p.door);}
    for(auto p:d.portal_indices)require(!(state.words[unsigned(p)/32]&(1u<<(unsigned(p)%32))),"explicit initialized Gemini door tick did not close assigned portal");
    const auto at=std::find_if(owners.begin(),owners.end(),[](const auto &o){return o.count!=0;});
    const auto packet=std::find_if(packets.begin(),packets.end(),[&](const auto &p){return p.owner.handle==at->handle;});
    objects.visible(at->handle,false);machines.synchronize(objects);
    const auto hidden=machines.portal_owners();require(hidden.size()==packets.size(),"hidden portal machine disappeared from source ownership iterator");
    state.destroy(d,*at);objects.destroy(at->handle);machines.synchronize(objects);
    require(machines.portal_owners().size()+1==packets.size(),"destroyed generation remained in portal snapshot");
    for(unsigned i=unsigned(at->first);i<unsigned(at->first)+unsigned(at->count);++i){const auto portal=unsigned(d.portal_indices[i]);
        require(state.words[portal/32]&(1u<<(portal%32)),"destroyed Gemini door failed source portal restore");}
    // Geometry/history owns named-group recreation admission; mask ownership
    // itself is never rebound on birth by this snapshot method.
    const auto name=content.objects.at(packet->source_object).name;
    if(name>=0){const auto born=objects.create(name,false,1);machines.synchronize(objects);
        const auto replacement=machines.portal_owners();const auto p=std::find_if(replacement.begin(),replacement.end(),[&](const auto &p){return p.owner.handle==born;});
        require(p!=replacement.end() && p->created_tick==1 && p->owner.first==-1 && p->owner.count==0,
            "recreated portal snapshot inherited old source range/birth");}
}
}
int main(){try{
    std::ifstream fixture("tests/fixtures/retail_source_portal_machine_initial.tsv");require(bool(fixture),"original portal machine initialization fixture missing");
    std::map<std::string,std::vector<Record>> maps;std::string line;
    while(std::getline(fixture,line)){if(line.empty() || line[0]=='#')continue;std::istringstream in(line);std::string map;Record r;
        in>>map>>r.ordinal>>std::hex>>r.definition>>r.flags>>std::dec>>r.power>>r.position>>std::hex;
        for(auto &w:r.result)in>>w;require(bool(in),"invalid original portal machine initializer record");maps[map].push_back(r);}
    unsigned records=0,packets=0,gemini=0;
    for(const auto &[map,rows]:maps){const auto path=std::filesystem::exists("assets/disc/maps/"+map+".map")?"assets/disc/maps/"+map+".map":"assets/map-pack/maps/"+map+".map";
        Cache cache(path);const auto content=cache.load_content(0,true);CollisionWorld collision(content.collision);Mesh mesh;
        SourceMachineScene machines(cache,content.objects,0,mesh,collision);SceneObjects objects(content,0);
        bool rejected=false;try{machines.portal_owners();}catch(const FormatError &){rejected=true;}
        require(rejected,"portal bridge accepted unbound initial machine generation");machines.synchronize(objects);
        const auto live=machines.portal_owners();packets+=live.size();
        for(const auto &r:rows){++records;const auto p=cache.load_source_machine_placement(r.ordinal);const auto def=cache.load_source_machine_definition(r.definition);
            require(p.device_flags==r.flags && p.power_group==r.power && p.position_group==r.position,"source portal machine placement metadata differs");
            require(source_machine_initial_device_flags(p.device_flags)==r.result[0],"source1076E0 runtime flags differ");
            require(std::bit_cast<unsigned>(p.initial_power)==r.result[1] && std::bit_cast<unsigned>(p.initial_position)==r.result[2],"source1076E0 current body channels differ");
            require(r.result[3] && r.result[7],"source private/named group was not successfully present");
            require(p.power_group_flags==r.result[4] && p.position_group_flags==r.result[8],"source private/named group flags differ");
            if(def.render_model==0xffffffffu)continue;
            const auto packet=std::find_if(live.begin(),live.end(),[&](const auto &packet){return content.objects.at(packet.source_object).placement==int(r.ordinal);});
            require(packet!=live.end(),"source rendered machine omitted actual portal owner packet");const auto &authored=content.objects.at(packet->source_object);
            require(objects.get(packet->owner.handle).placement==packet->source_object && packet->created_tick==0,"portal packet lost full lifetime/global content identity");
            require(packet->owner.identity.unique_id==authored.unique_id && packet->owner.identity.type==authored.type &&
                packet->owner.identity.source==authored.source && packet->owner.identity.origin_bsp==authored.origin_bsp,"portal packet substituted authored identity");
            require(packet->owner.first==-1 && packet->owner.count==0 && packet->owner.position_group==0xffffffffu,"portal snapshot invented binding or Xbox group datum");
            require(packet->door.group_present && packet->door.device_flags==r.result[0] &&
                std::bit_cast<unsigned>(packet->door.current_position)==r.result[2] && std::bit_cast<unsigned>(packet->door.group_position)==r.result[9],"portal bridge differs from source initialized device/group bank");
            require(packet->initialized_group_state_only==(def.type!=2),"portal bridge lost initialized-state boundary");
            if(map=="gemini" && packet->owner.definition.type==0)++gemini;
        }
        if(map=="gemini")gemini_lifetimes(cache,content,machines,objects,mesh);
    }
    require(records==49 && packets==48 && gemini==6,"source portal machine input inventory changed");
    std::cout<<"49 original initialization records,48 actual synchronized rendered owners,6 Gemini doors; hidden/destruction controls passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}

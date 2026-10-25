// Read actual ordinary-game state through the native authenticated LAN client.
// This observer joins one neutral peer. It never writes host memory or supplies
// an admission, pose, velocity, projectile, damage or rendering result.
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/collision.hpp"
#include "halo2/navigation.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <thread>
#include <cstdlib>
#include <queue>
#include <limits>
#include <cmath>
using namespace halo2;
using Json=nlohmann::json;
namespace {
Json vector(Vec3 p){return {p.x,p.y,p.z};}
void check(bool condition,const char* message){if(!condition)throw FormatError(message);}
void publish(const std::filesystem::path& path,const Json& value){
    const auto text=value.dump();check(text.size()<=512*1024,"LAN observer snapshot exceeds bounded budget");
    auto temporary=path;temporary+=".tmp";{std::ofstream out(temporary);out<<text<<'\n';}
    std::filesystem::rename(temporary,path);
}

// Input-automation route only. Sample actual static collision floors at every
// height, retain capsule clearance, and search adjacent walkable samples. This
// neither changes the host world nor purports to implement retail AI navigation.
std::vector<Vec3> collision_walk_route(const CollisionWorld& world,
        const BipedDefinition& player,Vec3 start,Vec3 goal) {
    constexpr float spacing=.4f,margin=24.f;
    const float x0=std::min(start.x,goal.x)-margin,y0=std::min(start.y,goal.y)-margin;
    const int nx=int(std::ceil((std::abs(start.x-goal.x)+2*margin)/spacing))+1;
    const int ny=int(std::ceil((std::abs(start.y-goal.y)+2*margin)/spacing))+1;
    if(nx<=0 || ny<=0 || std::size_t(nx)*ny>180000)return {};
    float low=std::min(start.z,goal.z)-4,high=std::max(start.z,goal.z)+4;
    for(const auto& t:world.triangles())for(const auto p:{t.a,t.b,t.c}) {
        low=std::min(low,p.z-1);high=std::max(high,p.z+1);
    }
    struct Node{Vec3 p;int x,y,parent=-1;float cost=std::numeric_limits<float>::infinity();};
    std::vector<Node> nodes;
    std::vector<std::optional<std::vector<int>>> cells(std::size_t(nx)*ny);
    auto cell=[&](int x,int y)->const std::vector<int>& {
        auto& found=cells[std::size_t(y)*nx+x];
        if(found)return *found;
        found.emplace();const float px=x0+x*spacing,py=y0+y*spacing;
        float top=high;
        for(unsigned layer=0;layer<48 && top>low;++layer) {
            const auto hit=world.raycast({px,py,top},{0,0,-1},top-low);
            if(!hit)break;
            top=hit->position.z-.015f;
            const Vec3 feet{px,py,hit->position.z+.02f};
            if(hit->normal.z<player.maximum_slope_cosine || hit->backface ||
                world.overlaps(feet,player.height,player.radius))continue;
            // A centre ray alone admits narrow trim and overhang edges that
            // cannot support a walking capsule. Require nearby floor support
            // around its footprint before treating this as a route node.
            bool support=true;
            for(unsigned sample=0;sample<8 && support;++sample) {
                const float angle=float(sample)*.7853981633974483f;
                const Vec3 offset{std::cos(angle)*player.radius*1.25f,
                                  std::sin(angle)*player.radius*1.25f,.35f};
                const auto around=world.raycast(feet+offset,{0,0,-1},.7f);
                support=around && !around->backface && around->normal.z>=player.maximum_slope_cosine;
            }
            if(!support)continue;
            found->push_back(int(nodes.size()));nodes.push_back({feet,x,y});
        }
        return *found;
    };
    auto closest=[&](Vec3 p) {
        int best=-1;float distance=2.f;
        const int cx=int(std::lround((p.x-x0)/spacing)),cy=int(std::lround((p.y-y0)/spacing));
        for(int y=std::max(0,cy-3);y<=std::min(ny-1,cy+3);++y)
            for(int x=std::max(0,cx-3);x<=std::min(nx-1,cx+3);++x)
                for(const auto index:cell(x,y)) {
                    const float d=length(nodes[index].p-p);
                    if(d<distance){best=index;distance=d;}
                }
        return best;
    };
    const int first=closest(start),last=closest(goal);
    if(first<0 || last<0)return {};
    using Pending=std::pair<float,int>;
    std::priority_queue<Pending,std::vector<Pending>,std::greater<Pending>> pending;
    nodes[first].cost=0;pending.push({length(nodes[first].p-nodes[last].p),first});
    std::set<int> closed;
    while(!pending.empty() && closed.size()<180000) {
        const int index=pending.top().second;pending.pop();
        if(!closed.insert(index).second)continue;
        if(index==last) {
            std::vector<Vec3> result;
            for(int at=last;at>=0;at=nodes[at].parent)result.push_back(nodes[at].p);
            std::reverse(result.begin(),result.end());result.push_back(goal);return result;
        }
        const auto current=nodes[index];
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx) {
            if(!dx && !dy)continue;
            const int x=current.x+dx,y=current.y+dy;
            if(x<0 || x>=nx || y<0 || y>=ny)continue;
            for(const int next:cell(x,y)) {
                if(closed.contains(next))continue;
                const auto point=nodes[next].p;
                if(std::abs(point.z-current.p.z)>.4f)continue;
                bool clear=true;
                for(int sample=1;sample<=3 && clear;++sample) {
                    auto p=current.p+(point-current.p)*(sample*.25f);
                    p.z=std::max(current.p.z,point.z)+.01f;
                    clear=!world.overlaps(p,player.height,player.radius);
                }
                if(!clear)continue;
                const float cost=current.cost+length(point-current.p);
                if(cost>=nodes[next].cost)continue;
                nodes[next].cost=cost;nodes[next].parent=index;
                pending.push({cost+length(point-nodes[last].p),next});
            }
        }
    }
    return {};
}
}
int main(int argc,char** argv)try {
    check(argc==6,"Supply MCC_MAP SHARED_MAPS HOST_ADDRESS OWNED_LATEST_JSON SECONDS");
    const std::filesystem::path map=argv[1],shared=argv[2],output=std::filesystem::weakly_canonical(std::filesystem::absolute(argv[4]));
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Use tools/run_test.py");
    const auto relative=output.lexically_relative(std::filesystem::canonical(scratch));
    check(!relative.empty() && *relative.begin()!="..","LAN observations must remain in owned scratch");
    const auto seconds=std::stoul(argv[5]);check(seconds>=1 && seconds<=600,"Declare a finite observation budget");
    const auto start=std::chrono::steady_clock::now(),deadline=start+std::chrono::seconds(seconds);
    std::optional<LanSessionDescription> description;
    while(std::chrono::steady_clock::now()<deadline && !description) {
        try{description=query_lan_session(argv[3],std::chrono::milliseconds(500));}
        catch(const std::exception&){std::this_thread::sleep_for(std::chrono::milliseconds(100));}
    }
    check(description.has_value(),"Ordinary game did not publish its actual LAN configuration");
    Cache cache(map,shared);check(cache.is_mcc(),"Ordinary game observation requires prepared MCC content");
    const auto settings=source_game_variant(description->variant);
    const auto fingerprint=custom_slayer_fingerprint(multiplayer_fingerprint(map,shared,0,
        std::uint32_t(source_game_variant_get(settings,89))),description->variant);
    check(fingerprint==description->fingerprint,"Actual game and independent LAN content identities differ");
    const auto content=cache.load_content(0,true,settings);
    const CollisionWorld terrain(content.collision);
    const NavigationWorld navigation(content.navigation);
    std::map<std::pair<std::uint32_t,unsigned>,std::vector<Vec3>> approach_routes;
    auto connection=std::make_unique<LanClient>(argv[3],fingerprint,1);std::array<Controls,1> controls{};
    unsigned snapshots{},startup_retries{};std::optional<unsigned> last_tick;
    while(std::chrono::steady_clock::now()<deadline) {
        auto& client=*connection;
        client.send(controls);client.receive();
        if(!client.error().empty()) {
            if(!snapshots && !client.ready() &&
                    client.snapshot_diagnostics().error==LanClient::ErrorCategory::admission_timeout) {
                // Discovery starts before cold shader compilation finishes.
                // Reattempt real admission within the same finite budget.
                ++startup_retries;
                std::cerr<<"Ordinary host startup: retrying native LAN admission "<<startup_retries<<'\n';
                connection=std::make_unique<LanClient>(argv[3],fingerprint,1);
                continue;
            }
            throw FormatError(client.error());
        }
        if(client.ready() && (!last_tick || *last_tick!=client.state().tick)) {
            const auto &state=client.state();last_tick=state.tick;++snapshots;
            Json row={{"ready",true},{"tick",state.tick},{"accepted_snapshots",snapshots},
                {"startup_admission_retries",startup_retries},
                {"neutral_observer_slots",std::vector<unsigned>(client.slots().begin(),client.slots().end())},{"players",Json::array()},
                {"vehicles",Json::array()},{"weapon_projectiles",state.weapon_projectiles.size()},
                {"source_body_poses_authoritative",state.source_body_poses_authoritative},
                {"runtime_XBE_argument",false},{"host_memory_written",false}};
            row["projectiles"]=Json::array();
            for(const auto &projectile:state.weapon_projectiles)
                row["projectiles"].push_back({{"datum",projectile.datum},{"definition",projectile.definition}});
            // A completed native contact can outlive a projectile that was
            // born and retired within one tick. Preserve that actual source
            // evidence separately from resident projectile snapshots.
            row["contact_projectiles"]=Json::array();
            for(const auto& contact:state.grenade_detonations)
                if(contact.projectile!=null_datum && contact.source_effect_instance!=null_datum &&
                    !contact.source_locations.empty())
                    row["contact_projectiles"].push_back({{"datum",contact.projectile},
                        {"definition",contact.definition},{"source_effect_instance",contact.source_effect_instance},
                        {"tick",contact.tick},{"unit",contact.unit},{"player",contact.player}});
            for(unsigned slot=0;slot<state.players.size();++slot) {
                const auto &p=state.players[slot];if(!p.connected)continue;
                row["players"].push_back({{"slot",slot},{"position",vector(p.position)},
                    {"yaw",p.yaw},{"pitch",p.pitch},{"dead",p.dead},{"unit",p.source_unit},
                    {"parent",p.source_parent},{"weapon",p.weapon},{"shot_tick",p.shot_tick},
                    {"vehicle_weapon_present",p.source_vehicle_hud.weapon.present},
                    {"vehicle_weapon",p.source_vehicle_hud.weapon.datum},
                    {"vehicle_weapon_definition",p.source_vehicle_hud.definition}});
                row["players"].back()["grounded"]=p.native_grounded;
                row["players"].back()["grenades"]=p.grenades;
                row["players"].back()["grenade_selected"]=p.grenade_current;
                row["players"].back()["source_action_word"]=p.source_action_word;
                row["players"].back()["action_prompt"]={{"action",p.action_prompt.target.action},
                    {"index",p.action_prompt.target.index},{"object",p.action_prompt.target.object},
                    {"definition",p.action_prompt.definition},{"message",p.action_prompt.message}};
                auto& projectiles=row["players"].back()["vehicle_projectile_definitions"];
                projectiles=Json::array();std::set<std::uint32_t> definitions;
                auto& triggers=row["players"].back()["vehicle_triggers"];triggers=Json::array();
                if(const auto weapon=content.weapons.find(p.source_vehicle_hud.definition);
                    p.source_vehicle_hud.weapon.present && weapon!=content.weapons.end()) {
                    if(weapon->second.projectile!=null_datum)definitions.insert(weapon->second.projectile);
                    for(const auto& barrel:weapon->second.barrel_views)
                        if(barrel && barrel->projectile!=null_datum)definitions.insert(barrel->projectile);
                    for(const auto& trigger:weapon->second.triggers) {
                        const auto index=trigger.primary_barrel;auto projectile=null_datum;
                        // Presentation barrel_views cover only specific
                        // charged weapons. Use the actual typed weap bank,
                        // as the native additional-trigger regression does.
                        const auto* definition=cache.query_tag({.handle=TagHandle{p.source_vehicle_hud.definition},
                            .group=tag_groups::weapon,.require_address=true,.minimum_size=0x2d8});
                        if(!definition)throw FormatError("Observer lost the seated typed weapon root");
                        const Bytes root(cache.read_tag_bytes(definition->address,0x2d8));
                        const auto count=root.u32(0x2d0),base=root.u32(0x2d4);
                        if(count>0x10000||std::uint64_t(base)+std::uint64_t(count)*0xecu>0x100000000ull)
                            throw FormatError("Observer weapon barrel bank wraps");
                        if(index>=0&&unsigned(index)<count) {
                            const Bytes barrel(cache.read_tag_bytes(base+unsigned(index)*0xecu,0xec));
                            projectile=barrel.u32(0x90);
                            if(projectile!=null_datum&&(barrel.u32(0x8c)!=tag_groups::projectile.value||
                                !cache.find_tag(TagHandle{projectile},tag_groups::projectile)))
                                throw FormatError("Observer weapon barrel lost its typed projectile owner");
                        }
                        triggers.push_back({{"input",trigger.input},{"primary_barrel",index},
                            {"secondary_barrel",trigger.secondary_barrel},{"behavior",trigger.behavior},
                            {"primary_projectile",projectile}});
                    }
                }
                for(const auto definition:definitions)projectiles.push_back(definition);
            }
            for(const auto &v:state.world_vehicles) {
                if(v.placement>=content.objects.size())continue;
                const auto &object=content.objects[v.placement];const auto *tag=cache.query_tag({.handle=TagHandle{object.definition}});
                auto template_index=std::size_t(v.placement);std::set<std::size_t> ancestors;
                while(content.objects[template_index].model_attachment) {
                    if(!ancestors.insert(template_index).second)throw FormatError("Observer found an authored vehicle attachment cycle");
                    template_index=content.objects.at(template_index).model_attachment->parent;
                    if(template_index>=content.objects.size())throw FormatError("Observer vehicle parent template is absent");
                }
                const auto netgame_row=content.objects[template_index].netgame_row;
                Json vehicle={{"handle",v.handle},{"placement",v.placement},{"definition",object.definition},
                    {"netgame_row",netgame_row},{"netgame_classification",netgame_row<0?-1:
                        content.netgame_equipment.at(unsigned(netgame_row)).classification},
                    {"name",tag?tag->name:std::string{}},{"position",vector(v.transform.position)},
                    {"forward",vector(v.transform.basis[0])},{"right",vector(v.transform.basis[1])},
                    {"bounding_radius",object.bounding_radius},{"player_radius",content.player.radius},
                    {"entries",Json::array()}};
                const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),
                    [&](const auto& p){return p.object==v.handle;});
                const auto definition=content.unit_interactions->units.find(object.definition);
                if(state.source_body_poses_authoritative && pose!=state.source_body_poses.end() &&
                        definition!=content.unit_interactions->units.end() && v.regions &&
                        v.regions->selection.resolved()) {
                    const auto &unit=definition->second;
                    for(unsigned seat=0;seat<unit.seats.size();++seat) {
                        const auto &s=unit.seats[seat];const auto marker=unit.markers.find(s.entry_marker);
                        if(marker==unit.markers.end())continue;
                        const auto candidates=pose_model_markers(marker->second,pose->nodes,
                            v.regions->selection.permutations,{},1);
                        if(candidates.empty())continue;
                        const auto position=candidates.front().world.position;
                        const auto approach=position+candidates.front().world.basis[0]*(s.radius*.5f);
                        const auto floor=terrain.raycast(approach+Vec3{0,0,2*content.player.height},
                            {0,0,-1},4*content.player.height);
                        if(!floor)continue;
                        const Vec3 feet{approach.x,approach.y,floor->position.z+content.player.radius};
                        vehicle["entries"].push_back({{"seat",seat},{"flags",s.flags},{"radius",s.radius},
                            {"marker",vector(position)},{"approach",vector(feet)},
                            {"authored_facing",s.facing},{"authored_aim",s.aim},{"maximum_velocity",s.maximum_velocity},
                            {"actual_WORLD_nodes",true},{"actual_region_choices",true}});
                        if(netgame_row>=0 && (content.netgame_equipment.at(unsigned(netgame_row)).classification==6 ||
                            content.netgame_equipment.at(unsigned(netgame_row)).classification==4) &&
                            state.players[0].connected && !state.players[0].dead && state.players[0].source_unit!=null_datum) {
                            const auto [at,inserted]=approach_routes.try_emplace({v.handle,seat});
                            if(inserted) {
                                at->second=navigation.route(state.players[0].position,feet,false);
                                if(at->second.empty())at->second=collision_walk_route(terrain,content.player,
                                    state.players[0].position,feet);
                            }
                            auto& route=vehicle["entries"].back()["walking_route"];route=Json::array();
                            for(const auto point:at->second)route.push_back(vector(point));
                            vehicle["entries"].back()["route_source"]="input automation over authored navigation or capsule-clear static collision floors; no retail AI claim";
                        }
                    }
                }
                row["vehicles"].push_back(std::move(vehicle));
            }
            publish(output,row);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    check(snapshots>0,"No real ordinary-game snapshots were accepted");
    std::cout<<"PASS ordinary-game LAN observation: "<<snapshots<<" accepted native snapshots\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

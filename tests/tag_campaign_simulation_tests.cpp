#include "halo2/simulation.hpp"
#include "halo2/original_data.hpp"
#include "halo2/residency.hpp"
#include "halo2/campaign_text.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

// Explicit, headless diagnostic target. It uses the production cache importer,
// campaign startup and Simulation::step. It does not replace unsupported script
// services, inject world state, initialize a renderer or open audio devices.
int main(int argc, char **argv) try {
    using namespace halo2;
    if (argc < 6 || argc > 10) throw FormatError("Supply MAP SHARED XBE TICKS OUTPUT_DIRECTORY [--require-completion] [--no-skip] [--route=VOLUME,...] [--route-after=DEVICE]");
    bool require_completion{}, allow_skip=true;
    std::vector<std::string> route_names;
    std::string route_after;
    for(int i=6;i<argc;++i) {
        const std::string_view option=argv[i];
        if(option=="--require-completion")require_completion=true;
        else if(option=="--no-skip")allow_skip=false;
        else if(option.starts_with("--route-after="))route_after=option.substr(14);
        else if(option.starts_with("--route=")) {
            auto names=option.substr(8);
            while(!names.empty()) {
                const auto comma=names.find(',');
                route_names.emplace_back(names.substr(0,comma));
                if(comma==std::string_view::npos)break;
                names.remove_prefix(comma+1);
            }
            if(route_names.empty())throw FormatError("Campaign route needs an authored volume name");
        }
        else throw FormatError("Unknown campaign simulation option");
    }
    const auto limit = std::stoul(argv[4]);
    if (!limit || limit > 100000) throw FormatError("Invalid campaign tick limit");
    const std::filesystem::path output = argv[5];
    std::filesystem::create_directories(output);
    Cache cache(argv[1], argv[2], OriginalData::load_xbe(argv[3]));
    auto content = cache.load_content(0);
    std::vector<unsigned> volume_route;
    for(const auto &name:route_names) {
        const auto at=std::find_if(content.trigger_volumes.begin(),content.trigger_volumes.end(),
            [&](const auto &v){return v.name==name;});
        if(at==content.trigger_volumes.end())throw FormatError("Unknown campaign route volume: "+name);
        volume_route.push_back(unsigned(at-content.trigger_volumes.begin()));
    }
    std::size_t volume_waypoint{};
    int route_device=-1;
    if(!route_after.empty()) {
        const auto at=std::find_if(content.object_names.begin(),content.object_names.end(),
            [&](const auto &n){return n.name==route_after;});
        if(at==content.object_names.end())throw FormatError("Unknown campaign route device: "+route_after);
        route_device=int(at-content.object_names.begin());
    }
    const auto start = std::find_if(cache.spawns().begin(), cache.spawns().end(),
                                  [](const auto &spawn) { return spawn.bsp == 0; });
    if (start == cache.spawns().end()) throw FormatError("No authored BSP zero player start");
    std::cout << content.describe() << std::endl;
    NativeResidency residency(cache);
    const auto text = load_campaign_text(cache, argv[1]);
    Simulation simulation(content, BspContent{std::move(content.collision),content.navigation}, 0, *start,
        [&](auto graph, auto name) { return cache.load_named_animation(graph, name); },
        [&](std::size_t bsp) { return cache.load_bsp_content(bsp); },
        [&](const Simulation::Prediction &request) {
            if (request.kind == Simulation::Prediction::Barrier) residency.barrier();
            else if (request.kind == Simulation::Prediction::Sound) residency.sound(request.datum);
            else if (request.kind == Simulation::Prediction::Model) residency.model(request.datum, request.section);
            else if (request.kind == Simulation::Prediction::Object) residency.object(request.datum);
            else residency.structure(request.datum, request.section, request.instances);
        });
    bool look_inverted{};
    simulation.controller_look_settings([&] { return look_inverted; },
                                       [&](bool value) { look_inverted = value; });
    simulation.training_definitions(text.training_events);
    simulation.cinematic_text_services([&](auto id) { return text.subtitles.contains(id); },
        [&](auto id) { const auto sound = cache.load_sound(id); return float(sound.samples) / sound.sample_rate; });
    simulation.set_difficulty(1);
    simulation.start_level();
    std::ofstream trace(output / "ticks.tsv");
    if(!trace)throw FormatError("Cannot write campaign trace");
    trace << "step\ttick\tbsp\tx\ty\tz\thealth\tshield\tactors\tshots\thits\tscripts\tfaults\tcheckpoint\trestores\tfinished\ttarget\ttarget_x\ttarget_y\ttarget_z\troute_points\tgoal_x\tgoal_y\tgoal_z\tforward\tright\tyaw\tpitch\tjump\tgrounded\tcontact_triangle\n";
    const auto initial = simulation.player().body.position;
    NavigationWorld navigation(content.navigation);
    std::vector<Vec3> ally_route;
    unsigned moved{}, maximum_actors{}, skip_inputs{};
    bool skip_held{};
    bool light_diagnostic{};
    unsigned steps{};
    for (unsigned step = 0; step < limit && !simulation.level_finished(); ++step) {
        Controls input;
        std::uint32_t selected_target=null_datum;Vec3 selected_position{},selected_goal{};
        const auto &player = simulation.player();
        input.yaw = player.yaw;
        input.pitch = simulation.player_pitch();
        // An ordinary player requests a cinematic skip after its checkpoint is
        // available. A release tick prevents repeated input across a restore.
        input.skip_cinematic = allow_skip && simulation.cinematic_skippable() && !skip_held;
        skip_held = input.skip_cinematic;
        skip_inputs += input.skip_cinematic;
        if (!simulation.presentation().cinematic) {
            const auto origin = player.body.position + Vec3{0, 0, player.definition.camera_height};
            const Unit *target{};
            for (const auto &actor : simulation.actors())
                if (actor.exists && actor.state != Unit::Dead && !simulation.allied(player.team, actor.team) &&
                    (!target || length(actor.body.position - origin) < length(target->body.position - origin)))
                    target = &actor;
            if (target) {
                selected_target=target->source_datum;selected_position=target->body.position;
                const auto delta = target->body.position + Vec3{0, 0, target->body.height * .55f} - origin;
                const auto distance = length(delta);
                const auto wall = simulation.collision().raycast(origin, normalized(delta), distance);
                input.yaw = std::atan2(delta.y, delta.x);
                input.pitch = std::clamp(std::atan2(delta.z, std::hypot(delta.x, delta.y)), -1.5f, 1.5f);
                input.fire = !wall || wall->distance >= distance - .15f;
                input.forward = distance > 5 ? .8f : 0;
                input.reload = player.weapon.loaded == 0 && player.weapon.reserve > 0;
                input.jump = step % 120 == 0;
            } else {
                // A player follows the nearest living ally when an encounter
                // is quiet. Looking and ordinary button taps also exercise the
                // Armory's authored controller-training gates.
                input.pitch = .5f * std::sin(float(step) / 30.f);
                input.look_stick = std::cos(float(step) / 30.f);
                input.accept = step % 30 == 0;
                input.pickup = step % 60 < 30;
                input.pickup_milliseconds = input.pickup ? 1000 : 0;
                input.flashlight = step % 90 == 0;
                for(const auto &actor:simulation.actors())
                    if(actor.exists && actor.state!=Unit::Dead && simulation.allied(player.team,actor.team) &&
                        (!target || length(actor.body.position-origin)<length(target->body.position-origin)))target=&actor;
                if(target) {
                    selected_target=target->source_datum;selected_position=target->body.position;
                    if(simulation.bsp()!=0)ally_route.clear();
                    else if(step%30==0)ally_route=navigation.route(player.body.position,target->body.position);
                    while(!ally_route.empty() && length(ally_route.front()-player.body.position)<.4f)
                        ally_route.erase(ally_route.begin());
                    const auto goal=ally_route.empty()?target->body.position:ally_route.front();
                    if(simulation.bsp()==0 && step%30==0)ally_route=navigation.route(player.body.position,goal);
                    while(!ally_route.empty() && std::hypot(ally_route.front().x-player.body.position.x,
                        ally_route.front().y-player.body.position.y)<.4f)ally_route.erase(ally_route.begin());
                    selected_goal=ally_route.empty()?goal:ally_route.front();
                    const auto delta=selected_goal-player.body.position;
                    input.yaw=std::atan2(delta.y,delta.x);
                    const bool nearby=std::hypot(delta.x,delta.y)<=1.5f;
                    input.forward=nearby?.4f:.8f;
                    input.right=nearby?.6f*std::sin(float(step)/30.f):0;
                }
                // The authored Armory lights are live named scene objects.
                // Sweep their real centers through normal look controls; a
                // timed pitch oscillation cannot satisfy horizontal targets.
                std::vector<std::uint32_t> lights;
                for(unsigned i=0;i<content.object_names.size();++i)
                    if(content.object_names[i].name.starts_with("looker_light_") &&
                        content.object_names[i].name.ends_with("_red")) {
                        const auto handle=simulation.scene().resolve(int(i));
                        if(handle!=null_datum)lights.push_back(handle);
                    }
                if(lights.size()>1)light_diagnostic=true;
                if(lights.empty())light_diagnostic=false;
                if(light_diagnostic && !lights.empty()) {
                    input.forward=0;
                    const auto delta=simulation.scene().center(lights[(step/30)%lights.size()])-origin;
                    input.yaw=std::atan2(delta.y,delta.x);
                    input.pitch=std::clamp(std::atan2(delta.z,std::hypot(delta.x,delta.y)),-1.5f,1.5f);
                } else if(!volume_route.empty() && (route_device<0 || [&] {
                    const auto owner=simulation.devices().owners().find(simulation.scene().resolve(route_device));
                    return owner!=simulation.devices().owners().end() && owner->second.motion.position>=.999f;
                }())) {
                    // An explicit player itinerary uses ordinary movement to
                    // reach authored volumes. It never writes mission state.
                    const auto &volume=content.trigger_volumes[volume_route[volume_waypoint]];
                    if(volume.object_name!=-1)throw FormatError("Attached route volume needs its posed node owner");
                    const auto goal=volume.position+volume.forward*(volume.extents.x*.5f)+
                        cross(volume.up,volume.forward)*(volume.extents.y*.5f)+volume.up*(volume.extents.z*.5f);
                    selected_goal=goal;
                    const auto delta=goal-player.body.position;
                    input.yaw=std::atan2(delta.y,delta.x);
                    input.forward=std::hypot(delta.x,delta.y)>.2f?.7f:0;
                    input.right=0;
                    const auto offset=player.definition.bounding_offset;
                    const auto center=player.body.position+direction(player.yaw)*offset.x+
                        Vec3{-std::sin(player.yaw),std::cos(player.yaw),0}*offset.y+Vec3{0,0,offset.z};
                    if(volume_waypoint+1<volume_route.size() && simulation.scene().contains(volume,center))++volume_waypoint;
                }
            }
        }
        simulation.step(input);
        ++steps;
        simulation.advance_audio(tick_seconds);
        simulation.take_sounds();
        const auto &p = simulation.player();
        if (!std::isfinite(p.body.position.x) || !std::isfinite(p.body.position.y) ||
            !std::isfinite(p.body.position.z)) throw FormatError("Nonfinite campaign player position");
        moved += length(p.body.position - initial) > .5f;
        maximum_actors = std::max(maximum_actors, unsigned(simulation.actors().size()));
        trace << step << '\t' << simulation.ticks() << '\t' << simulation.bsp() << '\t'
              << p.body.position.x << '\t' << p.body.position.y << '\t' << p.body.position.z << '\t'
              << p.vitality.body << '\t' << p.vitality.shield << '\t' << simulation.actors().size() << '\t'
              << simulation.shots() << '\t' << simulation.hits() << '\t' << simulation.scripts().running() << '\t'
              << simulation.scripts().diagnostics().size() << '\t' << simulation.has_checkpoint() << '\t'
              << simulation.checkpoint_restores() << '\t' << simulation.level_finished() << '\t'
              << selected_target << '\t' << selected_position.x << '\t' << selected_position.y << '\t' << selected_position.z << '\t'
              << ally_route.size() << '\t' << selected_goal.x << '\t' << selected_goal.y << '\t' << selected_goal.z << '\t'
              << input.forward << '\t' << input.right << '\t' << input.yaw << '\t' << input.pitch << '\t' << input.jump << '\t'
              << p.body.grounded << '\t';
        if(p.body.support_triangle)trace<<*p.body.support_triangle;else trace<<-1;
        trace<<'\n';
        if (!simulation.scripts().diagnostics().empty()) {
            for (const auto &fault : simulation.scripts().diagnostics()) std::cerr << "Script fault: " << fault << '\n';
            std::cerr << simulation.status() << '\n';
            throw FormatError("Authored campaign script failed");
        }
    }
    if(require_completion && !simulation.level_finished()) {
        const auto &player=simulation.player();
        std::cerr<<"Campaign route at limit: waypoint="<<volume_waypoint
            <<" navigation_sector="<<navigation.locate(player.body.position).value_or(std::size_t(-1))<<'\n';
        const auto middle=player.body.position+Vec3{0,0,player.body.height*.5f};
        if(const auto wall=simulation.collision().raycast(middle,direction(player.yaw),1.f)) {
            std::cerr<<"Campaign forward obstruction: triangle="<<wall->triangle
                <<" bsp="<<wall->source.bsp<<" record="<<wall->source.record
                <<" surface="<<wall->source.surface<<" instance="<<wall->source.instance
                <<" distance="<<wall->distance<<" normal="<<wall->normal.x<<','<<wall->normal.y<<','<<wall->normal.z<<'\n';
        }
        std::cerr<<"Campaign training at limit: enabled="<<simulation.training().enabled
                 <<" show="<<simulation.training().show<<" text="<<simulation.training().text
                 <<" suppressed="<<simulation.training().suppressed<<'\n';
        for(const auto &[handle,owner]:simulation.devices().owners()) {
            const auto name=content.objects[owner.placement].name;
            if(name>=0 && content.object_names[unsigned(name)].name.starts_with("looker_light_"))
                std::cerr<<content.object_names[unsigned(name)].name<<" handle="<<handle
                         <<" position="<<owner.motion.position<<" power="<<owner.motion.power<<'\n';
        }
        throw FormatError("Campaign step budget ended before authored level completion");
    }
    if(!trace)throw FormatError("Campaign trace write failed");
    std::cout << "PASS bounded campaign simulation: " << simulation.status()
              << " moved_ticks=" << moved << " peak_actors=" << maximum_actors
              << " skip_inputs=" << skip_inputs << " steps=" << steps
              << " level_finished=" << simulation.level_finished()
              << " machine_owners=" << simulation.devices().owners().size()
              << " resident_models=" << residency.model_count()
              << " resident_sounds=" << residency.sound_count() << std::endl;
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}

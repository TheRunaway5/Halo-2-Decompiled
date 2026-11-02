#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/source_campaign_model.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <set>
#if defined(HALO2_SCENERY_GPU)
#include "native_mcc_scenery_animation_gpu.hpp"
#endif

namespace {
using namespace halo2;
void require(bool value,const char* why){if(!value)throw FormatError(why);}
unsigned authored_motion_tick(const Cache& cache,unsigned graph,unsigned animation){
    const auto index=cache.find_local_animation_index(graph,animation);require(index.has_value(),"Scenery lifecycle clip name is absent");
    const auto clip=cache.load_animation(graph,*index);
    require(clip.nodes.size()==1&&clip.channels.size()==1,"Scenery lifecycle carrier fixture changed its authored hierarchy");
    unsigned result=clip.frame_count;
    for(unsigned channel=0;channel<3;++channel){
        const auto& track=clip.channels[0][channel];
        require(track.times.size()==track.keys.size(),"Scenery lifecycle key/time counts differ");
        if(track.keys.empty())continue;
        for(unsigned k=1;k<track.keys.size();++k)if(track.keys[k]!=track.keys.front()){
            require(track.times[k-1]>=60,"Scenery lifecycle fixture lacks its authored60-frame hold");
            // Translation/scale select a changed key without mistaking a
            // quaternion sign or interpolation roundoff for visible motion.
            if(channel!=0)result=std::min(result,track.times[k]);break;
        }
    }
    require(result>60&&result+10<clip.frame_count-2,"Scenery lifecycle fixture lacks a changed key and replay interval");
    std::cout<<"PHASE authored carrier hold=60 first changed translation/scale key="<<result<<'\n'<<std::flush;
    return result;
}
void hs_start(PhysicsLiveWorldTu15& world,unsigned object,unsigned graph,unsigned animation,unsigned named=null_datum){
    auto& memory=world.memory();
    const auto word=[&](unsigned at){return Bytes(memory.view(at,4)).u32(0);};
    const auto write=[&](unsigned at,unsigned value,unsigned width=4){
        std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8*n));
        world.write_source_call_input(at,std::span(bytes).first(width));
    };
    require(word(0x480c00+184*4)==0x457914&&word(0x457918)==0x2accf0,
        "World scenery descriptor differs from authenticated original184");
    constexpr unsigned fixture=0xb0000000,pool=fixture+0x2000,syntax=fixture+0x2100,
        thread=fixture+0x3000,frame=thread+0x40,nodes=fixture+0x4000,output=fixture+0x7000;
    memory.mount(fixture,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,
        "explicit scenery HS caller fixture; animation effect remains the actual native World owner");
    std::vector<std::pair<unsigned,std::vector<std::byte>>> saved;
    for(const auto [address,size]:{std::pair(0x505e5cu,4u),std::pair(0x505e60u,1u),
        std::pair(0x505e64u,4u),std::pair(0x505e6cu,4u)}){
        std::vector<std::byte> bytes(size);memory.read(address,bytes);saved.emplace_back(address,std::move(bytes));
    }
    const auto restore=[&]{for(const auto&[address,bytes]:saved)world.write_source_call_input(address,bytes);memory.release(fixture);};
    try{
        const auto node=[&](unsigned index,unsigned opcode,unsigned type,unsigned flags,unsigned next,unsigned value){
            const auto at=nodes+index*20;write(at,0x9000+index,2);write(at+2,opcode,2);write(at+4,type,2);
            write(at+6,flags,2);write(at+8,next);write(at+16,value);
        };
        write(0x505e5c,pool);write(0x505e60,1,1);write(0x505e6c,syntax);write(pool+0x44,thread);
        write(syntax+0x20,5);write(syntax+0x24,20);write(syntax+0x44,nodes);
        node(0,184,4,8,0xffffffff,0x90010001);node(1,2,2,9,0x90020002,0);
        if(named==null_datum)node(2,55,55,9,0x90030003,object);
        else node(2,61,55,9,0x90030003,0xffff0000u|named);
        node(3,36,36,9,0x90040004,graph);
        node(4,11,11,9,0xffffffff,animation);
        write(thread+2,1,1);write(thread+3,1,1);write(thread+0xc,0x100,2);write(thread+0x10,frame);
        write(frame,thread+0x18);write(frame+4,0x90000000);write(frame+0xc,0,2);
        write(thread+0x20,output);write(output,0xdeadbeef);
        auto abi=world.prepare_source_call();abi.gp[7]=0xa0010000;const auto before=abi;
        std::set<unsigned> stores;
        world.run_actual(0x211b10,abi,[&](const auto& store){stores.insert(store.instruction);});
        require(stores.contains(0x2acd2b)&&stores.contains(0x212096)&&stores.contains(0x212117)&&stores.contains(0x211e5e),
            "World scenery did not traverse its original wrapper/parser/completion stores");
        require(word(output)==0&&word(thread+0x10)==thread+0x18&&abi.gp[4]==before.gp[4]+4,
            "World scenery VOID completion/frame retirement/RET12 differs");
        for(unsigned reg:{3u,5u,6u,7u})require(abi.gp[reg]==before.gp[reg],"World scenery changed a nonvolatile register");
        require(word(frame+14)==object&&word(frame+18)==graph&&word(frame+22)==animation,
            "World scenery parser changed actual MCC object/graph/name arguments");
        require(word(world.source_object_body(object)+0x138u)==null_datum,
            "Three-argument native HS scenery start retained an animation anchor");
    }catch(...){restore();throw;}
    restore();
}
void named_lifecycle(PhysicsLiveWorldTu15& world,MultiplayerObjectRuntime& objects,const ScenarioObject& placement,
    unsigned object,unsigned name,unsigned graph,unsigned animation){
    const auto lookup=[&]{
        auto abi=world.prepare_source_call();abi.gp[0]=0xffff0000u|name;
        world.run_actual(0xbd790u,abi);return abi.gp[0];
    };
    const auto live=[&](unsigned datum){return std::ranges::any_of(objects.live_objects(),
        [&](const auto& row){return row.object==datum;});};
    require(lookup()==object&&live(object),"Native object name did not resolve its born salted lifetime");
    const auto remaining=source_campaign_cinematic_animation_time_tu15(world,object,false);
    const auto checkpoint=objects.authority_checkpoint();
    objects.destroy_object(object);
    require(lookup()==null_datum&&!live(object),"Native object retirement retained its name association");
    const auto recreated=objects.create_scenario_object(placement,SourceScenarioBirth::runtime);
    require(recreated!=null_datum&&recreated!=object&&live(recreated)&&lookup()==recreated,
        "Native object recreation did not bind the name to its new salted lifetime");
    hs_start(world,recreated,graph,animation,name);
    require(source_campaign_cinematic_animation_time_tu15(world,recreated,false)==remaining,
        "Named HS caller did not start the recreated object's authored animation");
    auto replacement=objects.prepare_authority(checkpoint);
    require(objects.commit_authority(std::move(replacement)),"Native named-object checkpoint publication failed");
    require(lookup()==object&&live(object)&&!live(recreated)&&
        source_campaign_cinematic_animation_time_tu15(world,object,false)==remaining,
        "Native checkpoint did not restore the original name, salted lifetime and animation");
    hs_start(world,object,graph,animation,name);
    require(source_campaign_cinematic_animation_time_tu15(world,object,false)==remaining,
        "Named HS caller did not resolve the checkpoint-restored object");
    objects.destroy_object(object);
    require(lookup()==null_datum&&!live(object),"Restored object retirement retained its native name association");
    std::cout<<"PASS native object-name birth/retirement/recreation/checkpoint and three actual named-HS calls\n";
}
void lifecycle(PhysicsLiveWorldTu15& world,MultiplayerObjectRuntime& objects,const Cache& cache,const ScenarioObject& placement,
    unsigned object,unsigned motion_tick,unsigned graph,unsigned animation,unsigned anchor,bool rendered=false,bool inactive=false,
    const LevelContent* gpu_content=nullptr){
    const auto word=[&](unsigned at){return Bytes(world.memory().view(at,4)).u32(0);};
    const auto tick=[&]{return word(word(0x51df54)+8);};
    const auto remaining=[&]{return source_campaign_cinematic_animation_time_tu15(world,object,false);};
    const auto bytes_equal=[](const auto& a,const auto& b){
        return std::ranges::equal(std::as_bytes(std::span(a)),std::as_bytes(std::span(b)));
    };
    struct Sample {std::vector<Transform> nodes;std::vector<Vertex> vertices;bool empty()const{return nodes.empty();}};
    const auto same=[&](const Sample& a,const Sample& b){return bytes_equal(a.nodes,b.nodes)&&bytes_equal(a.vertices,b.vertices);};
    const auto geometry=rendered?std::make_unique<SourceCampaignModel>(cache,placement):nullptr;
#if defined(HALO2_SCENERY_GPU)
    const auto gpu=gpu_content?std::make_unique<SceneryAnimationGpuConsumer>(cache,*gpu_content,placement,objects,object,motion_tick,*geometry):nullptr;
#else
    require(!gpu_content,"Scenery GPU mode requires its actual renderer target");
#endif
    const auto nodes=[&]{
        if(!rendered)return Sample{objects.world_nodes(object),{}};
        world.begin_model_frame();world.begin_model_view();
        const auto collector=objects.build_world_model(object,0,1,false,0.f,0.f);
        auto pose=objects.rendered_world_nodes(object);
        require(pose.has_value(),"Native scenery model builder did not publish a render pose");
        auto vertices=geometry->vertices(object,collector,*pose);
        require(!vertices.empty()&&vertices.size()%3==0,"Native scenery collector selected no complete authored triangles");
#if defined(HALO2_SCENERY_GPU)
        if(gpu)gpu->sample(collector,*pose);
#endif
        return Sample{std::move(*pose),std::move(vertices)};
    };
    const auto first_tick=tick();const auto first=remaining();const auto initial=nodes();
    const auto camera_first=source_campaign_camera_time_tu15(world);
    for(unsigned n=0;n<60;++n){world.step_objects();require(tick()==first_tick+n+1,"Scenery lifecycle source clock did not advance once");}
    const auto held=remaining();const auto held_pose=nodes();
    require(first-held==60&&same(initial,held_pose),"MCC scenery did not retain its authored opening hold and exact timer");
    unsigned first_stall=0;auto previous_time=held;
    for(unsigned n=60;n<motion_tick;++n){
        world.step_objects();require(tick()==first_tick+n+1,"Scenery motion source clock differs");
        const auto now=remaining();if(!first_stall&&previous_time-now!=1)first_stall=n+1;previous_time=now;
    }
    const auto later=remaining();const auto pose=nodes();
    if(inactive){
        const auto header=word(world.source_object_pool()+0x44)+(object&65535u)*12;
        const auto cluster=word(header+4)&65535u,game=word(0x4f3218);
        require(first_stall==175&&first-later==174&&cluster!=65535u&&!(word(header)>>16&1u)&&
            !(word(game+0x11b8+(cluster/32)*4)&(1u<<(cluster%32)))&&!same(initial,pose),
            "Missing-camera control did not preserve the original native activity/PVS gate");
        std::cout<<"PASS missing-camera control: native cluster remains outside PVS; game clock advances while scenery deactivates at174\n";
        return;
    }
    std::cout<<"PHASE native "<<(rendered?"model-builder":"collision")<<" WORLD consumer: remaining="<<first<<" -> "<<later
        <<" changed="<<!same(initial,pose)<<" actual_authored_vertices="<<pose.vertices.size()<<'\n';
    require(first-later==int(motion_tick)&&!pose.empty()&&!same(initial,pose),"MCC scenery clock/WORLD poses did not follow the changed authored key");
    require(!rendered||!bytes_equal(initial.vertices,pose.vertices),"Native scenery animation did not move its actual selected authored geometry");
    require(camera_first-source_campaign_camera_time_tu15(world)==int(motion_tick),"Authored native camera did not advance on the same game clock");
    const auto checkpoint=objects.authority_checkpoint();
    for(unsigned n=0;n<10;++n)world.step_objects();
    const auto future_time=remaining();const auto future_pose=nodes();
    auto replacement=objects.prepare_authority(checkpoint);
    require(objects.commit_authority(std::move(replacement)),"MCC scenery native checkpoint publication failed");
    require(tick()==first_tick+motion_tick&&remaining()==later&&same(nodes(),pose),
        "MCC scenery checkpoint did not restore its native clock/controller/WORLD hierarchy");
    for(unsigned n=0;n<10;++n)world.step_objects();
    require(tick()==first_tick+motion_tick+10&&remaining()==future_time&&same(nodes(),future_pose),
        "MCC scenery native animation replay differs after checkpoint restoration");
    std::cout<<"PASS actual MCC scenery motion and native checkpoint/replay: ticks="<<motion_tick<<"+10+restored10 remaining="<<remaining()<<'\n';
    for(int n=0;n<future_time;++n){
        world.step_objects();require(remaining()==future_time-n-1,"Native scenery timer stalled or skipped before completion");
    }
    require(remaining()==0,"Native scenery completion query did not clamp to zero");
    // The query anticipates completion by two frames. Let the original
    // scenery controller reach its held final frame before comparing poses.
    for(unsigned n=0;n<4;++n)world.step_objects();
    const auto final_pose=nodes();
    for(unsigned n=0;n<10;++n)world.step_objects();
    require(tick()==first_tick+unsigned(first)+14&&remaining()==0&&same(nodes(),final_pose),
        "Native scenery final held pose/timer did not survive later source ticks");
    require(!source_campaign_cinematic_animation_tu15(world,null_datum,graph,animation,anchor,false,false,false)&&
        !source_campaign_cinematic_animation_tu15(world,object,null_datum,animation,anchor,false,false,false)&&
        !source_campaign_cinematic_animation_tu15(world,object,graph,0,anchor,false,false,false),
        "Native scenery invalid object/graph/name inputs did not return false");
    if(anchor==null_datum)hs_start(world,object,graph,animation);
    else require(source_campaign_cinematic_animation_tu15(world,object,graph,animation,anchor,false,false,false),
        "Native authored relative scenery restart failed");
    require(remaining()==first&&same(nodes(),initial),"Native scenery restart did not restore the first clock and WORLD pose");
    world.step_objects();require(remaining()==first-1,"Native scenery restarted channel did not advance");
    objects.destroy_object(object);
    require(std::ranges::none_of(objects.live_objects(),[&](const auto& live){return live.object==object;}),
        "Native scenery retirement retained its full salted lifetime");
#if defined(HALO2_SCENERY_GPU)
    if(gpu)gpu->finish();
#endif
    std::cout<<"PASS actual MCC scenery completion/final hold/invalid-input/restart/retirement: initial_remaining="<<first<<'\n';
}
}
int main(int argc,char** argv)try {
    if((argc!=2&&argc!=3) || !std::filesystem::is_regular_file(argv[1]))return 77;
    require(argc==2||std::string_view(argv[2])=="--hs"||std::string_view(argv[2])=="--named-hs"||std::string_view(argv[2])=="--exercise"||
        std::string_view(argv[2])=="--render"||std::string_view(argv[2])=="--relative"||std::string_view(argv[2])=="--relative-render"||
        std::string_view(argv[2])=="--inactive"||std::string_view(argv[2])=="--gpu"||std::string_view(argv[2])=="--gpu-relative","Unknown scenery consumer option");
    const bool gpu=argc==3&&(std::string_view(argv[2])=="--gpu"||std::string_view(argv[2])=="--gpu-relative");
    const bool camera=argc==3&&(std::string_view(argv[2])=="--exercise"||std::string_view(argv[2])=="--render"||
        std::string_view(argv[2])=="--relative"||std::string_view(argv[2])=="--relative-render"||gpu);
    const bool relative=argc==3&&(std::string_view(argv[2])=="--relative"||std::string_view(argv[2])=="--relative-render"||std::string_view(argv[2])=="--gpu-relative");
    const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");
    require(scratch&&*scratch,"MCC scenery probe requires owned storage");
    const Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());
    require(cache.is_mcc(),"Scenery consumer requires prepared MCC content");
    const auto content=cache.load_content(0,false);
    const auto* tag=cache.find_tag(cache.scenario_tag());require(tag,"MCC scenery has no scenario");
    const auto scenario_bytes=cache.read_tag_bytes(tag->address,tag->size);const Bytes scenario(scenario_bytes);
    const auto node_bytes=cache.read_tag_bytes(scenario.u32(0x23c),std::size_t(scenario.u32(0x238))*20);const Bytes nodes(node_bytes);
    const auto string_bytes=cache.read_tag_bytes(scenario.u32(0x1b4),scenario.u32(0x1b0));const Bytes strings(string_bytes);
    const auto resolve=[&](unsigned datum) {
        const auto index=datum&65535u;require(datum!=0xffffffffu && index<scenario.u32(0x238),"Scenery recipe escapes syntax");
        const Bytes row(nodes.slice(index*20,20));require(row.u16(0)==datum>>16,"Scenery recipe salt differs");return row;
    };
    unsigned name_index=0xffffffffu,graph=0xffffffffu,animation=0,anchor_name=null_datum,recipe_datum=null_datum;
    unsigned camera_graph=null_datum,camera_animation=0,camera_flag=null_datum,fov_bits=0,fov_ticks=0;
    std::string recipe;
    for(unsigned index=0;index<scenario.u32(0x238);++index) {
        const Bytes call(nodes.slice(index*20,20));const auto flags=call.u16(6);
        if(!call.u16(0)||flags==0xbaba||(flags&3))continue;
        const auto function=resolve(call.u32(16));
        require((function.u16(6)&1)&&function.u16(4)==2,"MCC scenery call lacks its primitive function name");
        const auto offset=function.u32(12);require(offset<strings.size(),"MCC scenery function name escapes strings");
        const auto name=strings.string(offset,strings.size()-offset);
        if(name!="scenery_animation_start" && name!="scenery_animation_start_loop" &&
           name!="scenery_animation_start_relative" && name!="scenery_animation_start_relative_loop")continue;
        std::vector<Bytes> args;std::set<unsigned> seen;
        for(auto next=function.u32(8);next!=0xffffffffu;next=args.back().u32(8)) {
            require(seen.insert(next).second&&args.size()<8,"Scenery recipe sibling cycle");args.push_back(resolve(next));
        }
        if(args.size()<3 || args[0].u16(2)!=61 || args[1].u16(2)!=36 || args[2].u16(2)!=11)continue;
        if(std::ranges::any_of(args,[](const auto& row){return !(row.u16(6)&1)||(row.u16(6)&4);}))continue;
        const auto named=std::int16_t(args[0].u32(16));
        if(named<0 || unsigned(named)>=content.object_names.size() || content.object_names[named].type!=6)continue;
        const auto* animation_tag=cache.find_tag(TagHandle{args[1].u32(16)});
        if(!animation_tag || animation_tag->group!="jmad" || !args[2].u32(16))continue;
        name_index=unsigned(named);graph=args[1].u32(16);animation=args[2].u32(16);recipe=name;
        recipe_datum=(unsigned(call.u16(0))<<16)|index;
        if(args.size()==4&&args[3].u16(2)==56)anchor_name=std::uint16_t(args[3].u32(16));
        break;
    }
    require(name_index!=0xffffffffu,"MCC map has no authored literal scenery animation recipe");
    if(camera)for(unsigned index=0;index<scenario.u32(0x238);++index){
        const Bytes call(nodes.slice(index*20,20));
        if(!call.u16(0)||call.u16(6)==0xbaba||(call.u16(6)&3))continue;
        const auto function=resolve(call.u32(16));const auto offset=function.u32(12);
        require(offset<strings.size(),"Camera context name escapes strings");
        if(strings.string(offset,strings.size()-offset)!="begin")continue;
        std::vector<unsigned> siblings;std::set<unsigned> seen;
        for(auto next=function.u32(8);next!=null_datum;next=resolve(next).u32(8)){
            require(seen.insert(next).second,"Camera context sibling cycle");siblings.push_back(next);
        }
        const auto selected=std::ranges::find(siblings,recipe_datum);if(selected==siblings.end())continue;
        for(auto at=siblings.begin();at!=selected;++at){
            const auto row=resolve(*at);if(row.u16(6)&3)continue;
            const auto fn=resolve(row.u32(16));const auto text=fn.u32(12);
            require(text<strings.size(),"Camera action name escapes strings");const auto action=strings.string(text,strings.size()-text);
            if(action!="camera_set_animation_relative"&&action!="camera_set_field_of_view")continue;
            std::vector<Bytes> args;std::set<unsigned> arguments;
            for(auto next=fn.u32(8);next!=null_datum;next=args.back().u32(8)){
                require(arguments.insert(next).second&&args.size()<8,"Camera action sibling cycle");args.push_back(resolve(next));
            }
            require(std::ranges::all_of(args,[](const auto& a){return(a.u16(6)&1)&&!(a.u16(6)&4);}),"Camera setup needs authored literal arguments");
            if(action=="camera_set_animation_relative"){
                require(args.size()==4&&args[0].u16(2)==36&&args[1].u16(2)==11&&args[2].u32(16)==null_datum&&args[3].u16(2)==14,
                    "Camera setup differs from the authored graph/name/NONE/flag contract");
                camera_graph=args[0].u32(16);camera_animation=args[1].u32(16);camera_flag=args[3].u32(16)&65535;
            }else{
                require(args.size()==2&&args[0].u16(2)==6&&args[1].u16(2)==7,"Camera FOV setup signature differs");
                fov_bits=args[0].u32(16);fov_ticks=args[1].u32(16)&65535;
            }
        }
    }
    const auto& named=content.object_names.at(name_index);
    const auto placement=std::ranges::find_if(content.objects,[&](const auto& object){
        return object.type==named.type && object.placement==named.placement;
    });
    require(placement!=content.objects.end(),"MCC scenery recipe has no authored placement");
    std::cout<<"PHASE actual MCC recipe="<<recipe<<" object="<<named.name<<" graph="<<graph<<" animation="<<animation
        <<" animation_name="<<cache.string_id(animation)<<'\n'<<std::flush;
    const auto saves=std::filesystem::path(scratch)/"scenery";std::filesystem::create_directories(saves);
    MultiplayerObjectRuntime objects(cache,0,saves);auto& world=objects.tu15_world();
    world.initialize_map();world.configure_campaign_session(1,1,30,0x12345678);
    if(camera)source_campaign_camera_control_tu15(world,true);
    source_campaign_cinematic_control_tu15(world,true);
    unsigned anchor=null_datum,frame_anchor=null_datum;
    if(relative||gpu){
        require(recipe=="scenery_animation_start_relative"&&anchor_name<content.object_names.size(),"Scenery fixture lacks its authored reference frame");
        const auto& name=content.object_names.at(anchor_name);
        const auto row=std::ranges::find_if(content.objects,[&](const auto& p){return p.type==name.type&&p.placement==name.placement;});
        require(row!=content.objects.end(),"Relative diagnostic anchor has no authored placement");
        frame_anchor=objects.create_scenario_object(*row,SourceScenarioBirth::automatic);
        require(frame_anchor!=null_datum,"Native authored anchor birth returned NONE");
        if(relative)anchor=frame_anchor;
        std::cout<<"PHASE native authored reference frame="<<name.name<<" automatic birth; animation_relative="<<relative<<"; no activity override\n";
    }
    const auto object=objects.create_scenario_object(*placement,SourceScenarioBirth::runtime);
    require(object!=null_datum,"Native MCC scenery birth returned NONE");
    if(gpu&&!relative){
        const auto frame=objects.world_nodes(frame_anchor);require(!frame.empty(),"Native authored frame has no WORLD node");
        require(frame.front().scale==1.f,"Authored comparison frame changed its unit scale");
        const std::array inputs{frame.front().position,frame.front().basis[0],frame.front().basis[2]};
        const auto scratch=world.prepare_source_call().gp[4]+0x100u;
        world.write_source_call_input(scratch,std::as_bytes(std::span(inputs)));
        auto abi=world.prepare_source_call(std::array{object,0u,0u});
        abi.gp[0]=scratch;abi.gp[1]=scratch+12u;abi.gp[2]=scratch+24u;world.run_actual(0xb95d0u,abi);
        std::cout<<"PHASE original B95D0 places nonrelative fixture in the actual authored frame; animation anchor remains NONE\n";
    }
    source_campaign_cinematic_lod_tu15(world,object,true);
    if(camera){
        require(camera_graph!=null_datum&&camera_animation&&camera_flag!=null_datum&&fov_bits,"Scene lacks its authored preceding camera/FOV setup");
        source_campaign_camera_field_of_view_tu15(world,std::bit_cast<float>(fov_bits),std::int16_t(fov_ticks));
        const auto camera_anchor=std::int16_t(camera_flag);
        source_campaign_camera_animation_tu15(world,camera_graph,camera_animation,null_datum,camera_anchor);
        const auto cluster=world.call_actual(0x16fd40u);
        require((cluster&65535)!=65535,"Authored native camera has no BSP cluster");
        std::cout<<"PHASE native camera graph="<<camera_graph<<" animation="<<camera_animation
            <<" parsed_scene_flag="<<camera_flag<<" executed_flag="<<camera_anchor<<" source_cluster="<<cluster<<"; no PVS override\n";
    }
    require(source_campaign_cinematic_animation_tu15(world,object,graph,animation,anchor,false,false,false),
        "Actual native MCC scenery start returned false");
    if(gpu&&!relative)require(Bytes(world.memory().view(world.source_object_body(object)+0x138u,4)).u32(0)==null_datum,
        "Nonrelative scenery start retained an animation anchor");
    const auto remaining=source_campaign_cinematic_animation_time_tu15(world,object,false);
    const auto pose=objects.world_nodes(object);
    require(remaining>0 && !pose.empty(),"Native MCC scenery has no retained animation clock/WORLD hierarchy");
    std::cout<<"PASS actual MCC scenery constructor/start/query: remaining="<<remaining<<" WORLD nodes="<<pose.size()<<'\n';
    if(relative){
        require(Bytes(world.memory().view(world.source_object_body(object)+0x138u,4)).u32(0)==anchor,
            "Native relative start did not retain its authored anchor");
        lifecycle(world,objects,cache,*placement,object,authored_motion_tick(cache,graph,animation),graph,animation,anchor,std::string_view(argv[2])!="--relative",false,gpu?&content:nullptr);
        std::cout<<(gpu?"LIMIT isolated native CPU/GPU effect; MCC185 admission, full scene and complete mission delivery remain separate\n":
            "LIMIT relative native CPU effect only; submitted GPU frames, MCC185 and complete mission delivery remain separate\n");
        return 0;
    }
    if(argc==3){
        const bool named_hs=std::string_view(argv[2])=="--named-hs";
        if(named_hs)std::cout<<"PHASE native authored scenery_name61 -> scenery55 index="<<name_index<<'\n'<<std::flush;
        hs_start(world,object,graph,animation,named_hs?name_index:null_datum);
        require(source_campaign_cinematic_animation_time_tu15(world,object,false)>0&&!objects.world_nodes(object).empty(),
            "World HS scenery effect retained no actual MCC animation clock/WORLD hierarchy");
        std::cout<<"PASS actual native World HS dispatcher/parser/scenery effect/VOID completion; no supplied dependency returns\n";
        if(named_hs)named_lifecycle(world,objects,*placement,object,name_index,graph,animation);
        if(std::string_view(argv[2])=="--exercise")lifecycle(world,objects,cache,*placement,object,authored_motion_tick(cache,graph,animation),graph,animation,anchor);
        if(std::string_view(argv[2])=="--render")lifecycle(world,objects,cache,*placement,object,authored_motion_tick(cache,graph,animation),graph,animation,anchor,true);
        if(std::string_view(argv[2])=="--inactive")lifecycle(world,objects,cache,*placement,object,authored_motion_tick(cache,graph,animation),graph,animation,anchor,true,true);
        if(gpu)lifecycle(world,objects,cache,*placement,object,authored_motion_tick(cache,graph,animation),graph,animation,anchor,true,false,&content);
    }
    if(gpu)std::cout<<"LIMIT isolated native CPU/GPU effect; MCC185 admission, full scene and complete mission delivery remain separate\n";
    else if(argc==3&&(std::string_view(argv[2])=="--exercise"||std::string_view(argv[2])=="--render"))
        std::cout<<"LIMIT native CPU pose/checkpoint component only; submitted GPU frames, full mission and MCC admission remain required\n";
    else if(std::string_view(argc==3?argv[2]:"")!="--inactive")std::cout<<"LIMIT initialization consumer only; full clock advancement, checkpoint, rendering and MCC admission remain required\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

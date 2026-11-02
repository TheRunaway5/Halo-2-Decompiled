#include "halo2/cache.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/campaign_frame_clock.hpp"
#include "halo2/simulation.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
using namespace halo2;
namespace {
void need(bool value,const char* why){if(!value)throw FormatError(why);}
unsigned word(const SourceRuntimeMemory& memory,unsigned at){return Bytes(memory.view(at,4)).u32(0);}
std::array<std::byte,36> pose(const SourceCampaignCameraViewTu15& view){
    std::array<std::byte,36> result;const std::array vectors{view.position,view.forward,view.up};
    std::memcpy(result.data(),vectors.data(),result.size());return result;
}
ScriptProgram camera_controls(){
    ScriptProgram program;
    const auto node=[&](ScriptNode value){
        const auto index=unsigned(program.nodes.size());value.salt=std::uint16_t(0xe000+index);
        program.nodes.push_back(value);return(unsigned(value.salt)<<16)|index;
    };
    const auto call=[&](std::string name,std::vector<unsigned> args){
        const auto text=unsigned(program.strings.size());program.strings+=name;program.strings+='\0';
        for(unsigned n=0;n<args.size();++n)program.nodes[args[n]&65535].next=n+1<args.size()?args[n+1]:null_datum;
        const auto function=node({0,0,2,9,args.empty()?null_datum:args.front(),text,0});
        return node({0,0,4,8,null_datum,text,function});
    };
    const auto cinematic=call("cinematic_start",{}),enabled=node({0,5,5,9,null_datum,0,1});
    const auto camera=call("camera_control",{enabled}),root=call("begin",{cinematic,camera});
    program.scripts.push_back({"camera_hs_control_fixture",0,4,root});return program;
}
void hs_start(PhysicsLiveWorldTu15& world,unsigned graph,unsigned animation){
    auto& memory=world.memory();
    need(word(memory,0x480c00+452*4)==0x458e90&&word(memory,0x458e94)==0x2b27b0,
        "Camera HS descriptor differs from authenticated original452");
    constexpr unsigned fixture=0xb0000000,pool=fixture+0x2000,syntax=fixture+0x2100,
        thread=fixture+0x3000,frame=thread+0x40,nodes=fixture+0x4000,output=fixture+0x7000;
    memory.mount(fixture,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,
        "explicit original452 HS caller fixture; parser/camera/completion are actual native owners");
    const auto write=[&](unsigned at,unsigned value,unsigned width=4){
        std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8*n));
        world.write_source_call_input(at,std::span(bytes).first(width));
    };
    std::vector<std::pair<unsigned,std::vector<std::byte>>> saved;
    for(const auto [at,size]:{std::pair(0x505e5cu,4u),std::pair(0x505e60u,1u),
        std::pair(0x505e64u,4u),std::pair(0x505e6cu,4u)}){
        const auto bytes=memory.view(at,size);saved.emplace_back(at,std::vector(bytes.begin(),bytes.end()));
    }
    const auto restore=[&]{for(const auto&[at,bytes]:saved)world.write_source_call_input(at,bytes);memory.release(fixture);};
    try{
        const auto node=[&](unsigned index,unsigned opcode,unsigned type,unsigned flags,unsigned next,unsigned value){
            const auto at=nodes+index*20;write(at,0x9000+index,2);write(at+2,opcode,2);write(at+4,type,2);
            write(at+6,flags,2);write(at+8,next);write(at+16,value);
        };
        write(0x505e5c,pool);write(0x505e60,1,1);write(0x505e6c,syntax);write(pool+0x44,thread);
        write(syntax+0x20,4);write(syntax+0x24,20);write(syntax+0x44,nodes);
        node(0,452,4,8,null_datum,0x90010001);node(1,2,2,9,0x90020002,0);
        node(2,36,36,9,0x90030003,graph);node(3,11,11,9,null_datum,animation);
        write(thread+2,1,1);write(thread+3,1,1);write(thread+0xc,0x100,2);write(thread+0x10,frame);
        write(frame,thread+0x18);write(frame+4,0x90000000);write(frame+0xc,0,2);
        write(thread+0x20,output);write(output,0xdeadbeef);
        auto abi=world.prepare_source_call();abi.gp[7]=0xa0010000;const auto before=abi;
        std::set<unsigned> stores;
        world.run_actual(0x211b10,abi,[&](const auto& store){stores.insert(store.instruction);});
        for(const auto site:{0x2b27e3u,0x16ee58u,0x212096u,0x212117u,0x211e5eu})
            need(stores.contains(site),"Camera HS did not traverse its real wrapper/parser/effect/completion");
        need(word(memory,output)==0&&word(memory,thread+0x10)==thread+0x18&&abi.gp[4]==before.gp[4]+4,
            "Camera HS did not retire its VOID frame through the original completion owner");
        for(unsigned n:{3u,5u,6u,7u})need(abi.gp[n]==before.gp[n],"Camera HS changed a nonvolatile register");
        // Actual16EE83..16EE9B writes scalar XMM0/XMM1, followed by
        // FLD/FISTP at16EEA5/16EEA9. Do not impose callee SIMD/status
        // preservation on that real effect. The VOID call must balance x87.
        need(abi.x87_depth==before.x87_depth,"Camera HS left an unbalanced FPU stack");
        std::cout<<"PHASE native camera scalar SIMD outputs="<<abi.xmm[0][0]<<','<<abi.xmm[1][0]
            <<" x87_depth="<<abi.x87_depth<<'\n';
    }catch(...){restore();throw;}
    restore();
}
}
int main(int argc,char** argv)try{
    if(argc!=3||!std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());
    need(cache.is_mcc(),"Camera HS consumer requires prepared MCC content");
    auto content=cache.load_content(0,false);content.objects.clear();content.scripts=camera_controls();
    // Use the installed MCC graph/name namespace. Xbox stock fixture handles
    // differ. The explicit original452 caller exercises this actual authored
    // clip in the nonrelative NONE/NONE form, without changing map data.
    unsigned graph=null_datum,animation{},recipe=null_datum;
    const auto* scenario_tag=cache.find_tag(cache.scenario_tag());need(scenario_tag,"Missing camera scenario");
    const Bytes scenario(cache.read_tag_bytes(scenario_tag->address,scenario_tag->size));
    const Bytes nodes(cache.read_tag_bytes(scenario.u32(0x23c),std::size_t(scenario.u32(0x238))*20));
    const Bytes strings(cache.read_tag_bytes(scenario.u32(0x1b4),scenario.u32(0x1b0)));
    const auto resolve=[&](unsigned datum){
        need(datum!=null_datum&&(datum&65535)<nodes.size()/20,"Authored camera input escapes syntax");
        const Bytes row(nodes.slice((datum&65535)*20,20));need(row.u16(0)==datum>>16,"Authored camera input has a stale salt");return row;
    };
    for(unsigned n=0;n<nodes.size()/20;++n){
        const Bytes call(nodes.slice(n*20,20));if(!call.u16(0)||(call.u16(6)&7)||call.u16(2)!=455)continue;
        const auto function=resolve(call.u32(16));const auto offset=function.u32(12);
        need((function.u16(6)&1)&&function.u16(4)==2&&offset<strings.size(),"Camera call lacks its authored function identity");
        need(strings.string(offset,strings.size()-offset)=="camera_set_animation_relative","Camera opcode/name identity differs");
        const auto first=resolve(function.u32(8)),second=resolve(first.u32(8));
        if(first.u16(2)!=36||second.u16(2)!=11||(first.u16(6)&5)!=1||(second.u16(6)&5)!=1)continue;
        graph=first.u32(16);animation=second.u32(16);recipe=(unsigned(call.u16(0))<<16)|n;break;
    }
    need(graph!=null_datum&&animation&&recipe!=null_datum,"MCC map has no authored literal camera clip");
    const auto* graph_tag=cache.find_tag(TagHandle{graph});need(graph_tag&&graph_tag->group=="jmad","Authored camera lost its actual graph");
    std::cout<<"PHASE actual MCC camera root="<<recipe<<" graph="<<graph<<" animation="<<animation
        <<" name="<<cache.string_id(animation)<<'\n'<<std::flush;
    auto source=std::make_shared<CampaignSourceRuntime>(cache,0,1,0x12345678);
    Simulation simulation(content,0,cache.spawns().front(),[&](auto g,auto n){return cache.load_named_animation(g,n);},
        {},{},{},{},{},source);
    auto& world=source->objects().tu15_world();auto& memory=world.memory();
    const auto users=word(memory,0x4f54f0u);
    need(users&&word(memory,users+12)!=null_datum,"Camera consumer has no actual registered native player");
    simulation.start_level();simulation.step({});
    need(simulation.scripts().diagnostics().empty()&&simulation.presentation().scripted&&simulation.presentation().cinematic,
        "Actual camera/cinematic host control fixture did not complete");
    source_campaign_camera_field_of_view_tu15(world,60,0);
    hs_start(world,graph,animation);
    const auto director=word(memory,0x51df6c);
    need(word(memory,director+0x44)==graph&&word(memory,director+0x48)==animation&&
        Bytes(memory.view(director+0x4c,2)).u16(0)==0xffff&&word(memory,director+0x3c)==null_datum,
        "Nonrelative camera HS did not retain its real graph/name and NONE flag/object");
    const auto remaining=source_campaign_camera_time_tu15(world);need(remaining>1,"Camera HS retained no real animation time");
    unsigned frames{},zero_ticks{},one_tick{},many_ticks{};
    const auto frame_step=[&](float raw){
        const auto incoming=source_campaign_frame_interval_tu15(raw);const auto accepted=simulation.advance_frame_time(incoming,false);
        for(int n=0;n<accepted.ticks;++n)simulation.step({});
        simulation.advance_cinematic_render_time(accepted.elapsed,accepted.ticks,accepted.ticks);
        simulation.advance_cinematic_camera(incoming);
        const auto view=source_campaign_camera_view_tu15(memory,0);need(view.has_value(),"Native camera frame has no registered observer output");
        const auto scratch=world.prepare_source_call().gp[4]+0x100u;const std::array<std::byte,56> clear{};
        world.write_source_call_input(scratch,clear);auto abi=world.prepare_source_call(std::array{scratch,scratch+52u});
        world.run_actual(0x16fe00u,abi);need((abi.gp[0]&255u)!=0,"Native camera sampler rejected its real nonrelative clip");
        const auto sample=memory.view(scratch,52);std::array<std::byte,36> expected,produced;
        std::memcpy(expected.data(),sample.data()+40,12);std::memcpy(expected.data()+12,sample.data()+4,12);std::memcpy(expected.data()+24,sample.data()+28,12);
        const auto command=word(memory,0x4f64a8u);
        for(const auto [to,from]:{std::pair(0u,4u),std::pair(12u,0x2cu),std::pair(24u,0x38u)})
            std::memcpy(produced.data()+to,memory.view(command+from,12).data(),12);
        need(produced==expected,"Nonrelative native camera command differs from its real source sampler");
        ++frames;zero_ticks+=accepted.ticks==0;one_tick+=accepted.ticks==1;many_ticks+=accepted.ticks>1;
        return pose(*view);
    };
    const auto initial=frame_step(0),subframe=frame_step(1.f/120.f);
    need(initial!=subframe,"Nonrelative camera lost motion on a positive zero-tick frame");
    for(const float raw:std::array{1.f/30.f,.125f,0.f,1.f/30.f})(void)frame_step(raw);
    need(zero_ticks&&one_tick&&many_ticks,"Camera consumer lost zero/one/multiple-tick coverage");
    hs_start(world,graph,animation);need(source_campaign_camera_time_tu15(world)==remaining,"Nonrelative camera HS restart lost its initial time");
    std::ofstream(argv[2])<<nlohmann::json{{"passed",true},{"authored_recipe",recipe},{"graph",graph},{"animation",animation},
        {"frames",frames},{"zero_tick_frames",zero_ticks},
        {"one_tick_frames",one_tick},{"multiple_tick_frames",many_ticks},{"initial_remaining",remaining},
        {"real_HS_parser_wrapper_effect_completion",true},{"native_sampler_command_bits_equal",true},
        {"scope","Explicit original452 caller with actual stock clip and native camera frame consumers; no MCC454, E3, GPU or mission acceptance"}}.dump(2)<<'\n';
    std::cout<<"PASS actual native camera HS/parser/effect/VOID, NONE/NONE, restart and "<<frames<<" observer frames\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

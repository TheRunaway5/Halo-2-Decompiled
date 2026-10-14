#include "halo2/scripts.hpp"
#include "halo2/scene.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <limits>
#include <tuple>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}check(rejected,"Invalid/active script or presentation continuation accepted");}
struct Program {
    ScriptProgram data;
    std::uint32_t node(ScriptNode n){const auto i=std::uint32_t(data.nodes.size());n.salt=std::uint16_t(0xe000+i);data.nodes.push_back(n);return (std::uint32_t(n.salt)<<16)|i;}
    std::uint32_t scalar(int v,unsigned type=8){return node({0,std::uint16_t(type),std::uint16_t(type),9,null_datum,0,std::uint32_t(v)});}
    std::uint32_t global(unsigned v){return node({0,8,8,13,null_datum,0,v});}
    std::uint32_t call(const std::string &name,std::vector<std::uint32_t> args={},unsigned type=4){
        const auto at=std::uint32_t(data.strings.size());data.strings+=name;data.strings+='\0';
        for(unsigned i=0;i<args.size();++i)data.nodes[args[i]&65535u].next=i+1<args.size()?args[i+1]:null_datum;
        const auto fn=node({0,0,2,9,args.empty()?null_datum:args[0],at,0});return node({0,0,std::uint16_t(type),8,null_datum,at,fn});
    }
};
void scripts() {
    Program p;p.data.globals.push_back({"counter",8,p.scalar(0)});
    const auto main=p.call("begin",{p.call("set",{p.global(0),p.scalar(11)},8),p.call("sleep",{p.scalar(3)}),
        p.call("begin_random",{p.call("mark",{p.scalar(10)}),p.call("begin",{p.call("sleep",{p.scalar(2)}),p.call("mark",{p.scalar(20)})}),p.call("mark",{p.scalar(30)})},3),
        p.call("set",{p.global(0),p.scalar(29)},8),p.call("mark",{p.global(0)})});
    p.data.scripts.push_back({"main",0,4,main});
    p.data.scripts.push_back({"command",5,4,p.call("begin",{p.call("await"),p.call("mark",{p.scalar(40)})})});
    p.data.scripts.push_back({"fault",0,4,p.call("fault")}); // A handled native VM fault must survive import.
    auto other_program=p.data;ScriptVm *ap=nullptr,*bp=nullptr;
    std::vector<std::pair<int,std::optional<std::uint32_t>>> a_calls,b_calls;
    std::uint32_t arng=0x12345678,brng=arng;unsigned clock=0;bool interrupt_host=false;
    const auto host=[&](ScriptVm *vm,auto &calls,const auto &name,const auto &args){
        rejects([&]{vm->authority_checkpoint();});
        if(name=="fault")throw FormatError("Declared handled script fault");
        if(name=="await")vm->wait_for_command();else if(name=="mark")calls.emplace_back(int(args.at(0).number()),vm->command_actor());
        return ScriptValue{};
    };
    ScriptVm a(p.data,[&](const auto &n,const auto &args){if(interrupt_host)throw 17;return host(ap,a_calls,n,args);});
    ScriptVm b(other_program,[&](const auto &n,const auto &args){return host(bp,b_calls,n,args);});ap=&a;bp=&b;
    a.set_random_choice([&](auto count,auto visited){return source_hs_begin_random(arng,count,visited);});
    b.set_random_choice([&](auto count,auto visited){return source_hs_begin_random(brng,count,visited);});
    a.command_ready([&](auto){return clock>=6;});b.command_ready([&](auto){return clock>=6;});
    a.start_level();a.start_command(1,0xe1230007u);
    for(clock=0;clock<=3;++clock)a.tick(clock);
    a.suspend_tick();const auto saved=a.authority_checkpoint();check(saved.suspended && saved.diagnostics.size()==1,"Script suspension/fault history not captured");
    bool random_frame=false,waiting=false;
    for(const auto &t:saved.threads){waiting|=t.command_wait;for(const auto &f:t.stack)random_frame|=f.name=="begin_random" && f.random_visited;}
    check(random_frame && waiting,"Script fixture did not capture random continuation and actor wait");
    const auto before=b.authority_checkpoint();
    {auto staged=b.prepare_authority(saved);rejects([&]{b.tick(4);});rejects([&]{b.start_level();});
        rejects([&]{b.set_random_choice({});});rejects([&]{b.authority_checkpoint();});
        check(!a.commit_authority(std::move(staged)),"Foreign script owner accepted publication");}
    check(b.authority_checkpoint()==before,"Cancelled script preparation changed target");
    auto bad=saved;bad.version=2;rejects([&]{b.prepare_authority(bad);});
    bad=saved;bad.globals[0].type=9;rejects([&]{b.prepare_authority(bad);});
    bad=saved;bad.threads[0].stack[0].children[0]^=0x10000u;rejects([&]{b.prepare_authority(bad);});
    bad=saved;bad.threads[0].stack[0].name="other";rejects([&]{b.prepare_authority(bad);});
    bad=saved;bad.threads[0].stack[0].id=p.data.globals[0].root;rejects([&]{b.prepare_authority(bad);});
    auto different=p.data;different.strings+='x';ScriptVm mismatch(different,{});rejects([&]{mismatch.prepare_authority(saved);});
    auto staged=b.prepare_authority(saved);check(b.commit_authority(std::move(staged)) && !b.commit_authority(std::move(staged)),"Script publication failed or reused token");
    check(b.authority_checkpoint()==saved,"Script import lost complete continuation fields");
    brng=arng;a_calls.clear(); // The root separately restores the actual primary RNG stream.
    for(clock=4;clock<40;++clock){a.tick(clock);b.tick(clock);check(a.authority_checkpoint()==b.authority_checkpoint() && arng==brng && a_calls==b_calls,"Future script continuation/local services diverged");}
    check(a.globals()[0].number()==29 && a.diagnostics()==saved.diagnostics,"Imported script lost globals or handled fault history");
    rejects([&]{a.start("missing");});a.authority_checkpoint();
    rejects([&]{b.restore(a.snapshot());}); // Existing campaign identity policy remains unchanged.
    const auto recover=a.authority_checkpoint();const auto rng_recover=arng;
    a.start("main");interrupt_host=true;bool interrupted=false;
    try{for(clock=40;clock<48;++clock)a.tick(clock);}catch(int value){interrupted=value==17;}
    check(interrupted,"Script fixture did not interrupt an outer host callback");rejects([&]{a.authority_checkpoint();});
    staged=a.prepare_authority(recover);check(a.commit_authority(std::move(staged)),"Interrupted script dispatch did not release its transient context");
    arng=rng_recover;check(a.authority_checkpoint()==recover,"Admitted script recovery changed state");
    std::cout<<"PASS native script sleep/random/command continuation across independent programs, local RNG/host callbacks, handled fault/suspension, cancellation and malformed admission\n";
}
bool same(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool same(Vec3 a,Vec3 b){return same(a.x,b.x)&&same(a.y,b.y)&&same(a.z,b.z);}
void same(const Camera &a,const Camera &b){check(same(a.position,b.position)&&same(a.forward,b.forward)&&same(a.up,b.up)&&
    same(a.yaw,b.yaw)&&same(a.pitch,b.pitch)&&same(a.roll,b.roll)&&same(a.field_of_view,b.field_of_view)&&
    a.oriented==b.oriented && a.horizontal_field_of_view==b.horizontal_field_of_view && a.projection_vertical_angle==b.projection_vertical_angle &&
    same(a.projection_clip_scale.x,b.projection_clip_scale.x)&&same(a.projection_clip_scale.y,b.projection_clip_scale.y)&&
    same(a.projection_clip_offset.x,b.projection_clip_offset.x)&&same(a.projection_clip_offset.y,b.projection_clip_offset.y),"Restored complete camera sample differs");}
void same(const Presentation &a,const Presentation &b){
    const auto x=a.authority_checkpoint(),y=b.authority_checkpoint();
    check(x.content_binding==y.content_binding && x.cinematic==y.cinematic && x.scripted==y.scripted && x.cinematic_outro==y.cinematic_outro &&
        x.player_camera_enabled==y.player_camera_enabled && x.camera_duration==y.camera_duration && x.camera_elapsed==y.camera_elapsed &&
        same(x.fade_color,y.fade_color) && x.observer.velocity==y.observer.velocity && x.observer.acceleration==y.observer.acceleration &&
        same(x.observer.position,y.observer.position)&&same(x.observer.forward,y.observer.forward)&&same(x.observer.up,y.observer.up),"Presentation continuation state differs");
    same(x.camera,y.camera);same(x.camera_from,y.camera_from);same(x.camera_to,y.camera_to);
    for(unsigned i=0;i<4;++i)check(same(x.transitions[i].from,y.transitions[i].from)&&same(x.transitions[i].to,y.transitions[i].to)&&
        x.transitions[i].duration==y.transitions[i].duration&&x.transitions[i].elapsed==y.transitions[i].elapsed,"Presentation transition frontier differs");
    const auto p=a.effects(),q=b.effects();check(same(p.fade_color,q.fade_color)&&same(p.fade,q.fade)&&same(p.letterbox,q.letterbox)&&same(p.hud,q.hud),"Restored fade/letterbox/HUD output differs");
    for(float fraction:{0.f,.5f,1.f})same(a.camera({},fraction),b.camera({},fraction));
}
void presentation(const char *xbe,const char *map) {
    Cache ca(map,{},OriginalData::load_xbe(xbe)),cb(map,{},OriginalData::load_xbe(xbe));
    auto first=ca.load_content(0),second=cb.load_content(0);
    check(first.cutscene_flags.size()>17,"Real Cairo camera anchors unavailable");
    // Explicit native camera-command inputs exercise observer derivatives.
    // Cairo's actual animation/anchor and TU1.5 fade data are tested below;
    // these two points are not claimed to be authored Cairo scenario rows.
    first.camera_points={{"native observer A",{0,1,2},{.2f,.1f,.01f}},
        {"native observer B",{10,-4,3},{-.3f,.2f,.02f}}};second.camera_points=first.camera_points;
    Presentation *ap=nullptr,*bp=nullptr;unsigned a_loads=0,b_loads=0;bool interrupted=false;
    Presentation a(first,[&](auto graph,auto name){++a_loads;rejects([&]{ap->authority_checkpoint();});return ca.load_named_animation(graph,name);});
    Presentation b(second,[&](auto graph,auto name){++b_loads;rejects([&]{bp->authority_checkpoint();});
        if(interrupted)throw FormatError("Declared loader interruption");return cb.load_named_animation(graph,name);});ap=&a;bp=&b;
    a.control(true,{});a.cinematic_control(true);a.cinematic_outro=true;a.player_camera_enabled=false;
    a.camera_point(0,40);a.fade({.2f,.3f,.4f},true,50);a.hud(.3f,80);a.field_of_view(65,30);
    for(unsigned i=0;i<8;++i)a.tick();a.camera_point(1,50);for(unsigned i=0;i<3;++i)a.tick();
    const auto saved=a.authority_checkpoint();const auto before=b.authority_checkpoint();
    {auto staged=b.prepare_authority(saved);rejects([&]{b.tick();});rejects([&]{b.camera_point(0,10);});
        rejects([&]{b.authority_checkpoint();});check(!a.commit_authority(std::move(staged)),"Foreign presentation owner accepted publication");}
    same(before.camera,b.authority_checkpoint().camera);
    auto bad=saved;bad.transitions[0].elapsed=bad.transitions[0].duration+1;rejects([&]{b.prepare_authority(bad);});
    bad=saved;bad.observer.velocity[0]=std::numeric_limits<float>::infinity();rejects([&]{b.prepare_authority(bad);});
    auto changed=second;changed.camera_points[0].position.x+=1;Presentation mismatch(changed,[](auto,auto){return AnimationClip{};});
    rejects([&]{mismatch.prepare_authority(saved);});
    auto publish=b.prepare_authority(saved);check(b.commit_authority(std::move(publish)),"Presentation import failed");
    for(unsigned i=0;i<60;++i){a.tick();b.tick();same(a,b);}
    constexpr auto graph=0xf3f411bcu,name=0x06001c72u;
    a.camera_animation(graph,name,17);for(unsigned i=0;i<3;++i)a.tick();const auto animated=a.authority_checkpoint();
    publish=b.prepare_authority(animated);check(b.commit_authority(std::move(publish)) && b_loads==0,"Animation import invoked/replaced the local loader");
    for(unsigned i=0;i<28;++i){a.tick();b.tick();same(a,b);}
    a.camera_animation(graph,name,17);b.camera_animation(graph,name,17);check(a_loads==2 && b_loads==1,"Future camera animation used the old host loader");same(a,b);
    const auto recover=b.authority_checkpoint();interrupted=true;rejects([&]{b.camera_animation(graph,name,17);});rejects([&]{b.authority_checkpoint();});
    publish=b.prepare_authority(recover);check(b.commit_authority(std::move(publish)),"Interrupted presentation did not recover from admitted state");same(a,b);
    rejects([&]{b.fade({},true,-1);});b.authority_checkpoint();
    std::cout<<"PASS actual TU1.5 fade curve and Cairo camera animation/anchor, native observer fixture derivatives, sixty observer and twenty-eight animation ticks, local loaders, staging and fault recovery\n";
}
}
int main(int argc,char **argv)try {
    std::cout<<std::unitbuf;scripts();check(argc==3,"Supply TU1.5 XBE and Cairo cache");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    presentation(argv[1],argv[2]);return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

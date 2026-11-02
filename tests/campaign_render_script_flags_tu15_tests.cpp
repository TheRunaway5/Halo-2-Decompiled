#include "halo2/source_campaign_lighting_tu15.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/simulation.hpp"
#include "halo2/original_data.hpp"
#include <iostream>
using namespace halo2;
namespace {
void check(bool v,const char* message){if(!v)throw FormatError(message);}
template<class F>void rejects(F f,const char* message){bool bad=false;try{f();}catch(const std::exception&){bad=true;}check(bad,message);}
unsigned byte(PhysicsLiveWorldTu15& w,std::uint32_t p){return std::to_integer<unsigned>(w.memory().view(p,1)[0]);}
unsigned word(PhysicsLiveWorldTu15& w,std::uint32_t p){return Bytes(w.memory().view(p,4)).u32(0);}
void only(const SourceRuntimeMemory::Checkpoint& a,const SourceRuntimeMemory::Checkpoint& b,std::uint32_t p,unsigned n) {
    check(a.regions.size()==b.regions.size(),"Render flag changed retained topology");bool found=false;
    for(std::size_t i=0;i<a.regions.size();++i){const auto& x=a.regions[i];const auto& y=b.regions[i];
        check(x.address==y.address&&x.bytes.size()==y.bytes.size()&&x.access==y.access,"Render flag changed retained ownership");
        for(std::size_t j=0;j<x.bytes.size();++j){const auto at=std::uint64_t(x.address)+j;
            if(at>=p&&at<std::uint64_t(p)+n)found=true;else check(x.bytes[j]==y.bytes[j],"Render flag wrote outside its original extent");}
    }check(found,"Render flag source extent absent");
}
struct Program {
    ScriptProgram p;
    std::uint32_t node(ScriptNode n){const auto at=std::uint32_t(p.nodes.size());n.salt=std::uint16_t(0xe000+at);p.nodes.push_back(n);return (std::uint32_t(n.salt)<<16)|at;}
    std::uint32_t lit(unsigned type,unsigned bits){return node({0,std::uint16_t(type),std::uint16_t(type),9,null_datum,0,bits});}
    std::uint32_t global(){return node({0,8,8,13,null_datum,0,0});}
    std::uint32_t call(std::string name,std::vector<std::uint32_t> args={},unsigned type=4){
        const auto at=std::uint32_t(p.strings.size());p.strings+=name;p.strings+='\0';
        for(unsigned n=0;n<args.size();++n)p.nodes[args[n]&65535u].next=n+1<args.size()?args[n+1]:null_datum;
        const auto fn=node({0,0,2,9,args.empty()?null_datum:args[0],at,0});return node({0,0,std::uint16_t(type),8,null_datum,at,fn});
    }
    Program(std::string command={},unsigned bad_type=0,bool bad_arity=false) {
        p.globals.push_back({"phase",8,lit(8,0)});
        if(!command.empty()) {
            std::vector<std::uint32_t> args;
            if(!bad_arity) {
                if(command=="object_cinematic_visibility")args.push_back(lit(50,0));
                args.push_back(lit(bad_type?bad_type:5,1));
            }
            p.scripts.push_back({"render_flags",0,4,call(command,args)});return;
        }
        const auto phase=[&](unsigned n){return call("set",{global(),lit(8,n)},8);};
        const auto root=call("begin",{phase(1),call("weapon_enable_warthog_chaingun_light",{lit(5,0)}),
            phase(2),call("object_cinematic_visibility",{lit(50,0),lit(5,0)}),
            phase(3),call("weapon_enable_warthog_chaingun_light",{lit(5,0x10001)}),
            phase(4),call("object_cinematic_visibility",{lit(50,0),lit(5,0x10001)}),phase(5)});
        p.scripts.push_back({"render_flags",0,4,root});
    }
};
}
int main(int argc,char** argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    auto content=cache.load_content(0,false);content.objects.clear();content.scripts=Program{}.p;
    auto source=std::make_shared<CampaignSourceRuntime>(cache,argv[1],0,1,0x12345678u);
    Simulation simulation(content,0,cache.spawns().front(),{},{},{},{},{},{},source);
    auto& objects=source->objects();auto& world=objects.tu15_world();auto& memory=world.memory();
    const auto object=simulation.player().source_datum;check(object!=null_datum,"Real campaign player absent");
    const auto flags=world.source_object_body(object)+4u,latch=word(world,0x4eabc4u)+0x82u;
    check(byte(world,latch)==0,"Original object globals did not initialize the enable latch");
    for(bool enable:{true,false,false,true}) {
        const auto before=memory.checkpoint();source_campaign_warthog_chaingun_light_tu15(world,enable);
        only(before,memory.checkpoint(),latch,1);check(byte(world,latch)==unsigned(!enable),"Warthog disable latch polarity differs");
        const auto before_visibility=memory.checkpoint();const auto old=word(world,flags);
        source_campaign_object_cinematic_visibility_tu15(world,object,enable);only(before_visibility,memory.checkpoint(),flags,4);
        check(word(world,flags)==(enable?old|0x200000u:old&0xffdfffffu),"Cinematic visibility clobbered another flag");
    }
    const auto none_before=memory.checkpoint();source_campaign_object_cinematic_visibility_tu15(world,null_datum,true);
    check(memory.checkpoint()==none_before,"NONE cinematic visibility changed retained state");
    source_campaign_warthog_chaingun_light_tu15(world,false);source_campaign_object_cinematic_visibility_tu15(world,object,true);
    simulation.request_checkpoint();simulation.step({});check(simulation.has_checkpoint(),"Render flag checkpoint missing");
    source_campaign_warthog_chaingun_light_tu15(world,true);source_campaign_object_cinematic_visibility_tu15(world,object,false);
    simulation.request_revert();simulation.step({});
    check(byte(world,latch)==1&&(word(world,flags)&0x200000u),"Render flags did not survive Simulation checkpoint/rebind");
    SourceExecutionInputs recorded;recorded.track_calls();
    {SourceExecutionInputBinding binding(memory,recorded);source_campaign_warthog_chaingun_light_tu15(world,true);source_campaign_object_cinematic_visibility_tu15(world,object,false);}
    const auto inputs=recorded.finish();const auto calls=recorded.calls();
    check(inputs.empty()&&calls.size()==2,"Native flags omitted checked writer journals or manufactured inputs");
    check(calls[0].kind==SourceExecutionCallKind::WriteSourceCallInput&&calls[1].kind==SourceExecutionCallKind::WriteSourceCallInput,
        "Render flags bypassed their checked World publisher");
    const auto before_borrow=memory.checkpoint();
    world.inspect_render_memory([&](const auto&,const auto&){
        source_campaign_object_cinematic_visibility_tu15(world,null_datum,true);
        rejects([&]{source_campaign_warthog_chaingun_light_tu15(world,false);},"Read-only render borrow admitted a light write");
        rejects([&]{source_campaign_object_cinematic_visibility_tu15(world,object,true);},"Read-only render borrow admitted a visibility write");
    });check(memory.checkpoint()==before_borrow,"Rejected borrowed flags mutated retained state");
    source_campaign_object_cinematic_visibility_tu15(world,object,true);
    const int observer_owner{};std::vector<std::pair<unsigned,std::uint32_t>> seen;
    source_runtime_register_write_observer(memory,&observer_owner,[&](auto at,auto n){
        const auto phase=unsigned(simulation.scripts().globals().at(0).number());
        if(phase>=1&&phase<=4&&((at==latch&&n==1)||(at==flags&&n==4)))seen.emplace_back(phase,at);
    });
    simulation.start_level();simulation.step({});source_runtime_unregister_write_observer(memory,&observer_owner);
    for(const auto& d:simulation.scripts().diagnostics())std::cerr<<d<<'\n';
    check(simulation.scripts().diagnostics().empty(),"Render flag script host remains unsupported");
    check(seen==std::vector<std::pair<unsigned,std::uint32_t>>{{1,latch},{2,flags},{3,latch},{4,flags}}&&
        simulation.scripts().globals()[0].number()==5&&byte(world,latch)==0&&(word(world,flags)&0x200000u),
        "Render flag hosts lost VM evaluation order or the resolved player alias");
    for(const auto& command:{std::string("weapon_enable_warthog_chaingun_light"),std::string("object_cinematic_visibility")}) {
        for(const auto [type,arity]:{std::pair(0u,true),std::pair(6u,false)}) {
            content.scripts=Program(command,type,arity).p;Simulation invalid(content,0,cache.spawns().front());
            invalid.start_level();invalid.step({});check(invalid.scripts().diagnostics().size()==1,"Invalid render flag arity/type did not reject");
            check(invalid.scripts().diagnostics().front().find(arity?"arity":"BOOLEAN")!=std::string::npos,"Render flag rejected at the wrong input boundary");
        }
    }
    std::cout<<"PASS exact retained render flags, NONE, checkpoint/rebind, checked writes, read-only refusal, VM ordering and invalid arity/type controls\n";
    std::cout<<"LIMIT authored03a render/frame and function-value consumer acceptance remain separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

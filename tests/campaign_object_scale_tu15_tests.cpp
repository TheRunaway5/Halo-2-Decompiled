#include "halo2/native_tu15_object_scale.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/simulation.hpp"
#include "halo2/original_data.hpp"
#include <bit>
#include <iostream>
using namespace halo2;
namespace {
void check(bool v,const char* message){if(!v)throw FormatError(message);}
template<class F>void rejects(F f,const char* message){bool bad=false;try{f();}catch(const std::exception&){bad=true;}check(bad,message);}
unsigned word(PhysicsLiveWorldTu15& w,std::uint32_t p){return Bytes(w.memory().view(p,4)).u32(0);}
void equal(const SourceRuntimeMemory::Checkpoint& a,const SourceRuntimeMemory::Checkpoint& b) {
    check(a.regions.size()==b.regions.size(),"Object scale changed memory topology");
    for(std::size_t i=0;i<a.regions.size();++i) {
        const auto& x=a.regions[i];const auto& y=b.regions[i];
        if(x.address==0x60000000u)continue; // literal/native caller scratch is intentionally different
        if(x!=y)throw FormatError("Native object scale differs in complete retained region "+x.owner);
    }
}
ScriptProgram program(std::vector<ScriptValue> values) {
    ScriptProgram p;p.strings="object_set_scale";p.strings+='\0';
    p.nodes.push_back({0xe000,77,4,8,null_datum,0,0xe0010001u});
    p.nodes.push_back({0xe001,77,2,9,values.empty()?null_datum:0xe0020002u,0,0});
    for(unsigned i=0;i<values.size();++i)p.nodes.push_back({std::uint16_t(0xe002+i),values[i].type,values[i].type,9,
        i+1<values.size()?(std::uint32_t(0xe003+i)<<16)|(i+3):null_datum,0,values[i].bits});
    p.scripts={{"scale",0,4,0xe0000000u}};return p;
}
}
int main(int argc,char** argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    auto content=cache.load_content(0,false);content.objects.clear();
    const std::vector<ScriptValue> values{{50,0},ScriptValue::numeric(.5,6),{7,30}};
    content.scripts=program(values);
    auto source=std::make_shared<CampaignSourceRuntime>(cache,argv[1],0,1,0x12345678u);
    Simulation simulation(content,0,cache.spawns().front(),{},{},{},{},{},{},source);
    auto& objects=source->objects();auto& world=objects.tu15_world();auto& memory=world.memory();
    const auto object=simulation.player().source_datum;check(object!=null_datum,"Scale fixture lacks a real campaign player");
    const auto scale_at=world.source_object_body(object)+0xa0u;
    check(word(world,scale_at)==0x3f800000,"Original player scale is not one");
    const auto none_before=memory.checkpoint();source_campaign_object_scale_tu15(world,null_datum,0x7f812345,0xffff);
    check(memory.checkpoint()==none_before,"NONE scale changed the World");
    simulation.start_level();simulation.step({});
    for(const auto& d:simulation.scripts().diagnostics())std::cerr<<d<<'\n';
    check(simulation.scripts().diagnostics().empty()&&word(world,scale_at)==0x3f000000,"Real VM scale host failed its player alias/effect");
    simulation.request_checkpoint();simulation.step({});check(simulation.has_checkpoint(),"Scale checkpoint missing");
    source_campaign_object_scale_tu15(world,object,0x3f800000,0);
    simulation.request_revert();simulation.step({});
    check(word(world,scale_at)==0x3f000000,"Scale lost Simulation checkpoint/rebind state");
    const auto before_borrow=memory.checkpoint();
    world.inspect_render_memory([&](const auto&,const auto&){
        source_campaign_object_scale_tu15(world,null_datum,0x3f800000,0);
        rejects([&]{source_campaign_object_scale_tu15(world,object,0x3f800000,0);},"Read-only World borrow admitted scale");
    });check(memory.checkpoint()==before_borrow,"Refused scale borrow mutated memory");
    for(auto invalid:{std::vector<ScriptValue>{},std::vector<ScriptValue>{{50,0},{5,1},{7,30}}}) {
        content.scripts=program(invalid);Simulation bad(content,0,cache.spawns().front());bad.start_level();bad.step({});
        check(bad.scripts().diagnostics().size()==1,"Bad scale arity/type did not reject");
    }
    // Genuine model/attachment storage and both actual service closures are
    // used on each side. A source checkpoint supplies the same initial bank.
    const auto baseline=objects.authority_checkpoint();unsigned compared{};
    for(const auto scale:{0x3f800000u,0x3f000000u,0x40000000u,0u,0xbf800000u,0x7fc12345u}) {
        check(objects.commit_authority(objects.prepare_authority(baseline)),"Source scaler fixture restore failed");
        auto original=world.prepare_source_call(std::array{0u});original.gp[0]=object;original.xmm[3][0]=scale;
        world.run_actual(0xb96b0u,original);const auto expected=memory.checkpoint();
        check(objects.commit_authority(objects.prepare_authority(baseline)),"Native scaler fixture restore failed");
        const auto initial=world.prepare_source_call();const auto control=initial.floating_point.value();
        NativeTu15PhysicsFrameFp fp{control.x87_control_word,initial.x87_status,control.mxcsr};
        const auto service=[&](auto entry,auto args,auto esi,auto& arithmetic) {
            auto abi=world.prepare_source_call(args);abi.gp[6]=esi;abi.x87_status=arithmetic.status;
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{arithmetic.control_word,arithmetic.mxcsr};
            world.run_actual(entry,abi);arithmetic={abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
        };
        NativeTu15ObjectScaleServices services;
        services.interpolate=[&](auto object,auto seconds,auto& arithmetic){service(0xbc380u,std::array{seconds},object,arithmetic);};
        services.refresh=[&](auto object,auto& arithmetic){service(0xbf050u,std::array{object},0u,arithmetic);};
        native_tu15_object_scale(memory,object,scale,0,fp,services,
            [&](auto at,auto bytes){world.write_source_call_input(at,bytes);});
        equal(expected,memory.checkpoint());++compared;
    }
    std::cout<<"PASS "<<compared<<" complete non-stack World banks against original B96B0, real interpolation/bounds, VM/player alias, checkpoint/rebind, NONE, borrow and invalid input controls\n";
    std::cout<<"LIMIT BC380/BF050 remain residual services; authored rendered and Xbox matched-frame acceptance remain open\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

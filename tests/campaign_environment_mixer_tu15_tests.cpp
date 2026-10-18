#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace halo2;using Json=nlohmann::json;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
template<class T>void write(std::ostream& out,const T& value){out.write(reinterpret_cast<const char*>(&value),sizeof(value));check(bool(out),"Mixer export failed");}
Json abi_json(const SourcePhysicsWorldRuntimeAbi& a){return Json{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},
    {"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr}};}
}
int main(int argc,char** argv)try {
    if(argc!=3 && argc!=4)throw FormatError("Supply TU15_XBE 03A_MAP [FRESH_OUTPUT]");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    auto path=(std::filesystem::temp_directory_path()/"halo2-mixer-probe-XXXXXX").string();check(::mkdtemp(path.data()),"Mixer probe storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code ec;std::filesystem::remove_all(path,ec);}}cleanup{path};
    std::cout<<"PHASE ordinary native03a source world\n"<<std::flush;
    const auto output=argc==4?std::filesystem::path(argv[3]):std::filesystem::path(path)/"probe-output";
    check(!std::filesystem::exists(output),"Mixer output must be fresh");std::filesystem::create_directories(output);
    MultiplayerObjectRuntime objects(cache,0,path);auto& world=objects.tu15_world();world.initialize_map();world.configure_campaign_session(1,1,30,0x12345678u);
    const auto word=[&](std::uint32_t p){return Bytes(world.memory().view(p,4)).u32(0);};
    const auto scenario=word(0x4ecc20u),mixer=word(0x4f3010u),rows=word(scenario+0x3c0u),definitions=word(scenario+0x3c4u);
    check(rows==1u && definitions && mixer,"Actual03a authored mixer census changed");const auto name=word(definitions);check(name && name!=0xffffffffu,"Authored mixer lacks its real string ID");
    const auto initial=objects.authority_checkpoint();Json report{{"map","03a_oldmombasa"},{"authored_mixer_rows",rows},{"authored_mixer_name",name},{"cases",Json::array()}};
    bool retain{},retained{};
    const auto keep=[&](const char* label,const SourcePhysicsWorldRuntimeAbi& a){
        const auto directory=output/"active-owner";std::filesystem::create_directories(directory);
        std::ofstream bank(directory/(std::string(label)+".bin"),std::ios::binary);
        world.memory().visit_committed([&](auto p,auto bytes){write(bank,p);write(bank,std::uint32_t(bytes.size()));bank.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());});
        std::ofstream(directory/(std::string(label)+"-abi.json"))<<abi_json(a).dump(2)<<'\n';};
    SourcePhysicsEntryObserverBinding observe(world.memory(),[&](bool before,std::uint32_t entry,const auto& a){
        if(entry!=0x1378e0u || !retain)return;
        if(before){check(!retained,"Mixer oracle endpoint repeated");keep("before",a);}
        else{keep("after",a);retained=true;}});
    for(const bool eased:{false,true}) {
        check(objects.commit_authority(objects.prepare_authority(initial)),"Initial mixer world restore failed");
        // Literal public HS calls2B5705/2B5754: authored name, requested
        // target1, duration1 second. Source137BF0/137C60 sets all live fields;
        // the test never writes a mixer flag, weight, clock or result.
        auto request=world.prepare_source_call(std::array{std::bit_cast<std::uint32_t>(1.f)});
        request.xmm[2][0]=std::bit_cast<std::uint32_t>(1.f);
        if(eased)request.gp[0]=name;else request.gp[3]=name;
        world.run_actual(eased?0x137c60u:0x137bf0u,request);
        check(request.gp[0]==0u && (std::to_integer<unsigned>(world.memory().view(mixer+0x20u,1)[0])&1u),"Original authored mixer request was not activated");
        struct Frame{unsigned ticks;float seconds;};
        for(const auto frame:std::array{Frame{0,0.f},Frame{0,1.f/120.f},Frame{1,1.f/30.f},Frame{3,.1f}}) {
            for(unsigned n=0;n<frame.ticks;++n)world.step_objects();
            const auto before=objects.authority_checkpoint();const auto before_weight=word(mixer+4u);
            SourceExecutionInputs capture;capture.track_calls();std::vector<SourceExecutionInput> inputs;std::vector<SourceExecutionCall> calls;
            retain=!eased && frame.ticks==1u;
            {SourceExecutionInputBinding binding(world.memory(),capture);objects.advance_effect_frame(frame.seconds);}
            const auto sampled=capture.finish();inputs.assign(sampled.begin(),sampled.end());const auto journal=capture.calls();calls.assign(journal.begin(),journal.end());
            retain=false;
            std::vector<std::uint32_t> order;unsigned mixer_calls{};
            for(const auto& call:calls) {
                if(call.kind==SourceExecutionCallKind::CallActualSource) {
                    const auto bytes=Bytes(call.arguments);const auto entry=bytes.u32(0);order.push_back(entry);
                    if(entry==0x1378e0u){++mixer_calls;check(call.arguments.size()==12u && bytes.u32(4)==1u && bytes.u32(8)==std::bit_cast<std::uint32_t>(frame.seconds),"Mixer did not receive exactly the accepted original delta");}
                }else if(call.kind==SourceExecutionCallKind::EffectParticles)order.push_back(0x179df0u);
            }
            check(order==(frame.seconds>0?std::vector<std::uint32_t>{0x17ead0u,0x180520u,0x1378e0u,0x179df0u}:std::vector<std::uint32_t>{0x179df0u}),"Zero/one/multi-tick frame omitted/repeated/reordered an effect owner");
            const auto after_weight=word(mixer+4u);if(!frame.ticks)check(after_weight==before_weight,"No-tick authored fade advanced game-clock weight");
            else check(std::bit_cast<float>(after_weight)>std::bit_cast<float>(before_weight),"Active authored mixer did not evolve after real source ticks");
            const auto after_memory=world.memory().checkpoint();
            check(objects.commit_authority(objects.prepare_authority(before)),"Active mixer replay restore failed");
            {SourceExecutionInputs replay(inputs,calls);{SourceExecutionInputBinding binding(world.memory(),replay);objects.advance_effect_frame(frame.seconds);}(void)replay.finish();}
            check(world.memory().checkpoint()==after_memory,"Accepted-delta source journal replay changed retained world bytes");
            report["cases"].push_back({{"eased",eased},{"ticks",frame.ticks},{"seconds",frame.seconds},{"mixer_calls",mixer_calls},{"weight_before",before_weight},{"weight_after",after_weight},{"complete_native_bank_replay_equal",true}});
            std::cout<<"CASE eased="<<eased<<" ticks="<<frame.ticks<<" delta="<<frame.seconds<<" weight="<<std::bit_cast<float>(after_weight)<<" calls="<<order.size()<<'\n'<<std::flush;
        }
    }
    check(retained,"Actual active1378E0 completed endpoint was not retained");
    report["scope"]="Real ordinary-native03a authored mixer activation by actual public-source request bodies, real source ticks, accepted-delta journal/replay. No live-script scheduling, full13A9B0 or rendered effect claim.";
    std::ofstream(output/"report.json")<<report.dump(2)<<'\n';std::cout<<"PASS8 active authored mixer zero/one/multi-tick frame cases and complete native-bank journal replay; original active-owner comparison pending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

#include "halo2/cache.hpp"
#include "halo2/campaign_frame_clock.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/mcc_script_layout.hpp"
#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/mcc_sound_runtime_delivery.hpp"
#include "halo2/native_mcc_script_builtin.hpp"
#include "halo2/simulation.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#if defined(HALO2_E3_CAMERA_GPU)
#include "native_workshop_e3_camera_gpu.hpp"
#endif

namespace {
using namespace halo2;
using Json=nlohmann::json;
void need(bool value,const char* why){if(!value)throw FormatError(why);}
unsigned word(const SourceRuntimeMemory& memory,unsigned at){return Bytes(memory.view(at,4)).u32(0);}
struct CameraCall {unsigned root,graph,name;std::array<unsigned,4> records;};
// Explicit camera-only test input, confined to the decoded nonconst Cache.
// Production readiness is first checked on the complete unmodified script
// bank. The test then keeps the real camera rows and clears unrelated script
// definitions/global initializers. No source file or production gate changes.
class CameraInput {
    struct Saved {std::span<const std::byte> target;std::vector<std::byte> original;};
    std::vector<Saved> saved_;
    void replace(std::span<const std::byte> target,std::span<const std::byte> value) {
        need(target.size()==value.size(),"Camera fixture field extent differs");
        saved_.push_back({target,{target.begin(),target.end()}});
        std::copy(value.begin(),value.end(),const_cast<std::byte*>(target.data()));
    }
public:
    std::vector<CameraCall> calls;
    std::vector<std::byte> projected;
    unsigned full_requirements{};
    explicit CameraInput(Cache& cache) {
        const auto* tag=cache.find_tag(cache.scenario_tag());need(tag,"E3 camera input lost its scenario");
        const auto metadata=cache.read_tag_bytes(tag->address,0x240);const Bytes scenario(metadata);
        const auto bank=[&](unsigned at,unsigned stride){
            const auto count=scenario.u32(at);return count?cache.read_tag_bytes(scenario.u32(at+4),std::size_t(count)*stride):std::span<const std::byte>{};
        };
        const auto syntax=bank(0x238,20),strings=bank(0x1b0,1),scripts=bank(0x1b8,0x28);
        const auto complete=project_mcc_script_layout(cache,syntax,strings,scripts,{true});
        const NativeMccScriptBuiltinProfile owned(complete,strings,true);
        MccScriptLayout unowned;std::set<std::string> names;
        for(const auto& requirement:complete.requirements)if(!owned.owns(requirement)){
            unowned.requirements.push_back(requirement);names.insert(requirement.name);
        }
        full_requirements=names.size();
        need(full_requirements==22 && names.contains("bitmap_predict") && !names.contains("camera_set_animation"),
            "Private camera profile changed another complete-E3 requirement");
        bool refused=false;try{unowned.require_native_ready();}catch(const FormatError&){refused=true;}
        need(refused,"Complete unmodified E3 unexpectedly passed native script readiness");
        const Bytes text(strings);
        const auto resolve=[&](unsigned datum){
            need(datum!=null_datum && (datum&65535)<syntax.size()/20,"E3 camera operand escapes syntax");
            const Bytes row(syntax.subspan(std::size_t(datum&65535)*20,20));
            need(row.u16(0)==datum>>16 && row.u16(6)!=0xbaba,"E3 camera operand has a stale salt");return row;
        };
        std::vector<std::byte> isolated(syntax.size());
        for(unsigned n=0;n<syntax.size()/20;++n){
            const Bytes root(syntax.subspan(n*20,20));
            if(!root.u16(0)||root.u16(6)==0xbaba||(root.u16(6)&7)||root.u16(2)!=454)continue;
            const auto function=resolve(root.u32(16));const auto string=function.u32(12);
            need((function.u16(6)&1) && function.u16(4)==2 && string<text.size() &&
                text.string(string,text.size()-string)=="camera_set_animation","E3 camera lost its MCC function identity");
            const auto first=resolve(function.u32(8)),second=resolve(first.u32(8));
            need(first.u16(4)==36 && second.u16(4)==11 && second.u32(8)==null_datum &&
                (first.u16(6)&5)==1 && (second.u16(6)&5)==1,"E3 camera no longer has its authored literal arguments");
            CameraCall call{(unsigned(root.u16(0))<<16)|n,first.u32(16),second.u32(16),
                {(unsigned(root.u16(0))<<16)|n,root.u32(16),function.u32(8),first.u32(8)}};
            for(const auto datum:call.records){const auto at=std::size_t(datum&65535)*20;
                std::copy_n(syntax.begin()+at,20,isolated.begin()+at);}
            calls.push_back(call);
        }
        need(calls.size()==55,"E3 native camera fixture lost authored callers");
        const auto layout=project_mcc_script_layout(isolated,strings,{});
        need(layout.native_ready() && layout.converted_calls==55 && layout.rewritten_calls==55,
            "Private authored camera-only fixture did not project to original452");
        for(const auto& call:calls){const auto at=std::size_t(call.root&65535)*20;
            need(Bytes(layout.nodes).u16(at+2)==452,"E3 camera did not select its original descriptor");}
        projected=layout.nodes;
        const std::array<std::byte,4> zero{};
        try {
            replace(metadata.subspan(0x1b8,4),zero);replace(metadata.subspan(0x1c0,4),zero);
            replace(syntax,isolated);
        }catch(...){restore();throw;}
    }
    void restore()noexcept {
        for(auto it=saved_.rbegin();it!=saved_.rend();++it)
            std::copy(it->original.begin(),it->original.end(),const_cast<std::byte*>(it->target.data()));
    }
    bool restored()const {
        return std::ranges::all_of(saved_,[](const auto& saved){return std::ranges::equal(saved.target,saved.original);});
    }
    ~CameraInput(){restore();}
};
ScriptProgram camera_controls(){
    ScriptProgram program;
    const auto node=[&](ScriptNode value){const auto n=unsigned(program.nodes.size());value.salt=std::uint16_t(0xe000+n);
        program.nodes.push_back(value);return(unsigned(value.salt)<<16)|n;};
    const auto call=[&](std::string name,std::vector<unsigned> args){
        const auto text=unsigned(program.strings.size());program.strings+=name;program.strings+='\0';
        for(unsigned n=0;n<args.size();++n)program.nodes[args[n]&65535].next=n+1<args.size()?args[n+1]:null_datum;
        const auto function=node({0,0,2,9,args.empty()?null_datum:args.front(),text,0});
        return node({0,0,4,8,null_datum,text,function});
    };
    const auto cinematic=call("cinematic_start",{}),enabled=node({0,5,5,9,null_datum,0,1});
    const auto camera=call("camera_control",{enabled}),root=call("begin",{cinematic,camera});
    program.scripts.push_back({"explicit_e3_camera_control_fixture",0,4,root});return program;
}
void hs_start(PhysicsLiveWorldTu15& world,std::span<const std::byte> bank,unsigned root) {
    auto& memory=world.memory();
    need(word(memory,0x480c00+452*4)==0x458e90 && word(memory,0x458e94)==0x2b27b0,
        "E3 camera HS descriptor differs from authenticated original452");
    constexpr unsigned fixture=0xb0000000,pool=fixture+0x2000,syntax=fixture+0x2100,
        thread=fixture+0x3000,frame=thread+0x40,nodes=fixture+0x10000,output=fixture+0x1ff000;
    need(bank.size()<output-nodes && bank.size()%20==0,"E3 camera caller exceeds owned HS fixture");
    memory.mount(fixture,std::vector<std::byte>(0x200000),SourceRuntimeMemory::Access::ReadWrite,
        "authored E3 MCC454->452 camera callers; actual native parser/wrapper/effect/completion");
    const auto write=[&](unsigned at,unsigned value,unsigned width=4){std::array<std::byte,4> bytes{};
        for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8*n));world.write_source_call_input(at,std::span(bytes).first(width));};
    std::vector<std::pair<unsigned,std::vector<std::byte>>> saved;
    for(const auto[at,size]:{std::pair(0x505e5cu,4u),std::pair(0x505e60u,1u),std::pair(0x505e64u,4u),std::pair(0x505e6cu,4u)}){
        const auto bytes=memory.view(at,size);saved.emplace_back(at,std::vector(bytes.begin(),bytes.end()));}
    const auto restore=[&]{for(const auto&[at,bytes]:saved)world.write_source_call_input(at,bytes);memory.release(fixture);};
    try {
        world.write_source_call_input(nodes,bank);write(0x505e5c,pool);write(0x505e60,1,1);write(0x505e6c,syntax);
        write(pool+0x44,thread);write(syntax+0x20,unsigned(bank.size()/20));write(syntax+0x24,20);write(syntax+0x44,nodes);
        write(thread+2,1,1);write(thread+3,1,1);write(thread+0xc,0x100,2);write(thread+0x10,frame);
        write(frame,thread+0x18);write(frame+4,root);write(frame+0xc,0,2);write(thread+0x20,output);write(output,0xdeadbeef);
        auto abi=world.prepare_source_call();abi.gp[7]=0xa0010000;const auto before=abi;
        std::set<unsigned> stores;world.run_actual(0x211b10,abi,[&](const auto& store){stores.insert(store.instruction);});
        for(const auto site:{0x2b27e3u,0x16ee58u,0x212096u,0x212117u,0x211e5eu})
            need(stores.contains(site),"E3 camera did not traverse actual HS/parser/wrapper/effect/completion");
        need(word(memory,output)==0 && word(memory,thread+0x10)==thread+0x18 && abi.gp[4]==before.gp[4]+4,
            "E3 camera HS did not complete its actual VOID frame");
        for(unsigned n:{3u,5u,6u,7u})need(abi.gp[n]==before.gp[n],"E3 camera HS changed a nonvolatile register");
        need(abi.x87_depth==before.x87_depth,"E3 camera HS left an unbalanced FPU stack");
    }catch(...){restore();throw;}restore();
}
std::array<unsigned,9> pose_bits(std::span<const std::byte> bytes){
    need(bytes.size()==36,"E3 camera pose extent differs");std::array<unsigned,9> words;
    for(unsigned n=0;n<9;++n){const auto value=Bytes(bytes).f32(n*4);need(std::isfinite(value),"E3 native camera pose is nonfinite");
        words[n]=std::bit_cast<unsigned>(value);}return words;
}
}
int main(int argc,char** argv){
    if(argc!=5){std::cerr<<"Supply MAP SHARED_MAPS PREPARED_ROOT REPORT\n";return 1;}
    Json report={{"scope","Actual E3 camera callers and native HS/observer composition in an explicit camera-only decoded Cache fixture; complete scripts, actors, GPU and missions remain unqualified"},
        {"passed",false},{"calls",Json::array()}};
#if defined(HALO2_E3_CAMERA_DELIVERY_ONLY)
    report["scope"]="Complete unchanged E3 native canonical tag/BSP/sound metadata delivery; no camera-only fixture, native HS/frame/GPU execution or mission admission";
    report["native_HS_executed"]=false;
#if defined(HALO2_E3_CAMERA_FIXTURE_DELIVERY_ONLY)
    report["scope"]="Explicit E3 camera-only decoded Cache fixture, retained full-script refusal, canonical tag/BSP/sound delivery and exact fixture restoration; no native HS/frame/GPU execution or mission admission";
#endif
#endif
    try {
        const auto signature=mcc_script_builtin_signature(454);
        need(signature && signature->xbox_opcode==452 && signature->name=="camera_set_animation",
            "E3 camera consumer requires its authenticated admitted declaration");
        Cache cache(argv[1],argv[2],{},argv[3]);need(cache.is_mcc(),"E3 camera consumer requires MCC content");
#if defined(HALO2_E3_CAMERA_DELIVERY_ONLY)
        std::optional<CameraInput> fixture_input;
#if defined(HALO2_E3_CAMERA_FIXTURE_DELIVERY_ONLY)
        fixture_input.emplace(cache);
        report["complete_script_requirements_retained"]=fixture_input->full_requirements;
        report["fixture_authored_camera_calls"]=fixture_input->calls.size();
#endif
        const auto* tag=cache.find_tag(cache.scenario_tag());need(tag,"E3 delivery lost its scenario");
        const Bytes scenario(cache.read_tag_bytes(tag->address,0x240));
        const auto nodes=cache.read_tag_bytes(scenario.u32(0x23c),std::size_t(scenario.u32(0x238))*20);
        const std::vector<std::byte> original(nodes.begin(),nodes.end());
        unsigned captures{};std::optional<MccSoundRuntimePlan> sound;
        const auto delivery=MccRuntimeCacheDelivery::plan(cache,0x90000000u,0x60000000u,
            [&](MccSoundRuntimePlan&& value){++captures;sound.emplace(std::move(value));});
        report["issues"]=Json::array();report["issue_count"]=delivery.issues().size();
        for(const auto& issue:delivery.issues())if(report["issues"].size()<128)
            report["issues"].push_back({{"tag",issue.tag.value},{"field",issue.field},{"reason",issue.reason}});
        need(delivery.issues().empty(),"Complete E3 native canonical delivery has unresolved schema/ownership issues");
        need(captures==1 && sound.has_value(),"E3 canonical delivery did not transfer its actual sound metadata owner exactly once");
        const auto image=delivery.tag_image();
        const auto read=[&](unsigned address,std::size_t size){
            for(const auto& region:image.regions)if(address>=region.address &&
                std::uint64_t(address)-region.address+size<=region.bytes.size())
                return Bytes(region.bytes).slice(address-region.address,size);
            throw FormatError("E3 canonical camera metadata has no complete published region");
        };
        const Bytes runtime_scenario(read(delivery.runtime_address(tag->address,0x240),0x240));
        need(runtime_scenario.u32(0x238)==scenario.u32(0x238) &&
            runtime_scenario.u32(0x1b8)==scenario.u32(0x1b8) && runtime_scenario.u32(0x1c0)==scenario.u32(0x1c0),
            "Canonical E3 delivery changed complete authored script/global bank extents");
        need(std::ranges::equal(nodes,read(runtime_scenario.u32(0x23c),nodes.size())),
            "Canonical E3 delivery changed authored syntax before explicit opcode projection");
        report["complete_script_bank_bytes_verified"]=Json::object();
        for(const auto[field,stride]:{std::pair(0x1b0u,1u),std::pair(0x1b8u,0x28u),std::pair(0x1c0u,0x28u),std::pair(0x238u,20u)}){
            const auto count=scenario.u32(field);need(runtime_scenario.u32(field)==count,"Canonical E3 script bank count changed");
            const auto size=std::size_t(count)*stride;
            if(size)need(std::ranges::equal(cache.read_tag_bytes(scenario.u32(field+4),size),read(runtime_scenario.u32(field+4),size)),
                "Canonical E3 delivery changed an authored string/script/global/syntax bank");
            report["complete_script_bank_bytes_verified"][std::to_string(field)]=size;
        }
        unsigned bsps{};std::size_t bsp_bytes{};
        for(std::size_t n=0;n<cache.bsps().size();++n){const auto bsp=delivery.bsp_image(n);
            need(!bsp.region.bytes.empty(),"E3 native delivery lost an authored BSP metadata owner");++bsps;bsp_bytes+=bsp.region.bytes.size();}
        need(std::ranges::equal(original,cache.read_tag_bytes(scenario.u32(0x23c),nodes.size())),
            "Canonical E3 delivery mutated its borrowed authored syntax");
        report["source_syntax_nodes"]=nodes.size()/20;report["complete_script_count"]=scenario.u32(0x1b8);
        report["complete_global_count"]=scenario.u32(0x1c0);report["canonical_tags"]=image.count;
        report["native_reservation_bytes"]=delivery.reservation_bytes();report["canonical_regions"]=image.regions.size();
        report["canonical_sound_definitions"]=sound->sounds.size();report["canonical_BSPs"]=bsps;
        report["canonical_BSP_bytes"]=bsp_bytes;report["camera_only_fixture_used"]=fixture_input.has_value();
        if(fixture_input){fixture_input->restore();need(fixture_input->restored(),"Camera-only canonical fixture did not restore its original decoded source");
            report["decoded_scenario_restored"]=true;}
        report["passed"]=true;
#else
        CameraInput input(cache);report["complete_script_requirements_retained"]=input.full_requirements;
        {
            auto content=cache.load_content(0,false);report["authored_object_count"]=content.objects.size();
            // Actor/mission flow is outside this camera fixture. The registered
            // player, native map/world and camera resources still belong to E3.
            content.objects.clear();content.scripts=camera_controls();const auto spawns=cache.spawns();
            need(!spawns.empty(),"E3 camera consumer has no authored player spawn");
            auto source=std::make_shared<CampaignSourceRuntime>(cache,0,1,0x12345678);
            Simulation simulation(content,0,spawns.front(),[&](auto g,auto n){return cache.load_named_animation(g,n);},
                {},{},{},{},{},source);
            auto& world=source->objects().tu15_world();auto& memory=world.memory();
            const auto users=word(memory,0x4f54f0u);need(users && word(memory,users+12)!=null_datum,"E3 has no registered native camera player");
            simulation.start_level();simulation.step({});
            need(simulation.scripts().diagnostics().empty() && simulation.presentation().scripted && simulation.presentation().cinematic,
                "E3 camera fixture did not enable actual native camera/cinematic presentation");
            source_campaign_camera_field_of_view_tu15(world,60,0);
#if defined(HALO2_E3_CAMERA_GPU)
            E3CameraGpuConsumer gpu(cache,world);
#endif
            unsigned zero{},one{},many{},motion{},frames{},valid_clusters{};
            for(const auto& call:input.calls){
                report["current_root"]=call.root;report["current_name"]=cache.string_id(call.name);
                std::cout<<"PHASE E3 camera root="<<call.root<<" name="<<cache.string_id(call.name)<<'\n'<<std::flush;
                hs_start(world,input.projected,call.root);const auto director=word(memory,0x51df6cu);
                need(word(memory,director+0x44)==call.graph && word(memory,director+0x48)==call.name &&
                    Bytes(memory.view(director+0x4c,2)).u16(0)==0xffff && word(memory,director+0x3c)==null_datum,
                    "E3 camera HS changed authored graph/name or retained a relative object/flag");
                const auto remaining=source_campaign_camera_time_tu15(world);need(remaining>0,"E3 camera start retained no real clip time");
                Json record={{"root",call.root},{"graph",call.graph},{"name",cache.string_id(call.name)},
                    {"name_id",call.name},{"initial_remaining",remaining},{"frames",Json::array()}};
                std::optional<std::array<unsigned,9>> first;bool moved=false;
                unsigned frame_index{};
                for(const float raw:std::array{0.f,1.f/120.f,1.f/30.f,.125f,0.f,1.f/30.f}) {
                    const auto incoming=source_campaign_frame_interval_tu15(raw);
                    const auto accepted=simulation.advance_frame_time(incoming,false);
                    for(int n=0;n<accepted.ticks;++n)simulation.step({});
                    simulation.advance_cinematic_render_time(accepted.elapsed,accepted.ticks,accepted.ticks);
                    simulation.advance_cinematic_camera(incoming);
                    const auto view=source_campaign_camera_view_tu15(memory,0);need(view.has_value(),"E3 camera has no registered native observer output");
                    const auto scratch=world.prepare_source_call().gp[4]+0x100u;const std::array<std::byte,56> clear{};
                    world.write_source_call_input(scratch,clear);auto abi=world.prepare_source_call(std::array{scratch,scratch+52u});
                    world.run_actual(0x16fe00u,abi);need(abi.gp[0]&255,"Native sampler rejected an actual E3 camera clip");
                    const auto sample=memory.view(scratch,52);std::array<std::byte,36> expected{},produced{},observed{};
                    std::memcpy(expected.data(),sample.data()+40,12);std::memcpy(expected.data()+12,sample.data()+4,12);std::memcpy(expected.data()+24,sample.data()+28,12);
                    const auto command=word(memory,0x4f64a8u);
                    for(const auto[to,from]:{std::pair(0u,4u),std::pair(12u,0x2cu),std::pair(24u,0x38u)})
                        std::memcpy(produced.data()+to,memory.view(command+from,12).data(),12);
                    need(produced==expected,"E3 native camera command differs from its actual sampler pose");
                    const std::array vectors{view->position,view->forward,view->up};std::memcpy(observed.data(),vectors.data(),36);
                    const auto native_pose=pose_bits(produced),observer=pose_bits(observed);
                    if(!first)first=native_pose;else moved|=native_pose!=*first;
                    record["frames"].push_back({{"raw",raw},{"ticks",accepted.ticks},{"native_pose",native_pose},{"observer_pose",observer}});
                    ++frames;zero+=accepted.ticks==0;one+=accepted.ticks==1;many+=accepted.ticks>1;
#if defined(HALO2_E3_CAMERA_GPU)
                    gpu.sample(call.root,frame_index);
#endif
                    ++frame_index;
                }
                const auto cluster=world.call_actual(0x16fd40u);record["cluster"]=cluster;valid_clusters+=(cluster&65535)!=65535;
                hs_start(world,input.projected,call.root);
                need(source_campaign_camera_time_tu15(world)==remaining,"E3 camera HS restart lost its initial clip time");
                record["restart_passed"]=true;record["motion"]=moved;report["calls"].push_back(std::move(record));motion+=moved;
            }
            need(frames==330 && zero && one && many && motion,"E3 native camera coverage lost frames, tick regimes or motion");
            report["frames"]=frames;report["HS_calls"]=110;report["zero_tick_frames"]=zero;report["one_tick_frames"]=one;
            report["multiple_tick_frames"]=many;report["moving_calls"]=motion;report["calls_with_BSP0_cluster"]=valid_clusters;
#if defined(HALO2_E3_CAMERA_GPU)
            report["GPU"]=gpu.finish();
#endif
        }
        input.restore();need(input.restored(),"Camera-only decoded Cache fixture did not restore its original metadata");
        report["decoded_scenario_restored"]=true;report["native_HS_executed"]=true;report["passed"]=true;
#endif
    }catch(const std::exception& error){report["error"]=error.what();std::cerr<<error.what()<<'\n';}
    std::ofstream(argv[4])<<report.dump(2)<<'\n';
    if(report["passed"]==true)std::cout<<"PASS E3 native camera prerequisite/consumer scope: "<<report["scope"]<<'\n';
    return report["passed"]==true?0:1;
}

#include "halo2/source_campaign_screen_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/simulation.hpp"
#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;using Json=nlohmann::json;
namespace {
void check(bool value,const char *reason){if(!value)throw FormatError(reason);}
template<class F>void rejects(F f,const char *reason){bool failed{};try{f();}catch(const std::exception &){failed=true;}check(failed,reason);}
std::vector<std::byte> unhex(const std::string &s){check(!(s.size()%2),"Invalid oracle hex extent");std::vector<std::byte>b;for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return b;}
unsigned word(const SourceRuntimeMemory &m,unsigned p){return Bytes(m.view(p,4)).u32(0);}
void put(SourceRuntimeMemory &m,unsigned p,unsigned v){m.write(p,std::as_bytes(std::span(&v,1)));}
constexpr unsigned owner=0x9003f000,sp=0x60008000,stop=0x70000000;
void mount(SourceRuntimeMemory &m,unsigned p,unsigned n){m.mount(p,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,"declared CPU DoF oracle caller backing");}
std::array<unsigned,7> args(const Json &j){return j.get<std::array<unsigned,7>>();}
void call(SourceRuntimeMemory &m,const std::array<unsigned,7> &v){
 SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=sp;a.xmm[0][0]=v[0];a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
 put(m,sp,stop);m.write(sp+4,std::as_bytes(std::span(v).subspan<1>()));
 source_run_campaign_screen_tu15(m,0x1fc10,a,*a.floating_point,{});
 check(a.gp[4]==sp+28,"Complete original DoF setter changed RET24 ABI");
}
void compare(std::span<const std::byte> a,const std::vector<std::byte>&b,const char *reason){check(std::ranges::equal(a,b),reason);}
void pure(const Json &r){
 SourceRuntimeMemory m;mount(m,0x515ec0,4);mount(m,0x4920c0,8);mount(m,owner,0x3fc);mount(m,0x60000000,65536);
 for(const auto *group:{"wrapper_cases","direct_cases"})for(const auto &c:r.at(group)){
  put(m,0x515ec0,group==std::string("direct_cases")&&!c.at("owner").get<bool>()?0:owner);m.write(owner,unhex(c.at("before")));m.write(0x4920c0,unhex(c.at("clock_hex")));call(m,args(c.at("arguments")));compare(m.view(owner,0x3fc),unhex(c.at("after")),"Native complete1FC10 raw bank differs from unchanged original");
 }
 for(const auto &[p,n]:std::array<std::pair<unsigned,unsigned>,5>{{{0x4920b8,4},{0x491e22,2},{0x491e68,8},{0x4c686c,4},{0x4920d8,4}}})mount(m,p,n);
 put(m,0x515ec0,owner);m.write(owner+0xac,std::array{std::byte{1}});
 for(const auto &c:r.at("admission_cases")){
  const auto special=c.at("special").get<std::uint16_t>();m.write(0x491e22,std::as_bytes(std::span(&special,1)));put(m,0x4920b8,c.at("mode"));put(m,0x4c686c,c.at("views"));
  const auto rectangle=c.at("rectangle").get<std::array<std::uint16_t,4>>();m.write(0x491e68,std::as_bytes(std::span(rectangle)));m.write(owner+0x3c,std::array{std::byte(c.at("active").get<bool>())});put(m,owner+0x40,c.at("focus_word"));
  const auto f=source_campaign_depth_of_field_frame_tu15(m,c.at("modifier"),c.at("ready"));check(f.enabled==c.at("enabled").get<bool>(),"Native DoF admission differs from original13671");
 }
 put(m,0x515ec0,0);check(!source_campaign_depth_of_field_frame_tu15(m,false,true).enabled,"Null screen bank admitted DoF");put(m,0x515ec0,owner);m.write(owner+0xac,std::array{std::byte{0}});check(!source_campaign_depth_of_field_frame_tu15(m,false,true).enabled,"Inactive1FD00 screen bank admitted stale DoF output");
 for(const auto &c:r.at("depth_cases")){
  const auto f=[](const Json &v){return std::bit_cast<float>(v.get<unsigned>());};const auto plan=source_campaign_depth_of_field_scenes_tu15(f(c.at("near_word")),f(c.at("far_word")),f(c.at("focus_word")),c.at("current_target"));const auto &p=plan.at(c.at("mode").get<unsigned>()-1);
  check(p.mode==c.at("mode")&&p.target==c.at("selected_target")&&std::bit_cast<unsigned>(p.minimum_w)==c.at("minimum")&&std::bit_cast<unsigned>(p.maximum_w)==c.at("maximum"),"Native DoF scene W/target recipe differs from original13430");
 }
 std::cout<<"PASS64 authored+70 raw/null whole-setter banks,16 source admission boundaries and10 ordered W/target recipe controls\n";
}
ScriptProgram one_call(std::vector<ScriptValue> values){
 const std::string name="cinematic_screen_effect_set_depth_of_field";ScriptProgram p;p.strings=name+'\0';p.nodes.push_back({0xe000,706,4,8,null_datum,0,0xe0010001u});p.nodes.push_back({0xe001,706,2,9,values.empty()?null_datum:0xe0020002u,0,0});
 for(unsigned i=0;i<values.size();++i)p.nodes.push_back({std::uint16_t(0xe002+i),values[i].type,values[i].type,9,i+1<values.size()?(std::uint32_t(0xe003+i)<<16)|(i+3):null_datum,0,values[i].bits});
 p.scripts={{"dof_bad",0,4,0xe0000000u}};return p;
}
}
int main(int argc,char **argv)try{
 if(argc!=4)throw FormatError("Supply TU15_XBE INTRO_MAP ORIGINAL_DOF_JSON");
 for(int i=1;i<4;++i)if(!std::filesystem::is_regular_file(argv[i])){std::cout<<"SKIP optional original DoF assets/oracle; see tests/fixtures/retail_cinematic_dof_tu15.md\n";return 77;}
 Json oracle;std::ifstream(argv[3])>>oracle;
 check(oracle.at("status")=="PASS_UNCHANGED_DOF_AUTHORED64_PARSER_RAW_SETTER_UPDATE_ADMISSION_AND_DEPTH"&&oracle.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Present DoF oracle failed authentication/status");
 check(oracle.at("wrapper_cases").size()==64&&oracle.at("direct_cases").size()==70&&oracle.at("update_cases").size()==36&&oracle.at("admission_cases").size()==16&&oracle.at("depth_cases").size()==10,"Present DoF oracle incomplete");pure(oracle);
 Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));check(cache.name()=="00a_introduction","Authored native DoF host requires Introduction");auto content=cache.load_content(0,false);
 auto source=std::make_shared<CampaignSourceRuntime>(cache,std::filesystem::path(argv[1]),0,1,0x12345678);auto &objects=source->objects();auto &world=objects.tu15_world();auto &m=world.memory();const auto p=word(m,0x515ec0);check(p!=0,"Native source interface bank missing");
 for(unsigned off=0xd8;off<0xfc;off+=4)check(word(m,p+off)==0,"Actual interface constructor did not clear DoF control state");
 const auto initial=objects.authority_checkpoint();
 for(const auto &c:oracle.at("update_cases")){
  world.write_source_call_input(p,unhex(c.at("before")));world.write_source_call_input(0x4920c0,unhex(c.at("clock_hex")));source_campaign_screen_update_tu15(world);compare(m.view(p,0x3fc),unhex(c.at("after")),"Native ordinary-world whole1FD00 bank differs from unchanged original");
 }
 check(objects.commit_authority(objects.prepare_authority(initial)),"DoF source fixture reset failed");
 const std::array<unsigned,7> authored{0x3fc00000,0x3f800000,0x3f800000,0,0,0,0};
 source_campaign_screen_clock_tu15(m,0.);source_campaign_depth_of_field_tu15(world,authored);const auto timed=std::vector<std::byte>(m.view(p+0xd8,0x24).begin(),m.view(p+0xd8,0x24).end());
 source_campaign_screen_start_tu15(m,true);compare(m.view(p+0xd8,0x24),timed,"Screen prefix reset changed retained DoF controls");source_campaign_screen_update_tu15(world);check(m.view(p+0x3c,1)[0]==std::byte{1}&&word(m,p+0x40)==authored[0]&&word(m,p+0x44)==authored[1],"Native DoF controls did not reach actual screen output");
 source_campaign_cinematic_control_tu15(world,true);compare(m.view(p+0xd8,0x24),timed,"Cinematic start incorrectly cleared DoF timing");source_campaign_screen_stop_tu15(world);check(std::ranges::all_of(m.view(p,0x3fc),[](auto b){return b==std::byte{};}),"Actual screen stop did not clear complete DoF bank");
 source_campaign_depth_of_field_tu15(world,authored);source_campaign_cinematic_control_tu15(world,false);for(unsigned off=0xd8;off<0xfc;off+=4)check(word(m,p+off)==0,"Actual cinematic stop did not clear DoF controls");
 const auto saved=objects.authority_checkpoint();SourceExecutionInputs capture;capture.track_calls();{SourceExecutionInputBinding binding(m,capture);source_campaign_depth_of_field_tu15(world,authored);}const auto inputs=capture.finish();const auto calls=capture.calls();
 check(inputs.empty()&&calls.size()==2&&calls[0].kind==SourceExecutionCallKind::PrepareSourceCall&&calls[1].kind==SourceExecutionCallKind::RunActualSourceCall&&Bytes(calls[1].arguments).u32(0)==0x1fc10,"Native DoF omitted exact prepare+actual-call transcript");const auto completed=m.checkpoint();
 check(objects.commit_authority(objects.prepare_authority(saved)),"DoF replay source reset failed");SourceExecutionInputs replay(inputs,calls);{SourceExecutionInputBinding binding(m,replay);source_campaign_depth_of_field_tu15(world,authored);}replay.finish();check(m.checkpoint()==completed,"DoF replay changed complete retained bank");
 check(objects.commit_authority(objects.prepare_authority(saved)),"DoF wrong-input source reset failed");auto wrong_args=authored;wrong_args[1]^=1;SourceExecutionInputs wrong(inputs,calls);{SourceExecutionInputBinding binding(m,wrong);rejects([&]{source_campaign_depth_of_field_tu15(world,wrong_args);},"Wrong DoF recorded input admitted");}rejects([&]{wrong.finish();},"Wrong DoF replay was publishable");check(word(m,p+0xd8)==0,"Wrong DoF replay changed control bank before admission");
 world.inspect_render_memory([&](const auto &,const auto &){rejects([&]{source_campaign_depth_of_field_tu15(world,authored);},"Read-only renderer borrower accepted DoF command");});
 const auto &call_node=content.scripts.nodes.at(0x1dbe),&fn=content.scripts.nodes.at(0x1dbf);check(call_node.salt==0x8132&&call_node.opcode==706&&call_node.value==0x81331dbfu&&fn.salt==0x8133,"Actual authored Introduction DoF call identity differs");
 ScriptProgram isolated;isolated.strings=content.scripts.strings;isolated.nodes.resize(0x1dc7);for(unsigned i=0x1dbe;i<0x1dc7;++i)isolated.nodes[i]=content.scripts.nodes.at(i);isolated.nodes[0x1dbe].next=null_datum;
 for(unsigned i=0;i<7;++i)check(isolated.nodes[0x1dc0+i].salt==0x8134+i&&isolated.nodes[0x1dc0+i].type==6&&isolated.nodes[0x1dc0+i].value==authored[i],"Actual authored seven-REAL DoF arguments differ");isolated.scripts={{"dof_authored",0,4,0x81321dbeu}};content.objects.clear();content.scripts=std::move(isolated);
 Simulation simulation(content,0,cache.spawns().front(),{},{},{},{},{},{},source);simulation.start_level();simulation.step({});for(const auto &d:simulation.scripts().diagnostics())std::cerr<<"SCRIPT "<<d<<'\n';check(simulation.scripts().diagnostics().empty(),"Authored campaign DoF host was not dispatched");check(word(m,p+0xd8)==authored[0]&&word(m,p+0xdc)==authored[1]&&word(m,p+0xe0)==authored[2],"Actual seven-REAL DoF host did not reach source bank");
 simulation.request_checkpoint();simulation.step({});check(simulation.has_checkpoint(),"Owning DoF Simulation checkpoint missing");source_campaign_depth_of_field_tu15(world,std::array<unsigned,7>{});simulation.request_revert();simulation.step({});check(word(m,p+0xd8)==authored[0]&&word(m,p+0xdc)==authored[1],"Owning Simulation restore lost DoF controls");
 std::vector<ScriptValue> values(6,ScriptValue::numeric(1.f,6));content.scripts=one_call(values);Simulation invalid(content,0,cache.spawns().front());invalid.start_level();invalid.step({});check(invalid.scripts().diagnostics().size()==1&&invalid.scripts().diagnostics()[0]=="dof_bad: builtin 'cinematic_screen_effect_set_depth_of_field': invalid cinematic_screen_effect_set_depth_of_field arity","Invalid DoF arity did not reject at exact boundary");
 values.push_back(ScriptValue::numeric(1.f,6));content.scripts=one_call(values);Simulation missing(content,0,cache.spawns().front());missing.start_level();missing.step({});check(missing.scripts().diagnostics().size()==1&&missing.scripts().diagnostics()[0]=="dof_bad: builtin 'cinematic_screen_effect_set_depth_of_field': Cinematic depth of field requires its retained TU1.5 world","Missing DoF owner did not reject at exact boundary");
 std::cout<<"PASS36 whole original update banks, actual00a seven-REAL authored host, source start/prefix/stop, record/replay/wrong-input/guard and source+Simulation restores\nLIMIT private source/recipe controls only; Vulkan near/far scene targets, original filter/composite and matched-frame proof remain required\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

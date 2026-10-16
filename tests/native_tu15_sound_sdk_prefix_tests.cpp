#include "halo2/native_tu15_audio_preference.hpp"
#include "halo2/native_tu15_sound_sdk_prefix_data.hpp"
#include "halo2/source_runtime_sound_dma_resources.hpp"
#include "halo2/source_runtime_sound_sdk_resources.hpp"
#include "halo2/source_runtime_sound_reset_resources.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#include "halo2/source_sound_startup_tu15.hpp"
#include "halo2/source_startup_sound_storage_tu15.hpp"
#include "halo2/source_render_object_startup_tu15.hpp"
#include "halo2/source_sound_hardware_storage_tu15.hpp"
#include "halo2/source_sound_dma_storage_tu15.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace halo2;
namespace {
using Json=nlohmann::json;
constexpr std::uint32_t stack=0x2008000,output_cell=0x2001000,stop=0x2100000,fs=0x60200000;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void put(SourceRuntimeMemory& memory,std::uint32_t address,std::uint32_t value){
    std::array<std::byte,4> raw{};for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(i*8));memory.write(address,raw);
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address){
    const auto raw=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);return value;
}
std::vector<std::byte> unhex(const std::string& text){
    check(text.size()%2==0,"Malformed original SDK byte input");
    auto digit=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};std::vector<std::byte> raw(text.size()/2);
    for(std::size_t i=0;i<raw.size();++i)raw[i]=std::byte((digit(text[i*2])<<4)|digit(text[i*2+1]));return raw;
}
template<class F>void rejected(F action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}check(failed,"Partial sound SDK state was accepted");}
void compare_abi(const SourcePhysicsWorldRuntimeAbi& abi,const Json& expected){
    check(abi.gp==expected.at("returned_gp").get<decltype(abi.gp)>(),"Original/native SDK registers differ");
    check((abi.eflags&0xcc5)==(expected.at("returned_eflags").get<std::uint32_t>()&0xcc5),"Original/native SDK defined flags differ");
    check(abi.fs_base==fs&&abi.x87_depth==0&&abi.x87_status==0&&abi.floating_point->x87_control_word==0x27f&&abi.floating_point->mxcsr==0x1f80,
        "SDK prefix changed unrelated execution/FP context");
}
struct ResetBoundary {};
void import(SourceRuntimeMemory& memory,std::uint32_t address,std::uint32_t value,bool readonly=true){
    std::array<std::byte,4> raw{};for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8*i));
    memory.mount(address,raw,readonly?SourceRuntimeMemory::Access::ReadOnly:SourceRuntimeMemory::Access::ReadWrite,
        "actual prior startup immutable import owner");
}
void data_ownership_rejections(){
    {SourceRuntimeMemory memory;import(memory,0x41b67c,0x80000125);rejected([&]{mount_native_tu15_sound_sdk_prefix_data(memory);});
     check(word(memory,0x41b67c)==0x80000125,"Rejected SDK loader replaced another owner's import bytes");}
    {SourceRuntimeMemory memory;import(memory,0x41b680,0x80000115,false);rejected([&]{mount_native_tu15_sound_sdk_prefix_data(memory);});
     check(!memory.is_read_only(0x41b680,4),"Rejected SDK loader rewrote another owner's access policy");}
    {SourceRuntimeMemory memory;mount_native_tu15_sound_sdk_prefix_data(memory);rejected([&]{mount_native_tu15_sound_sdk_prefix_data(memory);});
     check(word(memory,0x3912a0)==0&&word(memory,0x3912a4)==0,"SDK loader preset a prior CRT constructor binding");}
}
Json run(const Json& expected,const std::shared_ptr<SdlSoundPlatform>& output,const std::filesystem::path& config){
    const bool driver=expected.value("driver",false),through_reset=expected.value("through_reset",false),through_storage=expected.value("through_storage",false);
    SourceRuntimeMemory memory;NativeRuntimeAddress prior_enter{},prior_leave{},prior_physical{};
    if(expected.at("fill")!=0) {
        import(memory,0x41b67c,0x80000126);import(memory,0x41b680,0x80000115);import(memory,0x41b640,0x800000a6);
        prior_leave=memory.native_address(0x41b67c,4);prior_enter=memory.native_address(0x41b680,4);prior_physical=memory.native_address(0x41b640,4);
    }
    mount_native_tu15_sound_sdk_prefix_data(memory);mount_native_tu15_sound_dma_imports(memory);
    if(driver) {
        mount_native_tu15_startup_sound_storage_data(memory);
        import(memory,0x41b6d4,0x800000b8);
        memory.mount(0x56e225,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,"source loader-zero CRC table flag");
        memory.mount(0x56e258,std::vector<std::byte>(0x400),SourceRuntimeMemory::Access::ReadWrite,"source loader-zero CRC table destination");
    }
    if(prior_enter.value)check(memory.native_address(0x41b67c,4)==prior_leave&&memory.native_address(0x41b680,4)==prior_enter&&
        memory.native_address(0x41b640,4)==prior_physical,"SDK data mount replaced prior immutable import owners");
    memory.mount(0x2000000,std::vector<std::byte>(0x20000,std::byte(expected.at("fill").get<unsigned>())),
        SourceRuntimeMemory::Access::ReadWrite,"declared SDK caller stack/output state");
    memory.mount(fs,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"native passive sound executor context");
    put(memory,output_cell,0);
    NativeTu15AudioPreference preference(memory,output,native_tu15_load_audio_preference(config));
    check(preference.flags()==0&&preference.output_device()==output->device_id(),"SDK preference lost its actual output");
    preference.publish();
    SourceRuntimeSoundSdkResources resources(memory,fs);resources.require_quiescent();
    unsigned foreign_rejections=0;bool foreign_quiescence=false;
    std::thread foreign([&]{
        foreign_quiescence=!resources.quiescent()&&!preference.quiescent()&&!preference.published();
        auto refused=[&](auto action){try{action();}catch(const std::exception& error){
            if(std::string(error.what()).find("owner thread")!=std::string::npos)++foreign_rejections;}};
        refused([&]{resources.live_allocations();});refused([&]{resources.lock_depth();});
        refused([&]{SourceRuntimeSoundSdkResources::CallScope scope(&resources);});
        SourcePhysicsWorldRuntimeAbi other{};other.gp[4]=0xffffffffu;
        refused([&]{resources.dispatch(memory,{0x387975,0x8000000f,false},other);});
        refused([&]{resources.dispatch(memory,{0x2e101c,0x80000018,true},other);});
    });foreign.join();check(foreign_quiescence&&foreign_rejections==5,"Foreign SDK ownership checks did not reject before mutable state");
    auto dma=std::make_unique<SourceRuntimeSoundDmaResources>(memory,output);
    check(dma->quiescent()&&dma->device_id()==preference.output_device(),"SDK DMA/output identity differs");
    std::unique_ptr<SourceRuntimeSoundResetResources> reset;
    if(through_reset)reset=std::make_unique<SourceRuntimeSoundResetResources>(memory,output,fs);
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=expected.at("initial_gp").get<decltype(abi.gp)>();abi.fs_base=fs;abi.eflags=0x202;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    Json instructions=Json::array(),stores=Json::array(),platform=Json::array(),wide=Json::array(),dma_platform=Json::array(),dma_wide=Json::array();
    Json raw_allocations=Json::array();
    SourcePhysicsPoolHostStorage arena_owner(memory,0x10000000,[&](const auto& event){
        check(event.allocation,"Unexpected sound arena free in SDK prefix");
        raw_allocations.push_back({{"source",event.result[1]},{"requested",event.request[2]},{"bytes",event.result[2]}});
    });
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){
        check(pc!=0x37da9a&&pc!=0x33c29d&&pc!=0x2e101c,"Native explicit preference entered console setting/hardware readers");
        check((through_reset||pc!=0x38ec0e)&&pc!=0x38a37c&&(through_storage||pc!=0x38e996)&&pc!=0x38e8c9,
            "SDK prefix bypassed its current required hardware frontier");instructions.push_back(pc);
    };
    SourcePhysicsContactRecordObserver observer=[&](const auto& store){
        check(store.written.size()<=8,"Unexpected SDK CPU store width");std::uint64_t value=0;
        for(std::size_t i=0;i<store.written.size();++i)value|=std::uint64_t(std::to_integer<unsigned>(store.written[i]))<<(8*i);
        stores.push_back({store.instruction,store.address,store.written.size(),value});
    };
    if(driver) {
        source_prepare_native_tu15_sound_arena(memory,abi,*abi.floating_point,services,arena_owner,observer);
        compare_abi(abi,expected.at("arena_prefix"));
        check(instructions==expected.at("arena_prefix").at("instructions")&&stores==expected.at("arena_prefix").at("source_stores"),
            "Original main sound arena prerequisite instruction/store order differs");
        check(raw_allocations.size()==1&&raw_allocations[0]["source"]==0x10000000&&raw_allocations[0]["requested"]==0x23000&&
            raw_allocations[0]["bytes"]==0x23000&&word(memory,0x51db00)==0x10000000&&!word(memory,0x51db04)&&!word(memory,0x51db08),
            "Original main prefix did not construct its actual native sound arena");
        instructions=Json::array();stores=Json::array();
    }
    for(const auto& prior:expected.at("prior")){
        put(memory,stack,stop);abi.gp[4]=stack;
        source_run_sound_sdk_tu15(memory,prior.at("entry"),abi,*abi.floating_point,services,observer);
        compare_abi(abi,prior);check(instructions==prior.at("instructions")&&stores==prior.at("source_stores"),"Original SDK CRT statistics constructor differs");
        instructions=Json::array();stores=Json::array();
    }
    check(word(memory,0x3912a4)==0x390bc0&&word(memory,0x3912a0)==0x390bc4,"Genuine SDK CRT statistics bindings were not constructed");
    if(driver){put(memory,stack,0x128055);put(memory,stack+4,0x455b20);}
    else{put(memory,stack,stop);put(memory,stack+4,0);put(memory,stack+8,output_cell);put(memory,stack+12,0);}
    abi.gp[4]=stack;
    check(abi.gp==expected.at("input_gp").get<decltype(abi.gp)>(),"SDK input no longer follows its actual prior constructors");
    unsigned frontier_requests{},reset_requests{},dma_requests{},factory_requests{};
    services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observe){
        if(call.instruction==0x224f03){
            check(driver&&call.target==0x166a80&&!call.tail,"Original driver CRC request differs");
            source_run_render_object_startup_tu15(bank,call.target,state,*state.floating_point,services,observe);return;
        }
        if(call.instruction==0x224f26){
            ++factory_requests;const auto sp=state.gp[4],source_driver=word(memory,0x52d674);
            check(driver&&call.target==0x387877&&!call.tail&&word(memory,sp)==0x224f2b&&word(memory,sp+4)==0&&
                word(memory,sp+8)==source_driver+0x2ab0&&word(memory,sp+12)==0&&source_driver&&!word(memory,source_driver+0x2ab0),
                "Original driver/SDK factory request frame differs");
            check(state.gp==expected.at("factory").at("gp").get<decltype(state.gp)>()&&
                (state.eflags&0xcc5)==(expected.at("factory").at("eflags").get<unsigned>()&0xcc5)&&
                instructions==expected.at("factory").at("instructions")&&stores==expected.at("factory").at("source_stores"),
                "Original driver prefix to factory differs");
            source_run_sound_sdk_tu15(bank,call.target,state,*state.floating_point,services,observe);return;
        }
        if(call.instruction==0x38ec8f) {
            ++reset_requests;
            check(!call.tail&&call.target==0x38ec0e&&word(memory,state.gp[4])==0x38ec94&&
                word(memory,state.gp[4]+12)==0x389389&&state.gp[1]==expected.at("frontier").at("source_subobject"),"SDK reset frontier frame differs");
            if(through_reset){check(reset->try_call(bank,call,state,*state.floating_point,services,observe),"Reached native SDK reset was not owned");return;}
        }
        if(call.instruction==expected.at("frontier").at("instruction")){
            ++frontier_requests;check(!call.tail&&call.target==expected.at("frontier").at("target")&&
                word(memory,state.gp[4])==expected.at("frontier").at("return_address"),"SDK final required frontier differs");
            compare_abi(state,expected);
            check(instructions==expected.at("instructions")&&stores==expected.at("source_stores"),"Original/native whole SDK prefix instruction/store order differs");
            check(resources.live_allocations()==expected.at("allocations").size()&&resources.lock_depth()==1&&
                dma->allocations().size()==expected.value("dma_allocations",Json::array()).size(),"SDK object/lock/DMA ownership differs before frontier");
            const auto destination=driver?word(memory,0x52d674)+0x2ab0:output_cell;
            check(word(memory,destination)==0&&word(memory,0x390be4)==preference.flags()&&
                word(memory,0x390bc0)==expected.at("statistics")&&word(memory,0x390bc4)==expected.value("dma_statistics",0u),"SDK prefix synthesized output or lost original accounting");
            for(const auto& region:expected.at("regions"))
                check(std::ranges::equal(memory.view(region.at("address"),region.at("bytes")),unhex(region.at("hex"))),"SDK persistent source constructor bytes differ");
            check(std::ranges::equal(memory.view(0x2000000,0x20000),unhex(expected.at("stack_hex"))),"Complete SDK caller/private stack bytes differ");
            if(driver){check(std::ranges::equal(memory.view(0x10000000,0x23000),unhex(expected.at("arena_hex"))),"Complete source driver/sound arena bytes differ");
                check(memory.native_address(0x10000000,0x23000).value>0xffffffffu&&memory.view(word(memory,0x52d674),1)[0]==std::byte{},
                    "Sound driver is not native64 or synthesized initialized=true");}
            for(const auto& allocation:expected.at("allocations")){
                const auto source=allocation.at("source").get<std::uint32_t>(),size=allocation.at("bytes").get<unsigned>();
                check(std::ranges::equal(memory.view(source,size),unhex(allocation.at("hex"))),"Complete SDK native object backing differs");
                const auto native=memory.native_address(source,size);check(native.value>0xffffffffu&&memory.source_address(native,size)==source,"SDK object is not retained in actual native64 storage");
                wide.push_back({{"source",source},{"bytes",size},{"native",native.value},{"capacity",allocation.at("capacity")}});
            }
            check(platform==expected.at("platform"),"SDK native allocation/capacity/exclusion request sequence differs");
            if(through_storage){
                check(dma_platform==expected.at("dma_boundaries"),"Whole SDK native DMA request sequence differs");
                const auto actual=dma->allocations();
                for(std::size_t i=0;i<actual.size();++i){const auto& row=actual[i];const auto& original=expected.at("dma_allocations").at(i);
                    check(row.source==original.at("source")&&row.physical==original.at("physical")&&row.requested==original.at("requested")&&
                        row.capacity==original.at("capacity")&&row.alignment==original.at("alignment")&&row.protection==original.at("protection")&&
                        row.source_descriptor==original.at("source_descriptor")&&row.native.value>0xffffffffu&&memory.source_address(row.native,row.capacity)==row.source,
                        "Whole SDK actual native64 DMA ownership differs");
                    check(std::ranges::equal(memory.view(row.source,row.capacity),unhex(original.at("hex"))),"Whole SDK native DMA capacity/source tail pool bytes differ");
                    dma_wide.push_back({{"source",row.source},{"native",row.native.value},{"capacity",row.capacity},{"requested",row.requested}});
                }
            }
            if(through_reset){const auto state=reset->state();check(reset->quiescent()&&state.reset_count==1&&state.generation==1&&
                state.completed_commands>0&&!state.queued_commands&&!state.gp_program_loaded&&!state.ep_program_loaded&&
                state.output_device==preference.output_device()&&state.native_apu.value>0xffffffffu&&!state.faulted,
                "Native reset failed to retire real work or fabricated a processor program");}
            throw ResetBoundary{};
        }
        if(through_storage&&source_sound_hardware_storage_tu15_owns_entry(call.target)){
            source_run_sound_hardware_storage_tu15(bank,call.target,state,*state.floating_point,services,observe);return;
        }
        if(through_storage&&source_sound_dma_storage_tu15_owns_entry(call.target)){
            source_run_sound_dma_storage_tu15(bank,call.target,state,*state.floating_point,services,observe);return;
        }
        if(through_storage&&source_sound_sdk_tu15_owns_entry(call.target)){
            source_run_sound_sdk_tu15(bank,call.target,state,*state.floating_point,services,observe);return;
        }
        if(reset&&reset->try_call(bank,call,state,*state.floating_point,services,observe))return;
        if(call.instruction==0x3879dc||call.instruction==0x3879e9||call.instruction==0x38e74b){
            const auto sp=state.gp[4];
            const auto requested=word(memory,sp+4),alignment=call.instruction==0x3879dc?word(memory,sp+16):0;
            check(dma->dispatch(bank,call,state),"SDK DMA request was not actually owned");++dma_requests;
            if(call.instruction==0x3879dc){const auto rows=dma->allocations();const auto found=std::ranges::find_if(rows,[&](const auto& row){return row.source==state.gp[0];});
                check(found!=rows.end(),"Native SDK DMA request lost its returned first-fit allocation");const auto& row=*found;
                dma_platform.push_back({{"kind","allocate"},{"source",row.source},
                {"physical",row.physical},{"requested",requested},{"capacity",row.capacity},{"alignment",alignment},{"protection",row.protection},{"source_descriptor",row.source_descriptor}});}
            else dma_platform.push_back({{"kind",call.instruction==0x3879e9?"capacity":"physical"},{"source",requested},{"value",state.gp[0]}});
            return;
        }
        const auto sp=state.gp[4];const unsigned pop=call.instruction==0x387975?12:8;
        Json args=Json::array();for(unsigned i=4;i<pop;i+=4)args.push_back(word(memory,sp+i));
        const auto returned=word(memory,sp);
        check(resources.dispatch(bank,call,state),"SDK full prefix reached another unowned platform boundary");
        const char* kind=call.instruction==0x383f92?"enter":call.instruction==0x387975?"allocate":call.instruction==0x387982?"capacity":"leave";
        Json result=nullptr;if(call.instruction==0x387975||call.instruction==0x387982)result=state.gp[0];
        platform.push_back({{"kind",kind},{"target",call.target},{"returned",returned},{"args",args},{"result",result},{"lock_depth",resources.lock_depth()}});
    };
    bool stopped=false;
    try{
        SourceRuntimeSoundSdkResources::CallScope raw_scope(&resources);SourceRuntimeSoundDmaResources::CallScope dma_scope(dma.get());
        SourceRuntimeSoundResetResources::CallScope reset_scope(reset.get());
        if(driver)source_run_sound_startup_tu15(memory,0x224ec0,abi,*abi.floating_point,services,observer);
        else source_run_sound_sdk_tu15(memory,0x387877,abi,*abi.floating_point,services,observer);
    }catch(const ResetBoundary&){stopped=true;}
    check(stopped&&frontier_requests==1&&reset_requests==1&&dma_requests==expected.value("dma_boundaries",Json::array()).size()&&
        factory_requests==unsigned(driver),"SDK composed prefix did not stop at its actual required frontier");
    compare_abi(abi,expected);
    check(resources.live_allocations()==0&&resources.lock_depth()==0&&!resources.quiescent()&&dma->quiescent()!=through_storage,
        "SDK abort leaked raw resources or admitted partial source state");
    rejected([&]{resources.require_quiescent();});
    rejected([&]{SourceRuntimeSoundSdkResources::CallScope scope(&resources);});
    for(const auto& row:expected.at("allocations"))rejected([&]{memory.view(row.at("source"),row.at("bytes"));});
    check(word(memory,driver?word(memory,0x52d674)+0x2ab0:output_cell)==0&&SDL_GetAudioDeviceStatus(preference.output_device())==SDL_AUDIO_PLAYING,
        "SDK failure fabricated a device result or closed the separately retained native output");
    if(reset)check(reset->state().faulted&&!reset->quiescent(),"Interrupted whole SDK graph reused the native reset/control owner");
    reset.reset(); // Retire native source borrowers before the preference/output lease.
    dma.reset();
    for(const auto& row:expected.value("dma_allocations",Json::array()))rejected([&]{memory.view(row.at("source"),row.at("capacity"));});
    preference.retire();preference.require_quiescent();check(word(memory,0x390be8)==0xffffffffu,"Native preference retained an abandoned SDK binding");
    std::cout<<"Original "<<(driver?"driver + ":"")<<"SDK"<<(through_reset?" + reset":"")<<" prefix: "<<instructions.size()<<" instructions, "<<stores.size()<<" stores; next required entry "
        <<std::hex<<expected.at("frontier").at("target").get<std::uint32_t>()<<std::dec<<'\n';
    return {{"passed",true},{"fill",expected.at("fill")},{"instructions",instructions.size()},{"ordered_source_stores",stores.size()},
        {"native_platform_calls",platform},{"native64_objects",wide},{"shared_output_device",output->device_id()},
        {"reset_requests",reset_requests},{"dma_requests",dma_requests},{"real_raw_allocations_before_frontier",expected.at("allocations").size()},
        {"native64_dma_allocations",dma_wide},{"dma_platform_calls",dma_platform},{"immutable_thread_first_rejection_checks",foreign_rejections},
        {"driver_factory_requests",factory_requests},{"genuine_sound_arena",raw_allocations},{"completed_native_resets",unsigned(through_reset)},
        {"frontier",expected.at("frontier")},
        {"raw_allocations_after_abort",resources.live_allocations()},{"locks_after_abort",resources.lock_depth()},
        {"partial_source_checkpoint_rejected",true},{"native_preference_retired",true}};
}
}
int main(int argc,char** argv){try{
    check(argc==4,"Supply original SDK reference, private config directory and output report");
    std::ifstream input(argv[1]);Json reference;input>>reference;
    check(reference.at("passed")==true&&reference.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
        "Wrong SDK source reference");
    check(reference.at("instruction_patches").empty()&&reference.at("supplied_game_or_sdk_results").empty()&&reference.at("console_eeprom_or_smc_values_supplied").empty(),
        "SDK source reference supplied game/hardware results");
    data_ownership_rejections();
    const std::filesystem::path directory(argv[2]);std::filesystem::create_directories(directory);const auto config=directory/"menu.cfg";
    auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,64,{}});
    Json cases=Json::array();const std::array<unsigned,3> settings{0,1,4};unsigned index=0;
    for(const auto& expected:reference.at("cases")){
        std::ofstream file(config);file<<"pc_speakers "<<settings.at(index++)<<'\n';file.close();cases.push_back(run(expected,output,config));
    }
    Json prior_entries=Json::array();for(const auto& row:reference.at("cases").at(0).at("prior"))prior_entries.push_back(row.at("entry"));
    Json report={{"passed",true},{"original_xbe_sha256",reference.at("original_xbe_sha256")},{"cases",cases},
        {"original_instruction_sites",reference.at("executed_instruction_bytes").size()},{"native_backend",SDL_GetCurrentAudioDriver()},
        {"shared_native_output_device",output->device_id()},{"actual_output_channels",output->output_channels()},
        {"genuine_prior_crt_constructors",prior_entries},{"sdk_com_constructor_prefix_retained",true},
        {"existing_immutable_import_owners_reused",true},{"mismatched_bytes_or_access_rejected",true},{"cold_destinations_exclusive",true},
        {"whole_factory_shortcut_used",false},{"eeprom_or_smc_input_required",false},{"runtime_reference_executable_required",false},
        {"source_output_published",false},{"full_sdk_factory_completed",false},{"hardware_dsp_voice_caps_or_game_startup_verified",false}};
    report["actual_native_reset_completed"]=reference.value("native_reset_control_inputs_declared",false);
    report["original_driver_factory_request_verified"]=reference.value("original_whole224ec0_prefix",false);
    output.reset();std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot save SDK native prefix evidence");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

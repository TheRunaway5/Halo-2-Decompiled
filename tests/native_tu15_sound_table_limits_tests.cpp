#include "halo2/source_runtime_sound_table_limits.hpp"
#include "halo2/native_tu15_audio_preference.hpp"
#include "halo2/native_tu15_sound_sdk_prefix_data.hpp"
#include "halo2/source_runtime_sound_sdk_resources.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#include "halo2/source_sound_hardware_storage_tu15.hpp"
#include "halo2/source_sound_dma_storage_tu15.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
template<class F>void rejected(F action){bool caught=false;try{action();}catch(const std::exception&){caught=true;}check(caught,"Invalid native audio table request was accepted");}
void put(SourceRuntimeMemory& memory,std::uint32_t address,std::uint32_t value){std::array<std::byte,4> raw{};for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8*i));memory.write(address,raw);}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address){const auto raw=memory.view(address,4);std::uint32_t result{};for(unsigned i=0;i<4;++i)result|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);return result;}
std::uint32_t written(const Json& row){const auto hex=row.at(4).get<std::string>();std::uint32_t result{};for(unsigned i=0;i<4;++i)result|=std::stoul(hex.substr(i*2,2),nullptr,16)<<(8*i);return result;}
struct Frontier{};
using Inspect=std::function<void(SourceRuntimeMemory&,const std::shared_ptr<SourceRuntimeSoundDmaResources>&,
    const std::shared_ptr<SourceRuntimeSoundEpOutputResources>&,SourceRuntimeSoundResetResources&,
    const SourcePhysicsWorldRuntimeAbi&)>;
void genuine_prefix(const std::shared_ptr<SdlSoundPlatform>& output,const std::filesystem::path& config,const Inspect& inspect){
    constexpr std::uint32_t fs=0x60200000,stack=0x2008000;
    SourceRuntimeMemory memory;mount_native_tu15_sound_sdk_prefix_data(memory);mount_native_tu15_sound_dma_imports(memory);
    memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"genuine SDK table prerequisite caller");
    memory.mount(fs,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"actual passive sound table executor");
    NativeTu15AudioPreference preference(memory,output,native_tu15_load_audio_preference(config));preference.publish();
    SourceRuntimeSoundSdkResources sdk(memory,fs);auto dma=std::make_shared<SourceRuntimeSoundDmaResources>(memory,output);
    SourceRuntimeSoundResetResources reset(memory,output,fs);
    SourcePhysicsWorldRuntimeAbi abi{};abi.fs_base=fs;abi.gp[4]=stack;abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    SourcePhysicsWorldRuntimeServices services;
    for(auto entry:{0x387baau,0x387bb5u,0x38a988u,0x38a993u}){put(memory,stack,0x2100000);abi.gp[4]=stack;source_run_sound_sdk_tu15(memory,entry,abi,*abi.floating_point,services);}
    services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observer){
        if(call.target==0x38e8c9){
            check(call.instruction==0x38eca3&&!call.tail&&word(memory,state.gp[4])==0x38eca8&&dma->allocations().size()==16,"Table capacities did not follow genuine SDK/reset/16 DMA construction");
            auto ep=std::make_shared<SourceRuntimeSoundEpOutputResources>(memory,dma,output);inspect(memory,dma,ep,reset,state);throw Frontier{};
        }
        if(reset.try_call(bank,call,state,*state.floating_point,services,observer))return;
        if(dma->dispatch(bank,call,state))return;
        if(sdk.dispatch(bank,call,state,observer))return;
        if(source_sound_hardware_storage_tu15_owns_entry(call.target)){source_run_sound_hardware_storage_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        if(source_sound_dma_storage_tu15_owns_entry(call.target)){source_run_sound_dma_storage_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        if(source_sound_sdk_tu15_owns_entry(call.target)){source_run_sound_sdk_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        throw std::runtime_error("Unowned original SDK resource before table prerequisite");
    };
    put(memory,stack,0x2100000);put(memory,stack+4,0);put(memory,stack+8,0x2001000);put(memory,stack+12,0);abi.gp[4]=stack;
    bool stopped=false;try{SourceRuntimeSoundSdkResources::CallScope sdk_scope(&sdk);SourceRuntimeSoundDmaResources::CallScope dma_scope(dma.get());SourceRuntimeSoundResetResources::CallScope reset_scope(&reset);source_run_sound_sdk_tu15(memory,0x387877,abi,*abi.floating_point,services);}catch(const Frontier&){stopped=true;}
    check(stopped&&word(memory,0x2001000)==0,"Table prerequisite invented original factory/output success");preference.retire();
}
}
int main(int argc,char** argv){try{
    check(argc==5,"Supply original configuration/base reports, private directory and output report");
    std::ifstream input(argv[1]);Json reference;input>>reference;
    check(reference.at("passed")==true&&reference.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong table capacity source reference");
    std::ifstream base_input(argv[2]);Json base_reference;base_input>>base_reference;
    check(base_reference.at("passed")==true&&base_reference.at("original_xbe_sha256")==reference.at("original_xbe_sha256")&&
        base_reference.at("supplied_fifo_space")==false&&base_reference.at("whole38e0ce_returned")==false,
        "Wrong bounded original table base source reference");
    std::filesystem::create_directories(argv[3]);const auto config=std::filesystem::path(argv[3])/"menu.cfg";{std::ofstream out(config);out<<"pc_speakers 0\n";}
    auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,64,{}});
    unsigned observed{},records{},foreign_guards{},bank_rejections{},bounded_views{},context_observed{},physical_views{},base_observed{},bound_views{};
    std::uint64_t actual_capacity{};Json ranges=Json::array();
    genuine_prefix(output,config,[&](auto& memory,const auto& dma,const auto& ep,auto& reset,const auto& frontier){
        SourceRuntimeSoundTableLimits owner(memory,dma,ep,reset.control_lease());
        const auto unchanged=memory.checkpoint();
        genuine_prefix(output,config,[&](auto&,const auto& foreign_dma,const auto& foreign_ep,auto& foreign_reset,const auto&){
            auto reject_bank=[&](const auto& candidate_dma,const auto& candidate_ep,auto candidate_control){rejected([&]{SourceRuntimeSoundTableLimits invalid(memory,candidate_dma,candidate_ep,candidate_control);});++bank_rejections;check(memory.checkpoint()==unchanged,"Foreign/hybrid table owner changed current source bank");};
            reject_bank(foreign_dma,foreign_ep,reset.control_lease());reject_bank(dma,foreign_ep,reset.control_lease());
            reject_bank(foreign_dma,ep,reset.control_lease());reject_bank(dma,ep,foreign_reset.control_lease());
        });
        using Table=SourceRuntimeSoundTableLimits::Table;
        rejected([&]{owner.record(Table::Voice,0);});
        rejected([&]{owner.bound_record(Table::Voice,0);});
        rejected([&]{owner.apply_physical_base(0x38e104,0xfe80202c,owner.diagnostics().ranges[0].allocation.physical);});
        check(!owner.apply_limit(0x38e8c9,0xfe801004,0xd8)&&!owner.diagnostics().configuration_ready,"Capacity component admitted unknown interrupt policy");
        const std::array<std::array<std::uint32_t,3>,5> requests{{
            {0x38e90d,0xfe801104,0xff},{0x38e917,0xfe801108,0x3f},{0x38e921,0xfe80111c,0x7f},
            {0x38e92b,0xfe801124,0x1fff},{0x38e935,0xfe801138,0x7ff}}};
        // Every subrequest must be present, with the same exact site/address/
        // width/value, in the independently executed unchanged source oracle.
        const auto& stores=reference.at("cases").at(0).at("stores");
        for(std::size_t i=0;i<requests.size();++i){const auto& request=requests[i];
            check(std::ranges::any_of(stores,[&](const Json& row){return row.at(0)==request[0]&&row.at(1)==request[1]&&row.at(2)==4&&written(row)==request[2];}),"Native limit request is absent from actual source oracle");
            rejected([&]{owner.apply_limit(request[0],request[1],request[2]+1);});
            rejected([&]{owner.apply_limit(request[0]+1,request[1],request[2]);});
            check(owner.apply_limit(request[0],request[1],request[2]),"Exact original table limit was not owned");++observed;
            rejected([&]{owner.apply_limit(request[0],request[1],request[2]);});
            const auto range=owner.diagnostics().ranges[i];records+=range.records;
            for(const auto index:{0u,range.records-1}){const auto view=owner.record(static_cast<Table>(i),index);
                check(view.source==range.allocation.source+index*range.record_bytes&&view.native==memory.native_address(view.source,range.record_bytes)&&
                    view.bytes.size()==range.record_bytes&&std::ranges::equal(view.bytes,memory.view(view.source,range.record_bytes)),"Native table record is not its actual bounded native64 page view");++bounded_views;}
            rejected([&]{owner.record(static_cast<Table>(i),range.records);});
            ranges.push_back({{"descriptor",range.allocation.source_descriptor},{"records",range.records},{"record_bytes",range.record_bytes},{"native",range.allocation.native.value},{"requested",range.allocation.requested}});
        }
        rejected([&]{owner.record(static_cast<Table>(255),0);});
        for(const auto& case_row:base_reference.at("cases")){
            check(case_row.at("entry")==0x38e0db&&case_row.at("end_exclusive")==0x38e131&&
                case_row.at("voice_index_stores")==256,"Table base reference crossed its declared source interval");
            check(case_row.at("physical_base_publications")==base_reference.at("cases").at(0).at("physical_base_publications"),
                "Original table base publications differ across fill patterns");
        }
        for(const auto& row:base_reference.at("cases").at(0).at("physical_base_publications")){
            const auto instruction=row.at(0).get<std::uint32_t>(),address=row.at(1).get<std::uint32_t>(),physical=written(row);
            const auto state=owner.diagnostics();const auto at=std::ranges::find_if(state.ranges,[&](const auto& range){return range.allocation.physical==physical;});
            check(at!=state.ranges.end()&&row.at(2)==4,"Original table base has no actual native retained DMA allocation");
            const auto table=at->table;rejected([&]{owner.bound_record(table,0);});
            rejected([&]{owner.apply_physical_base(instruction+1,address,physical);});
            rejected([&]{owner.apply_physical_base(instruction,address,physical+4);});
            const auto another=std::ranges::find_if(state.ranges,[&](const auto& range){return range.allocation.physical!=physical;});
            rejected([&]{owner.apply_physical_base(instruction,address,another->allocation.physical);});
            check(owner.apply_physical_base(instruction,address,physical),"Exact original physical table publication was not owned");++base_observed;
            rejected([&]{owner.apply_physical_base(instruction,address,physical);});
            for(const auto index:{0u,at->records-1}){
                const auto view=owner.bound_record(table,index);const auto raw=owner.record(table,index);
                check(view.source==raw.source&&view.native==raw.native&&view.bytes.data()==raw.bytes.data()&&
                    view.native.value>0xffffffffu,"Published table record did not retain its actual native64 backing");++bound_views;
            }
            rejected([&]{owner.bound_record(table,at->records);});
        }
        rejected([&]{owner.bound_record(static_cast<Table>(255),0);});
        check(!owner.apply_physical_base(0x38e136,0xfe80115c,0)&&!owner.apply_physical_base(0x38e0ce,0xfe820010,0x20),
            "Table base owner admitted notifier methods or FIFO space");
        using Context=SourceRuntimeSoundTableLimits::Context;
        const std::array<std::array<std::uint32_t,3>,6> contexts{{
            {0x38e949,0xfe80112c,0},{0x38e954,0xfe801130,0x08000000},
            {0x38e959,0xfe801140,0},{0x38e95f,0xfe801144,0x08000000},
            {0x38e964,0xfe801150,0},{0x38e96d,0xfe801154,0x08000000}}};
        const auto allocations=dma->allocations();
        for(const auto& row:allocations)actual_capacity+=row.capacity;
        rejected([&]{owner.resolve_physical(Context::StreamingScatterGather,allocations.front().physical,1);});
        for(std::size_t i=0;i<contexts.size();++i){const auto& request=contexts[i];
            check(std::ranges::any_of(stores,[&](const Json& row){return row.at(0)==request[0]&&row.at(1)==request[1]&&row.at(2)==4&&written(row)==request[2];}),"Native context bound is absent from actual source oracle");
            if(!(i&1)){const auto& length=contexts[i+1];rejected([&]{owner.apply_context_bound(length[0],length[1],length[2]);});}
            rejected([&]{owner.apply_context_bound(request[0],request[1],request[2]+1);});
            rejected([&]{owner.apply_context_bound(request[0]+1,request[1],request[2]);});
            check(owner.apply_context_bound(request[0],request[1],request[2]),"Exact original DMA context bound was not owned");++context_observed;
            rejected([&]{owner.apply_context_bound(request[0],request[1],request[2]);});
            if(!(i&1))rejected([&]{owner.resolve_physical(static_cast<Context>(i/2),allocations.front().physical,1);});
        }
        for(unsigned i=0;i<3;++i)for(const auto& row:allocations){
            for(const auto offset:{0u,row.capacity-1}){
                const auto view=owner.resolve_physical(static_cast<Context>(i),row.physical+offset,1);
                check(view.source==row.source+offset&&view.native.value==row.native.value+offset&&view.bytes[0]==memory.view(row.source+offset,1)[0],"Native context did not resolve its actual first/last owned physical page byte");++physical_views;
            }
            rejected([&]{owner.resolve_physical(static_cast<Context>(i),row.physical+row.capacity-1,2);});
        }
        rejected([&]{owner.resolve_physical(static_cast<Context>(255),allocations.front().physical,1);});
        rejected([&]{owner.resolve_physical(Context::InputScatterGather,1,1);});
        rejected([&]{owner.resolve_physical(Context::InputScatterGather,0x08000000,1);});
        rejected([&]{owner.resolve_physical(Context::InputScatterGather,0xfffffff0,0x100);});
        rejected([&]{owner.resolve_physical(Context::InputScatterGather,allocations.front().physical,0);});
        check(!owner.apply_context_bound(0x38e8c9,0xfe801004,0xd8),"Physical context component admitted interrupt policy");
        check(memory.checkpoint()==unchanged,"Native table limit validation changed original CPU/source storage");
        std::thread foreign([&]{check(!owner.quiescent(),"Foreign table diagnostics advertised quiescence");auto guard=[&](auto action){try{action();}catch(const std::exception& error){if(std::string(error.what()).find("owner thread")!=std::string::npos)++foreign_guards;}};
            guard([&]{owner.apply_limit(0x38e90d,0xfe801104,0xff);});guard([&]{owner.record(Table::Voice,0);});guard([&]{owner.diagnostics();});guard([&]{owner.retire();});guard([&]{SourceRuntimeSoundTableLimits::CallScope scope(&owner);});
            guard([&]{owner.apply_context_bound(0x38e949,0xfe80112c,0);});guard([&]{owner.resolve_physical(Context::InputScatterGather,0xffffffff,1);});
            guard([&]{owner.apply_physical_base(0x38e104,0xfe80202c,0);});guard([&]{owner.bound_record(Table::Voice,0);});});foreign.join();
        check(foreign_guards==9,"Foreign table operation touched mutable source/native resources");
        SourceRuntimeSoundTableLimits interrupted(memory,dma,ep,reset.control_lease());
        try{SourceRuntimeSoundTableLimits::CallScope scope(&interrupted);check(interrupted.apply_limit(0x38e90d,0xfe801104,0xff),"Native table fault fixture lacked actual capacity");throw std::runtime_error("declared interrupted parent source transaction");}catch(const std::runtime_error&){}
        check(interrupted.diagnostics().faulted,"Interrupted native table scope did not fault its transaction");rejected([&]{interrupted.record(Table::Voice,0);});interrupted.retire();
        // Genuine repeated native reset, rather than a forged generation, must
        // invalidate all prior table views/admission before another bank read.
        auto repeated=frontier;repeated.gp[4]=0x201a000;repeated.gp[6]=repeated.gp[1];put(memory,repeated.gp[4],0x38ec94);put(memory,repeated.gp[4]+12,0x389389);
        check(reset.try_call(memory,{0x38ec8f,0x38ec0e,false},repeated,*repeated.floating_point,{}),"Genuine repeated native reset was not owned");
        rejected([&]{owner.record(Table::Voice,0);});rejected([&]{owner.apply_limit(0x38e90d,0xfe801104,0xff);});
        rejected([&]{owner.resolve_physical(Context::InputScatterGather,allocations.front().physical,1);});
        rejected([&]{owner.bound_record(Table::Voice,0);});rejected([&]{owner.apply_physical_base(0x38e104,0xfe80202c,allocations.front().physical);});
        owner.retire();check(owner.quiescent()&&owner.diagnostics().retired&&!owner.diagnostics().configuration_ready,"Table retirement admitted full source configuration");rejected([&]{owner.record(Table::Voice,0);});
    });
    Json report={{"passed",true},{"native_backend",SDL_GetCurrentAudioDriver()},{"actual_shared_output_device",output->device_id()},
        {"genuine_dma_allocations_per_bank",16},{"independent_source_banks",2},{"foreign_or_hybrid_bank_rejections",bank_rejections},{"foreign_thread_rejections",foreign_guards},
        {"authenticated_limit_subrequests",observed},{"actual_record_capacity",records},{"bounded_first_last_native_views",bounded_views},{"ranges",ranges},
        {"authenticated_context_bound_subrequests",context_observed},{"logical_context_windows",3},{"declared_context_window_bytes",0x08000000},
        {"actual_total_dma_backing_bytes",actual_capacity},{"bounded_physical_page_views",physical_views},{"dummy_context_backing_allocated",false},{"sge_execution_ready",false},
        {"authenticated_physical_base_subrequests",base_observed},{"bounded_published_table_views",bound_views},
        {"fifo_space_supplied",false},{"whole38e0ce_return_supplied",false},
        {"source_bytes_unchanged_by_limits",true},{"genuine_repeated_reset_rejected_old_views",true},{"voice_or_spatial_processing_ready",false},
        {"ien_policy_admitted",false},{"whole38e8c9_return_supplied",false},{"initializer45_complete",false},{"dsp_program_ready",false}};
    std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot save native audio table evidence");std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

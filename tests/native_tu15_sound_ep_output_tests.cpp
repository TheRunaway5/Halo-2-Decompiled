#include "halo2/native_tu15_audio_preference.hpp"
#include "halo2/native_tu15_sound_sdk_prefix_data.hpp"
#include "halo2/source_runtime_sound_ep_output_resources.hpp"
#include "halo2/source_runtime_sound_sdk_resources.hpp"
#include "halo2/source_runtime_sound_reset_resources.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#include "halo2/source_sound_hardware_storage_tu15.hpp"
#include "halo2/source_sound_dma_storage_tu15.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <fstream>
#include <future>
#include <iostream>
#include <thread>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void rejected(F action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}check(failed,"Invalid or unfinished native EP resource was accepted");}
void put(SourceRuntimeMemory& memory,std::uint32_t p,std::uint32_t value){std::array<std::byte,4> raw{};for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8*i));memory.write(p,raw);}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t p){auto bytes=memory.view(p,4);std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return value;}
std::vector<std::byte> unhex(const std::string& text){auto n=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};std::vector<std::byte> result(text.size()/2);for(std::size_t i=0;i<result.size();++i)result[i]=std::byte((n(text[i*2])<<4)|n(text[i*2+1]));return result;}
struct FrontendBoundary {};
void byte_probes(SourceRuntimeMemory& memory,SourceRuntimeSoundEpOutputResources& owner,
    const Json& reference,const std::shared_ptr<SourceRuntimeSoundEpOutputResources::CallbackLease>& lease){
    const auto layout=owner.diagnostics().layout;unsigned fifo=0;
    for(const auto& ring:reference.at("circular_native_byte_probes")){
        check(layout.rings[fifo].base==ring.at("base")&&layout.rings[fifo].end==ring.at("end")&&
            layout.rings[fifo].pending_source_cursor==ring.at("initial"),"Native EP circular range differs from original source layout");
        for(const auto& operation:ring.at("operations")){
            bool accepted;
            if(operation.at("kind")=="write")accepted=lease->write(fifo,unhex(operation.at("input_hex")));
            else{
                std::vector<std::byte> out(operation.at("bytes"),std::byte{0xa5});const auto before=out;accepted=lease->read(fifo,out);
                if(accepted)check(out==unhex(operation.at("returned_hex")),"Native EP wrap read differs from independent byte queue");
                else check(out==before,"Rejected EP underflow changed destination bytes");
            }
            const auto state=lease->state(fifo);
            check(accepted==operation.at("accepted")&&state.read_cursor==operation.at("read_cursor")&&
                state.write_cursor==operation.at("write_cursor")&&state.queued==operation.at("queued"),"Native EP overflow/wrap/accounting differs");
            check(std::ranges::equal(memory.view(layout.data.source+layout.rings[fifo].base,state.capacity),unhex(operation.at("ring_hex"))),
                "Native EP complete circular backing differs");
        }
        ++fifo;
    }
    const auto before=lease->state(0);rejected([&]{lease->write(2,{});});rejected([&]{lease->read(2,{});});
    auto native=memory.mutable_view(layout.data.source,1);
    rejected([&]{lease->write(0,native);});
    check(lease->state(0).queued==before.queued&&lease->state(0).write_cursor==before.write_cursor,"Rejected EP alias changed cursor/accounting");
    std::array<std::byte,1> input{std::byte{0x71}},output{};check(lease->write(0,input),"Native EP byte probe could not enqueue one byte");
    rejected([&]{owner.retire();});
    rejected([&]{lease->read(0,native);});check(lease->read(0,output)&&input==output,"Rejected EP read alias lost queued bytes");
    check(!owner.diagnostics().producer_ready&&!owner.source_scatter_gather_constructed(),"Native byte probe invented a source DSP producer/SGE publication");
}
}
int main(int argc,char** argv){try{
    check(argc==4,"Supply original EP layout reference, private config directory and report output");
    std::ifstream input(argv[1]);Json reference;input>>reference;
    check(reference.at("passed")==true&&reference.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
        reference.at("instruction_patches").empty()&&!reference.at("whole38e2e0_returned").get<bool>(),"Wrong or overclaimed original EP reference");
    const std::filesystem::path private_dir(argv[2]);std::filesystem::create_directories(private_dir);const auto config=private_dir/"menu.cfg";
    {std::ofstream file(config);file<<"pc_speakers 0\n";}
    SourceRuntimeMemory memory;mount_native_tu15_sound_sdk_prefix_data(memory);mount_native_tu15_sound_dma_imports(memory);
    memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"declared SDK caller stack and output");
    constexpr std::uint32_t fs=0x60200000,stack=0x2008000;
    memory.mount(fs,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"actual passive native sound executor context");
    auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,64,{}});
    NativeTu15AudioPreference preference(memory,output,native_tu15_load_audio_preference(config));preference.publish();
    SourceRuntimeSoundSdkResources sdk(memory,fs);auto dma=std::make_shared<SourceRuntimeSoundDmaResources>(memory,output);
    auto reset=std::make_unique<SourceRuntimeSoundResetResources>(memory,output,fs);
    std::unique_ptr<SourceRuntimeSoundEpOutputResources> ep;
    std::shared_ptr<SourceRuntimeSoundEpOutputResources::CallbackLease> retained_callback;
    SourcePhysicsWorldRuntimeAbi abi{};abi.fs_base=fs;abi.gp[4]=stack;abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    SourcePhysicsWorldRuntimeServices services;
    for(const auto entry:{0x387baau,0x387bb5u,0x38a988u,0x38a993u}){
        put(memory,stack,0x2100000);abi.gp[4]=stack;source_run_sound_sdk_tu15(memory,entry,abi,*abi.floating_point,services);
    }
    unsigned foreign_dma{},foreign_ep{},observed_cursors{},frontend_requests{};
    std::atomic<std::uint64_t> callback_iterations{};std::atomic<bool> callback_cancelled{};
    services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observer){
        if(call.target==0x38e8c9){
            ++frontend_requests;check(call.instruction==0x38eca3&&!call.tail&&word(memory,state.gp[4])==0x38eca8&&dma->allocations().size()==16,
                "EP prerequisite was not reached after genuine SDK reset/storage");
            ep=std::make_unique<SourceRuntimeSoundEpOutputResources>(memory,dma,output);
            rejected([&]{SourceRuntimeSoundEpOutputResources duplicate(memory,dma,output);});
            rejected([&]{ep->acquire_native_byte_transport();});
            const auto layout=ep->diagnostics().layout;
            check(layout.data.source_descriptor==0x391370&&layout.scatter_gather.source_descriptor==0x391400&&
                layout.data.native.value>0xffffffffu&&layout.scatter_gather.native.value>0xffffffffu&&
                layout.pending_page_entries==reference.at("layout_fragment").at("page_entries").get<decltype(layout.pending_page_entries)>(),
                "Native EP pending physical/SGE layout differs from original CPU construction");
            check(!ep->source_scatter_gather_constructed()&&!ep->diagnostics().producer_ready,"EP prerequisite invented a constructed source table/producer");
            for(const auto& request:reference.at("pending_cursor_stores")){
                check(request.at(2)==4,"Original EP cursor store width differs");ep->apply_pending_cursor(request.at(0),request.at(1),request.at(3));++observed_cursors;
            }
            rejected([&]{ep->apply_pending_cursor(0x38e7c4,0xfe80402c,0x600);});
            for(const auto& request:reference.at("unowned_scheduler_stores"))
                rejected([&]{ep->apply_pending_cursor(request.at(0),request.at(1),request.at(3));});
            auto lease=ep->acquire_native_byte_transport();byte_probes(memory,*ep,reference,lease);
            SDL_PauseAudioDevice(output->device_id(),1);
            check(SDL_GetAudioDeviceStatus(output->device_id())==SDL_AUDIO_PAUSED,"Actual shared output did not pause");
            rejected([&]{ep->acquire_native_byte_transport();});rejected([&]{lease->read(0,{});});
            SDL_PauseAudioDevice(output->device_id(),0);
            check(SDL_GetAudioDeviceStatus(output->device_id())==SDL_AUDIO_PLAYING&&lease->read(0,{}),"Actual shared output did not resume its valid byte lease");
            bool diagnostic_guards=false;
            std::thread foreign([&]{
                diagnostic_guards=!dma->quiescent()&&!ep->quiescent();
                auto dma_guard=[&](auto action){try{action();}catch(const std::exception& error){if(std::string(error.what()).find("owner thread")!=std::string::npos)++foreign_dma;}};
                dma_guard([&]{dma->allocations();});dma_guard([&]{dma->capacity(0);});dma_guard([&]{dma->native_address(0);});dma_guard([&]{dma->device_id();});
                dma_guard([&]{SourceRuntimeSoundDmaResources::CallScope scope(dma.get());});
                SourcePhysicsWorldRuntimeAbi invalid{};invalid.gp[4]=0xffffffffu;
                dma_guard([&]{dma->dispatch(memory,{0x3879dc,0x800000a6,false},invalid);});
                dma_guard([&]{dma->dispatch(memory,{0x3879e9,0x800000b4,false},invalid);});
                auto ep_guard=[&](auto action){try{action();}catch(const std::exception& error){if(std::string(error.what()).find("owner thread")!=std::string::npos)++foreign_ep;}};
                ep_guard([&]{ep->diagnostics();});ep_guard([&]{ep->acquire_native_byte_transport();});
                ep_guard([&]{ep->source_scatter_gather_constructed();});ep_guard([&]{SourceRuntimeSoundEpOutputResources::CallScope scope(ep.get());});
            });foreign.join();check(diagnostic_guards&&foreign_dma==7&&foreign_ep==4,"Foreign diagnostics/frame/scope touched native EP/DMA state");
            retained_callback=ep->acquire_native_byte_transport();std::promise<void> began;auto started=began.get_future();std::jthread callback;
            // Scope destruction cancels real byte copies before jthread joins,
            // then before the outer reset/DMA/SDK scopes unwind their resources.
            SourceRuntimeSoundEpOutputResources::CallScope ep_scope(ep.get());
            callback=std::jthread([callback_lease=retained_callback,&began,&callback_iterations,&callback_cancelled]{
                bool notified=false;std::array<std::byte,16> input{},result{};
                try{for(;;){
                    if(!callback_lease->write(0,input)||!callback_lease->read(0,result)||result!=input)throw std::runtime_error("Native EP callback byte transport failed");
                    ++callback_iterations;if(!notified){notified=true;began.set_value();}
                }}catch(const std::exception&){callback_cancelled=true;if(!notified)began.set_value();}
            });started.wait();check(callback_iterations>0,"Actual native EP callback did not execute a retained byte copy");
            throw FrontendBoundary{};
        }
        if(reset->try_call(bank,call,state,*state.floating_point,services,observer))return;
        if(dma->dispatch(bank,call,state))return;
        if(sdk.dispatch(bank,call,state,observer))return;
        if(source_sound_hardware_storage_tu15_owns_entry(call.target)){source_run_sound_hardware_storage_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        if(source_sound_dma_storage_tu15_owns_entry(call.target)){source_run_sound_dma_storage_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        if(source_sound_sdk_tu15_owns_entry(call.target)){source_run_sound_sdk_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        throw std::runtime_error("Unowned SDK prerequisite before EP transport");
    };
    put(memory,stack,0x2100000);put(memory,stack+4,0);put(memory,stack+8,0x2001000);put(memory,stack+12,0);abi.gp[4]=stack;
    bool stopped=false;try{
        SourceRuntimeSoundSdkResources::CallScope sdk_scope(&sdk);SourceRuntimeSoundDmaResources::CallScope dma_scope(dma.get());
        SourceRuntimeSoundResetResources::CallScope reset_scope(reset.get());
        source_run_sound_sdk_tu15(memory,0x387877,abi,*abi.floating_point,services);
    }catch(const FrontendBoundary&){stopped=true;}
    check(stopped&&frontend_requests==1&&observed_cursors==2&&callback_cancelled&&ep&&!sdk.quiescent()&&!dma->quiescent(),
        "EP prerequisite bypassed frontend or admitted a faulted SDK/DMA transaction");
    const auto diagnostic=ep->diagnostics();check(diagnostic.faulted&&!diagnostic.retired&&!diagnostic.producer_ready&&
        diagnostic.callback_leases==1&&diagnostic.active_copies==0,"EP callback retirement/fault diagnostics differ");
    rejected([&]{ep->acquire_native_byte_transport();});rejected([&]{retained_callback->read(0,{});});
    rejected([&]{SourceRuntimeSoundEpOutputResources after_fault(memory,dma,output);});
    std::weak_ptr<SourceRuntimeSoundDmaResources> weak_dma=dma;dma.reset();
    check(!weak_dma.expired()&&memory.native_address(diagnostic.layout.data.source,diagnostic.layout.data.capacity)==diagnostic.layout.data.native,
        "Shared EP lease lost actual DMA pages before creating-thread retirement");
    ep->retire();check(weak_dma.expired()&&!ep->quiescent(),"EP retirement released its last callback lease or failed to release real DMA");
    rejected([&]{memory.view(diagnostic.layout.data.source,diagnostic.layout.data.capacity);});
    rejected([&]{retained_callback->state(0);});retained_callback.reset();check(ep->quiescent(),"Cancelled callback handles did not become quiescent");
    reset.reset();preference.retire();preference.require_quiescent();
    check(SDL_GetAudioDeviceStatus(output->device_id())==SDL_AUDIO_PLAYING&&word(memory,0x2001000)==0,"EP prerequisite supplied source device success or closed shared output");
    Json report={{"passed",true},{"native_backend",SDL_GetCurrentAudioDriver()},{"actual_shared_output_device",output->device_id()},
        {"actual_dma_descriptors",16},{"data_source",diagnostic.layout.data.source},{"data_physical",diagnostic.layout.data.physical},
        {"data_native",diagnostic.layout.data.native.value},{"sg_native",diagnostic.layout.scatter_gather.native.value},
        {"pending_cursor_requests",observed_cursors},{"original_layout_instructions",reference.at("layout_fragment").at("instructions").size()},
        {"native_byte_probe_rings",2},{"native_callback_copies",callback_iterations.load()},{"foreign_dma_rejections",foreign_dma},{"foreign_ep_rejections",foreign_ep},
        {"fault_scope_cancelled_before_dma_teardown",true},{"last_dma_reference_released_on_creating_thread",true},
        {"nonempty_retirement_rejected",true},{"actual_paused_output_rejected",true},
        {"source_sge_published",false},{"source_producer_ready",false},{"dsp_produced_bytes",0},{"source_frontend_ac97_or_dsp_configured",false},
        {"whole38e75b_or38e8c9_return_supplied",false},{"source_sdk_output_published",false},{"full_startup_completed",false}};
    std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot save EP prerequisite evidence");std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

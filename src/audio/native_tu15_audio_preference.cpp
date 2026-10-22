#include "halo2/native_tu15_audio_preference.hpp"
#include <SDL.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
namespace halo2 {
namespace {
constexpr std::uint32_t override_cell=0x390be8,source_stereo=0;
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address){
    const auto raw=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);return value;
}
void put(SourceRuntimeMemory& memory,std::uint32_t address,std::uint32_t value){
    std::array<std::byte,4> raw{};for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8*i));memory.write(address,raw);
}
}
NativeTu15PersistedAudioPreference native_tu15_load_audio_preference(const std::filesystem::path& path){
    if(path.empty())throw std::invalid_argument("Native audio preference requires its actual application configuration path");
    NativeTu15PersistedAudioPreference result{DisplaySettings{}.speakers,path,true};
    std::ifstream stream(path);
    if(!stream){if(std::filesystem::exists(path))throw std::runtime_error("Native audio configuration is unreadable");return result;}
    std::string line;
    while(std::getline(stream,line)){
        std::istringstream record(line);std::string kind;unsigned selected{};
        if(record>>kind&&kind=="pc_speakers"&&record>>selected&&selected<5){result.speakers=selected;result.application_default=false;}
    }
    if(stream.bad())throw std::runtime_error("Native audio configuration read failed");return result;
}
NativeTu15AudioPreference::NativeTu15AudioPreference(SourceRuntimeMemory& memory,
    std::shared_ptr<SdlSoundPlatform> output,NativeTu15PersistedAudioPreference preference)
    :memory_(memory),output_(std::move(output)),preference_(std::move(preference)){
    if(!output_||preference_.configuration.empty()||preference_.speakers>=5)
        throw std::invalid_argument("Native audio preference lacks its real output or application input");
    const auto actual=native_tu15_load_audio_preference(preference_.configuration);
    if(actual.speakers!=preference_.speakers||actual.application_default!=preference_.application_default)
        throw std::invalid_argument("Native audio preference differs from its actual persisted input");
    if(preference_.speakers==2||preference_.speakers==3)
        throw std::runtime_error("Native SDK preference requires implemented discrete surround routes");
    // Inspect the obtained resource without reopening it. The current host
    // mixer supports only this actual stereo pair, including the default
    // device selected by its original native construction request.
    device_=output_->device_id();require_live();
}
NativeTu15AudioPreference::~NativeTu15AudioPreference(){
    // The bank outlives this owner. Withdraw only our retained SDK input;
    // source-owned device/refcount/capability fields are never synthesized.
    if(published_)try{if(word(memory_,override_cell)==source_stereo)put(memory_,override_cell,0xffffffffu);}catch(...){}
}
void NativeTu15AudioPreference::retire(){
    if(std::this_thread::get_id()!=thread_||!output_)throw std::invalid_argument("Native audio preference has no owned output to retire");
    if(published_)withdraw();output_.reset();device_=0;
}
bool NativeTu15AudioPreference::quiescent()const noexcept{return std::this_thread::get_id()==thread_&&!published_&&!output_;}
void NativeTu15AudioPreference::require_quiescent()const{if(!quiescent())throw std::runtime_error("Live native audio preference/output cannot cross a source checkpoint");}
void NativeTu15AudioPreference::require_live()const{
    const auto* backend=SDL_GetCurrentAudioDriver();
    if(std::this_thread::get_id()!=thread_||!output_||!device_||output_->device_id()!=device_||
        output_->output_channels()!=2||SDL_GetAudioDeviceStatus(device_)!=SDL_AUDIO_PLAYING||
        !backend||std::string_view(backend)=="dummy"||std::string_view(backend)=="disk")
        throw std::runtime_error("Native audio preference lost its actual two-channel host output owner");
}
std::uint32_t NativeTu15AudioPreference::flags()const{require_live();return source_stereo;}
std::uint32_t NativeTu15AudioPreference::output_device()const{require_live();return device_;}
SdlSoundPlatform& NativeTu15AudioPreference::output()const{require_live();return *output_;}
void NativeTu15AudioPreference::publish(){
    require_live();memory_.validate_mutable_view(override_cell,4);
    if(published_||word(memory_,override_cell)!=0xffffffffu)
        throw std::invalid_argument("Native audio preference would replace an existing SDK override input");
    put(memory_,override_cell,flags());published_=true;
}
void NativeTu15AudioPreference::withdraw(){
    if(std::this_thread::get_id()!=thread_)throw std::invalid_argument("Native audio preference cleanup has a foreign executor");
    memory_.validate_mutable_view(override_cell,4);
    if(!published_||word(memory_,override_cell)!=source_stereo)
        throw std::invalid_argument("Native audio preference no longer owns the SDK override input");
    put(memory_,override_cell,0xffffffffu);published_=false;
}
}

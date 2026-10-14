#include "halo2/source_runtime_sound_output.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <exception>
namespace halo2 {namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b;m.read(p,b);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);return v;}
void put(SourceRuntimeMemory&m,std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b;for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(i*8));m.write(p,b);}
}
struct SourceRuntimeSoundOutput::Impl {
    SourceRuntimeMemory &bank;
    SdlSoundPlatformConfiguration configuration;
    std::unique_ptr<SdlSoundPlatform> platform;
    std::uint32_t output{},prior{};
    bool cleanup_fault{};
    Impl(SourceRuntimeMemory&m,SdlSoundPlatformConfiguration c):bank(m),configuration(std::move(c)){}
};
SourceRuntimeSoundOutput::SourceRuntimeSoundOutput(SourceRuntimeMemory&m,SdlSoundPlatformConfiguration c):impl_(std::make_unique<Impl>(m,std::move(c))){}
SourceRuntimeSoundOutput::~SourceRuntimeSoundOutput(){abort_new_device();}
std::uint32_t SourceRuntimeSoundOutput::device_id()const noexcept{return impl_->platform?impl_->platform->device_id():0;}
bool SourceRuntimeSoundOutput::quiescent()const noexcept{return !impl_->platform&&!impl_->cleanup_fault;}
void SourceRuntimeSoundOutput::require_quiescent()const{if(!quiescent())throw FormatError("Live or faulted native sound output cannot cross a source world checkpoint");}
void SourceRuntimeSoundOutput::close(){
    if(impl_->cleanup_fault)throw FormatError("Native sound output cleanup lost its source binding");
    if(!impl_->platform)return;
    if(word(impl_->bank,impl_->output)!=device_id())throw FormatError("Native sound output identity was changed by another owner");
    put(impl_->bank,impl_->output,impl_->prior);impl_->platform.reset();impl_->output=impl_->prior=0;
}
void SourceRuntimeSoundOutput::abort_new_device()noexcept{
    try{close();}catch(...){impl_->platform.reset();impl_->cleanup_fault=true;}
}
SourceRuntimeSoundOutput::CallScope::CallScope(SourceRuntimeSoundOutput*p):owner_(p),existing_(p&&p->device_id()),exceptions_(std::uncaught_exceptions()){}
SourceRuntimeSoundOutput::CallScope::~CallScope(){if(owner_&&!existing_&&std::uncaught_exceptions()>exceptions_)owner_->abort_new_device();}
bool SourceRuntimeSoundOutput::dispatch(SourcePhysicsContactMemory&m,const SourcePhysicsContactRuntimeCall&call,SourcePhysicsWorldRuntimeAbi&a){
    if(call.target!=0x387877u)return false;
    if(&m!=&impl_->bank||call.tail||call.instruction!=0x224f26u||word(m,a.gp[4])!=0x224f2bu)
        throw FormatError("Native output creation requires its reached TU1.5 driver caller ABI");
    const auto sp=a.gp[4],guid=word(m,sp+4),output=word(m,sp+8),outer=word(m,sp+12),driver=word(m,0x52d674u);
    if(!driver||output!=driver+0x2ab0u||guid||outer||word(m,output)||std::to_integer<unsigned>(impl_->bank.view(driver,1)[0]))
        throw FormatError("Unsupported native output request or already initialized source driver");
    require_quiescent();(void)impl_->bank.mutable_view(output,4);
    auto device=std::make_unique<SdlSoundPlatform>(impl_->configuration);
    const auto identity=device->device_id();if(!identity)throw FormatError("Native sound constructor did not produce an actual device");
    // Publish only after SDL has opened the requested device. Capabilities,
    // driver-ready flags and all downstream results are left to their owners.
    put(impl_->bank,output,identity);impl_->output=output;impl_->prior=0;impl_->platform=std::move(device);
    a.gp[0]=0;a.gp[4]+=16;return true;
}
}

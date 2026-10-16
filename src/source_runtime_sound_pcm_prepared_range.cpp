#include "halo2/source_runtime_sound_pcm_prepared_range.hpp"
#include <stdexcept>

namespace halo2 {
SourceRuntimeSoundPcmPreparedRange::SourceRuntimeSoundPcmPreparedRange(SourceRuntimeMemory& memory,
    std::shared_ptr<SourceRuntimeSoundPcmBuffers> buffers,Kind kind):kind_(kind),buffers_(std::move(buffers)){
    if(!buffers_)throw std::invalid_argument("Native prepared PCM range lacks its actual binding owner");
    buffers_->require_thread();
    if(&memory!=&buffers_->memory_)
        throw std::invalid_argument("Native prepared PCM range belongs to another source bank");
    const auto& binding=buffers_->resolve(kind_);
    const auto ep=buffers_->ep_->diagnostics();
    if(binding.buffer_source!=ep.layout.data.source||binding.buffer_physical!=ep.layout.data.physical||
        binding.native_buffer!=ep.layout.data.native||binding.bytes!=0x2000||ep.layout.rings[0].base||
        ep.layout.rings[0].end!=0x2000||ep.layout.rings[0].pending_source_cursor!=0x600)
        throw std::invalid_argument("Native prepared PCM range differs from the genuine default shared EP0 binding");
    native_=binding.native_buffer;
    lease_=buffers_->ep_->acquire_prepared_range(native_,0,0x600);
    // Default analog/SPDIF descriptors mirror one EP0 physical domain. Either
    // channel's retirement must revoke its shared prepared-byte borrower.
    try{for(const auto& [kind,row]:buffers_->bindings_)
        if(row.native_buffer==native_)buffers_->prepared_borrowers_[kind]=lease_;}
    catch(...){lease_->cancel();throw;}
}
SourceRuntimeSoundPcmPreparedRange::~SourceRuntimeSoundPcmPreparedRange(){
    if(std::this_thread::get_id()!=thread_)std::terminate();retire();
}
void SourceRuntimeSoundPcmPreparedRange::require_thread()const{
    if(std::this_thread::get_id()!=thread_)throw std::runtime_error("Native prepared PCM owner requires its creating owner thread");
}
std::shared_ptr<SourceRuntimeSoundPcmPreparedRange::Lease> SourceRuntimeSoundPcmPreparedRange::borrow()const{
    require_thread();if(retired_||!lease_||lease_->state().cancelled)
        throw std::runtime_error("Native prepared PCM owner is retired or its byte lease was revoked");
    return lease_;
}
SourceRuntimeSoundPcmPreparedRange::State SourceRuntimeSoundPcmPreparedRange::diagnostics()const{
    require_thread();return {kind_,0,0x600,0x600,native_,retired_||!lease_||lease_->state().cancelled,false,false,false};
}
void SourceRuntimeSoundPcmPreparedRange::retire(){
    require_thread();if(retired_)return;if(lease_)lease_->cancel();retired_=true;lease_.reset();buffers_.reset();
}
bool SourceRuntimeSoundPcmPreparedRange::quiescent()const noexcept{return std::this_thread::get_id()==thread_&&retired_;}
}

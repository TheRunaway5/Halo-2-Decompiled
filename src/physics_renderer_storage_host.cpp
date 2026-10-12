#include "halo2/physics_renderer_storage_runtime.hpp"
#include "physics_renderer_storage_call_frames.hpp"
namespace halo2 {
void source_run_physics_renderer_storage_with_native_pages(SourceRuntimeMemory &m,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices &services,
    SourcePhysicsPoolHostStorage &host,const SourcePhysicsContactRecordObserver &observe) {
    if(!host.backs(m))throw std::runtime_error("Original renderer storage bank owner differs");
    auto joined=services;
    joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &observer) {
        if(&bank!=&m)throw std::runtime_error("Original renderer storage retained bank differs");
        if(host.try_call(bank,call,state))return;
        for(const auto &frame:physics_renderer_storage_frames::frames) {
            if(frame.site!=call.instruction || frame.target!=call.target)continue;
            std::array<std::byte,4> raw;bank.read(state.gp[4],raw);std::uint32_t ret=0;
            for(unsigned i=0;i<4;++i)ret|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);
            if(call.tail || ret!=frame.return_address)throw std::runtime_error("Original TU1.5 renderer storage call frame differs");
            using Module=physics_renderer_storage_frames::Module;
            if(frame.module==Module::Pool) {
                source_run_physics_pool_startup_runtime(bank,frame.entry,state,
                    SourcePhysicsContactImage::TitleUpdate15,fp,joined,observer);return;
            }
            if(services.required_owner){services.required_owner(bank,call,state,observer);return;}
            if(frame.module==Module::Heap)throw SourcePhysicsRendererAllocatorUnavailable(call);
            throw std::runtime_error("Required original renderer allocation-error owner unavailable");
        }
        if(!services.required_owner)throw std::runtime_error("Required original renderer storage callee unavailable");
        services.required_owner(bank,call,state,observer);
    };
    source_run_physics_renderer_storage_runtime(m,entry,abi,SourcePhysicsContactImage::TitleUpdate15,fp,joined,observe);
}
} // namespace halo2

#include "halo2/physics_early_initializer_runtime.hpp"
#include "halo2/physics_game_arena_runtime.hpp"
#include "halo2/physics_main_heap_runtime.hpp"
#include "physics/physics_early_initializer_call_frames.hpp"
namespace halo2 {
void source_run_physics_early_initializer_with_native_storage(SourceRuntimeMemory &m,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices &services,
    SourcePhysicsPoolHostStorage &host,const SourcePhysicsContactRecordObserver &observe) {
    if(!host.backs(m))throw std::runtime_error("Original early initializer bank owner differs");
    auto joined=services;
    joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &observer) {
        if(&bank!=&m)throw std::runtime_error("Original early initializer retained bank differs");
        if(host.try_call(bank,call,state))return;
        for(const auto &frame:physics_early_initializer_frames::frames) {
            if(frame.site!=call.instruction || frame.target!=call.target)continue;
            std::array<std::byte,4> raw;bank.read(state.gp[4],raw);std::uint32_t ret=0;
            for(unsigned i=0;i<4;++i)ret|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);
            if(call.tail || ret!=frame.return_address)
                throw std::runtime_error("Original TU1.5 early initializer call frame differs");
            using Module=physics_early_initializer_frames::Module;
            switch(frame.module) {
            case Module::Arena:
                source_run_physics_game_arena_runtime(bank,frame.entry,state,
                    SourcePhysicsContactImage::TitleUpdate15,fp,joined,observer);return;
            case Module::Pool:
                source_run_physics_pool_startup_runtime(bank,frame.entry,state,
                    SourcePhysicsContactImage::TitleUpdate15,fp,joined,observer);return;
            case Module::MainHeap:
                source_run_physics_main_heap_runtime(bank,frame.entry,state,
                    SourcePhysicsContactImage::TitleUpdate15,fp,joined,observer);return;
            case Module::Renderer:
                if(services.required_owner){services.required_owner(bank,call,state,observer);return;}
                throw SourcePhysicsEarlyRendererUnavailable(call);
            case Module::Required:
                if(services.required_owner){services.required_owner(bank,call,state,observer);return;}
                throw std::runtime_error("Required original main heap allocation-error owner unavailable");
            }
        }
        if(!services.required_owner)
            throw std::runtime_error("Required original early initializer callee unavailable");
        services.required_owner(bank,call,state,observer);
    };
    source_run_physics_early_initializer_runtime(m,entry,abi,
        SourcePhysicsContactImage::TitleUpdate15,fp,joined,observe);
}
} // namespace halo2

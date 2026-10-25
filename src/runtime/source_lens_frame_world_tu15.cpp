#include "halo2/source_lens_frame_world_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"

namespace halo2 {
SourceLensFrameWorldTu15::SourceLensFrameWorldTu15(PhysicsLiveWorldTu15 &world):owner_(&world) {
    static constexpr Operations operations{
        [](void *p)->SourceRuntimeMemory & {return static_cast<PhysicsLiveWorldTu15 *>(p)->memory();},
        [](void *p,std::span<const std::uint32_t> a){return static_cast<PhysicsLiveWorldTu15 *>(p)->prepare_source_call(a);},
        [](void *p,std::uint32_t e,SourcePhysicsWorldRuntimeAbi &a){static_cast<PhysicsLiveWorldTu15 *>(p)->run_actual(e,a);},
        [](void *p,std::uint32_t e){return static_cast<PhysicsLiveWorldTu15 *>(p)->call_actual(e);},
        [](void *p,std::uint32_t at,std::span<const std::byte> b){static_cast<PhysicsLiveWorldTu15 *>(p)->write_source_call_input(at,b);},
        [](void *p,const Cache &c,std::uint32_t at,std::size_t n){return static_cast<PhysicsLiveWorldTu15 *>(p)->resident_cache_address(c,at,n);}
    };
    operations_=&operations;
}
SourceRuntimeMemory &SourceLensFrameWorldTu15::memory()const {return operations_->memory(owner_);}
SourcePhysicsWorldRuntimeAbi SourceLensFrameWorldTu15::prepare_source_call(std::span<const std::uint32_t> a)const {
    return operations_->prepare(owner_,a);
}
void SourceLensFrameWorldTu15::run_actual(std::uint32_t e,SourcePhysicsWorldRuntimeAbi &a)const {operations_->run(owner_,e,a);}
std::uint32_t SourceLensFrameWorldTu15::call_actual(std::uint32_t e)const {
    return operations_->call(owner_,e);
}
void SourceLensFrameWorldTu15::write_source_call_input(std::uint32_t at,std::span<const std::byte> b)const {operations_->input(owner_,at,b);}
NativeRuntimeAddress SourceLensFrameWorldTu15::resident_cache_address(const Cache &c,std::uint32_t at,std::size_t n)const {
    return operations_->resident(owner_,c,at,n);
}
}

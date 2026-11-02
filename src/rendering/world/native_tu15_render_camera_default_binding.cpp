#include "halo2/native_tu15_render_camera_default_binding.hpp"
#include "halo2/native_tu15_render_camera_default.hpp"
#include <atomic>
#include <cstring>
#include <stdexcept>
namespace halo2 {
namespace {
std::atomic<std::uint64_t> calls{};
std::atomic<std::uint64_t> initializations{};
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at) {
    std::array<std::byte,4> raw{};memory.read(at,raw);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(8u*n);return value;
}
class Argument final:public NativeTu15RenderCameraGridArgument {
    const SourcePhysicsContactMemory &memory_;std::uint32_t stack_;
public:
    Argument(const SourcePhysicsContactMemory &memory,std::uint32_t stack):memory_(memory),stack_(stack){}
    std::uint32_t grid()const override{return read(memory_,stack_+4u);}
};
}
bool native_tu15_render_camera_default_owns_entry(std::uint32_t entry){return entry==0x2f7e0u || entry==0x380450u;}
void native_tu15_render_camera_default_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(entry==0x380450u) {
        if(caller!=0u && caller!=0x2dbbf9u)throw std::invalid_argument("Unqualified native default camera initializer caller");
        initializations.fetch_add(1,std::memory_order_relaxed);
        native_tu15_render_camera_default_initialize(memory,observe);
        (void)read(memory,abi.gp[4]);abi.gp[4]+=4u;return;
    }
    if(entry!=0x2f7e0u || abi.gp[0]!=0u || (caller!=0x229b95u&&caller!=0x229c30u))
        throw std::invalid_argument("Unqualified native render camera entry/caller contract");
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth>8)
        throw std::invalid_argument("Native render camera requires its actual retained FP policy");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};Argument argument(memory,abi.gp[4]);
    NativeTu15RenderCameraX87State state;state.occupied=std::uint8_t((1u<<abi.x87_depth)-1u);
    for(unsigned n=0;n<abi.x87_depth;++n)std::memcpy(state.values[n].data(),&abi.x87[n],10);
    const auto publish_fp=[&] {
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
        unsigned depth{};while(depth<8 && (state.occupied&(1u<<depth)))++depth;
        if(state.occupied!=std::uint8_t((1u<<depth)-1u))throw NativeTu15RenderCameraDefaultX87AbiRequired(state);
        for(unsigned n=0;n<depth;++n)std::memcpy(&abi.x87[n],state.values[n].data(),10);
        abi.x87_depth=depth;
    };
    calls.fetch_add(1,std::memory_order_relaxed);
    try{native_tu15_render_camera_default_packet(memory,abi.gp[1],argument,fp,state,observe);}
    catch(...){publish_fp();throw;}
    publish_fp();
    (void)read(memory,abi.gp[4]);abi.gp[4]+=8u;
}
std::uint64_t native_tu15_render_camera_default_calls(){return calls.load(std::memory_order_relaxed);}
std::uint64_t native_tu15_render_camera_default_initialize_calls(){return initializations.load(std::memory_order_relaxed);}
}

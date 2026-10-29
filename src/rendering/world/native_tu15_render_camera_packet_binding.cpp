#include "halo2/native_tu15_render_camera_packet_binding.hpp"
#include "halo2/native_tu15_render_camera_packet.hpp"
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {
std::atomic<std::uint64_t> calls{};
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
bool native_tu15_render_camera_packet_owns_entry(std::uint32_t entry){return entry==0x2f7e0u;}
void native_tu15_render_camera_packet_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!native_tu15_render_camera_packet_owns_entry(entry) || (caller!=0x229b95u&&caller!=0x229c30u))
        throw std::invalid_argument("Unqualified native render camera entry/caller contract");
    if(!abi.floating_point)throw std::invalid_argument("Native render camera requires its actual retained FP policy");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};Argument argument(memory,abi.gp[4]);
    calls.fetch_add(1,std::memory_order_relaxed);
    try{native_tu15_render_camera_packet(memory,abi.gp[0],abi.gp[1],abi.gp[2],argument,fp,observe);}
    catch(...){abi.floating_point->mxcsr=fp.mxcsr;throw;}
    abi.floating_point->mxcsr=fp.mxcsr;
    (void)read(memory,abi.gp[4]);abi.gp[4]+=8u;
}
std::uint64_t native_tu15_render_camera_packet_calls(){return calls.load(std::memory_order_relaxed);}
}

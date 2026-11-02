#include "halo2/native_tu15_camera_bsp_selection_binding.hpp"
#include "halo2/native_tu15_camera_bsp_selection.hpp"
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {
std::atomic<std::uint64_t> calls{};
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> raw{};memory.read(address,raw);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(8u*n);return value;
}
class Argument final:public NativeTu15CameraBspSelectionArgument {
    const SourcePhysicsContactMemory &memory_;std::uint32_t stack_;
public:
    Argument(const SourcePhysicsContactMemory &memory,std::uint32_t stack):memory_(memory),stack_(stack){}
    std::uint32_t cluster_output()const override{return read(memory_,stack_+4u);}
};
}
bool native_tu15_camera_bsp_selection_owns_entry(std::uint32_t entry){return entry==0x2b700u;}
void native_tu15_camera_bsp_selection_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!native_tu15_camera_bsp_selection_owns_entry(entry)||caller!=0x2b7e6u)
        throw std::invalid_argument("Unqualified native camera BSP entry/caller contract");
    if(!abi.floating_point)throw std::invalid_argument("Native camera BSP requires its actual FP policy");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    Argument argument(memory,abi.gp[4]);bool selected;calls.fetch_add(1,std::memory_order_relaxed);
    try{selected=native_tu15_camera_bsp_select(memory,abi.gp[2],abi.gp[7],argument,fp,observe);}
    catch(...){abi.floating_point->mxcsr=fp.mxcsr;throw;}
    abi.floating_point->mxcsr=fp.mxcsr;
    (void)read(memory,abi.gp[4]);abi.gp[0]=(abi.gp[0]&0xffffff00u)|(selected?1u:0u);abi.gp[4]+=8u;
}
std::uint64_t native_tu15_camera_bsp_selection_calls(){return calls.load(std::memory_order_relaxed);}
}

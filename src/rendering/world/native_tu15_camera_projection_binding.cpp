#include "halo2/native_tu15_camera_projection_binding.hpp"
#include "halo2/native_tu15_camera_projection.hpp"
#include <atomic>
#include <cstring>
namespace halo2 {
NativeTu15CameraProjectionX87AbiRequired::NativeTu15CameraProjectionX87AbiRequired(
    const NativeTu15CameraProjectionX87State &state,const NativeTu15PhysicsFrameFp &fp,std::exception_ptr cause):
    std::runtime_error("Native projection requires an owner for its sparse x87 ABI result"),state_(state),fp_(fp),cause_(std::move(cause)){}
namespace {
std::atomic<std::uint64_t> calls{};
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> raw{};memory.read(address,raw);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(8u*n);return value;
}
}
bool native_tu15_camera_projection_owns_entry(std::uint32_t entry){return entry==0x2fb50u||entry==0x2fd70u;}
bool native_tu15_camera_projection_owns_call(std::uint32_t entry,std::uint32_t caller) {
    return (entry==0x2fb50u&&caller==0x2fd79u)||
        (entry==0x2fd70u&&(caller==0x13c9bu||caller==0x2bab7u||caller==0x2bafcu));
}
void native_tu15_camera_projection_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!(native_tu15_camera_projection_owns_call(entry,caller)||(entry==0x144090u&&caller==0x2ffe9u)))
        throw std::invalid_argument("Unqualified native projection entry/caller");
    if(!abi.floating_point||(abi.eflags&0x400u)||abi.active_mmx||abi.x87_depth>8)
        throw std::invalid_argument("Native projection requires its actual FP/direction caller policy");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    NativeTu15CameraProjectionX87State state;state.occupied=std::uint8_t((1u<<abi.x87_depth)-1u);
    for(unsigned n=0;n<abi.x87_depth;++n)std::memcpy(state.values[n].data(),&abi.x87[n],10);
    calls.fetch_add(1,std::memory_order_relaxed);std::exception_ptr failure;
    try {
        if(entry==0x2fb50u)native_tu15_camera_projection_program_publish(memory,abi.gp[1],abi.gp[2],abi.gp[0],fp,state,observe);
        else if(entry==0x144090u)native_tu15_camera_inverse_frame(memory,abi.gp[1],abi.gp[0],fp,observe);
        else native_tu15_camera_projection(memory,abi.gp[6],abi.gp[2],abi.gp[3],fp,state,observe);
    }catch(...){failure=std::current_exception();}
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
    unsigned depth{};while(depth<8&&(state.occupied&(1u<<depth)))++depth;
    if(state.occupied!=std::uint8_t((1u<<depth)-1u))throw NativeTu15CameraProjectionX87AbiRequired(state,fp,failure);
    for(unsigned n=0;n<depth;++n)std::memcpy(&abi.x87[n],state.values[n].data(),10);
    abi.x87_depth=depth;if(failure)std::rethrow_exception(failure);
    (void)read(memory,abi.gp[4]);abi.gp[4]+=4u;
}
std::uint64_t native_tu15_camera_projection_calls(){return calls.load(std::memory_order_relaxed);}
}

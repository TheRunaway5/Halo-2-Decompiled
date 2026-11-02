#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <array>
#include <exception>
#include <stdexcept>
namespace halo2 {
// Native hardware context at an ABI boundary, not an arithmetic interpreter.
// All projection operations execute on actual SSE/x87 hardware. Logical80-bit
// values and validity preserve older callers and source fault boundaries.
struct NativeTu15CameraProjectionX87State {
    std::array<std::array<std::byte,10>,8> values{};
    std::uint8_t occupied{};
};
class NativeTu15CameraProjectionX87AbiRequired final:public std::runtime_error {
    NativeTu15CameraProjectionX87State state_;NativeTu15PhysicsFrameFp fp_;std::exception_ptr cause_;
public:
    NativeTu15CameraProjectionX87AbiRequired(const NativeTu15CameraProjectionX87State &,
        const NativeTu15PhysicsFrameFp &,std::exception_ptr = {});
    const NativeTu15CameraProjectionX87State &state()const{return state_;}
    const NativeTu15PhysicsFrameFp &floating_point()const{return fp_;}
    const std::exception_ptr &cause()const{return cause_;}
};
struct NativeTu15CameraProjectionProgram {
    std::array<std::uint32_t,4> bounds{};
    std::uint32_t width{},height{},half_width{},half_height{},center_x{},center_y{},horizontal{},vertical{};
    std::array<std::uint32_t,4> clipping{};
};
// Actual2FB50, including lazy mutable context owners and native FPTAN. The
// value variant gives its caller a real private projection program without
// creating a source stack address; raw publication retains source aliases.
NativeTu15CameraProjectionProgram native_tu15_camera_projection_program(
    SourcePhysicsContactMemory &,std::uint32_t camera,std::uint32_t bounds,
    NativeTu15PhysicsFrameFp &,NativeTu15CameraProjectionX87State &);
void native_tu15_camera_projection_program_publish(SourcePhysicsContactMemory &,
    std::uint32_t camera,std::uint32_t bounds,std::uint32_t destination,
    NativeTu15PhysicsFrameFp &,NativeTu15CameraProjectionX87State &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete144090 retained-layout affine inverse, including its zero-scale
// thirteen-DWORD publication and source read/publication order. The ordinary
// DF=0 caller profile gives its zero-scale forward clear its original meaning.
void native_tu15_camera_inverse_frame(SourcePhysicsContactMemory &,std::uint32_t source,
    std::uint32_t destination,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete2FD70→2FB50/144090 retained0xC0 view/projection producer.
void native_tu15_camera_projection(SourcePhysicsContactMemory &,std::uint32_t camera,
    std::uint32_t bounds,std::uint32_t destination,NativeTu15PhysicsFrameFp &,
    NativeTu15CameraProjectionX87State &,const SourcePhysicsContactRecordObserver & = {});
}

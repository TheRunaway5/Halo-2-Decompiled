#pragma once
#include <array>
#include <cstdint>

namespace halo2 {
class SourcePhysicsContactMemory;
// Actual retained4C6190/515EDC camera bank,51DF50 cinematic auxiliary
// pointer/+5 byte,491E27 submitting-view byte, and1DCD0 target rows18/20.
// Allocation presence is the actual record's +88 word; +95 retires a row.
struct SourceTextureCameraTargetTu15 {
    std::uint32_t allocation{};
    std::uint8_t retiring{};
};
struct SourceTextureCameraInputsTu15 {
    std::array<std::uint32_t,12> camera_words{};
    std::uint32_t cinematic_auxiliary{};
    std::uint8_t cinematic{},texture_camera_view{},copy_complete{};
    SourceTextureCameraTargetTu15 target18,target20;
};
// Reads every required owner; unavailable banks are errors, never zero inputs.
SourceTextureCameraInputsTu15 source_texture_camera_inputs_tu15(const SourcePhysicsContactMemory &);
enum class SourceTextureCameraTargetActionTu15 {none,allocate18,retire18};
struct SourceTextureCameraLifecycleTu15 {
    bool needed{};
    SourceTextureCameraTargetActionTu15 action{};
    std::uint32_t width{},height{};
};
// Exact1DE30 suffix after the independent busy-resource reclamation loop.
// This describes an actual allocation/retirement request; it does not perform
// GPU allocation, supply an allocation result, or render the camera view.
SourceTextureCameraLifecycleTu15 source_texture_camera_lifecycle_tu15(const SourceTextureCameraInputsTu15 &);
enum class SourceTextureCameraTextureTu15 {default2d,target18,target20};
class SourceTextureCameraBindingTu15 {
public:
    SourceTextureCameraTextureTu15 texture() const {return texture_;}
private:
    friend SourceTextureCameraBindingTu15 source_texture_camera_binding_tu15(const SourceTextureCameraInputsTu15 &);
    explicit SourceTextureCameraBindingTu15(SourceTextureCameraTextureTu15 texture):texture_(texture){}
    SourceTextureCameraTextureTu15 texture_;
};
// Exact19510 extern18/25940 selection. A still-pending1DE30 request rejects
// before binding, so an unavailable native producer cannot select a default
// bitmap by omitting its resource. Real target image ownership remains with
// the caller even when this CPU selection returns target18 or target20.
SourceTextureCameraBindingTu15 source_texture_camera_binding_tu15(const SourceTextureCameraInputsTu15 &);
}

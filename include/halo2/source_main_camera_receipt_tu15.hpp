#pragma once
#include "halo2/native_runtime_address.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
namespace halo2 {
class PhysicsLiveWorldTu15;class SourceRuntimeMemory;class SourceMainCameraRasterUseTu15;
namespace detail {struct MainReceiptFrame;struct MainReadBorrow;}
// Private CPU prototype: actual completed camera and logical raster operands.
// Physical Renderer surfaces and original SDK submission are not owned here.
struct SourceMainCameraOperandsTu15 {
    std::array<std::uint32_t,48> camera_words{},projection_words{};
    std::array<std::uint32_t,29> raw_camera_words{};
    std::array<std::uint32_t,4> clip_words{};
    std::array<std::int16_t,4> viewport{};
    std::array<std::uint32_t,2> source_aa{},near_far_words{};
    std::int32_t local_players{};
    std::int16_t submitting_mode{};
    std::uint8_t submitting_scaled{};
    std::uint32_t timed_effect_source{},game_arena_source{},game_arena_used{};
    NativeRuntimeAddress timed_effect_backing;
    std::array<std::uint32_t,5> timed_effect_overrides{}; // +AC,+3C,+40,+190,+194
    std::uint32_t frame{},tick{},bsp{},clock{},view{};
    bool operator==(const SourceMainCameraOperandsTu15&)const=default;
};
class SourceRenderMemoryBorrowTu15 {
public:
    SourceRenderMemoryBorrowTu15(const SourceRenderMemoryBorrowTu15&)=delete;
    SourceRenderMemoryBorrowTu15&operator=(const SourceRenderMemoryBorrowTu15&)=delete;
private:
    friend class PhysicsLiveWorldTu15;
    friend class SourceMainCameraRasterReceiptTu15;
    explicit SourceRenderMemoryBorrowTu15(std::weak_ptr<const detail::MainReadBorrow> scope):scope_(std::move(scope)){}
    std::weak_ptr<const detail::MainReadBorrow> scope_;
};
// Copyable synchronous consumer capability. It owns completed operands, but
// only weak actual World/frame and borrow-scope authority: a retained copy
// cannot keep the World or a read-only callback alive.
class SourceMainCameraRasterUseTu15 {
public:
    SourceMainCameraRasterUseTu15(const SourceMainCameraRasterUseTu15&)=default;
    SourceMainCameraRasterUseTu15&operator=(const SourceMainCameraRasterUseTu15&)=default;
    SourceMainCameraRasterUseTu15(SourceMainCameraRasterUseTu15&&)=default;
    SourceMainCameraRasterUseTu15&operator=(SourceMainCameraRasterUseTu15&&)=default;
    const SourceMainCameraOperandsTu15& operands()const &;
    const SourceMainCameraOperandsTu15& operands()const &&=delete;
private:
    friend class SourceMainCameraRasterReceiptTu15;
    SourceMainCameraRasterUseTu15(std::weak_ptr<const detail::MainReceiptFrame> frame,
        std::weak_ptr<const detail::MainReadBorrow> scope,SourceMainCameraOperandsTu15 data)
        :frame_(std::move(frame)),scope_(std::move(scope)),data_(std::move(data)){}
    std::weak_ptr<const detail::MainReceiptFrame> frame_;
    std::weak_ptr<const detail::MainReadBorrow> scope_;
    SourceMainCameraOperandsTu15 data_;
};
class SourceMainCameraRasterReceiptTu15 {
public:
    // Access requires a live genuine World read-only scope and actual view.
    const SourceMainCameraOperandsTu15& operands(const SourceRenderMemoryBorrowTu15&,unsigned view)const;
    SourceMainCameraRasterUseTu15 bind(const SourceRenderMemoryBorrowTu15&,unsigned view)const;
    bool expired()const noexcept;
private:
    friend class PhysicsLiveWorldTu15;
    SourceMainCameraRasterReceiptTu15(std::weak_ptr<const detail::MainReceiptFrame> frame,SourceMainCameraOperandsTu15 data):frame_(std::move(frame)),data_(std::move(data)){}
    std::weak_ptr<const detail::MainReceiptFrame> frame_;
    SourceMainCameraOperandsTu15 data_;
};
using SourceCompletedMainCameraObserverTu15=std::function<void(const SourceMainCameraRasterReceiptTu15&,const SourceRuntimeMemory&)>;
using SourceRenderMemoryInspectorTu15=std::function<void(const SourceRenderMemoryBorrowTu15&,const SourceRuntimeMemory&)>;
} //namespace halo2

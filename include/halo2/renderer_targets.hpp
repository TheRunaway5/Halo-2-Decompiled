#pragma once
#include "halo2/renderer_device_lease.hpp"
#include <functional>
#include <optional>
#include <thread>
#include <vector>

namespace halo2 {
class Renderer;
class SourceWeatherDepthAlias;
struct RendererTargetImage {
    std::uint64_t image{},view{};
    NativeRuntimeAddress image_backing,view_backing;
    std::uint32_t format{},width{},height{},usage{},samples{},mips{},layers{};
};
struct RendererTargetBank {
    std::uint64_t generation{};
    std::vector<RendererTargetImage> color;
    RendererTargetImage depth_stencil;
    // An observation of a submitted frame, not a GPU-completion guarantee.
    std::optional<std::uint32_t> last_submitted_color;
    std::uint32_t presentation_images{},next_source_color{};
    std::uint64_t source_frames_submitted{};
    bool source_double_buffer{};
};
// Actual renderer attachments. Explicit source mode realizes two independent
// color banks; presentation image count remains a separate backend fact.
// Uses retain the native implementation, reject retirement/recreation and are
// confined to the issuing renderer thread. Borrowed handles must not outlive
// the use or be consumed after valid() becomes false.
class RendererTargets {
public:
    class Use {
    public:
        Use(Use&&) noexcept=default;
        Use& operator=(Use&&) noexcept=default;
        bool valid() const;
        const RendererTargetBank& bank() const;
        NativeRuntimeAddress device_address() const;
        // Native-format bytes after the submitted producer; diagnostic only.
        std::vector<std::byte> source_color_pixels(std::uint32_t) const;
        // Borrow the actual renderer-owned producer for the original depth
        // texture/color views. Prepared resources are not produced data.
        // The borrower must retain this use and check valid() before every
        // operation; it cannot survive attachment recreation or retirement.
        SourceWeatherDepthAlias& depth_aliases() const;
        // Current SDK-style surface0 rotation after submitted presentation.
        // Unlike bank(), this observes the live source frame ordinal.
        std::uint32_t current_source_color() const;
    private:
        friend class RendererTargets;
        Use(RendererDeviceLease,RendererDeviceLease::Use,RendererTargetBank,
            std::function<std::uint64_t()>,std::function<std::vector<std::byte>(std::uint32_t)>,
            std::function<SourceWeatherDepthAlias&()>,std::function<std::uint32_t()>,std::thread::id);
        RendererDeviceLease device_;
        RendererDeviceLease::Use owner_;
        RendererTargetBank bank_;
        std::function<std::uint64_t()> generation_;
        std::function<std::vector<std::byte>(std::uint32_t)> readback_;
        std::function<SourceWeatherDepthAlias&()> depth_aliases_;
        std::function<std::uint32_t()> current_color_;
        std::thread::id thread_;
    };
    RendererTargets()=default;
    bool valid() const;
    Use acquire() const;
    // Realize the original two physical color banks independently of the
    // presentation pool. Idempotent; all earlier attachment uses expire.
    Use source_buffers(std::uint32_t count) const;
private:
    friend class Renderer;
    static RendererTargets issue(RendererDeviceLease,std::function<RendererTargetBank()>,
                                  std::function<std::uint64_t()>,std::function<void(std::uint32_t)>,
                                  std::function<std::vector<std::byte>(std::uint32_t)>,
                                  std::function<SourceWeatherDepthAlias&()>);
    RendererDeviceLease device_;
    std::function<RendererTargetBank()> query_;
    std::function<std::uint64_t()> generation_;
    std::function<void(std::uint32_t)> source_buffers_;
    std::function<std::vector<std::byte>(std::uint32_t)> readback_;
    std::function<SourceWeatherDepthAlias&()> depth_aliases_;
    std::thread::id thread_;
};
}

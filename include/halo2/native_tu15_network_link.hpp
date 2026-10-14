#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <optional>
#include <string>
#include <string_view>

namespace halo2 {
enum class NativeNetworkDuplex {Unknown,Half,Full};
struct NativeNetworkLinkObservation {
    std::string interface_name;
    bool administratively_up{};
    std::optional<bool> carrier;
    std::optional<std::uint32_t> speed_mbps;
    NativeNetworkDuplex duplex=NativeNetworkDuplex::Unknown;
};
// Native host observation, not an Xbox PHY snapshot. Unknown speed/duplex
// remains unknown; missing carrier on an enabled device is an error.
NativeNetworkLinkObservation observe_native_network_link(std::string_view preferred_interface={});
std::uint32_t native_tu15_network_link_flags(const NativeNetworkLinkObservation&);

// Native implementation of the XNetGetEthernetLinkStatus API used by the
// recovered game callers. It queries actual OS state. This cdecl port ABI
// returns EAX and consumes RET, preserving other source CPU state; it does
// not claim Xbox PHY initialization, IRQL or kernel clobber behavior.
class NativeTu15NetworkLink final {
public:
    explicit NativeTu15NetworkLink(SourceRuntimeMemory&,std::string preferred_interface={});
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    const std::optional<NativeNetworkLinkObservation>& last_observation()const noexcept{return last_;}
private:
    SourceRuntimeMemory& memory_;
    std::string preferred_;
    std::optional<NativeNetworkLinkObservation> last_;
};
}

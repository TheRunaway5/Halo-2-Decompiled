#pragma once
#include "halo2/native_tu15_network_startup.hpp"
#include <array>
#include <optional>
#include <string>
#include <vector>
namespace halo2 {
enum class NativeIpv4Assignment {Static,Dhcp};
struct NativeTitleAddressObservation {
    std::string interface_name;
    std::array<std::uint8_t,6> ethernet{};
    std::optional<std::array<std::uint8_t,4>> ipv4;
    std::optional<NativeIpv4Assignment> assignment;
    std::optional<std::array<std::uint8_t,4>> gateway;
    std::vector<std::string> dns;
    bool acquisition_pending{};
};
NativeTitleAddressObservation observe_native_title_address(std::string_view preferred_interface={});
std::array<std::byte,36> native_tu15_title_address_bytes(const NativeTitleAddressObservation&);
std::uint32_t native_tu15_title_address_flags(const NativeTitleAddressObservation&);
// Native LAN address API: real host IPv4/MAC and observed assignment/config.
// No Xbox Live address, port, identity, IRQL or Ethernet-driver state is forged.
class NativeTu15TitleAddress final {
public:
    NativeTu15TitleAddress(SourceRuntimeMemory&,NativeTu15NetworkStartup&,std::string preferred_interface={});
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&,
        const SourcePhysicsContactRecordObserver& = {});
    const std::optional<NativeTitleAddressObservation>& last_observation()const{return last_;}
private:
    SourceRuntimeMemory& memory_;
    NativeTu15NetworkStartup& network_;
    std::string preferred_;
    std::optional<NativeTitleAddressObservation> last_;
};
}

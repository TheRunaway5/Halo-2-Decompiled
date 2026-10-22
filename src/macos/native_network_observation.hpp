#pragma once
#include "halo2/native_tu15_network_link.hpp"
#include "halo2/native_tu15_title_address.hpp"

namespace halo2::macos_network {
using Ipv4=std::array<std::uint8_t,4>;
struct InterfaceState {
    std::string name;
    unsigned flags{};
    std::optional<std::array<std::uint8_t,6>> ethernet;
    std::vector<Ipv4> ipv4;
};
struct ServiceState {
    std::string id,interface_name,method;
    std::vector<Ipv4> addresses,configured_addresses;
    std::optional<Ipv4> gateway;
    std::vector<std::string> dns;
    bool dhcp_lease_observed{};
};
// Admission after SystemConfiguration parsing: configuration and kernel rows
// must describe the same selected service. Unknown origins stay unsupported.
NativeTitleAddressObservation admit_title_address(const InterfaceState&,const ServiceState&,bool busy);
NativeNetworkLinkObservation observe_link(std::string_view);
NativeTitleAddressObservation observe_title_address(std::string_view);
}

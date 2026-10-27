#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include "platform/windows_api.hpp"
#include <iphlpapi.h>
#include <netioapi.h>
#include <algorithm>
#include <cstring>
#include <map>
#include <vector>
#include "halo2/native_tu15_network_link.hpp"
#include "halo2/native_tu15_title_address.hpp"

namespace halo2::windows_network {
class Adapters {
    std::vector<std::byte> bytes_;
public:
    Adapters():bytes_(16384) {
        for(unsigned attempt=0;attempt<3;++attempt) {
            ULONG size=ULONG(bytes_.size());
            const auto result=::GetAdaptersAddresses(AF_INET,GAA_FLAG_INCLUDE_GATEWAYS,nullptr,first(),&size);
            if(result==NO_ERROR)return;
            if(result==ERROR_NO_DATA){bytes_.clear();return;}
            if(result!=ERROR_BUFFER_OVERFLOW)throw std::system_error(int(result),std::system_category(),"Native Windows adapter enumeration");
            bytes_.resize(size);
        }
        throw std::runtime_error("Native Windows adapter enumeration changed repeatedly");
    }
    IP_ADAPTER_ADDRESSES* first(){return bytes_.empty()?nullptr:reinterpret_cast<IP_ADAPTER_ADDRESSES*>(bytes_.data());}
};
inline NativeNetworkLinkObservation observe_link(std::string_view preferred) {
    Adapters adapters;
    std::map<std::string,IP_ADAPTER_ADDRESSES*> names;
    for(auto* adapter=adapters.first();adapter;adapter=adapter->Next)
        if(adapter->IfType!=IF_TYPE_SOFTWARE_LOOPBACK && adapter->AdapterName)
            names.emplace(adapter->AdapterName,adapter);
    const auto observe=[](const std::string& name,const IP_ADAPTER_ADDRESSES& adapter) {
        MIB_IF_ROW2 row{};row.InterfaceLuid=adapter.Luid;
        const auto error=::GetIfEntry2(&row);
        if(error)throw std::system_error(int(error),std::system_category(),"Native Windows link observation");
        NativeNetworkLinkObservation out;out.interface_name=name;
        out.administratively_up=row.AdminStatus==NET_IF_ADMIN_STATUS_UP;
        if(out.administratively_up)out.carrier=row.MediaConnectState==MediaConnectStateConnected;
        if(row.TransmitLinkSpeed)out.speed_mbps=std::uint32_t(std::min<ULONG64>(row.TransmitLinkSpeed/1000000,UINT32_MAX));
        return out;
    };
    if(!preferred.empty()) {
        const auto at=names.find(std::string(preferred));
        if(at==names.end())throw std::invalid_argument("Selected native Windows network interface is unavailable");
        return observe(at->first,*at->second);
    }
    std::optional<NativeNetworkLinkObservation> inactive;
    for(const auto& [name,adapter]:names) {
        auto result=observe(name,*adapter);
        if(result.administratively_up && result.carrier.value_or(false))return result;
        if(!inactive)inactive=std::move(result);
    }
    return inactive.value_or(NativeNetworkLinkObservation{});
}
inline std::array<std::uint8_t,4> ipv4(const SOCKADDR* address) {
    if(!address || address->sa_family!=AF_INET)throw std::runtime_error("Native Windows address has no IPv4 owner");
    std::array<std::uint8_t,4> out{};
    std::memcpy(out.data(),&reinterpret_cast<const sockaddr_in*>(address)->sin_addr,4);return out;
}
inline std::array<std::uint8_t,6> machine_identity() {
    Adapters adapters;
    for(auto* adapter=adapters.first();adapter;adapter=adapter->Next) {
        if(adapter->IfType==IF_TYPE_SOFTWARE_LOOPBACK || adapter->OperStatus!=IfOperStatusUp ||
           adapter->PhysicalAddressLength!=6)continue;
        std::array<std::uint8_t,6> out{};
        std::copy_n(adapter->PhysicalAddress,out.size(),out.begin());return out;
    }
    throw std::runtime_error("Native source player identity requires an up six-byte hardware interface");
}
inline NativeTitleAddressObservation observe_title_address(std::string_view preferred) {
    const auto link=observe_link(preferred);
    if(link.interface_name.empty())throw std::runtime_error("Native title address has no selected network interface");
    Adapters adapters;
    for(auto* adapter=adapters.first();adapter;adapter=adapter->Next) {
        if(!adapter->AdapterName || link.interface_name!=adapter->AdapterName)continue;
        NativeTitleAddressObservation out;out.interface_name=link.interface_name;
        if(adapter->PhysicalAddressLength!=out.ethernet.size())
            throw std::runtime_error("Native Windows title address requires a six-byte Ethernet address");
        std::copy_n(adapter->PhysicalAddress,out.ethernet.size(),out.ethernet.begin());
        for(auto* address=adapter->FirstUnicastAddress;address;address=address->Next) {
            if(!address->Address.lpSockaddr || address->Address.lpSockaddr->sa_family!=AF_INET)continue;
            if(address->DadState==IpDadStateTentative){out.acquisition_pending=true;continue;}
            if(address->DadState!=IpDadStatePreferred && address->DadState!=IpDadStateDeprecated)continue;
            out.ipv4=ipv4(address->Address.lpSockaddr);
            // DHCP capability is per adapter; manually configured secondary
            // addresses on that adapter retain their own assignment origin.
            if(address->PrefixOrigin==IpPrefixOriginDhcp)out.assignment=NativeIpv4Assignment::Dhcp;
            else if(address->PrefixOrigin==IpPrefixOriginManual)out.assignment=NativeIpv4Assignment::Static;
            else throw std::runtime_error("Native Windows IPv4 assignment origin is unavailable");
            out.acquisition_pending=false;break;
        }
        for(auto* address=adapter->FirstGatewayAddress;address;address=address->Next)
            if(address->Address.lpSockaddr && address->Address.lpSockaddr->sa_family==AF_INET) {
                out.gateway=ipv4(address->Address.lpSockaddr);break;
            }
        for(auto* address=adapter->FirstDnsServerAddress;address;address=address->Next) {
            if(!address->Address.lpSockaddr || address->Address.lpSockaddr->sa_family!=AF_INET)continue;
            const auto ip=ipv4(address->Address.lpSockaddr);
            out.dns.push_back(std::to_string(ip[0])+"."+std::to_string(ip[1])+"."+std::to_string(ip[2])+"."+std::to_string(ip[3]));
        }
        return out;
    }
    throw std::runtime_error("Native Windows title-address interface retired during enumeration");
}
}

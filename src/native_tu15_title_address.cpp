#include "halo2/native_tu15_title_address.hpp"
#include "halo2/native_tu15_network_link.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <memory>
#include <system_error>
#if defined(__linux__) && defined(HALO2_NATIVE_NETWORK_MANAGER)
#include <NetworkManager.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#endif
namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t p) {
    const auto bytes=memory.view(p,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return value;
}
#if defined(__linux__) && defined(HALO2_NATIVE_NETWORK_MANAGER)
std::array<std::uint8_t,4> ipv4(const char* text) {
    std::array<std::uint8_t,4> bytes{};
    if(!text || ::inet_pton(AF_INET,text,bytes.data())!=1)
        throw std::runtime_error("Native IPv4 configuration has an unsupported address");
    return bytes;
}
#endif
}
NativeTitleAddressObservation observe_native_title_address(std::string_view preferred) {
#if defined(__linux__) && defined(HALO2_NATIVE_NETWORK_MANAGER)
    const auto link=observe_native_network_link(preferred);
    if(link.interface_name.empty())throw std::runtime_error("Native title address has no selected network interface");
    NativeTitleAddressObservation out;out.interface_name=link.interface_name;
    ifaddrs* raw{};
    if(::getifaddrs(&raw))throw std::system_error(errno,std::generic_category(),"Native title-address enumeration");
    const std::unique_ptr<ifaddrs,decltype(&::freeifaddrs)> addresses(raw,::freeifaddrs);
    std::vector<std::array<std::uint8_t,4>> kernel_ipv4;bool ethernet=false;
    for(auto* row=raw;row;row=row->ifa_next) {
        if(!row->ifa_name || !row->ifa_addr || out.interface_name!=row->ifa_name)continue;
        if(row->ifa_addr->sa_family==AF_PACKET) {
            const auto* packet=reinterpret_cast<const sockaddr_ll*>(row->ifa_addr);
            if(packet->sll_halen!=6)throw std::runtime_error("Native title-address interface has no six-byte Ethernet address");
            std::copy_n(packet->sll_addr,6,out.ethernet.begin());ethernet=true;
        } else if(row->ifa_addr->sa_family==AF_INET) {
            std::array<std::uint8_t,4> bytes{};
            std::memcpy(bytes.data(),&reinterpret_cast<const sockaddr_in*>(row->ifa_addr)->sin_addr,4);kernel_ipv4.push_back(bytes);
        }
    }
    if(!ethernet)throw std::runtime_error("Native title-address Ethernet owner is unavailable");
    GError* raw_error{};NMClient* raw_client=nm_client_new(nullptr,&raw_error);
    const std::unique_ptr<GError,decltype(&g_error_free)> error(raw_error,::g_error_free);
    const std::unique_ptr<NMClient,void(*)(NMClient*)> client(raw_client,[](NMClient* value){g_object_unref(value);});
    if(!client || !nm_client_get_nm_running(client.get()))throw std::runtime_error("Native address-assignment observation requires NetworkManager");
    auto* device=nm_client_get_device_by_iface(client.get(),out.interface_name.c_str());
    if(!device)throw std::runtime_error("Selected native interface has no NetworkManager owner");
    const auto state=nm_device_get_state(device);
    if(state==NM_DEVICE_STATE_UNKNOWN)throw std::runtime_error("Native title-address configuration state is unknown");
    auto* config=nm_device_get_ip4_config(device);
    if(config) {
        if(const auto* gateway=nm_ip_config_get_gateway(config);gateway && *gateway)out.gateway=ipv4(gateway);
        // DNS availability is a configured capability flag. Retain the actual
        // manager strings, including DNS-over-TLS forms, rather than forcing
        // them into the IPv4 title-address payload.
        if(const auto* dns=nm_ip_config_get_nameservers(config))for(auto* entry=dns;*entry;++entry)
            if(**entry)out.dns.emplace_back(*entry);
    }
    const auto* configured=config?nm_ip_config_get_addresses(config):nullptr;
    if(!configured || !configured->len) {
        if(!kernel_ipv4.empty())throw std::runtime_error("Native interface IPv4 is outside the observed configuration owner");
        out.acquisition_pending=state>=NM_DEVICE_STATE_PREPARE && state<NM_DEVICE_STATE_ACTIVATED;return out;
    }
    auto* active=nm_device_get_active_connection(device);
    auto* connection=active?nm_active_connection_get_connection(active):nullptr;
    auto* setting=connection?nm_connection_get_setting_ip4_config(NM_CONNECTION(connection)):nullptr;
    const char* method=setting?nm_setting_ip_config_get_method(setting):nullptr;
    if(!method)throw std::runtime_error("Native IPv4 assignment method is unavailable");
    if(std::strcmp(method,NM_SETTING_IP4_CONFIG_METHOD_AUTO)==0) {
        auto* lease=nm_device_get_dhcp4_config(device);
        const char* address=lease?nm_dhcp_config_get_one_option(lease,"ip_address"):nullptr;
        if(!address)throw std::runtime_error("Automatic native IPv4 assignment lacks an observed DHCP address");
        out.ipv4=ipv4(address);out.assignment=NativeIpv4Assignment::Dhcp;
    } else if(std::strcmp(method,NM_SETTING_IP4_CONFIG_METHOD_MANUAL)==0 || std::strcmp(method,NM_SETTING_IP4_CONFIG_METHOD_SHARED)==0) {
        out.ipv4=ipv4(nm_ip_address_get_address(static_cast<NMIPAddress*>(configured->pdata[0])));out.assignment=NativeIpv4Assignment::Static;
    } else throw std::runtime_error("Native IPv4 assignment method is outside the supported title-address domain");
    bool in_config=false;
    for(unsigned i=0;i<configured->len;++i)
        in_config|=ipv4(nm_ip_address_get_address(static_cast<NMIPAddress*>(configured->pdata[i])))==*out.ipv4;
    if(!in_config || std::ranges::find(kernel_ipv4,*out.ipv4)==kernel_ipv4.end())
        throw std::runtime_error("Native title address differs between kernel, assignment and active configuration");
    return out;
#else
    throw std::runtime_error("Native title-address platform/assignment provider is unavailable");
#endif
}
std::array<std::byte,36> native_tu15_title_address_bytes(const NativeTitleAddressObservation& input) {
    std::array<std::byte,36> bytes{};
    if(input.ipv4)for(unsigned i=0;i<4;++i)bytes[i]=std::byte((*input.ipv4)[i]);
    for(unsigned i=0;i<6;++i)bytes[10+i]=std::byte(input.ethernet[i]);return bytes;
}
std::uint32_t native_tu15_title_address_flags(const NativeTitleAddressObservation& input) {
    if(input.acquisition_pending)return 0;
    if(bool(input.ipv4)!=bool(input.assignment))throw std::invalid_argument("Native title address has no matching observed assignment origin");
    std::uint32_t flags=input.ipv4?(*input.assignment==NativeIpv4Assignment::Dhcp?8:4):2;
    if(input.gateway && std::ranges::any_of(*input.gateway,[](auto byte){return byte!=0;}))flags|=0x20;
    if(!input.dns.empty())flags|=0x40;
    return flags;
}
NativeTu15TitleAddress::NativeTu15TitleAddress(SourceRuntimeMemory& memory,NativeTu15NetworkStartup& network,std::string preferred)
    :memory_(memory),network_(network),preferred_(std::move(preferred)){}
bool NativeTu15TitleAddress::try_call(SourcePhysicsContactMemory& bank,const SourcePhysicsContactRuntimeCall& call,
    SourcePhysicsWorldRuntimeAbi& abi,const SourcePhysicsContactRecordObserver& observe) {
    if(call.target!=0x3d725du)return false;
    if(&bank!=&memory_ || call.tail || (call.instruction!=0x70d0cu && call.instruction!=0x70833u) ||
        word(memory_,abi.gp[4])!=call.instruction+5)
        throw std::invalid_argument("Native title-address caller/bank differs");
    if(!network_.service())throw std::runtime_error("Native title-address query precedes its network service owner");
    const auto output=word(memory_,abi.gp[4]+4);
    if(output!=abi.gp[4]+8u)throw std::invalid_argument("Native title-address output lost its source caller frame");
    memory_.mutable_view(output,36);
    auto observation=observe_native_title_address(preferred_);
    const auto flags=native_tu15_title_address_flags(observation);
    const auto bytes=native_tu15_title_address_bytes(observation);
    const auto before=memory_.view(output,36);
    SourcePhysicsContactRecordStore store{call.instruction,output,{before.begin(),before.end()},{bytes.begin(),bytes.end()}};
    if(preferred_.empty())preferred_=observation.interface_name;last_=std::move(observation);
    memory_.write(output,bytes);if(observe)observe(store);abi.gp[0]=flags;abi.gp[4]+=8;return true;
}
}

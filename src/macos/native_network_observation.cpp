#include "native_network_observation.hpp"
#include "native_datagram.hpp"
#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <SystemConfiguration/SystemConfiguration.h>
#include <SystemConfiguration/SCDynamicStoreCopyDHCPInfo.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <sys/ioctl.h>

namespace halo2::macos_network {
namespace {
template<class T>class Owned {
    T value_;
public:
    explicit Owned(T value):value_(value){}
    ~Owned(){if(value_)CFRelease(value_);}
    Owned(const Owned&)=delete;
    Owned& operator=(const Owned&)=delete;
    T get()const{return value_;}
};
std::string text(CFTypeRef value) {
    if(!value)return {};
    if(CFGetTypeID(value)!=CFStringGetTypeID())throw std::runtime_error("Darwin network property is not a string");
    const auto string=static_cast<CFStringRef>(value);
    const auto bytes=CFStringGetMaximumSizeForEncoding(CFStringGetLength(string),kCFStringEncodingUTF8)+1;
    std::string result(std::size_t(bytes),'\0');
    if(!CFStringGetCString(string,result.data(),bytes,kCFStringEncodingUTF8))
        throw std::runtime_error("Darwin network property is not valid UTF-8");
    result.resize(std::strlen(result.c_str()));return result;
}
CFDictionaryRef dictionary(CFTypeRef value) {
    if(value && CFGetTypeID(value)!=CFDictionaryGetTypeID())throw std::runtime_error("Darwin network property is not a dictionary");
    return static_cast<CFDictionaryRef>(value);
}
CFTypeRef property(CFDictionaryRef value,CFStringRef key) {return value?CFDictionaryGetValue(value,key):nullptr;}
std::vector<std::string> strings(CFTypeRef value) {
    if(!value)return {};
    if(CFGetTypeID(value)!=CFArrayGetTypeID())throw std::runtime_error("Darwin network property is not an array");
    const auto array=static_cast<CFArrayRef>(value);std::vector<std::string> result;
    for(CFIndex i=0;i<CFArrayGetCount(array);++i)result.push_back(text(CFArrayGetValueAtIndex(array,i)));
    return result;
}
Ipv4 ipv4(const std::string &value) {
    Ipv4 result{};
    if(::inet_pton(AF_INET,value.c_str(),result.data())!=1)throw std::runtime_error("Darwin IPv4 configuration has an invalid address");
    return result;
}
std::vector<Ipv4> addresses(CFDictionaryRef dict) {
    std::vector<Ipv4> result;
    for(const auto &entry:strings(property(dict,kSCPropNetIPv4Addresses)))result.push_back(ipv4(entry));
    return result;
}
class Store {
    Owned<SCDynamicStoreRef> session_;
    Owned<CFDictionaryRef> rows_;
    static CFDictionaryRef capture(SCDynamicStoreRef session) {
        if(!session)throw std::runtime_error("Darwin network configuration session unavailable");
        const void *patterns[]={CFSTR("State:/Network/Global/IPv4"),
            CFSTR("State:/Network/Interface/.*/(Link|IPConfigurationBusy)"),
            CFSTR("State:/Network/Service/.*/(IPv4|DNS)"),
            CFSTR("Setup:/Network/Service/.*/(IPv4|Interface)")};
        const Owned<CFArrayRef> array(CFArrayCreate(nullptr,patterns,std::size(patterns),&kCFTypeArrayCallBacks));
        if(!array.get())throw std::runtime_error("Darwin network configuration patterns unavailable");
        const auto values=SCDynamicStoreCopyMultiple(session,nullptr,array.get());
        if(!values)throw std::runtime_error("Darwin network configuration snapshot unavailable");
        return values;
    }
public:
    Store():session_(SCDynamicStoreCreate(nullptr,CFSTR("Halo2 network observation"),nullptr,nullptr)),
        rows_(capture(session_.get())){}
    CFDictionaryRef row(const std::string &key)const {
        const Owned<CFStringRef> name(CFStringCreateWithCString(nullptr,key.c_str(),kCFStringEncodingUTF8));
        if(!name.get())throw std::runtime_error("Darwin network key encoding failed");
        return dictionary(CFDictionaryGetValue(rows_.get(),name.get()));
    }
    std::vector<std::string> service_ids()const {
        std::set<std::string> ids;
        std::vector<const void*> keys(std::size_t(CFDictionaryGetCount(rows_.get())));
        CFDictionaryGetKeysAndValues(rows_.get(),keys.data(),nullptr);
        for(const auto key:keys) {
            const auto name=text(key);
            for(const std::string prefix:{"State:/Network/Service/","Setup:/Network/Service/"}) {
                if(!name.starts_with(prefix))continue;
                const auto end=name.find('/',prefix.size());
                if(end!=std::string::npos && end>prefix.size())ids.insert(name.substr(prefix.size(),end-prefix.size()));
            }
        }
        return {ids.begin(),ids.end()};
    }
    std::string primary(CFStringRef key)const {return text(property(row("State:/Network/Global/IPv4"),key));}
    std::string interface_name(const std::string &id)const {
        const auto state=row("State:/Network/Service/"+id+"/IPv4");
        const auto interface=row("Setup:/Network/Service/"+id+"/Interface");
        const auto active_name=text(property(state,kSCPropInterfaceName));
        const auto configured_name=text(property(interface,kSCPropNetInterfaceDeviceName));
        if(!active_name.empty() && !configured_name.empty() && active_name!=configured_name)
            throw std::runtime_error("Darwin active/configured service interface differs");
        return active_name.empty()?configured_name:active_name;
    }
    ServiceState service(const std::string &id)const {
        ServiceState out;out.id=id;out.interface_name=interface_name(id);
        const auto state=row("State:/Network/Service/"+id+"/IPv4");
        const auto setup=row("Setup:/Network/Service/"+id+"/IPv4");
        out.method=text(property(setup,kSCPropNetIPv4ConfigMethod));
        out.addresses=addresses(state);out.configured_addresses=addresses(setup);
        const auto gateway=text(property(state,kSCPropNetIPv4Router));
        if(!gateway.empty())out.gateway=ipv4(gateway);
        out.dns=strings(property(row("State:/Network/Service/"+id+"/DNS"),kSCPropNetDNSServerAddresses));
        if(out.method=="DHCP" && !out.addresses.empty()) {
            const Owned<CFStringRef> identifier(CFStringCreateWithCString(nullptr,id.c_str(),kCFStringEncodingUTF8));
            if(!identifier.get())throw std::runtime_error("Darwin network service key encoding failed");
            const Owned<CFDictionaryRef> lease(SCDynamicStoreCopyDHCPInfo(session_.get(),identifier.get()));
            out.dhcp_lease_observed=lease.get() && DHCPInfoGetLeaseStartTime(lease.get());
        }
        return out;
    }
};
std::map<std::string,InterfaceState> interfaces() {
    ifaddrs *raw{};
    if(::getifaddrs(&raw))throw std::system_error(errno,std::generic_category(),"Darwin network interface enumeration");
    const std::unique_ptr<ifaddrs,decltype(&::freeifaddrs)> owned(raw,::freeifaddrs);
    std::map<std::string,InterfaceState> result;
    for(auto *row=raw;row;row=row->ifa_next) {
        if(!row->ifa_name || !row->ifa_addr || (row->ifa_flags&IFF_LOOPBACK))continue;
        const std::string name=row->ifa_name;
        if(name.empty() || name.size()>=IFNAMSIZ || name=="." || name==".." || name.find('/')!=std::string::npos)
            throw std::runtime_error("Darwin network interface name is invalid");
        auto &out=result[name];out.name=name;out.flags=row->ifa_flags;
        if(row->ifa_addr->sa_family==AF_LINK) {
            const auto *link=reinterpret_cast<const sockaddr_dl*>(row->ifa_addr);
            if(link->sdl_alen==6) {
                std::array<std::uint8_t,6> mac{};std::memcpy(mac.data(),LLADDR(link),6);out.ethernet=mac;
            }
        } else if(row->ifa_addr->sa_family==AF_INET) {
            Ipv4 address{};std::memcpy(address.data(),&reinterpret_cast<const sockaddr_in*>(row->ifa_addr)->sin_addr,4);
            out.ipv4.push_back(address);
        }
    }
    return result;
}
NativeNetworkLinkObservation link(const InterfaceState &interface,const Store &store) {
    NativeNetworkLinkObservation out;out.interface_name=interface.name;
    out.administratively_up=(interface.flags&IFF_UP)!=0;
    if(!out.administratively_up)return out;
    const auto active=property(store.row("State:/Network/Interface/"+interface.name+"/Link"),kSCPropNetLinkActive);
    if(active) {
        if(CFGetTypeID(active)!=CFBooleanGetTypeID())throw std::runtime_error("Darwin carrier property is not Boolean");
        out.carrier=CFBooleanGetValue(static_cast<CFBooleanRef>(active));
    }
    struct Socket {int fd;~Socket(){::close(fd);}} socket{native_network_detail::datagram_socket()};
    ifmediareq media{};std::memcpy(media.ifm_name,interface.name.c_str(),interface.name.size()+1);
    if(::ioctl(socket.fd,SIOCGIFXMEDIA,&media)==0 && (media.ifm_status&IFM_AVALID)) {
        const bool carrier=(media.ifm_status&IFM_ACTIVE)!=0;
        if(out.carrier && *out.carrier!=carrier)throw std::runtime_error("Darwin carrier changed between configuration and media query");
        out.carrier=carrier;
        if(carrier && IFM_TYPE(media.ifm_active)==IFM_ETHER) {
            switch(IFM_SUBTYPE(media.ifm_active)) {
            case IFM_10_T:case IFM_10_2:case IFM_10_5:case IFM_10_STP:case IFM_10_FL:out.speed_mbps=10;break;
            case IFM_100_TX:case IFM_100_FX:case IFM_100_T4:case IFM_100_VG:case IFM_100_T2:out.speed_mbps=100;break;
            case IFM_1000_SX:case IFM_1000_LX:case IFM_1000_CX:case IFM_1000_T:out.speed_mbps=1000;break;
            case IFM_2500_T:out.speed_mbps=2500;break;
            case IFM_5000_T:out.speed_mbps=5000;break;
            case IFM_10G_SR:case IFM_10G_LR:case IFM_10G_CX4:case IFM_10G_T:out.speed_mbps=10000;break;
            }
            if(media.ifm_active&IFM_FDX)out.duplex=NativeNetworkDuplex::Full;
            else if(media.ifm_active&IFM_HDX)out.duplex=NativeNetworkDuplex::Half;
        }
    }
    if(!out.carrier)throw std::runtime_error("Darwin enabled network interface has no carrier observation");
    return out;
}
const InterfaceState &select(const std::map<std::string,InterfaceState> &rows,const Store &store,std::string_view preferred) {
    if(!preferred.empty()) {
        const auto at=rows.find(std::string(preferred));
        if(at==rows.end() || !at->second.ethernet)throw std::invalid_argument("Selected Darwin Ethernet interface is unavailable");
        return at->second;
    }
    const auto primary=rows.find(store.primary(kSCDynamicStorePropNetPrimaryInterface));
    if(primary!=rows.end() && primary->second.ethernet)return primary->second;
    std::set<std::string> configured;
    for(const auto &id:store.service_ids()) {
        const auto name=store.interface_name(id);
        const auto at=rows.find(name);if(at!=rows.end() && at->second.ethernet)configured.insert(name);
    }
    const InterfaceState *inactive=nullptr;
    for(const auto &name:configured) {
        const auto &value=rows.at(name);const auto observation=link(value,store);
        if(observation.administratively_up && observation.carrier.value_or(false))return value;
        if(!inactive)inactive=&value;
    }
    if(inactive)return *inactive;
    throw std::runtime_error("Darwin network has no configured Ethernet interface");
}
}
NativeTitleAddressObservation admit_title_address(const InterfaceState &interface,const ServiceState &service,bool busy) {
    if(!interface.ethernet || interface.name!=service.interface_name)
        throw std::runtime_error("Darwin title address lacks its matching kernel Ethernet owner");
    NativeTitleAddressObservation out;out.interface_name=interface.name;out.ethernet=*interface.ethernet;
    out.gateway=service.gateway;out.dns=service.dns;
    if(service.addresses.empty()) {
        if(!interface.ipv4.empty())throw std::runtime_error("Darwin kernel IPv4 has no matching active service configuration");
        out.acquisition_pending=busy;return out;
    }
    for(const auto &address:service.addresses)
        if(std::ranges::find(interface.ipv4,address)==interface.ipv4.end())
            throw std::runtime_error("Darwin active IPv4 differs from the kernel interface");
    out.ipv4=service.addresses.front();
    if(service.method=="DHCP") {
        if(!service.dhcp_lease_observed || ((*out.ipv4)[0]==169 && (*out.ipv4)[1]==254))
            throw std::runtime_error("Darwin DHCP address lacks an observed lease or is automatic link-local fallback");
        out.assignment=NativeIpv4Assignment::Dhcp;
    } else if(service.method=="Manual" || service.method=="INFORM") {
        if(std::ranges::find(service.configured_addresses,*out.ipv4)==service.configured_addresses.end())
            throw std::runtime_error("Darwin manual IPv4 differs from its configured address");
        out.assignment=NativeIpv4Assignment::Static;
    } else throw std::runtime_error("Darwin IPv4 assignment method is outside the supported title-address domain");
    return out;
}
NativeNetworkLinkObservation observe_link(std::string_view preferred) {
    const Store store;const auto rows=interfaces();return link(select(rows,store,preferred),store);
}
NativeTitleAddressObservation observe_title_address(std::string_view preferred) {
    const Store store;const auto rows=interfaces();const auto &interface=select(rows,store,preferred);
    // Prefer the primary service only when it really belongs to this selected
    // interface. VPN DNS/gateway data must not leak into a LAN service record.
    auto ids=store.service_ids();const auto primary=store.primary(kSCDynamicStorePropNetPrimaryService);
    const auto at=std::ranges::find(ids,primary);if(at!=ids.end())std::rotate(ids.begin(),at,at+1);
    std::optional<ServiceState> inactive;
    for(const auto &id:ids) {
        if(store.interface_name(id)!=interface.name)continue;
        auto service=store.service(id);
        if(!service.addresses.empty())return admit_title_address(interface,service,false);
        if(!inactive)inactive=std::move(service);
    }
    if(!inactive)throw std::runtime_error("Darwin title address has no matching configuration service");
    // Apple's IPConfiguration daemon publishes an empty dictionary while any
    // IP acquisition is busy, removing it when complete. Only consult this
    // aggregate marker when no IPv4 address has been published for the service.
    const bool busy=store.row("State:/Network/Interface/"+interface.name+"/IPConfigurationBusy")!=nullptr;
    return admit_title_address(interface,*inactive,busy);
}
}

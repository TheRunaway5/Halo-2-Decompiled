#include "halo2/native_tu15_network_link.hpp"
#include "halo2/native_tu15_title_address.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#ifdef __APPLE__
#include "macos/native_network_observation.hpp"
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <arpa/inet.h>
#include <cstring>
#endif

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F &&f){bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}
    check(rejected,"Invalid network observation/call was accepted");}
void put(SourceRuntimeMemory &memory,std::uint32_t p,std::uint32_t value) {
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));memory.write(p,bytes);
}
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t p) {
    const auto bytes=memory.view(p,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(bytes[i])<<(8*i);return value;
}
SourcePhysicsWorldRuntimeAbi cpu(std::uint32_t sp) {
    SourcePhysicsWorldRuntimeAbi out{};out.gp={0x12345678,2,3,4,sp,6,7,8};out.eflags=0x246;out.fs_base=0x7000;
    return out;
}
void only_eax_ret(const SourcePhysicsWorldRuntimeAbi &before,const SourcePhysicsWorldRuntimeAbi &after,
                  std::uint32_t result,std::uint32_t bytes) {
    auto expected=before.gp;expected[0]=result;expected[4]+=bytes;
    check(after.gp==expected && before.xmm==after.xmm && before.eflags==after.eflags && before.fs_base==after.fs_base,
          "Native network call changed its integer/vector/flags return ABI");
}
void flag_and_payload_contracts() {
    NativeNetworkLinkObservation link;
    check(native_tu15_network_link_flags(link)==0,"Disabled interface became an active link");
    link.administratively_up=true;rejects([&]{native_tu15_network_link_flags(link);});
    link.carrier=false;check(native_tu15_network_link_flags(link)==0,"Inactive carrier became an active link");
    link.carrier=true;link.speed_mbps=10;link.duplex=NativeNetworkDuplex::Half;
    check(native_tu15_network_link_flags(link)==21,"Measured10Mbps/half-duplex source flags differ");
    link.speed_mbps=100;link.duplex=NativeNetworkDuplex::Full;
    check(native_tu15_network_link_flags(link)==11,"Measured100Mbps/full-duplex source flags differ");
    link.speed_mbps=1000;check(native_tu15_network_link_flags(link)==9,"Modern measured speed acquired a legacy10/100 bit");
    link.speed_mbps.reset();link.duplex=NativeNetworkDuplex::Unknown;
    check(native_tu15_network_link_flags(link)==1,"Unknown media acquired guessed speed/duplex flags");
    NativeTitleAddressObservation title;title.ethernet={2,4,6,8,10,12};
    check(native_tu15_title_address_flags(title)==2,"No IPv4 address acquired an assignment bit");
    title.ipv4=std::array<std::uint8_t,4>{192,0,2,17};rejects([&]{native_tu15_title_address_flags(title);});
    title.assignment=NativeIpv4Assignment::Static;check(native_tu15_title_address_flags(title)==4,"Static address flag differs");
    title.assignment=NativeIpv4Assignment::Dhcp;title.gateway=std::array<std::uint8_t,4>{192,0,2,1};title.dns={"2001:db8::53"};
    check(native_tu15_title_address_flags(title)==0x68,"DHCP/router/DNS capability flags differ");
    const auto bytes=native_tu15_title_address_bytes(title);
    for(unsigned i=0;i<bytes.size();++i) {
        const auto expected=i<4?(*title.ipv4)[i]:(i>=10 && i<16?title.ethernet[i-10]:0);
        check(bytes[i]==std::byte(expected),"Title address invented secure address/port bytes or lost IPv4/MAC");
    }
    title.acquisition_pending=true;check(native_tu15_title_address_flags(title)==0,"Pending address bypassed acquisition flag");
}
void network_startup_calls(SourceRuntimeMemory &memory) {
    constexpr std::uint32_t sp=0x1100,wsa=0x1800;
    NativeTu15NetworkStartup network(memory);
    put(memory,sp,0x6e09c);put(memory,sp+4,0);
    auto before=cpu(sp),state=before;
    check(network.try_call(memory,{0x6e097,0x3d7656,false},state),"Native XNet initialization was not handled");
    only_eax_ret(before,state,0,8);
    check(network.xnet_references()==1 && network.service(),"XNet failed to retain its actual native service");
    auto socket=network.service()->open_datagram({{127,0,0,1},0});const auto destination=socket.local_endpoint();
    put(memory,sp,0x6e0b0);put(memory,sp+4,3);put(memory,sp+8,wsa);
    const auto unchanged=memory.checkpoint();state=cpu(sp);
    rejects([&]{network.try_call(memory,{0x6e0ab,0x3d766d,false},state);});
    check(state.gp==cpu(sp).gp && network.winsock_references()==0 && memory.checkpoint()==unchanged,
          "Rejected Winsock version changed bank, CPU or resource references");
    put(memory,sp+4,2);before=cpu(sp);state=before;unsigned stores=0;
    check(network.try_call(memory,{0x6e0ab,0x3d766d,false},state,[&](const auto &store) {
        check(store.instruction==0x6e0ab && store.address>=wsa && store.address+store.written.size()<=wsa+400,
              "Winsock result escaped its source output record");++stores;
    }),"Winsock initialization was not handled");
    check(stores==7 && network.winsock_references()==1 && word(memory,wsa)==0x02020002 &&
          memory.view(wsa+4,1)[0]==std::byte{} && memory.view(wsa+0x105,1)[0]==std::byte{},
          "Winsock source output layout or references differ");
    only_eax_ret(before,state,0,12);
#ifdef __APPLE__
    NativeTu15TitleAddress title(memory,network);
    put(memory,sp,0x70d11);put(memory,sp+4,sp+8);before=cpu(sp);state=before;stores=0;
    check(title.try_call(memory,{0x70d0c,0x3d725d,false},state,[&](const auto &store) {
        check(store.instruction==0x70d0c && store.address==sp+8 && store.before.size()==36 && store.written.size()==36,
              "Title-address observation changed its actual output record");++stores;
    }),"Native title-address query was not handled");
    check(stores==1 && title.last_observation(),"Native title-address query lost its observation/store");
    only_eax_ret(before,state,native_tu15_title_address_flags(*title.last_observation()),8);
    check(std::ranges::equal(memory.view(sp+8,36),native_tu15_title_address_bytes(*title.last_observation())),
          "Native title-address source record differs from admitted live configuration");
#endif
    put(memory,sp,0x6e131);before=cpu(sp);state=before;
    check(network.try_call(memory,{0x6e12c,0x3d775b,false},state) && network.service(),
          "XNet cleanup discarded an outstanding Winsock service");only_eax_ret(before,state,0,4);
    put(memory,sp,0x6e12c);before=cpu(sp);state=before;
    check(network.try_call(memory,{0x6e127,0x3d7768,false},state) && !network.service(),
          "Final Winsock cleanup retained its network service");only_eax_ret(before,state,0,4);
    rejects([&]{socket.send({},destination);});
    std::cout<<"PASS native XNet/Winsock initialization/output/cleanup, call ABI, rejected version and socket retirement\n";
}
#ifdef __APPLE__
void darwin_admission_contracts() {
    using namespace macos_network;
    const Ipv4 address{192,0,2,17};
    InterfaceState interface{"en-test",0,std::array<std::uint8_t,6>{2,4,6,8,10,12},{address}};
    ServiceState service;service.interface_name=interface.name;service.addresses={address};service.method="DHCP";
    rejects([&]{admit_title_address(interface,service,false);});
    service.dhcp_lease_observed=true;
    check(admit_title_address(interface,service,true).assignment==NativeIpv4Assignment::Dhcp,
          "Published IPv4 lost its lease origin because another IP configuration was busy");
    service.method="Manual";rejects([&]{admit_title_address(interface,service,false);});
    service.configured_addresses={address};
    check(admit_title_address(interface,service,false).assignment==NativeIpv4Assignment::Static,"Observed manual assignment changed origin");
    service.method="INFORM";
    check(admit_title_address(interface,service,false).assignment==NativeIpv4Assignment::Static,"DHCP INFORM changed manual address ownership");
    service.method="BOOTP";rejects([&]{admit_title_address(interface,service,false);});
    service.method="DHCP";service.interface_name="other";rejects([&]{admit_title_address(interface,service,false);});
    service.interface_name=interface.name;interface.ipv4.clear();rejects([&]{admit_title_address(interface,service,false);});
    service.addresses.clear();
    check(admit_title_address(interface,service,true).acquisition_pending &&
          native_tu15_title_address_flags(admit_title_address(interface,service,false))==2,"Pending/absent IPv4 observation differs");
    interface.ipv4={address};rejects([&]{admit_title_address(interface,service,false);});
    interface.ipv4={{169,254,2,17}};service.addresses=interface.ipv4;
    rejects([&]{admit_title_address(interface,service,false);});
    interface.ethernet.reset();rejects([&]{admit_title_address(interface,service,false);});
    std::cout<<"PASS Darwin admission: lease/manual/INFORM origins, busy/absent IPv4, foreign service, mismatch, missing MAC and link-local rejection\n";
}
void darwin_live_kernel_match(const NativeNetworkLinkObservation &link) {
    const auto title=observe_native_title_address(link.interface_name);
    ifaddrs *raw{};check(!::getifaddrs(&raw),"Cannot independently query kernel interfaces");
    const std::unique_ptr<ifaddrs,decltype(&::freeifaddrs)> owned(raw,::freeifaddrs);
    bool mac=false,ip=!title.ipv4;
    for(auto *row=raw;row;row=row->ifa_next) {
        if(!row->ifa_name || !row->ifa_addr || title.interface_name!=row->ifa_name)continue;
        if(row->ifa_addr->sa_family==AF_LINK) {
            const auto *address=reinterpret_cast<const sockaddr_dl*>(row->ifa_addr);
            mac=address->sdl_alen==6 && !std::memcmp(title.ethernet.data(),LLADDR(address),6);
        } else if(row->ifa_addr->sa_family==AF_INET && title.ipv4) {
            ip|=!std::memcmp(title.ipv4->data(),&reinterpret_cast<const sockaddr_in*>(row->ifa_addr)->sin_addr,4);
        }
    }
    check(mac && ip && bool(title.ipv4)==bool(title.assignment),"Live Darwin title address does not match kernel MAC/IPv4");
    std::cout<<"PASS Darwin live kernel MAC/IPv4 and configuration origin; interface="<<title.interface_name
             <<" ipv4="<<bool(title.ipv4)<<" assignment="<<(!title.assignment?"none":*title.assignment==NativeIpv4Assignment::Dhcp?"dhcp":"static")
             <<" gateway="<<bool(title.gateway)<<" dns_count="<<title.dns.size()<<'\n';
}
#endif
}
int main()try {
    flag_and_payload_contracts();
    const auto link=observe_native_network_link();
    check(!link.interface_name.empty(),"Host has no selected live interface for platform validation");
    const auto flags=native_tu15_network_link_flags(link);
    rejects([&]{observe_native_network_link("missing/halo2-test");});
    SourceRuntimeMemory memory;memory.mount(0x1000,std::vector<std::byte>(0x2000,std::byte{0xcc}),
        SourceRuntimeMemory::Access::ReadWrite,"network resource adapter source frames");
    NativeTu15NetworkLink owner(memory,link.interface_name);put(memory,0x1100,0x6de85);
    auto before=cpu(0x1100),state=before;
    check(owner.try_call(memory,{0x6de80,0x37654d,false},state),"Native link query was not handled");
    only_eax_ret(before,state,flags,4);
    state=before;rejects([&]{owner.try_call(memory,{0x6de80,0x37654d,true},state);});
    check(state.gp==before.gp,"Rejected link ABI mutated CPU state");
#ifdef __APPLE__
    darwin_admission_contracts();darwin_live_kernel_match(link);
    rejects([&]{observe_native_title_address("missing/halo2-test");});
#endif
    network_startup_calls(memory);
    std::cout<<"PASS live link query, source link/title flags and payload; interface="<<link.interface_name
             <<" flags="<<flags<<" speed_mbps="<<(link.speed_mbps?std::to_string(*link.speed_mbps):"unknown")
             <<" duplex="<<(link.duplex==NativeNetworkDuplex::Unknown?"unknown":link.duplex==NativeNetworkDuplex::Full?"full":"half")<<'\n';
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

#include "halo2/native_tu15_network_link.hpp"
#include <array>
#include <cerrno>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <system_error>
#if defined(__linux__)
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#endif

namespace halo2 {
namespace {
std::string attribute(const std::string& name,const char* key,bool required) {
    const auto path=std::filesystem::path("/sys/class/net")/name/key;
    std::ifstream input(path);std::string value;
    if(input>>value)return value;
    if(required)throw std::runtime_error("Native network carrier is unavailable: "+path.string());
    return {};
}
NativeNetworkLinkObservation device(const std::string& name,unsigned flags) {
    NativeNetworkLinkObservation result;result.interface_name=name;
#if defined(__linux__)
    result.administratively_up=(flags&IFF_UP)!=0;
    if(!result.administratively_up)return result;
    const auto carrier=attribute(name,"carrier",true);
    if(carrier!="0" && carrier!="1")throw std::runtime_error("Native network carrier has an unsupported value");
    result.carrier=carrier=="1";
    if(!*result.carrier)return result;
    const auto speed=attribute(name,"speed",false);std::uint32_t mbps{};
    const auto parsed=std::from_chars(speed.data(),speed.data()+speed.size(),mbps);
    if(parsed.ec==std::errc{} && parsed.ptr==speed.data()+speed.size() && mbps)result.speed_mbps=mbps;
    const auto duplex=attribute(name,"duplex",false);
    if(duplex=="full")result.duplex=NativeNetworkDuplex::Full;
    else if(duplex=="half")result.duplex=NativeNetworkDuplex::Half;
#else
    throw std::runtime_error("Native network link platform is unavailable");
#endif
    return result;
}
std::uint32_t word(const SourcePhysicsContactMemory& memory,std::uint32_t p) {
    std::array<std::byte,4> bytes{};memory.read(p,bytes);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return value;
}
}
NativeNetworkLinkObservation observe_native_network_link(std::string_view preferred) {
#if defined(__linux__)
    ifaddrs* raw{};
    if(::getifaddrs(&raw))throw std::system_error(errno,std::generic_category(),"Native network interface enumeration");
    const std::unique_ptr<ifaddrs,decltype(&::freeifaddrs)> interfaces(raw,::freeifaddrs);
    std::map<std::string,unsigned> names;
    for(auto* row=raw;row;row=row->ifa_next) {
        if(!row->ifa_name || !row->ifa_addr || row->ifa_addr->sa_family!=AF_PACKET || (row->ifa_flags&IFF_LOOPBACK))continue;
        const std::string name=row->ifa_name;
        if(name.empty() || name=="." || name==".." || name.find('/')!=std::string::npos)
            throw std::runtime_error("Native network interface name is invalid");
        names.insert_or_assign(name,row->ifa_flags);
    }
    if(!preferred.empty()) {
        const auto at=names.find(std::string(preferred));
        if(at==names.end())throw std::invalid_argument("Selected native network interface is unavailable");
        return device(at->first,at->second);
    }
    // A desktop may expose several adapters. Prefer the first actual active
    // carrier in stable name order. Never turn an unreadable link into offline.
    std::optional<NativeNetworkLinkObservation> inactive;
    for(const auto& [name,flags]:names) {
        auto observed=device(name,flags);
        if(observed.administratively_up && observed.carrier.value_or(false))return observed;
        if(!inactive)inactive=std::move(observed);
    }
    return inactive.value_or(NativeNetworkLinkObservation{});
#else
    throw std::runtime_error("Native network link platform is unavailable");
#endif
}
std::uint32_t native_tu15_network_link_flags(const NativeNetworkLinkObservation& input) {
    if(!input.administratively_up)return 0;
    if(!input.carrier)throw std::invalid_argument("Enabled native network interface has no carrier observation");
    if(!*input.carrier)return 0;
    // Source 37658A..376590 admits only active links. The five legacy flag
    // values are shared by the reference XNet ABI. Newer speeds have no legacy
    // speed bit; retain their measured value in the observation instead.
    std::uint32_t flags=1;
    if(input.speed_mbps==100u)flags|=2;
    else if(input.speed_mbps==10u)flags|=4;
    if(input.duplex==NativeNetworkDuplex::Full)flags|=8;
    else if(input.duplex==NativeNetworkDuplex::Half)flags|=16;
    return flags;
}
NativeTu15NetworkLink::NativeTu15NetworkLink(SourceRuntimeMemory& memory,std::string preferred)
    :memory_(memory),preferred_(std::move(preferred)){}
bool NativeTu15NetworkLink::try_call(SourcePhysicsContactMemory& memory,const SourcePhysicsContactRuntimeCall& call,
    SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x37654du)return false;
    if(&memory!=&memory_ || call.tail || (call.instruction!=0x6de80u && call.instruction!=0x6e08du) ||
        word(memory,abi.gp[4])!=call.instruction+5u)
        throw std::invalid_argument("Native XNet link query caller/bank differs");
    auto observation=observe_native_network_link(preferred_);
    const auto flags=native_tu15_network_link_flags(observation);
    last_=std::move(observation);abi.gp[0]=flags;abi.gp[4]+=4;return true;
}
}

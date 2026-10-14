#include "halo2/native_tu15_network_service.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>
#include <vector>

namespace {
using namespace halo2;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> unhex(std::string_view value) {
    check(value.size()%2==0,"Odd fixture hex length");std::vector<std::byte> out(value.size()/2);
    for(std::size_t i=0;i<out.size();++i)out[i]=std::byte(std::stoul(std::string(value.substr(i*2,2)),nullptr,16));return out;
}
}
int main(int argc,char** argv)try {
#if !defined(__linux__)
    return 77;
#endif
    std::size_t cases=0;
    if(argc==2) {
        if(!std::filesystem::is_regular_file(argv[1]))return 77;
        std::ifstream input(argv[1]);auto fixture=nlohmann::json::parse(input);
        check(fixture.at("schema")=="H2_NATIVE_TU15_NETWORK_CONFIGURATION_ORIGINAL_V1" &&
            fixture.at("source_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
            fixture.at("instruction_patches").empty() && fixture.at("supplied_subroutine_results").empty(),"Network configuration reference is not unchanged TU1.5");
        for(const auto& row:fixture.at("rows")) {
            const auto bytes=row.at("input").is_null()?std::vector<std::byte>{}:unhex(row.at("input").get<std::string>());
            const auto normalized=native_tu15_network_configuration(bytes);const auto expected=unhex(row.at("normalized").get<std::string>());
            check(std::ranges::equal(std::as_bytes(std::span(normalized.bytes)),expected),"Native parameter normalization differs from original CPU");++cases;
        }
        check(cases==fixture.at("cases") && cases==22783,"Incomplete network configuration matrix");
    } else check(argc==1,"Unexpected native network service test argument");
    const auto input=unhex("0c000c0804081808080828ff");const auto config=native_tu15_network_configuration(input);
    check(config.private_pool_bytes()==49152 && config.max_sockets()==24 && config.receive_buffer_bytes()==8192 &&
        config.send_buffer_bytes()==8192 && config.key_slots()==8 && config.association_slots()==40 && config.qos_data_bytes()==1020,
        "TU1.5 startup record used a different SDK layout");
    bool truncated=false;try{native_tu15_network_configuration(std::as_bytes(std::span("\x0c",1)));}catch(const std::invalid_argument&){truncated=true;}
    check(truncated,"Truncated accepted startup record was read past its owner");
    NativeTu15NetworkService service(config);check(service.private_pool_address().value>0xffffffffu,"Native network pool was narrowed");
    const NativeIpv4Endpoint loopback{{127,0,0,1},0};
    auto sender=service.open_datagram(loopback);auto receiver=service.open_datagram(loopback);
    const auto from=sender.local_endpoint(),to=receiver.local_endpoint();check(from.port && to.port && from!=to,"Native UDP endpoints were not bound");
    std::array<std::byte,5> payload{std::byte{1},std::byte{2},std::byte{},std::byte{0xfe},std::byte{0xff}};
    NativeIpv4Endpoint peer{};std::array<std::byte,64> buffer{};check(!receiver.receive(buffer,peer),"Empty native socket invented a datagram");
    sender.send(payload,to);std::optional<std::size_t> received;
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    do {received=receiver.receive(buffer,peer);if(!received)std::this_thread::yield();}while(!received && std::chrono::steady_clock::now()<deadline);
    check(received==payload.size() && peer==from && std::ranges::equal(payload,std::span(buffer).first(payload.size())),"Actual native UDP roundtrip bytes or endpoint differ");
    const auto buffers=sender.observed_buffer_bytes();check(buffers[0] && buffers[1],"Native socket buffer observation unavailable");
    std::vector<NativeTu15Datagram> sockets;
    for(unsigned i=2;i<config.max_sockets();++i)sockets.push_back(service.open_datagram(loopback));
    bool limited=false;try{service.open_datagram(loopback);}catch(const std::length_error&){limited=true;}
    check(limited && service.live_sockets()==24,"Source-configured socket budget was ignored or leaked");
    sockets.pop_back();check(service.live_sockets()==23,"Native socket close did not release its slot");
    sockets.push_back(service.open_datagram(loopback));check(service.live_sockets()==24,"Native socket budget was not reusable");
    service.close();check(service.live_sockets()==0,"Native network shutdown retained sockets");
    bool retired=false;try{sender.send(payload,to);}catch(const std::runtime_error&){retired=true;}
    check(retired,"Retired native network context accepted a stale endpoint");
    bool retired_pool=false;try{service.private_pool_address();}catch(const std::runtime_error&){retired_pool=true;}
    check(retired_pool,"Retired native network pool remained addressable");
    std::cout<<"Native TU1.5 network service: "<<cases<<" original decoder cases; real UDP loopback, 24-socket limit, 64-bit pool and teardown passed; host buffers "<<buffers[0]<<'/'<<buffers[1]<<"; no Xbox network-protocol claim\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

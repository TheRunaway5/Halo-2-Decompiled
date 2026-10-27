#include "platform/native_event_resources.hpp"
#include "runtime/source_runtime_host_pages.hpp"
#include "runtime/source_runtime_host_physical_pages.hpp"
#include "halo2/native_tu15_network_service.hpp"
#include <array>
#include <chrono>
#include <iostream>
#include <thread>

namespace {
void check(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
template<class F>void await(F&& ready,const char* message) {
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    do {
        if(ready())return;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }while(std::chrono::steady_clock::now()<deadline);
    throw std::runtime_error(message);
}
}
int main()try {
    using namespace halo2;
    auto* memory=source_runtime_detail::host_pages::reserve(8192);
    source_runtime_detail::host_pages::make_writable(memory,4096,4096);
    static_cast<unsigned char*>(memory)[4096]=73;
    check(source_runtime_detail::host_pages::release(memory,8192)==0,"Windows reservation release failed");

    auto* physical=source_runtime_detail::physical_pages::allocate(4096);
    std::span<std::byte> bank(static_cast<std::byte*>(physical),4096);
    bank[0]=std::byte{42};
    source_runtime_detail::physical_pages::protect_range(bank,0,0,4096,true,{},{});
    check(bank[0]==std::byte{42},"Windows physical protection changed its backing");
    source_runtime_detail::physical_pages::reset(bank);bank[0]=std::byte{43};
    source_runtime_detail::physical_pages::restore_protection(bank,0,{1});
    check(bank[0]==std::byte{43},"Windows physical protection restore changed its backing");
    source_runtime_detail::physical_pages::release(physical,4096);

    native_events::Signal signal;native_events::Poller poller;
    poller.add(signal.descriptor(),1);check(!poller.ready(1),"Windows event started signaled");
    signal.signal();signal.signal();
    check(poller.ready(1)&&poller.ready(1),"Windows polling consumed an event");
    check(signal.take()==2&&!poller.ready(1)&&!signal.take(),"Windows event count was lost or delivered twice");

    native_events::Timer timer(native_events::Clock::Monotonic);poller.add(timer.descriptor(),2);
    check(!timer.arm_relative_100ns(200000),"Windows timer invented a prior schedule");
    await([&]{return poller.ready(2);},"Windows monotonic timer did not expire");
    check(timer.take()==1&&!poller.ready(2)&&!timer.take(),"Windows timer expiry was delivered twice");
    check(!timer.arm_relative_100ns(300000000),"Windows consumed timer retained an armed schedule");
    check(timer.remaining_100ns()>0&&timer.cancel()&&timer.disarmed()&&!timer.take(),"Windows timer cancellation left a live schedule/receipt");

    native_events::Timer wall(native_events::Clock::Realtime);
    wall.arm_realtime_100ns(native_events::windows_events::now_100ns(native_events::Clock::Realtime)+200000);
    await([&]{return bool(wall.take());},"Windows realtime timer did not expire");
    check(!wall.take(),"Windows realtime timer receipt repeated");

    NativeTu15NetworkService network(native_tu15_network_configuration());
    auto first=network.open_datagram({{127,0,0,1},0});
    auto second=network.open_datagram({{127,0,0,1},0});
    const std::array<std::byte,4> packet{std::byte{1},std::byte{2},std::byte{3},std::byte{4}};
    std::array<std::byte,4> received{};NativeIpv4Endpoint from{};std::optional<std::size_t> size;
    check(!second.receive(received,from),"Windows empty socket invented a packet");
    first.send(packet,second.local_endpoint());
    await([&]{size=second.receive(received,from);return size.has_value();},"Windows loopback packet did not arrive");
    check(size==4&&received==packet&&from==first.local_endpoint(),"Windows loopback bytes/endpoint differed");
    first.send({},second.local_endpoint());
    await([&]{size=second.receive({},from);return size.has_value();},"Windows empty packet did not arrive");
    check(size==0,"Windows empty packet had nonzero bytes");
    first.send(packet,second.local_endpoint());
    await([&]{try{second.receive(std::span(received).first(1),from);}
        catch(const std::length_error&){return true;}return false;},"Windows truncated datagram was accepted");
    network.close();check(network.live_sockets()==0,"Windows network shutdown retained sockets");
    bool retired=false;try{first.send(packet,from);}catch(const std::runtime_error&){retired=true;}
    check(retired,"Windows retired network owner accepted a packet");
    std::cout<<"Windows virtual/physical memory, counted events, monotonic/realtime timers and UDP exact/empty/truncated datagrams passed\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

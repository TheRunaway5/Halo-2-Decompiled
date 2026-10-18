#include "halo2/online_peer_transport.hpp"
#include <chrono>
#include <exception>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

using namespace halo2;
using namespace std::chrono_literals;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
struct Room {
    std::unique_ptr<OnlinePeerTransport> a=std::make_unique<OnlinePeerTransport>(0,std::vector<unsigned>{0,4},std::vector<std::string>{});
    std::unique_ptr<OnlinePeerTransport> b=std::make_unique<OnlinePeerTransport>(4,std::vector<unsigned>{0,4},std::vector<std::string>{});
    Room(){wait([&]{
        for(auto signal:a->signals()){signal.peer=0;b->signal(signal);}
        for(auto signal:b->signals()){signal.peer=4;a->signal(signal);}
        return a->ready(4)&&b->ready(0);
    });}
    template<class F>void wait(F done){const auto until=std::chrono::steady_clock::now()+15s;
        while(!done()){check(std::chrono::steady_clock::now()<until,"Direct runtime restart/exchange timed out");std::this_thread::sleep_for(2ms);}}
    void exchange(){std::vector<std::byte> expected(1404,std::byte{0x71}),actual;unsigned from{};
        check(a->send(expected,4),"Live room failed to send after another room closed");
        wait([&]{return b->receive(actual,from);});check(from==0&&actual==expected,"Live direct room changed peer/payload");
        check(b->send(expected,0),"Restarted runtime failed reverse send");
        wait([&]{return a->receive(actual,from);});check(from==4&&actual==expected,"Restarted direct room changed peer/payload");
    }
};
}
int main()try{
    {
        Room live;
        {Room departing;departing.exchange();}
        live.exchange();
        bool rejected{};try{OnlinePeerTransport invalid(0,{0,4},{"turn:127.0.0.1:3478"});}catch(const std::exception &){rejected=true;}
        check(rejected,"Invalid transport constructor was accepted");live.exchange();
    }
    {Room restarted;restarted.exchange();}
    std::mutex mutex;std::exception_ptr failure;
    const auto run=[&]{try{for(unsigned i=0;i<3;++i){Room room;room.exchange();}}
        catch(...){std::lock_guard lock(mutex);failure=std::current_exception();}};
    {std::jthread one(run),two(run);}
    if(failure)std::rethrow_exception(failure);
    std::cout<<"PASS direct runtime lifetime: independent active rooms, failed construction, final-owner shutdown, fresh-room restart and concurrent create/exchange/destroy with maximum-size authenticated UDP payloads\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

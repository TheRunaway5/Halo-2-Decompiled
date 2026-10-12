#include "halo2/physics_platform_locks.hpp"
#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <thread>

namespace {
using namespace halo2;
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
std::vector<std::byte> bytes(const std::string &s) {
    check(s.size()%2==0,"Invalid platform fixture bytes");
    std::vector<std::byte> result;
    for (std::size_t i=0;i<s.size();i+=2)
        result.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
    return result;
}
struct Memory : SourcePhysicsContactMemory {
    std::uint32_t address;
    std::vector<std::byte> contents;
    Memory(std::uint32_t p, std::vector<std::byte> b) : address(p),contents(std::move(b)) {}
    void read(std::uint32_t p,std::span<std::byte> result) const override {
        check(p>=address && std::uint64_t(p-address)+result.size()<=contents.size(),"Platform read outside owned input");
        std::copy_n(contents.begin()+(p-address),result.size(),result.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte> input) override {
        check(p>=address && std::uint64_t(p-address)+input.size()<=contents.size(),"Platform write outside owned input");
        std::copy(input.begin(),input.end(),contents.begin()+(p-address));
    }
};
unsigned compare(const std::filesystem::path &path) {
    std::ifstream input(path);
    const auto fixture=nlohmann::json::parse(input);
    check(!fixture.at("original_kernel_execution").get<bool>() &&
        !fixture.at("wait_wake_results_supplied").get<bool>(),"Incorrect compatibility evidence scope");
    PhysicsPlatformLocks locks;
    unsigned count=0, waits=0, wakes=0;
    for(const auto &row:fixture.at("cases")) {
        const auto p=row.at("address").get<std::uint32_t>();
        const auto token=row.at("token").get<std::uint32_t>();
        Memory memory(p,bytes(row.at("before_hex")));
        auto replay=memory;
        unsigned stores=0;
        const auto observer=[&](const PhysicsPlatformLockStore &store) {
            std::vector<std::byte> before(store.before.size());
            replay.read(store.address,before);
            check(before==store.before,"Platform observer before bytes differ");
            replay.write(store.address,store.written);
            ++stores;
        };
        PhysicsPlatformLockServices services;
        services.current_thread=[token]{return token;};
        bool stopped=false;
        try {
            switch(row.at("ordinal").get<unsigned>()) {
            case 291: locks.initialize(memory,p,observer); break;
            case 277: locks.enter(memory,p,services,observer); break;
            case 294: locks.leave(memory,p,services,observer); break;
            default: throw std::runtime_error("Unsupported platform fixture operation");
            }
        } catch(const PhysicsPlatformLockBoundary &boundary) {
            stopped=true;
            check(boundary.critical_section==p,"Platform boundary pointer differs");
            check(!row.at("boundary").is_null(),"Unexpected platform boundary");
            const auto name=row.at("boundary").get<std::string>();
            check((name=="wait" && boundary.kind==PhysicsPlatformLockBoundaryKind::Wait) ||
                (name=="wake" && boundary.kind==PhysicsPlatformLockBoundaryKind::Wake),"Platform boundary kind differs");
            if(name=="wait") ++waits; else ++wakes;
        }
        check(stopped==!row.at("boundary").is_null(),"Missing platform operation did not stop");
        check(memory.contents==bytes(row.at("after_hex")),"Full compatibility critical section differs");
        check(replay.contents==memory.contents && stores>0,"Platform stores do not account for changes");
        ++count;
    }
    check(count==1595 && waits==9 && wakes==9,"Incomplete compatibility lock fixture");
    return count;
}
void missing_identity() {
    PhysicsPlatformLocks locks;
    Memory memory(0x1000,std::vector<std::byte>(28,std::byte{0xa5}));
    const auto initial=memory.contents;
    bool stopped=false;
    try {locks.enter(memory,memory.address,{});}
    catch(const PhysicsPlatformLockBoundary &b) {stopped=b.kind==PhysicsPlatformLockBoundaryKind::CurrentThread;}
    check(stopped && memory.contents==initial,"Missing thread owner fabricated identity or changed input");
}
void supplied_host_handoff() {
    PhysicsPlatformLocks locks;
    Memory memory(0x1000,std::vector<std::byte>(28));
    locks.initialize(memory,memory.address);
    struct Event {
        std::mutex mutex;
        std::condition_variable cv;
        bool waiting=false, signalled=false;
        unsigned waits=0,wakes=0;
    } event;
    std::mutex ids_mutex;
    std::map<std::thread::id,std::uint32_t> ids;
    PhysicsPlatformLockServices services;
    services.current_thread=[&] {
        std::lock_guard guard(ids_mutex);
        const auto id=std::this_thread::get_id();
        auto [it,inserted]=ids.emplace(id,static_cast<std::uint32_t>(ids.size()+1));
        return it->second;
    };
    services.wait=[&](auto p) {
        check(p==memory.address,"Host wait object differs");
        std::unique_lock guard(event.mutex);
        ++event.waits;event.waiting=true;event.cv.notify_all();
        event.cv.wait(guard,[&]{return event.signalled;});event.signalled=false;
    };
    services.wake=[&](auto p) {
        check(p==memory.address,"Host wake object differs");
        std::lock_guard guard(event.mutex);
        ++event.wakes;event.signalled=true;event.cv.notify_all();
    };
    locks.enter(memory,memory.address,services);
    locks.enter(memory,memory.address,services);
    std::exception_ptr failure;
    bool acquired=false;
    std::jthread other([&] {
        try {
            locks.enter(memory,memory.address,services);
            {std::lock_guard guard(event.mutex);acquired=true;}
            locks.leave(memory,memory.address,services);
        } catch(...) {failure=std::current_exception();}
    });
    {
        std::unique_lock guard(event.mutex);
        // A timeout is a test failure shield, not a successful wait result.
        if(!event.cv.wait_for(guard,std::chrono::seconds(5),[&]{return event.waiting;})) {
            event.signalled=true;event.cv.notify_all();guard.unlock();other.join();
            throw std::runtime_error("Native contended enter never requested wait");
        }
        check(!acquired,"Native contender acquired before release");
    }
    locks.leave(memory,memory.address,services);
    {std::lock_guard guard(event.mutex);check(event.wakes==0 && !acquired,"Recursive release woke contender early");}
    locks.leave(memory,memory.address,services);
    other.join();
    if(failure) std::rethrow_exception(failure);
    check(acquired && ids.size()==2 && event.waits==1 && event.wakes==1,"Real host handoff did not complete");
    Memory expected(memory.address,std::vector<std::byte>(28));
    locks.initialize(expected,expected.address);
    check(memory.contents==expected.contents,"Host handoff did not release source-visible state");
}
}
int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply pinned compatibility lock fixture");
        if(!std::filesystem::exists(argv[1])) return 77;
        const auto cases=compare(argv[1]);
        missing_identity();
        supplied_host_handoff();
        std::cout<<"PASS native physics platform locks: "<<cases<<" pinned compatibility transitions, 18 missing wait/wake boundaries, missing identity, real two-thread recursive handoff; no original kernel ABI or live gameplay proof\n";
        return 0;
    } catch(const std::exception &e) {
        std::cerr<<"FAIL "<<e.what()<<'\n';return 1;
    }
}

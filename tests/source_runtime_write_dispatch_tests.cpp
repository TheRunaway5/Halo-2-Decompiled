#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <ctime>
#include <future>
#include <iostream>
#include <string_view>
#include <thread>
#include <vector>

using namespace halo2;
namespace {
void need(bool value,const char *message){if(!value)throw std::runtime_error(message);}
using Clock=std::chrono::steady_clock;
void policy_changes() {
    SourceRuntimeMemory bank,other;
    const std::array initial{std::byte{0},std::byte{0},std::byte{0},std::byte{0}};
    for(auto *memory:{&bank,&other})memory->mount(0x1000,initial,SourceRuntimeMemory::Access::ReadWrite,"dispatch owner");
    std::array<int,4> owners{};std::vector<unsigned> actual,expected;
    bool deny_guard{},deny_observer{};
    const auto guard=[&](unsigned i){return [&,i](auto p,auto n){
        need(p==0x1000 && n==4,"dispatch changed complete write extent");actual.push_back(i);
        if(deny_guard && i==1)throw std::runtime_error("guard denied");
    };};
    const auto observer=[&](unsigned i){return [&,i](auto,auto){
        actual.push_back(i);if(deny_observer && i==3)throw std::runtime_error("observer denied");
    };};
    // Pointer order is the original registry order, independent of insertion.
    const bool ascending=std::less<const void *>{}(&owners[0],&owners[1]);
    need(ascending,"test owner array is not in pointer order");
    source_runtime_register_write_guard(bank,&owners[1],guard(1));
    source_runtime_register_write_guard(bank,&owners[0],guard(0));
    source_runtime_register_write_observer(bank,&owners[3],observer(3));
    source_runtime_register_write_observer(bank,&owners[2],observer(2));
    source_runtime_register_write_observer(other,&owners[2],observer(9));
    const std::array value{std::byte{1},std::byte{2},std::byte{3},std::byte{4}};
    for(unsigned iteration=0;iteration<400;++iteration) {
        actual.clear();bank.write(0x1000,value);
        need(actual==std::vector<unsigned>{0,1,2,3},"dispatch lost guard/observer ordering");
        actual.clear();other.write(0x1000,value);need(actual==std::vector<unsigned>{9},"dispatch reused another bank's policies");
        actual.clear();bank.validate_mutable_view(0x1000,4);
        need(actual==std::vector<unsigned>{0,1},"validation-only admission notified write observers");
        actual.clear();deny_guard=true;
        try{bank.write(0x1000,initial);need(false,"guard accepted forbidden write");}catch(const std::runtime_error &e){need(std::string(e.what())=="guard denied","wrong guard exception");}
        deny_guard=false;
        need(actual==std::vector<unsigned>{0,1} && std::ranges::equal(bank.view(0x1000,4),value),"guard rejection changed bytes or delivered observers");
        actual.clear();deny_observer=true;
        try{bank.write(0x1000,initial);need(false,"observer accepted forbidden write");}catch(const std::runtime_error &e){need(std::string(e.what())=="observer denied","wrong observer exception");}
        deny_observer=false;
        need(actual==std::vector<unsigned>{0,1,2,3} && std::ranges::equal(bank.view(0x1000,4),value),"observer rejection changed bytes or ordering");
        source_runtime_unregister_write_observer(bank,&owners[2]);
        actual.clear();bank.write(0x1000,initial);need(actual==std::vector<unsigned>{0,1,3},"removed observer survived cached dispatch");
        source_runtime_register_write_observer(bank,&owners[2],observer(2));
    }
    for(unsigned i=0;i<2;++i)source_runtime_unregister_write_guard(bank,&owners[i]);
    for(unsigned i=2;i<4;++i)source_runtime_unregister_write_observer(bank,&owners[i]);
    source_runtime_unregister_write_observer(other,&owners[2]);
    actual.clear();bank.write(0x1000,initial);other.write(0x1000,initial);
    need(actual.empty(),"retired bank retained cached callbacks");
}
void concurrent_retirement() {
    SourceRuntimeMemory bank;int owner;
    std::promise<void> entered,release,removing;auto released=release.get_future();
    std::atomic<unsigned> calls{};
    source_runtime_register_write_observer(bank,&owner,[&](auto,auto){++calls;entered.set_value();released.wait();});
    std::thread writer([&]{source_runtime_validate_write(bank,0x1000,4);});
    entered.get_future().wait();
    auto retired=std::async(std::launch::async,[&]{removing.set_value();source_runtime_unregister_write_observer(bank,&owner);});
    removing.get_future().wait();
    const auto blocked=retired.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout;
    release.set_value();writer.join();retired.get();
    source_runtime_validate_write(bank,0x1000,4);
    need(blocked && calls==1,"policy retirement overtook active callback or cached a retired observer");
}
void recursive_and_large_banks() {
    SourceRuntimeMemory outer,inner;std::array<int,40> owners{};std::vector<unsigned> calls;
    source_runtime_register_write_guard(inner,&owners[0],[&](auto,auto){calls.push_back(1);});
    source_runtime_register_write_guard(outer,&owners[1],[&](auto,auto){calls.push_back(0);source_runtime_validate_write(inner,2,1);});
    source_runtime_register_write_observer(outer,&owners[2],[&](auto,auto){calls.push_back(2);});
    for(unsigned repeat=0;repeat<8;++repeat) {
        calls.clear();source_runtime_validate_write(outer,1,1);
        need(calls==std::vector<unsigned>{0,1,2},"recursive write replaced the active outer dispatch");
    }
    source_runtime_unregister_write_guard(inner,&owners[0]);
    source_runtime_unregister_write_guard(outer,&owners[1]);
    source_runtime_unregister_write_observer(outer,&owners[2]);
    for(unsigned i=0;i<owners.size();++i)
        source_runtime_register_write_observer(outer,&owners[i],[&,i](auto,auto){calls.push_back(i);});
    calls.clear();source_runtime_validate_write(outer,1,1);
    need(calls.size()==owners.size(),"dispatch truncated a large observer bank");
    for(unsigned i=0;i<owners.size();++i)need(calls[i]==i,"large observer bank changed pointer order");
    for(auto &owner:owners)source_runtime_unregister_write_observer(outer,&owner);
    calls.clear();source_runtime_validate_write(outer,1,1);need(calls.empty(),"empty dispatch retained large-bank callbacks");
}
void benchmark() {
    SourceRuntimeMemory bank;std::array<int,3> owners{};
    bank.mount(0x60000000,std::vector<std::byte>(65536),SourceRuntimeMemory::Access::ReadWrite,"real translated stack bank");
    std::atomic<std::uint64_t> first{},second{};
    source_runtime_register_write_guard(bank,&owners[0],[](auto p,auto n){if(std::uint64_t(p)+n>0x80000000ull)throw std::runtime_error("physical boundary");});
    source_runtime_register_write_observer(bank,&owners[1],[&](auto,auto){first.fetch_add(1,std::memory_order_relaxed);});
    source_runtime_register_write_observer(bank,&owners[2],[&](auto,auto){second.fetch_add(1,std::memory_order_relaxed);});
    constexpr unsigned iterations=2000000;const std::array value{std::byte{4},std::byte{3},std::byte{2},std::byte{1}};
    const auto start=Clock::now();
    const auto cpu_start=std::clock();
    for(unsigned i=0;i<iterations;++i)bank.write(0x60000000+(i&16383u)*4,value);
    const auto cpu_ns=double(std::clock()-cpu_start)*1.e9/(CLOCKS_PER_SEC*double(iterations));
    const auto ns=std::chrono::duration<double,std::nano>(Clock::now()-start).count()/iterations;
    need(first==iterations && second==iterations,"benchmark mutation auditing changed");
    for(unsigned i=0;i<16384;++i)need(std::ranges::equal(bank.view(0x60000000+i*4,4),value),"benchmark complete bytes changed");
    source_runtime_unregister_write_guard(bank,&owners[0]);
    for(unsigned i=1;i<3;++i)source_runtime_unregister_write_observer(bank,&owners[i]);
    std::cout<<"write_ns="<<ns<<" write_process_cpu_ns="<<cpu_ns<<" iterations="<<iterations<<" exact_audits="<<first<<','<<second<<'\n';
    std::array<std::byte,4> read{};
    const auto read_start=std::clock();
    for(unsigned i=0;i<iterations;++i)bank.read(0x60000000+(i&16383u)*4,read);
    const auto read_cpu_ns=double(std::clock()-read_start)*1.e9/(CLOCKS_PER_SEC*double(iterations));
    need(read==value,"benchmark small read changed complete scalar bytes");
    std::cout<<"read_process_cpu_ns="<<read_cpu_ns<<'\n';
}
}
int main(int argc,char **argv)try {
    policy_changes();concurrent_retirement();recursive_and_large_banks();
    if(argc==2 && std::string_view(argv[1])=="--benchmark")benchmark();
    std::cout<<"PASS dispatch ordering, exceptions, policy replacement, bank isolation and concurrent retirement\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

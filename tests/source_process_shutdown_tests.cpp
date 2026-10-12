#include "halo2/source_process_shutdown.hpp"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using J=nlohmann::json;
using M=halo2::SourceRuntimeMemory;
void check(bool value,const char *why) { if(!value)throw std::runtime_error(why); }
std::vector<std::byte> bytes(const std::string &s) {
    check(!(s.size()&1),"Invalid primary byte capture");
    std::vector<std::byte> b;
    for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
    return b;
}
void same(std::span<const std::byte> actual,const J &hex) {
    check(std::ranges::equal(actual,bytes(hex.get<std::string>())),"Primary retained list bytes differ");
}
}
int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply compiled primary shutdown-list fixture");
        if(!std::filesystem::exists(argv[1]))return 77;
        std::ifstream in(argv[1]);const auto fixture=J::parse(in);
        const auto head=fixture.at("head").get<std::uint32_t>();
        const auto records=fixture.at("records").get<std::uint32_t>();
        std::size_t cases=0,steps=0,stores=0,cycles=0;
        for(const auto &row:fixture.at("cases")) {
            M memory;
            memory.mount(records,bytes(row.at("input_records")),M::Access::ReadWrite,"explicit dirty record inputs");
            {
                halo2::SourceProcessShutdownRegistry registry(memory,head);
                check(registry.backs(memory) && registry.head()==head && registry.records().empty(),
                    "Native retained registry ownership differs");
                for(const auto &step:row.at("steps")) {
                    std::size_t store=0;
                    registry.registration(step.at("record"),step.at("add"),[&](const auto &actual) {
                        check(store<step.at("stores").size(),"Unexpected native list store");
                        const auto &expected=step.at("stores").at(store++);
                        check(actual.address==expected.at("address") && actual.before==bytes(expected.at("before")) &&
                            actual.written==bytes(expected.at("written")),"Compiled primary store order/value differs");
                    });
                    check(store==step.at("stores").size(),"Missing native list store");
                    same(memory.view(head,8),step.at("head"));same(memory.view(records,0x1000),step.at("records"));
                    bool cyclic=false;std::vector<halo2::SourceProcessShutdownRecord> actual;
                    try { actual=registry.records(); } catch(const std::runtime_error &) { cyclic=true; }
                    check(cyclic==step.at("cycle").get<bool>(),"Actual duplicate-record topology differs");
                    if(!cyclic) {
                        check(actual.size()==step.at("order").size(),"Primary linked record count differs");
                        for(std::size_t i=0;i<actual.size();++i) {
                            const auto &expected=step.at("order").at(i);
                            check(actual[i].address==expected.at("address") && actual[i].routine==expected.at("routine") &&
                                actual[i].priority==expected.at("priority"),"Primary signed priority/identity order differs");
                        }
                    }
                    ++steps;stores+=store;cycles+=cyclic;
                }
            }
            bool released=false;
            try { memory.view(head,8); } catch(const std::out_of_range &) { released=true; }
            check(released,"Registry head backing outlived its owner");
            // Destruction releases only the native head, never runs callbacks
            // or modifies the last actual source record bytes.
            same(memory.view(records,0x1000),row.at("steps").back().at("records"));
            ++cases;
        }
        std::cout<<cases<<" primary cases, "<<steps<<" operations, "<<stores
            <<" ordered list stores, "<<cycles<<" measured duplicate cycles and retained records match\n";
    } catch(const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}

#include "halo2/native_campaign_ai_query_tu15.hpp"
#include "halo2/source_script_lists.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::byte> bytes;
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        for(auto &b:out) {
            const auto found=bytes.find(at++);
            check(found!=bytes.end(),"AI actor-list query read outside its declared source bank");b=found->second;
        }
    }
    void write(std::uint32_t,std::span<const std::byte>)override {
        throw std::runtime_error("AI actor-list query mutated the retained source world");
    }
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply original AI actor-list fixture");
    std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
    check(fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
          "AI actor-list reference differs");
    check(fixture.at("cases").size()==144,"Incomplete original AI actor-list fixture");
    unsigned native_cases{};
    for(const auto &row:fixture.at("cases")) {
        if(row.at("wrapper").get<bool>() && !row.at("parsed").get<bool>()) {
            check(row.at("objects").empty() && row.at("events").size()==1 && row.at("events")[0]==0x212010,
                  "Failed original HS parsing entered the actor-list effect");
            continue;
        }
        Memory memory;
        for(const auto &region:fixture.at("regions"))
            for(unsigned n=0;n<region[1].get<unsigned>();++n)memory.bytes[region[0].get<unsigned>()+n]=std::byte{};
        for(const auto &w:row.at("writes"))for(unsigned n=0;n<w[1].get<unsigned>();++n)
            memory.bytes[w[0].get<unsigned>()+n]=std::byte(w[2].get<unsigned>()>>(8*n));
        SourceHsListBank bank;unsigned allocations{};
        const auto result=native_campaign_ai_actors_tu15(memory,row.at("reference"),{
            [&]{++allocations;return row.at("exhausted").get<bool>()?0xffffffffu:bank.allocate();},
            [&](auto list,auto object){bank.append(list,object);}});
        check((result==0xffffffffu)==(row.at("result").get<unsigned>()==0xffffffffu),
              "AI actor-list NONE/allocation result differs from original");
        check(allocations==(row.at("reference").get<unsigned>()==0xffffffffu?0u:1u),
              "AI actor-list allocated for NONE or lost its allocation boundary");
        check(bank.objects(result)==row.at("objects").get<std::vector<unsigned>>() && bank.count(result)==row.at("count"),
              "AI actor-list members, swarm expansion or prepend order differs from original");
        if(result!=0xffffffffu && !row.at("objects").empty())
            check(bank.get(result,-1)==row.at("objects")[0],"AI actor-list lost signed HS list access semantics");
        ++native_cases;
    }
    std::cout<<"PASS "<<native_cases<<" native AI actor-list cases from144 whole original effect/evaluator captures\n";
    std::cout<<"LIMIT allocation/collection and HS parse/completion service boundaries; full scene AI remains open\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

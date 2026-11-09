#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <iostream>
#include <stdexcept>
using namespace halo2;
namespace {
struct Memory:SourcePhysicsContactMemory {
    void read(std::uint32_t,std::span<std::byte>)const override{throw std::runtime_error("Cursor dispatch read memory");}
    void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("Cursor dispatch wrote memory");}
};
template<class State>unsigned check_dispatch(){
    unsigned checked{};
    for(bool literal:{false,true})for(unsigned policy=0;policy<17;++policy){
        Memory memory;SourcePhysicsContactRecordObserver observer;
        SourcePhysicsWorldRuntimeServices services;
        unsigned original{},replacement{};std::uint32_t position=0xa5a5a5a5u,alternate=0x5a5a5a5au;
        bool first{},second{};std::array sites{0x38846eu,0x21f590u,0x33b595u};
        const auto pc=policy==1?sites[0]:0x1f6617u;
        std::array changed{pc,0x21f590u,0x33b595u};
        services.instruction=[&](std::uint32_t value){++original;position=value;};
        auto&cursor=services.instruction_cursor;
        cursor.position=&position;cursor.callback_type=&services.instruction.target_type();
        cursor.executor=std::this_thread::get_id();cursor.service_sites=sites;
        cursor.literal_service_sites=true;cursor.first_trace=&first;cursor.second_trace=&second;
        if(policy==12)cursor.callback_type=nullptr;
        if(policy==13)cursor.literal_service_sites=false;
        State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
        unsigned expect_original{};unsigned expect_replacement{};
        auto expected_position=pc,expected_alternate=alternate;
        switch(policy){
        case 1:expect_original=1;break;
        case 2:first=true;expect_original=1;break;
        case 3:second=true;expect_original=1;break;
        case 4:services.instruction={};expected_position=position;break;
        case 5:services.instruction=[&](std::uint32_t){++replacement;};expect_replacement=1;expected_position=position;break;
        case 6:cursor.callback_type=nullptr;expect_original=1;break;
        case 7:cursor.executor={};expect_original=1;break;
        case 8:cursor.position=nullptr;expect_original=1;break;
        case 9:cursor.service_sites=changed;expect_original=1;break;
        case 10:cursor.literal_service_sites=false;break;
        case 11:cursor.position=&alternate;expected_position=position;expected_alternate=pc;break;
        case 12:cursor.callback_type=&services.instruction.target_type();expect_original=1;break;
        case 13:cursor.literal_service_sites=true;break;
        case 14:cursor.callback_type=&typeid(int);expect_original=1;break;
        case 15:cursor.service_sites={};break;
        case 16:cursor.service_sites=changed;cursor.literal_service_sites=false;expect_original=1;break;
        }
        if(literal)state.instruction_literal(pc);else state.instruction(pc);
        if(original!=expect_original||replacement!=expect_replacement||position!=expected_position||alternate!=expected_alternate)
            throw std::runtime_error("Live cursor policy mismatch: literal="+std::to_string(literal)+" policy="+std::to_string(policy)+
                " original="+std::to_string(original)+" replacement="+std::to_string(replacement)+" position="+std::to_string(position));
        ++checked;
    }
    return checked;
}
}
int main()try{
    const auto cases=check_dispatch<physics_contact_runtime_detail::State>()+check_dispatch<physics_world_runtime_detail::WorldState>();
    std::cout<<"{\"passed\":true,\"cursor_policy_cases\":"<<cases<<"}\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
